#include "D3D12Adapter.h"
#include "../Utils/Logger.h"
#include "../Resources/Mesh.h"
#include <d3dcompiler.h>
#include <stdexcept>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

// Структура вершины для примитивов (треугольник, квадрат, куб)
struct SimpleVertex {
    XMFLOAT3 position;
    XMFLOAT4 color;
};

D3D12Adapter::D3D12Adapter()
    : m_frameIndex(0)
    , m_rtvDescriptorSize(0)
    , m_dsvDescriptorSize(0)
    , m_cbvDescriptorSize(0)
    , m_width(0)
    , m_height(0)
    , m_fenceEvent(nullptr)
    , m_cbvDataBegin(nullptr)
    , m_currentObjectIndex(0)
    , m_initialized(false) {
    
    m_clearColor[0] = 0.2f;
    m_clearColor[1] = 0.3f;
    m_clearColor[2] = 0.4f;
    m_clearColor[3] = 1.0f;
    
    for (UINT i = 0; i < FrameCount; i++) {
        m_fenceValues[i] = 0;
    }
    
    m_modelMatrix = XMMatrixIdentity();
    m_viewMatrix = XMMatrixIdentity();
    m_projectionMatrix = XMMatrixIdentity();
    m_currentColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    
    XMStoreFloat4x4(&m_objectConstants.worldViewProj, XMMatrixIdentity());
    m_objectConstants.objectColor = m_currentColor;
}

D3D12Adapter::~D3D12Adapter() {
    Shutdown();
}

bool D3D12Adapter::Initialize(HWND hwnd, int width, int height) {
    LOG_INFO("Initializing DirectX 12 Adapter (Resource Manager version)...");
    
    m_width = width;
    m_height = height;
    
    if (!CreateDevice()) return false;
    if (!CreateCommandObjects()) return false;
    if (!CreateSwapChain(hwnd, width, height)) return false;
    if (!CreateRtvDescriptorHeap()) return false;
    if (!CreateDsvDescriptorHeap()) return false;
    if (!CreateRenderTargets()) return false;
    if (!CreateDepthStencil()) return false;
    if (!CreateDefaultRootSignature()) return false;
    if (!CreateDefaultPipelineState()) return false;
    if (!CreateGeometry()) return false;
    
    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width = static_cast<float>(width);
    m_viewport.Height = static_cast<float>(height);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;
    
    m_scissorRect.left = 0;
    m_scissorRect.top = 0;
    m_scissorRect.right = width;
    m_scissorRect.bottom = height;
    
    // Установка проекционной матрицы по умолчанию
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    m_projectionMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV4, aspectRatio, 0.1f, 100.0f);
    
    // Установка матрицы вида по умолчанию
    XMVECTOR eye = XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f);
    XMVECTOR at = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    m_viewMatrix = XMMatrixLookAtLH(eye, at, up);
    
    m_initialized = true;
    LOG_INFO("DirectX 12 Adapter initialized successfully");
    return true;
}

void D3D12Adapter::Shutdown() {
    if (!m_initialized) return;
    
    WaitForGpu();
    
    if (m_fenceEvent) {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }
    
    m_initialized = false;
    LOG_INFO("DirectX 12 Adapter shut down");
}

bool D3D12Adapter::CreateDevice() {
    UINT dxgiFactoryFlags = 0;
    
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif
    
    if (FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory)))) {
        LOG_ERROR("Failed to create DXGI factory");
        return false;
    }
    
    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)))) {
        LOG_ERROR("Failed to create D3D12 device");
        return false;
    }
    
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_cbvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    
    return true;
}

