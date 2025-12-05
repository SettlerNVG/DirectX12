//***************************************************************************************
// TerrainApp.cpp - Terrain rendering with QuadTree LOD and Frustum Culling
//***************************************************************************************

#include "../../Common/d3dApp.h"
#include "../../Common/MathHelper.h"
#include "../../Common/UploadBuffer.h"
#include "../../Common/GeometryGenerator.h"
#include "../../Common/Camera.h"
#include "../../Common/DDSTextureLoader.h"
#include "FrameResource.h"
#include "QuadTree.h"
#include "Terrain.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

const int gNumFrameResources = 3;

struct TerrainMaterial
{
    std::string Name;
    int MatCBIndex = -1;
    int DiffuseSrvHeapIndex = -1;
    XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
    float Roughness = 0.25f;
    XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
    int NumFramesDirty = gNumFrameResources;
};

class TerrainApp : public D3DApp
{
public:
    TerrainApp(HINSTANCE hInstance);
    ~TerrainApp();

    virtual bool Initialize() override;

private:
    virtual void OnResize() override;
    virtual void Update(const GameTimer& gt) override;
    virtual void Draw(const GameTimer& gt) override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

    void OnKeyboardInput(const GameTimer& gt);
    void UpdateCamera(const GameTimer& gt);
    void UpdateObjectCBs(const GameTimer& gt);
    void UpdateMainPassCB(const GameTimer& gt);
    void UpdateTerrainCB(const GameTimer& gt);
    void UpdateMaterialBuffer(const GameTimer& gt);

    void BuildRootSignature();
    void BuildDescriptorHeaps();
    void BuildShadersAndInputLayout();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildMaterials();
    
    void ExtractFrustumPlanes(XMFLOAT4* planes, const XMMATRIX& viewProj);
    void DrawTerrain();

    std::array<const CD3DX12_STATIC_SAMPLER_DESC, 2> GetStaticSamplers();

private:
    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    int mCurrFrameResourceIndex = 0;

    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

    std::unordered_map<std::string, std::unique_ptr<TerrainMaterial>> mMaterials;
    std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    // Terrain
    std::unique_ptr<Terrain> mTerrain;
    std::unique_ptr<QuadTree> mQuadTree;
    
    // Heightmap texture (loaded from DDS)
    ComPtr<ID3D12Resource> mHeightmapTexture;
    ComPtr<ID3D12Resource> mHeightmapUploadBuffer;
    
    // Diffuse/Color texture
    ComPtr<ID3D12Resource> mDiffuseTexture;
    
    // Normal map texture
    ComPtr<ID3D12Resource> mNormalTexture;
    
    // White texture fallback
    ComPtr<ID3D12Resource> mWhiteTexture;
    ComPtr<ID3D12Resource> mWhiteTextureUpload;

    PassConstants mMainPassCB;
    TerrainConstants mTerrainCB;
    Camera mCamera;

    bool mWireframe = false;
    bool mShowLODColors = false;
    
