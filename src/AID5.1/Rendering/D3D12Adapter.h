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
    DirectX::XMFLOAT4X4 worldViewProj;
    DirectX::XMFLOAT4 objectColor;
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
    
    // Новые методы для ECS
    void SetModelMatrix(const DirectX::XMMATRIX& model) override;
    void SetViewMatrix(const DirectX::XMMATRIX& view) override;
    void SetProjectionMatrix(const DirectX::XMMATRIX& projection) override;
    void SetColor(float r, float g, float b, float a) override;
    void SetViewportRect(float x, float y, float width, float height) override;
    void ResetViewportRect() override;
    
    void DrawTriangle() override;
    void DrawQuad() override;
    void DrawCube() override;
    
    void SetObjectTransform(float x, float y, float scale, float rotation) override;
    
    // UI методы
    void DrawUIRect(float x, float y, float width, float height, float r, float g, float b, float a) override;
    
    // Для отладки
    UINT GetCurrentObjectIndex() const;
    
    // Методы для ImGui
    ID3D12Device* GetDevice() const { return m_device.Get(); }
    ID3D12CommandQueue* GetCommandQueue() const { return m_commandQueue.Get(); }
    ID3D12GraphicsCommandList* GetCommandList() const { return m_commandList.Get(); }
    ID3D12DescriptorHeap* GetSRVHeap() const { return m_imguiSrvHeap.Get(); }
    static constexpr UINT GetFrameCount() { return 2; }

private:
    bool CreateDevice();
    bool CreateCommandObjects();
    bool CreateSwapChain(HWND hwnd, int width, int height);
    bool CreateRtvDescriptorHeap();
    bool CreateDsvDescriptorHeap();
    bool CreateImGuiDescriptorHeap();
    bool CreateRenderTargets();
    bool CreateDepthStencil();
    bool CreateRootSignature();
    bool CreatePipelineState();
    bool CreateGeometry();
    
    void UpdateConstantBuffer();
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
    ComPtr<ID3D12DescriptorHeap> m_imguiSrvHeap;
    
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    ComPtr<ID3D12Resource> m_depthStencil;
    
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    
    // Геометрия
    ComPtr<ID3D12Resource> m_triangleVertexBuffer;
    ComPtr<ID3D12Resource> m_quadVertexBuffer;
    ComPtr<ID3D12Resource> m_cubeVertexBuffer;
    ComPtr<ID3D12Resource> m_cubeIndexBuffer;
    ComPtr<ID3D12Resource> m_constantBuffer;
    
    D3D12_VERTEX_BUFFER_VIEW m_triangleVertexBufferView;
    D3D12_VERTEX_BUFFER_VIEW m_quadVertexBufferView;
    D3D12_VERTEX_BUFFER_VIEW m_cubeVertexBufferView;
    D3D12_INDEX_BUFFER_VIEW m_cubeIndexBufferView;
    
    // Для множественных объектов
    static const UINT MaxObjectsPerFrame = 100;
    UINT m_currentObjectIndex;
    
    // UI буферы
    ComPtr<ID3D12Resource> m_uiVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_uiVertexBufferView;
    UINT8* m_uiVertexDataBegin;
    
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
    
    // Матрицы и константы
    DirectX::XMMATRIX m_modelMatrix;
    DirectX::XMMATRIX m_viewMatrix;
    DirectX::XMMATRIX m_projectionMatrix;
    DirectX::XMFLOAT4 m_currentColor;
    
    ObjectConstants m_objectConstants;
    UINT8* m_cbvDataBegin;
    
    float m_clearColor[4];
};
