#pragma once

#include "Resource.h"
#include <DirectXMath.h>
#include <vector>
#include <d3d12.h>

// Структура вершины
struct Vertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 uv;
    
    Vertex() : position(0, 0, 0), normal(0, 1, 0), uv(0, 0) {}
    Vertex(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& norm, const DirectX::XMFLOAT2& texCoord)
        : position(pos), normal(norm), uv(texCoord) {}
};

// Класс меша
class Mesh : public Resource {
public:
    Mesh(const std::string& path);
    ~Mesh();
    
    const std::vector<Vertex>& GetVertices() const { return m_vertices; }
    const std::vector<uint32_t>& GetIndices() const { return m_indices; }
    
    void SetVertices(const std::vector<Vertex>& vertices) { m_vertices = vertices; }
    void SetIndices(const std::vector<uint32_t>& indices) { m_indices = indices; }
    
    // GPU буферы
    ID3D12Resource* GetVertexBuffer() const { return m_vertexBuffer; }
    ID3D12Resource* GetIndexBuffer() const { return m_indexBuffer; }
    
    void SetVertexBuffer(ID3D12Resource* buffer) { m_vertexBuffer = buffer; }
    void SetIndexBuffer(ID3D12Resource* buffer) { m_indexBuffer = buffer; }
    
    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const { return m_vertexBufferView; }
    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const { return m_indexBufferView; }
    
    void SetVertexBufferView(const D3D12_VERTEX_BUFFER_VIEW& view) { m_vertexBufferView = view; }
    void SetIndexBufferView(const D3D12_INDEX_BUFFER_VIEW& view) { m_indexBufferView = view; }
    
private:
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    
    // GPU ресурсы
    ID3D12Resource* m_vertexBuffer;
    ID3D12Resource* m_indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
};
