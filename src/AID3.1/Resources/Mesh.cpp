#include "Mesh.h"

Mesh::Mesh(const std::string& path)
    : Resource(path)
    , m_vertexBuffer(nullptr)
    , m_indexBuffer(nullptr) {
    m_vertexBufferView = {};
    m_indexBufferView = {};
}

Mesh::~Mesh() {
    // GPU буферы освобождаются через RenderAdapter
}