bool D3D12Adapter::CreateCommandObjects() {
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    
    if (FAILED(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)))) {
        LOG_ERROR("Failed to create command queue");
        return false;
    }
    
    for (UINT i = 0; i < FrameCount; i++) {
        if (FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, 
            IID_PPV_ARGS(&m_commandAllocators[i])))) {
            LOG_ERROR("Failed to create command allocator");
            return false;
        }
    }
    
    if (FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, 
        m_commandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&m_commandList)))) {
        LOG_ERROR("Failed to create command list");
        return false;
    }
    
    m_commandList->Close();
    
    if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)))) {
        LOG_ERROR("Failed to create fence");
        return false;
    }
    
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_fenceEvent) {
        LOG_ERROR("Failed to create fence event");
        return false;
    }
    
    return true;
}

bool D3D12Adapter::CreateSwapChain(HWND hwnd, int width, int height) {
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    
    ComPtr<IDXGISwapChain1> swapChain;
    if (FAILED(m_factory->CreateSwapChainForHwnd(m_commandQueue.Get(), hwnd, 
        &swapChainDesc, nullptr, nullptr, &swapChain))) {
        LOG_ERROR("Failed to create swap chain");
        return false;
    }
    
    if (FAILED(swapChain.As(&m_swapChain))) {
        LOG_ERROR("Failed to cast swap chain");
        return false;
    }
    
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    
    return true;
}

bool D3D12Adapter::CreateRtvDescriptorHeap() {
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = FrameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    
    if (FAILED(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)))) {
        LOG_ERROR("Failed to create RTV descriptor heap");
        return false;
    }
    
    return true;
}

bool D3D12Adapter::CreateDsvDescriptorHeap() {
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    
    if (FAILED(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)))) {
        LOG_ERROR("Failed to create DSV descriptor heap");
        return false;
    }
    
    // Создаем heap для CBV с достаточным количеством дескрипторов
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
    cbvHeapDesc.NumDescriptors = MaxObjectsPerFrame;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    
    if (FAILED(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap)))) {
        LOG_ERROR("Failed to create CBV descriptor heap");
        return false;
    }
    
    return true;
}

bool D3D12Adapter::CreateRenderTargets() {
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    
    for (UINT i = 0; i < FrameCount; i++) {
        if (FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])))) {
            LOG_ERROR("Failed to get swap chain buffer");
            return false;
        }
        
        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += m_rtvDescriptorSize;
    }
    
    return true;
}

bool D3D12Adapter::CreateDepthStencil() {
    D3D12_RESOURCE_DESC depthStencilDesc = {};
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Width = m_width;
    depthStencilDesc.Height = m_height;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    
    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;
    
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    
    if (FAILED(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, 
        &depthStencilDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, 
        IID_PPV_ARGS(&m_depthStencil)))) {
        LOG_ERROR("Failed to create depth stencil buffer");
        return false;
    }
    
    m_device->CreateDepthStencilView(m_depthStencil.Get(), nullptr, 
        m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
    
    return true;
}

bool D3D12Adapter::CreateDefaultRootSignature() {
    D3D12_DESCRIPTOR_RANGE cbvTable = {};
    cbvTable.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    cbvTable.NumDescriptors = 1;
    cbvTable.BaseShaderRegister = 0;
    cbvTable.RegisterSpace = 0;
    cbvTable.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    
    D3D12_ROOT_PARAMETER rootParameter = {};
    rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameter.DescriptorTable.NumDescriptorRanges = 1;
    rootParameter.DescriptorTable.pDescriptorRanges = &cbvTable;
    
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 1;
    rootSigDesc.pParameters = &rootParameter;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    
    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    
    if (FAILED(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, 
        &signature, &error))) {
        if (error) {
            LOG_ERROR("Failed to serialize root signature: " + 
                std::string((char*)error->GetBufferPointer()));
        }
        return false;
    }
    
    if (FAILED(m_device->CreateRootSignature(0, signature->GetBufferPointer(), 
        signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)))) {
        LOG_ERROR("Failed to create root signature");
        return false;
    }
    
    return true;
}

