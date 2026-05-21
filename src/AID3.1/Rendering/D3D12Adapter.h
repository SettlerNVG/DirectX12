#pragma once

#include "RenderAdapter.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <memory>

using Microsoft::WRL::ComPtr;

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
    
    // Матрицы трансформации
    void SetModelMatrix(const DirectX::XMMATRIX& model) override;
    void SetViewMatrix(const DirectX::XMMATRIX& view) override;
    void SetProjectionMatrix(const DirectX::XMMATRIX& projection) override;
    void SetColor(float r, float g, float b, float a) override;
    
    // НОВЫЕ МЕТОДЫ ДЛЯ РЕСУРСОВ
    ID3D12Resource* CreateVertexBuffer(const void* data, size_t size) override;
    ID3D12Resource* CreateIndexBuffer(const void* data, size_t size) override;
    ID3D12Resource* CreateTexture2D(int width, int height, int channels, const void* pixels) override;
    ID3D12PipelineState* CompileShader(const std::string& vertexShader, const std::string& pixelShader) override;
    ID3D12RootSignature* CreateRootSignature() override;
    void DrawMesh(ID3D12Resource* vertexBuffer, ID3D12Resource* indexBuffer, 
                 size_t indexCount, ID3D12PipelineState* pipelineState) override;
    void SetTexture(ID3D12Resource* texture) override;
    
    // Старые методы для совместимости
    void DrawTriangle() override;
    void DrawQuad() override;
    void DrawCube() override;
    void SetObjectTransform(float x, float y, float scale, float rotation) override;
    void DrawUIRect(float x, float y, float width, float height, float r, float g, float b, float a) override;

private:
    bool CreateDevice();
    bool CreateCommandObjects();
    bool CreateSwapChain(HWND hwnd, int width, int height);
    bool CreateRtvDescriptorHeap();
    bool CreateDsvDescriptorHeap();
    bool CreateRenderTargets();
    bool CreateDepthStencil();
    bool CreateDefaultRootSignature();
    bool CreateDefaultPipelineState();
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
    
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    ComPtr<ID3D12Resource> m_depthStencil;
    
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    
    // Геометрия для примитивов
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
    
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValues[FrameCount];
    HANDLE m_fenceEvent;
    
    UINT m_rtvDescriptorSize;
    UINT m_dsvDescriptorSize;
    UINT m_cbvDescriptorSize;
    
    int m_width;
    int m_height;
    bool m_initialized;
    
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
