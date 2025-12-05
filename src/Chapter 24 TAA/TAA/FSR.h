//***************************************************************************************
// FSR.h - AMD FidelityFX Super Resolution 1.0
//
// FSR is a spatial upscaling technology that produces high quality upscaled images
// from lower resolution input. It works on any GPU (AMD, NVIDIA, Intel).
//
// Quality modes:
// - Ultra Quality: 1.3x scale (77% of native resolution)
// - Quality: 1.5x scale (67% of native resolution)
// - Balanced: 1.7x scale (59% of native resolution)
// - Performance: 2.0x scale (50% of native resolution)
//***************************************************************************************

#pragma once

#include "../../Common/d3dUtil.h"

enum class FSRQualityMode
{
    UltraQuality,  // 1.3x scale
    Quality,       // 1.5x scale
    Balanced,      // 1.7x scale
    Performance    // 2.0x scale
};

struct FSRConstants
{
    DirectX::XMFLOAT4 Const0;
    DirectX::XMFLOAT4 Const1;
    DirectX::XMFLOAT4 Const2;
    DirectX::XMFLOAT4 Const3;
    float RCASSharpness;
    DirectX::XMFLOAT3 Padding;
};

class FSR
{
public:
    FSR(ID3D12Device* device, UINT outputWidth, UINT outputHeight, 
        DXGI_FORMAT format, FSRQualityMode quality = FSRQualityMode::Quality);
    
    FSR(const FSR& rhs) = delete;
    FSR& operator=(const FSR& rhs) = delete;
    ~FSR() = default;

    // Get render resolution (lower than output)
    UINT RenderWidth() const { return mRenderWidth; }
    UINT RenderHeight() const { return mRenderHeight; }
    
    // Get output resolution
    UINT OutputWidth() const { return mOutputWidth; }
    UINT OutputHeight() const { return mOutputHeight; }
    
    // Get scale factor
    float GetScaleFactor() const;
    
    // Get quality mode
    FSRQualityMode GetQualityMode() const { return mQualityMode; }
    void SetQualityMode(FSRQualityMode mode);
    
    // Get sharpness (0.0 = max, 2.0 = none)
    float GetSharpness() const { return mSharpness; }
    void SetSharpness(float sharpness) { mSharpness = clamp(sharpness, 0.0f, 2.0f); }
    
    // Get FSR constants for shader
    FSRConstants GetConstants() const;
    
    // Resources
    ID3D12Resource* IntermediateResource() { return mIntermediateBuffer.Get(); }
    
    CD3DX12_GPU_DESCRIPTOR_HANDLE IntermediateSrv() const { return mhIntermediateGpuSrv; }
    CD3DX12_CPU_DESCRIPTOR_HANDLE IntermediateRtv() const { return mhIntermediateCpuRtv; }
    
    void BuildDescriptors(
        CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
        CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
        CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv);
    
    void OnResize(UINT outputWidth, UINT outputHeight);

private:
    void CalculateRenderResolution();
    void BuildResource();
    void BuildDescriptors();
    
    static float clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

private:
    ID3D12Device* md3dDevice = nullptr;
    
    UINT mOutputWidth = 0;
    UINT mOutputHeight = 0;
    UINT mRenderWidth = 0;
    UINT mRenderHeight = 0;
    
    DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    FSRQualityMode mQualityMode = FSRQualityMode::Quality;
    float mSharpness = 0.0f; // Default sharpness (0.0 = max, 1.0 = none)
    
    CD3DX12_CPU_DESCRIPTOR_HANDLE mhIntermediateCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mhIntermediateGpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mhIntermediateCpuRtv;
    
    Microsoft::WRL::ComPtr<ID3D12Resource> mIntermediateBuffer = nullptr;
};