bool D3D12Adapter::CreateDefaultPipelineState() {
    // Шейдер для загруженных мешей (с нормалями)
    const char* shaderCode = R"(
        cbuffer ObjectConstants : register(b0)
        {
            float4x4 gWorldViewProj;
            float4 gObjectColor;
        };
        
        struct VertexIn
        {
            float3 PosL : POSITION;
            float3 Normal : NORMAL;
            float2 TexCoord : TEXCOORD;
        };
        
        struct VertexOut
        {
            float4 PosH : SV_POSITION;
            float3 Normal : NORMAL;
            float4 Color : COLOR;
        };
        
        VertexOut VS(VertexIn vin)
        {
            VertexOut vout;
            vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
            vout.Normal = vin.Normal;
            
            // Простое освещение
            float3 lightDir = normalize(float3(1.0f, 1.0f, -1.0f));
            float diffuse = max(dot(vin.Normal, lightDir), 0.3f);
            vout.Color = gObjectColor * diffuse;
            
            return vout;
        }
        
        float4 PS(VertexOut pin) : SV_Target
        {
            return pin.Color;
        }
    )";
    
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    ComPtr<ID3DBlob> errors;
    
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    
    if (FAILED(D3DCompile(shaderCode, strlen(shaderCode), nullptr, nullptr, nullptr, 
        "VS", "vs_5_0", compileFlags, 0, &vertexShader, &errors))) {
        if (errors) {
            LOG_ERROR("Vertex shader compilation failed: " + 
                std::string((char*)errors->GetBufferPointer()));
        }
        return false;
    }
    
    if (FAILED(D3DCompile(shaderCode, strlen(shaderCode), nullptr, nullptr, nullptr, 
        "PS", "ps_5_0", compileFlags, 0, &pixelShader, &errors))) {
        if (errors) {
            LOG_ERROR("Pixel shader compilation failed: " + 
                std::string((char*)errors->GetBufferPointer()));
        }
        return false;
    }
    
    // Input layout для загруженных мешей (position, normal, texcoord)
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, 
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, 
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, 
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
    psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.SampleDesc.Count = 1;
    
    if (FAILED(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)))) {
        LOG_ERROR("Failed to create pipeline state");
        return false;
    }
    
    return true;
}

bool D3D12Adapter::CreateGeometry() {
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    
    D3D12_RANGE readRange = { 0, 0 };
    
    // Константный буфер - достаточно большой для всех объектов
    UINT constantBufferSize = (sizeof(ObjectConstants) + 255) & ~255;
    UINT totalBufferSize = constantBufferSize * MaxObjectsPerFrame;
    resourceDesc.Width = totalBufferSize;
    
    if (FAILED(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, 
        &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, 
        IID_PPV_ARGS(&m_constantBuffer)))) {
        LOG_ERROR("Failed to create constant buffer");
        return false;
    }
    
    // Создаем CBV для каждого слота
    D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle = m_cbvHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_VIRTUAL_ADDRESS cbvAddress = m_constantBuffer->GetGPUVirtualAddress();
    
    for (UINT i = 0; i < MaxObjectsPerFrame; i++) {
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = cbvAddress + (i * constantBufferSize);
        cbvDesc.SizeInBytes = constantBufferSize;
        
        m_device->CreateConstantBufferView(&cbvDesc, cbvHandle);
        cbvHandle.ptr += m_cbvDescriptorSize;
    }
    
    m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_cbvDataBegin));
    
    LOG_INFO("Created constant buffer for " + std::to_string(MaxObjectsPerFrame) + " objects");
    
    return true;
}