    POINT mLastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        TerrainApp theApp(hInstance);
        if (!theApp.Initialize())
            return 0;
        return theApp.Run();
    }
    catch (DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

TerrainApp::TerrainApp(HINSTANCE hInstance) : D3DApp(hInstance)
{
    mMainWndCaption = L"Terrain Demo - QuadTree LOD + Frustum Culling";
}

TerrainApp::~TerrainApp()
{
    if (md3dDevice != nullptr)
        FlushCommandQueue();
}

bool TerrainApp::Initialize()
{
    if (!D3DApp::Initialize())
        return false;

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    mCamera.SetPosition(0.0f, 150.0f, -200.0f);
    mCamera.LookAt(mCamera.GetPosition3f(), XMFLOAT3(0, 0, 0), XMFLOAT3(0, 1, 0));

    // Create terrain - will use DDS heightmap from TerrainDetails/003
    // Terrain size 512 units, height range 0-150
    mTerrain = std::make_unique<Terrain>(md3dDevice.Get(), mCommandList.Get(), 
                                          512.0f, 0.0f, 150.0f);
    
    // Try to load heightmap from DDS, fallback to procedural
    if (!mTerrain->LoadHeightmapDDS(L"TerrainDetails/003/Height_Out.dds", md3dDevice.Get(), mCommandList.Get()))
    {
        // Fallback: generate procedural heightmap
        mTerrain->GenerateProceduralHeightmap(512, 512, 4.0f, 6);
    }
    mTerrain->BuildGeometry(md3dDevice.Get(), mCommandList.Get());

    // Initialize QuadTree for LOD
    mQuadTree = std::make_unique<QuadTree>();
    mQuadTree->Initialize(512.0f, 32.0f, 5); // 512 terrain size, 32 min node, 5 LOD levels
    
    // Set LOD distances for smooth transitions
    std::vector<float> lodDistances = { 64.0f, 128.0f, 256.0f, 512.0f, 1024.0f };
    mQuadTree->SetLODDistances(lodDistances);

    BuildRootSignature();
    BuildDescriptorHeaps();
    BuildShadersAndInputLayout();
    BuildMaterials();
    BuildFrameResources();
    BuildPSOs();

    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
    FlushCommandQueue();

    return true;
}

void TerrainApp::OnResize()
{
    D3DApp::OnResize();
    mCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 2000.0f);
}


void TerrainApp::Update(const GameTimer& gt)
{
    OnKeyboardInput(gt);
    UpdateCamera(gt);

    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

    if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

    // Extract frustum planes for culling
    XMMATRIX view = mCamera.GetView();
    XMMATRIX proj = mCamera.GetProj();
    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    
    XMFLOAT4 frustumPlanes[6];
    ExtractFrustumPlanes(frustumPlanes, viewProj);
    
    // Update QuadTree with camera position and frustum
    mQuadTree->Update(mCamera.GetPosition3f(), frustumPlanes);

    UpdateObjectCBs(gt);
    UpdateMainPassCB(gt);
    UpdateTerrainCB(gt);
    UpdateMaterialBuffer(gt);
}

void TerrainApp::Draw(const GameTimer& gt)
{
    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;
    ThrowIfFailed(cmdListAlloc->Reset());

    if (mWireframe)
        ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["terrain_wireframe"].Get()));
    else
        ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["terrain"].Get()));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), 
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

    ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    // Set pass constants
    auto passCB = mCurrFrameResource->PassCB->Resource();
    mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

    // Set terrain constants
    auto terrainCB = mCurrFrameResource->TerrainCB->Resource();
    mCommandList->SetGraphicsRootConstantBufferView(2, terrainCB->GetGPUVirtualAddress());

    // Set textures
    CD3DX12_GPU_DESCRIPTOR_HANDLE texHandle(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    mCommandList->SetGraphicsRootDescriptorTable(3, texHandle);

    // Draw terrain
    DrawTerrain();

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    ThrowIfFailed(mCommandList->Close());

    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    mCurrFrameResource->Fence = ++mCurrentFence;
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void TerrainApp::DrawTerrain()
{
    auto geo = mTerrain->GetGeometry();
    
    mCommandList->IASetVertexBuffers(0, 1, &geo->VertexBufferView());
    mCommandList->IASetIndexBuffer(&geo->IndexBufferView());
    mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Get visible nodes from QuadTree
    std::vector<TerrainNode*> visibleNodes;
    mQuadTree->GetVisibleNodes(visibleNodes);

    auto objectCB = mCurrFrameResource->ObjectCB->Resource();
    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

    for (auto* node : visibleNodes)
    {
        // Set object constants
        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + 
                                                  node->ObjectCBIndex * objCBByteSize;
        mCommandList->SetGraphicsRootConstantBufferView(0, objCBAddress);

        // Select LOD mesh
        const char* lodMesh = Terrain::GetLODMeshName(node->LODLevel);
        auto& submesh = geo->DrawArgs[lodMesh];

        mCommandList->DrawIndexedInstanced(submesh.IndexCount, 1, 
            submesh.StartIndexLocation, submesh.BaseVertexLocation, 0);
    }
}

void TerrainApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;
    SetCapture(mhMainWnd);
}

void TerrainApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void TerrainApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if ((btnState & MK_LBUTTON) != 0)
    {
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));
        mCamera.Pitch(dy);
        mCamera.RotateY(dx);
    }
    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

void TerrainApp::OnKeyboardInput(const GameTimer& gt)
{
    const float dt = gt.DeltaTime();
    float speed = 50.0f;

    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
        speed *= 3.0f;

    if (GetAsyncKeyState('W') & 0x8000)
        mCamera.Walk(speed * dt);
    if (GetAsyncKeyState('S') & 0x8000)
        mCamera.Walk(-speed * dt);
    if (GetAsyncKeyState('A') & 0x8000)
        mCamera.Strafe(-speed * dt);
    if (GetAsyncKeyState('D') & 0x8000)
        mCamera.Strafe(speed * dt);
    if (GetAsyncKeyState('Q') & 0x8000)
        mCamera.SetPosition(mCamera.GetPosition3f().x, mCamera.GetPosition3f().y + speed * dt, mCamera.GetPosition3f().z);
    if (GetAsyncKeyState('E') & 0x8000)
        mCamera.SetPosition(mCamera.GetPosition3f().x, mCamera.GetPosition3f().y - speed * dt, mCamera.GetPosition3f().z);

    // Toggle wireframe
    static bool wKeyPressed = false;
    if (GetAsyncKeyState('1') & 0x8000)
    {
        if (!wKeyPressed) { mWireframe = !mWireframe; wKeyPressed = true; }
    }
    else { wKeyPressed = false; }
}

void TerrainApp::UpdateCamera(const GameTimer& gt)
{
    mCamera.UpdateViewMatrix();
}


void TerrainApp::UpdateObjectCBs(const GameTimer& gt)
{
    auto currObjectCB = mCurrFrameResource->ObjectCB.get();
    
    std::vector<TerrainNode*> visibleNodes;
    mQuadTree->GetVisibleNodes(visibleNodes);

    for (auto* node : visibleNodes)
    {
        // Create world matrix for this terrain tile
        XMMATRIX scale = XMMatrixScaling(node->Size, 1.0f, node->Size);
        XMMATRIX translation = XMMatrixTranslation(node->X, 0.0f, node->Z);
        XMMATRIX world = scale * translation;

        ObjectConstants objConstants;
        XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
        
        // Texture transform maps [0,1] to heightmap region
        float halfTerrain = mTerrain->GetTerrainSize() * 0.5f;
        float u0 = (node->X - node->Size * 0.5f + halfTerrain) / mTerrain->GetTerrainSize();
        float v0 = (node->Z - node->Size * 0.5f + halfTerrain) / mTerrain->GetTerrainSize();
        float uScale = node->Size / mTerrain->GetTerrainSize();
        float vScale = node->Size / mTerrain->GetTerrainSize();
        
        XMMATRIX texScale = XMMatrixScaling(uScale, vScale, 1.0f);
        XMMATRIX texTrans = XMMatrixTranslation(u0, v0, 0.0f);
        XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texScale * texTrans));
        
        objConstants.MaterialIndex = 0;
        objConstants.LODLevel = node->LODLevel;

        currObjectCB->CopyData(node->ObjectCBIndex, objConstants);
    }
}

void TerrainApp::UpdateMainPassCB(const GameTimer& gt)
{
    XMMATRIX view = mCamera.GetView();
    XMMATRIX proj = mCamera.GetProj();
    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

    XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
    
    mMainPassCB.EyePosW = mCamera.GetPosition3f();
    mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
    mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
    mMainPassCB.NearZ = 1.0f;
    mMainPassCB.FarZ = 2000.0f;
    mMainPassCB.TotalTime = gt.TotalTime();
    mMainPassCB.DeltaTime = gt.DeltaTime();
    mMainPassCB.AmbientLight = { 0.3f, 0.3f, 0.35f, 1.0f };

    // Directional light (sun)
    mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
    mMainPassCB.Lights[0].Strength = { 0.9f, 0.85f, 0.8f };

    // Extract frustum planes
    ExtractFrustumPlanes(mMainPassCB.FrustumPlanes, viewProj);

    auto currPassCB = mCurrFrameResource->PassCB.get();
    currPassCB->CopyData(0, mMainPassCB);
}

