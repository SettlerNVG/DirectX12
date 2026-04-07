#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <string>

using Microsoft::WRL::ComPtr;

struct TextVertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT2 texCoord;
};

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();
    
    bool Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
    void Shutdown();
    
    void DrawText(ID3D12GraphicsCommandList* commandList, const std::string& text, 
                  float x, float y, float scale, DirectX::XMFLOAT4 color);
    
    void DrawRect(ID3D12GraphicsCommandList* commandList, float x, float y, 
                  float width, float height, DirectX::XMFLOAT4 color);

private:
    bool CreatePipelineState(ID3D12Device* device);
    bool CreateRootSignature(ID3D12Device* device);
    
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    ComPtr<ID3D12Resource> m_vertexBuffer;
    ComPtr<ID3D12Resource> m_constantBuffer;
    
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    UINT8* m_cbvDataBegin;
    
    struct TextConstants {
        DirectX::XMFLOAT4X4 transform;
        DirectX::XMFLOAT4 color;
    };
};