void D3D12Adapter::UpdateConstantBuffer() {
    UINT constantBufferSize = (sizeof(ObjectConstants) + 255) & ~255;
    UINT offset = m_currentObjectIndex * constantBufferSize;
    
    XMMATRIX worldViewProj = m_modelMatrix * m_viewMatrix * m_projectionMatrix;
    XMStoreFloat4x4(&m_objectConstants.worldViewProj, XMMatrixTranspose(worldViewProj));
    m_objectConstants.objectColor = m_currentColor;
    
    memcpy(m_cbvDataBegin + offset, &m_objectConstants, sizeof(ObjectConstants));
    
    D3D12_GPU_DESCRIPTOR_HANDLE cbvHandle = m_cbvHeap->GetGPUDescriptorHandleForHeapStart();
    cbvHandle.ptr += m_currentObjectIndex * m_cbvDescriptorSize;
    m_commandList->SetGraphicsRootDescriptorTable(0, cbvHandle);
    
    m_currentObjectIndex++;
    if (m_currentObjectIndex >= MaxObjectsPerFrame) {
        LOG_WARNING("Too many objects in frame! Max is " + std::to_string(MaxObjectsPerFrame));
        m_currentObjectIndex = MaxObjectsPerFrame - 1;
    }
}

void D3D12Adapter::BeginFrame() {
    m_currentObjectIndex = 0;
    
    m_commandAllocators[m_frameIndex]->Reset();
    m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), m_pipelineState.Get());
    
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    
    ID3D12DescriptorHeap* ppHeaps[] = { m_cbvHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    
    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);
    
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    
    m_commandList->ResourceBarrier(1, &barrier);
    
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += m_frameIndex * m_rtvDescriptorSize;
    
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
    
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    m_commandList->ClearRenderTargetView(rtvHandle, m_clearColor, 0, nullptr);
    m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 
        1.0f, 0, 0, nullptr);
}

void D3D12Adapter::EndFrame() {
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    
    m_commandList->ResourceBarrier(1, &barrier);
    m_commandList->Close();
    
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    
    m_swapChain->Present(1, 0);
    
    MoveToNextFrame();
}

void D3D12Adapter::Clear(float r, float g, float b, float a) {
    m_clearColor[0] = r;
    m_clearColor[1] = g;
    m_clearColor[2] = b;
    m_clearColor[3] = a;
}

void D3D12Adapter::SetModelMatrix(const XMMATRIX& model) {
    m_modelMatrix = model;
}

void D3D12Adapter::SetViewMatrix(const XMMATRIX& view) {
    m_viewMatrix = view;
}

void D3D12Adapter::SetProjectionMatrix(const XMMATRIX& projection) {
    m_projectionMatrix = projection;
}

void D3D12Adapter::SetColor(float r, float g, float b, float a) {
    m_currentColor = XMFLOAT4(r, g, b, a);
}

// НОВЫЕ МЕТОДЫ ДЛЯ РЕСУРСОВ

ID3D12Resource* D3D12Adapter::CreateVertexBuffer(const void* data, size_t size) {
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = size;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    
    ID3D12Resource* buffer = nullptr;
    if (FAILED(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, 
        &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, 
        IID_PPV_ARGS(&buffer)))) {
        LOG_ERROR("Failed to create vertex buffer");
        return nullptr;
    }
    
    // Копируем данные
    D3D12_RANGE readRange = { 0, 0 };
    UINT8* pDataBegin;
    buffer->Map(0, &readRange, reinterpret_cast<void**>(&pDataBegin));
    memcpy(pDataBegin, data, size);
    buffer->Unmap(0, nullptr);
    
    LOG_DEBUG("Created vertex buffer: " + std::to_string(size) + " bytes");
    return buffer;
}

ID3D12Resource* D3D12Adapter::CreateIndexBuffer(const void* data, size_t size) {
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = size;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    
    ID3D12Resource* buffer = nullptr;
    if (FAILED(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, 
        &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, 
        IID_PPV_ARGS(&buffer)))) {
        LOG_ERROR("Failed to create index buffer");
        return nullptr;
    }
    
    // Копируем данные
    D3D12_RANGE readRange = { 0, 0 };
    UINT8* pDataBegin;
    buffer->Map(0, &readRange, reinterpret_cast<void**>(&pDataBegin));
    memcpy(pDataBegin, data, size);
    buffer->Unmap(0, nullptr);
    
    LOG_DEBUG("Created index buffer: " + std::to_string(size) + " bytes");
    return buffer;
}