void TerrainApp::UpdateTerrainCB(const GameTimer& gt)
{
    mTerrainCB.MinHeight = mTerrain->GetMinHeight();
    mTerrainCB.MaxHeight = mTerrain->GetMaxHeight();
    mTerrainCB.TerrainSize = mTerrain->GetTerrainSize();
    mTerrainCB.TexelSize = 1.0f / mTerrain->GetHeightmapWidth();
    mTerrainCB.HeightMapSize = XMFLOAT2((float)mTerrain->GetHeightmapWidth(), 
                                         (float)mTerrain->GetHeightmapHeight());

    auto currTerrainCB = mCurrFrameResource->TerrainCB.get();
    currTerrainCB->CopyData(0, mTerrainCB);
}

void TerrainApp::UpdateMaterialBuffer(const GameTimer& gt)
{
    auto currMaterialBuffer = mCurrFrameResource->MaterialBuffer.get();
    for (auto& e : mMaterials)
    {
        TerrainMaterial* mat = e.second.get();
        if (mat->NumFramesDirty > 0)
        {
            MaterialData matData;
            matData.DiffuseAlbedo = mat->DiffuseAlbedo;
            matData.FresnelR0 = mat->FresnelR0;
            matData.Roughness = mat->Roughness;
            XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(XMLoadFloat4x4(&mat->MatTransform)));
            matData.DiffuseMapIndex = mat->DiffuseSrvHeapIndex;

            currMaterialBuffer->CopyData(mat->MatCBIndex, matData);
            mat->NumFramesDirty--;
        }
    }
}

void TerrainApp::ExtractFrustumPlanes(XMFLOAT4* planes, const XMMATRIX& viewProj)
{
    XMFLOAT4X4 M;
    XMStoreFloat4x4(&M, viewProj);

    // Left plane
    planes[0].x = M._14 + M._11;
    planes[0].y = M._24 + M._21;
    planes[0].z = M._34 + M._31;
    planes[0].w = M._44 + M._41;

    // Right plane
    planes[1].x = M._14 - M._11;
    planes[1].y = M._24 - M._21;
    planes[1].z = M._34 - M._31;
    planes[1].w = M._44 - M._41;

    // Bottom plane
    planes[2].x = M._14 + M._12;
    planes[2].y = M._24 + M._22;
    planes[2].z = M._34 + M._32;
    planes[2].w = M._44 + M._42;

    // Top plane
    planes[3].x = M._14 - M._12;
    planes[3].y = M._24 - M._22;
    planes[3].z = M._34 - M._32;
    planes[3].w = M._44 - M._42;

    // Near plane
    planes[4].x = M._13;
    planes[4].y = M._23;
    planes[4].z = M._33;
    planes[4].w = M._43;

    // Far plane
    planes[5].x = M._14 - M._13;
    planes[5].y = M._24 - M._23;
    planes[5].z = M._34 - M._33;
    planes[5].w = M._44 - M._43;

    // Normalize planes
    for (int i = 0; i < 6; ++i)
    {
        XMVECTOR p = XMLoadFloat4(&planes[i]);
        p = XMPlaneNormalize(p);
        XMStoreFloat4(&planes[i], p);
    }
}


