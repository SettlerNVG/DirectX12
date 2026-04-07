#pragma once

#include "RenderAdapter.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <memory>

using Microsoft::WRL::ComPtr;

struct Vertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
};

struct ObjectConstants {
    DirectX::XMFLOAT4X4 worldMatrix;
};

class D3D12Adapter : public RenderAdapter {
public:
    D3D12Adapter();
    ~D3D12Adapter() override;
    
    bool Initialize(HWND hwnd, int width, int height) override;
    void Shutdown() override;
    
    void BeginFrame() override;
    void EndFrame() override;
    
    void Clear(float r, float g, float b, float a) override;
    void DrawTriangle() override;
    void DrawQuad() override;
    
    void SetObjectTransform(float x, float y, float scale, float rotation) override;
    
    // UI методы
    void DrawUIRect(float x, float y, float width, float height, float r, float g, float b, float a);

private:
    bool CreateDevice();
    bool CreateCommandObjects();
    bool CreateSwapChain(HWND hwnd, int width, int height);
    bool CreateRtvDescriptorHeap();
    bool CreateDsvDescriptorHeap();
    bool CreateRenderTargets();
    bool CreateDepthStencil();
    bool CreateRootSignature();
    bool CreatePipelineState();
    bool CreateGeometry();
    
    void WaitForGpu();
    void MoveToNextFrame();
    
    static const UINT FrameCount = 2;
    UINT m_frameIndex;
    
    ComPtr<ID3D12Device> m_device;
    ComPtr<IDXGIFactory4> m_factory;
    ComPtr<IDXGISwapChain3> m_swapChain;
    
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12CommandAllocator> m_commandAllocators[FrameCount];
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    ComPtr<ID3D12DescriptorHeap> m_cbvHeap;
    
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    ComPtr<ID3D12Resource> m_depthStencil;
    
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    
    ComPtr<ID3D12Resource> m_vertexBuffer;
    ComPtr<ID3D12Resource> m_indexBuffer;
    ComPtr<ID3D12Resource> m_constantBuffer;
    
    // UI буферы
    ComPtr<ID3D12Resource> m_uiVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_uiVertexBufferView;
    UINT8* m_uiVertexDataBegin;
    
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
    
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValues[FrameCount];
    HANDLE m_fenceEvent;
    
    UINT m_rtvDescriptorSize;
    UINT m_dsvDescriptorSize;
    UINT m_cbvDescriptorSize;
    
    int m_width;
    int m_height;
    
    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;
    
    ObjectConstants m_objectConstants;
    UINT8* m_cbvDataBegin;
    
    float m_clearColor[4];
};
