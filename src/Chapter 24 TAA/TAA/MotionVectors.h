//***************************************************************************************
// MotionVectors.h - Motion vector buffer for TAA
//***************************************************************************************

#pragma once

#include "../../Common/d3dUtil.h"

class MotionVectors
{
public:
    MotionVectors(ID3D12Device* device, UINT width, UINT height);
    
    MotionVectors(const MotionVectors& rhs) = delete;
    MotionVectors& operator=(const MotionVectors& rhs) = delete;
    ~MotionVectors() = default;

    UINT Width() const;
    UINT Height() const;
    ID3D12Resource* Resource();
    
    CD3DX12_GPU_DESCRIPTOR_HANDLE Srv() const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE Rtv() const;

    void BuildDescriptors(
        CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
        CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
        CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv);

    void OnResize(UINT newWidth, UINT newHeight);

private:
    void BuildDescriptors();
    void BuildResource();

private:
    ID3D12Device* md3dDevice = nullptr;

    UINT mWidth = 0;
    UINT mHeight = 0;
    DXGI_FORMAT mFormat = DXGI_FORMAT_R16G16_FLOAT; // RG for velocity

    CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuRtv;

    Microsoft::WRL::ComPtr<ID3D12Resource> mMotionVectorMap = nullptr;
};