void TerrainApp::BuildRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE texTable;
    texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, 0); // heightmap, diffuse, normal

    CD3DX12_ROOT_PARAMETER slotRootParameter[4];
    slotRootParameter[0].InitAsConstantBufferView(0); // Object CB
    slotRootParameter[1].InitAsConstantBufferView(1); // Pass CB
    slotRootParameter[2].InitAsConstantBufferView(2); // Terrain CB
    slotRootParameter[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_ALL);

    auto staticSamplers = GetStaticSamplers();

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
        (UINT)staticSamplers.size(), staticSamplers.data(),
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if (errorBlob != nullptr)
        OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    ThrowIfFailed(hr);

    ThrowIfFailed(md3dDevice->CreateRootSignature(0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void TerrainApp::BuildDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 3; // heightmap, diffuse/weathering, normal
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

    // Load textures from TerrainDetails/003
    
    // Load heightmap DDS
    HRESULT hr = DirectX::CreateDDSTextureFromFile12(
        md3dDevice.Get(), mCommandList.Get(), L"TerrainDetails/003/Height_Out.dds",
        mHeightmapTexture, mHeightmapUploadBuffer);
    
    if (FAILED(hr))
    {
        // Fallback: create procedural heightmap texture
        UINT width = mTerrain->GetHeightmapWidth();
        UINT height = mTerrain->GetHeightmapHeight();
        
        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.DepthOrArraySize = 1;
        texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_R32_FLOAT;
        texDesc.SampleDesc.Count = 1;
        texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        
        ThrowIfFailed(md3dDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
            &texDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&mHeightmapTexture)));
        
        const UINT64 uploadSize = GetRequiredIntermediateSize(mHeightmapTexture.Get(), 0, 1);
        ThrowIfFailed(md3dDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(uploadSize), D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&mHeightmapUploadBuffer)));
        
        std::vector<float> heightData(width * height);
        for (UINT z = 0; z < height; ++z)
        {
            for (UINT x = 0; x < width; ++x)
            {
                float worldX = (float)x / width * mTerrain->GetTerrainSize() - mTerrain->GetTerrainSize() * 0.5f;
                float worldZ = (float)z / height * mTerrain->GetTerrainSize() - mTerrain->GetTerrainSize() * 0.5f;
                float h = mTerrain->GetHeight(worldX, worldZ);
                heightData[z * width + x] = (h - mTerrain->GetMinHeight()) / (mTerrain->GetMaxHeight() - mTerrain->GetMinHeight());
            }
        }
        
        D3D12_SUBRESOURCE_DATA subData = {};
        subData.pData = heightData.data();
        subData.RowPitch = width * sizeof(float);
        subData.SlicePitch = subData.RowPitch * height;
        
        UpdateSubresources(mCommandList.Get(), mHeightmapTexture.Get(), mHeightmapUploadBuffer.Get(), 0, 0, 1, &subData);
        mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
            mHeightmapTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
    }
    
    // Load diffuse/weathering texture
    ComPtr<ID3D12Resource> diffuseUploadBuffer;
    hr = DirectX::CreateDDSTextureFromFile12(
        md3dDevice.Get(), mCommandList.Get(), L"TerrainDetails/003/Weathering_Out.dds",
        mDiffuseTexture, diffuseUploadBuffer);
    
    // Load normal map texture
    ComPtr<ID3D12Resource> normalUploadBuffer;
    hr = DirectX::CreateDDSTextureFromFile12(
        md3dDevice.Get(), mCommandList.Get(), L"TerrainDetails/003/Normals_Out.dds",
        mNormalTexture, normalUploadBuffer);
    
    // Create white texture fallback
    D3D12_RESOURCE_DESC whiteTexDesc = {};
    whiteTexDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    whiteTexDesc.Width = 1;
    whiteTexDesc.Height = 1;
    whiteTexDesc.DepthOrArraySize = 1;
    whiteTexDesc.MipLevels = 1;
    whiteTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    whiteTexDesc.SampleDesc.Count = 1;
    whiteTexDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
        &whiteTexDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&mWhiteTexture)));

    const UINT64 whiteUploadSize = GetRequiredIntermediateSize(mWhiteTexture.Get(), 0, 1);
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(whiteUploadSize), D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&mWhiteTextureUpload)));

    UINT whitePixel = 0xFFFFFFFF;
    D3D12_SUBRESOURCE_DATA whiteData = {};
    whiteData.pData = &whitePixel;
    whiteData.RowPitch = 4;
    whiteData.SlicePitch = 4;

    UpdateSubresources(mCommandList.Get(), mWhiteTexture.Get(), mWhiteTextureUpload.Get(), 0, 0, 1, &whiteData);
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        mWhiteTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

    // Create SRVs
    CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = -1; // All mips
    srvDesc.Texture2D.MostDetailedMip = 0;

    // Heightmap SRV
    D3D12_RESOURCE_DESC hmDesc = mHeightmapTexture->GetDesc();
    srvDesc.Format = hmDesc.Format;
    srvDesc.Texture2D.MipLevels = hmDesc.MipLevels;
    md3dDevice->CreateShaderResourceView(mHeightmapTexture.Get(), &srvDesc, hDescriptor);
    hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);

    // Diffuse SRV
    if (mDiffuseTexture)
    {
        D3D12_RESOURCE_DESC diffDesc = mDiffuseTexture->GetDesc();
        srvDesc.Format = diffDesc.Format;
        srvDesc.Texture2D.MipLevels = diffDesc.MipLevels;
        md3dDevice->CreateShaderResourceView(mDiffuseTexture.Get(), &srvDesc, hDescriptor);
    }
    else
    {
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.Texture2D.MipLevels = 1;
        md3dDevice->CreateShaderResourceView(mWhiteTexture.Get(), &srvDesc, hDescriptor);
    }
    hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);

    // Normal SRV
    if (mNormalTexture)
    {
        D3D12_RESOURCE_DESC normDesc = mNormalTexture->GetDesc();
        srvDesc.Format = normDesc.Format;
        srvDesc.Texture2D.MipLevels = normDesc.MipLevels;
        md3dDevice->CreateShaderResourceView(mNormalTexture.Get(), &srvDesc, hDescriptor);
    }
    else
    {
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.Texture2D.MipLevels = 1;
        md3dDevice->CreateShaderResourceView(mWhiteTexture.Get(), &srvDesc, hDescriptor);
    }
}

