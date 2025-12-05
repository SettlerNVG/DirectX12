//***************************************************************************************
// FSR.cpp - AMD FidelityFX Super Resolution 1.0 implementation
//***************************************************************************************

#include "FSR.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

FSR::FSR(ID3D12Device* device, UINT outputWidth, UINT outputHeight, 
         DXGI_FORMAT format, FSRQualityMode quality)
{
    md3dDevice = device;
    mOutputWidth = outputWidth;
    mOutputHeight = outputHeight;
    mFormat = format;
    mQualityMode = quality;
    
    CalculateRenderResolution();
    BuildResource();
}

float FSR::GetScaleFactor() const
{
    switch (mQualityMode)
    {
    case FSRQualityMode::UltraQuality: return 1.3f;
    case FSRQualityMode::Quality:      return 1.5f;
    case FSRQualityMode::Balanced:     return 1.7f;
    case FSRQualityMode::Performance:  return 2.0f;
    default: return 1.5f;
    }
}

void FSR::SetQualityMode(FSRQualityMode mode)
{
    if (mQualityMode != mode)
    {
        mQualityMode = mode;
        CalculateRenderResolution();
        BuildResource();
    }
}

FSRConstants FSR::GetConstants() const
{
    FSRConstants constants;
    
    float inputWidth = (float)mRenderWidth;
    float inputHeight = (float)mRenderHeight;
    float outputWidth = (float)mOutputWidth;
    float outputHeight = (float)mOutputHeight;
    
    // Const0: scaling factors
    constants.Const0 = XMFLOAT4(
        inputWidth / outputWidth,
        inputHeight / outputHeight,
        0.5f * inputWidth / outputWidth - 0.5f,
        0.5f * inputHeight / outputHeight - 0.5f
    );
    
    // Const1: texel sizes
    constants.Const1 = XMFLOAT4(
        1.0f / inputWidth,
        1.0f / inputHeight,
        1.0f / outputWidth,
        1.0f / outputHeight
    );
    
    // Const2: additional sampling offsets
    constants.Const2 = XMFLOAT4(
        -1.0f / inputWidth,
        2.0f / inputHeight,
        1.0f / inputWidth,
        2.0f / inputHeight
    );
    
    // Const3: more offsets
    constants.Const3 = XMFLOAT4(
        0.0f,
        4.0f / inputHeight,
        0.0f,
        0.0f
    );
    
    constants.RCASSharpness = mSharpness;
    constants.Padding = XMFLOAT3(0.0f, 0.0f, 0.0f);
    
    return constants;
}

void FSR::BuildDescriptors(
    CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
    CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
    CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv)
{
    mhIntermediateCpuSrv = hCpuSrv;
    mhIntermediateGpuSrv = hGpuSrv;
    mhIntermediateCpuRtv = hCpuRtv;
    
    BuildDescriptors();
}

void FSR::OnResize(UINT outputWidth, UINT outputHeight)
{
    if (mOutputWidth != outputWidth || mOutputHeight != outputHeight)
    {
        mOutputWidth = outputWidth;
        mOutputHeight = outputHeight;
        CalculateRenderResolution();
        BuildResource();
        BuildDescriptors();
    }
}

void FSR::CalculateRenderResolution()
{
    float scale = GetScaleFactor();
    mRenderWidth = (UINT)(mOutputWidth / scale);
    mRenderHeight = (UINT)(mOutputHeight / scale);
    
    // Ensure minimum resolution
    mRenderWidth = max(mRenderWidth, 1U);
    mRenderHeight = max(mRenderHeight, 1U);
}

void FSR::BuildResource()
{
    // Create intermediate buffer for EASU output (at output resolution)
    D3D12_RESOURCE_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = mOutputWidth;
    texDesc.Height = mOutputHeight;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = mFormat;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    CD3DX12_CLEAR_VALUE optClear(mFormat, clearColor);

    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &optClear,
        IID_PPV_ARGS(&mIntermediateBuffer)));
}

void FSR::BuildDescriptors()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = mFormat;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = mFormat;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;

    md3dDevice->CreateShaderResourceView(mIntermediateBuffer.Get(), &srvDesc, mhIntermediateCpuSrv);
    md3dDevice->CreateRenderTargetView(mIntermediateBuffer.Get(), &rtvDesc, mhIntermediateCpuRtv);
}