ID3D12Resource* D3D12Adapter::CreateTexture2D(int width, int height, int channels, const void* pixels) {
    LOG_DEBUG("CreateTexture2D called (stub) - " + std::to_string(width) + "x" + std::to_string(height));
    // TODO: Полная реализация создания текстуры
    return nullptr;
}

ID3D12PipelineState* D3D12Adapter::CompileShader(const std::string& vertexShader, const std::string& pixelShader) {
    LOG_DEBUG("CompileShader called (stub)");
    // TODO: Компиляция кастомных шейдеров
    // Пока используем дефолтный PSO
    return m_pipelineState.Get();
}

ID3D12RootSignature* D3D12Adapter::CreateRootSignature() {
    LOG_DEBUG("CreateRootSignature called - returning default");
    return m_rootSignature.Get();
}

void D3D12Adapter::DrawMesh(ID3D12Resource* vertexBuffer, ID3D12Resource* indexBuffer, 
                           size_t indexCount, ID3D12PipelineState* pipelineState) {
    if (!vertexBuffer || !indexBuffer) {
        LOG_WARNING("DrawMesh called with null buffers");
        return;
    }
    
    UpdateConstantBuffer();
    
    // Создаем vertex buffer view
    D3D12_VERTEX_BUFFER_VIEW vbView = {};
    vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vbView.StrideInBytes = sizeof(Vertex);  // position(12) + normal(12) + uv(8) = 32 bytes
    vbView.SizeInBytes = static_cast<UINT>(vertexBuffer->GetDesc().Width);
    
    // Создаем index buffer view
    D3D12_INDEX_BUFFER_VIEW ibView = {};
    ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    ibView.Format = DXGI_FORMAT_R32_UINT;
    ibView.SizeInBytes = static_cast<UINT>(indexBuffer->GetDesc().Width);
    
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->IASetVertexBuffers(0, 1, &vbView);
    m_commandList->IASetIndexBuffer(&ibView);
    m_commandList->DrawIndexedInstanced(static_cast<UINT>(indexCount), 1, 0, 0, 0);
}

void D3D12Adapter::SetTexture(ID3D12Resource* texture) {
    LOG_DEBUG("SetTexture called (stub)");
    // TODO: Установка текстуры
}

// СТАРЫЕ МЕТОДЫ ДЛЯ СОВМЕСТИМОСТИ

void D3D12Adapter::DrawTriangle() {
    LOG_DEBUG("DrawTriangle called (not implemented for mesh renderer)");
}

void D3D12Adapter::DrawQuad() {
    LOG_DEBUG("DrawQuad called (not implemented for mesh renderer)");
}

void D3D12Adapter::DrawCube() {
    LOG_DEBUG("DrawCube called (not implemented for mesh renderer)");
}

void D3D12Adapter::SetObjectTransform(float x, float y, float scale, float rotation) {
    XMMATRIX scaleMatrix = XMMatrixScaling(scale, scale, 1.0f);
    XMMATRIX rotationMatrix = XMMatrixRotationZ(rotation);
    XMMATRIX translationMatrix = XMMatrixTranslation(x, y, 0.0f);
    
    m_modelMatrix = scaleMatrix * rotationMatrix * translationMatrix;
}

void D3D12Adapter::DrawUIRect(float x, float y, float width, float height, float r, float g, float b, float a) {
    LOG_DEBUG("DrawUIRect called (not implemented)");
}

void D3D12Adapter::WaitForGpu() {
    const UINT64 fence = m_fenceValues[m_frameIndex];
    m_commandQueue->Signal(m_fence.Get(), fence);
    
    if (m_fence->GetCompletedValue() < fence) {
        m_fence->SetEventOnCompletion(fence, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

void D3D12Adapter::MoveToNextFrame() {
    const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
    m_commandQueue->Signal(m_fence.Get(), currentFenceValue);
    
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    
    if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex]) {
        m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
    
    m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}