void TerrainApp::BuildShadersAndInputLayout()
{
    mShaders["terrainVS"] = d3dUtil::CompileShader(L"Shaders\\Terrain.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["terrainPS"] = d3dUtil::CompileShader(L"Shaders\\Terrain.hlsl", nullptr, "PS", "ps_5_1");
    mShaders["terrainWirePS"] = d3dUtil::CompileShader(L"Shaders\\Terrain.hlsl", nullptr, "PS_Wireframe", "ps_5_1");

    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
}

void TerrainApp::BuildPSOs()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS = { mShaders["terrainVS"]->GetBufferPointer(), mShaders["terrainVS"]->GetBufferSize() };
    psoDesc.PS = { mShaders["terrainPS"]->GetBufferPointer(), mShaders["terrainPS"]->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = mBackBufferFormat;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.DSVFormat = mDepthStencilFormat;

    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs["terrain"])));

    // Wireframe PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC wirePsoDesc = psoDesc;
    wirePsoDesc.PS = { mShaders["terrainWirePS"]->GetBufferPointer(), mShaders["terrainWirePS"]->GetBufferSize() };
    wirePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&wirePsoDesc, IID_PPV_ARGS(&mPSOs["terrain_wireframe"])));
}

void TerrainApp::BuildFrameResources()
{
    for (int i = 0; i < gNumFrameResources; ++i)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(
            md3dDevice.Get(), 1, 1024, (UINT)mMaterials.size()));
    }
}

void TerrainApp::BuildMaterials()
{
    auto terrain = std::make_unique<TerrainMaterial>();
    terrain->Name = "terrain";
    terrain->MatCBIndex = 0;
    terrain->DiffuseSrvHeapIndex = 1;
    terrain->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    terrain->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    terrain->Roughness = 0.9f;
    mMaterials["terrain"] = std::move(terrain);
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 2> TerrainApp::GetStaticSamplers()
{
    const CD3DX12_STATIC_SAMPLER_DESC linearWrap(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP);

    const CD3DX12_STATIC_SAMPLER_DESC linearClamp(1, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    return { linearWrap, linearClamp };
}
