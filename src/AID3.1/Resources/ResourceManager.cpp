#include "ResourceManager.h"
#include "MeshLoader.h"
#include "TextureLoader.h"
#include "ShaderLoader.h"
#include "Mesh.h"
#include "../Rendering/RenderAdapter.h"
#include "../Utils/Logger.h"

std::shared_ptr<Mesh> ResourceManager::LoadMesh(const std::string& path) {
    // Проверяем кэш
    auto it = m_meshCache.find(path);
    if (it != m_meshCache.end()) {
        LOG_INFO("Mesh loaded from cache: " + path);
        return it->second;
    }
    
    // Загружаем новый меш
    auto mesh = MeshLoader::Load(path);
    if (mesh && mesh->IsLoaded()) {
        // Создаем GPU буферы если есть RenderAdapter
        if (m_renderAdapter && !mesh->GetVertices().empty() && !mesh->GetIndices().empty()) {
            const auto& vertices = mesh->GetVertices();
            const auto& indices = mesh->GetIndices();
            
            // Создаем vertex buffer
            ID3D12Resource* vb = m_renderAdapter->CreateVertexBuffer(
                vertices.data(), 
                vertices.size() * sizeof(Vertex)
            );
            
            // Создаем index buffer
            ID3D12Resource* ib = m_renderAdapter->CreateIndexBuffer(
                indices.data(), 
                indices.size() * sizeof(uint32_t)
            );
            
            if (vb && ib) {
                mesh->SetVertexBuffer(vb);
                mesh->SetIndexBuffer(ib);
                LOG_INFO("GPU buffers created for mesh: " + path);
            } else {
                LOG_ERROR("Failed to create GPU buffers for mesh: " + path);
            }
        }
        
        m_meshCache[path] = mesh;
        LOG_INFO("Mesh cached: " + path);
    }
    
    return mesh;
}

std::shared_ptr<Texture> ResourceManager::LoadTexture(const std::string& path) {
    // Проверяем кэш
    auto it = m_textureCache.find(path);
    if (it != m_textureCache.end()) {
        LOG_INFO("Texture loaded from cache: " + path);
        return it->second;
    }
    
    // Загружаем новую текстуру
    auto texture = TextureLoader::Load(path);
    if (texture && texture->IsLoaded()) {
        m_textureCache[path] = texture;
        LOG_INFO("Texture cached: " + path);
    }
    
    return texture;
}

std::shared_ptr<Shader> ResourceManager::LoadShader(const std::string& path) {
    // Проверяем кэш
    auto it = m_shaderCache.find(path);
    if (it != m_shaderCache.end()) {
        LOG_INFO("Shader loaded from cache: " + path);
        return it->second;
    }
    
    // Загружаем новый шейдер
    auto shader = ShaderLoader::Load(path);
    if (shader && shader->IsLoaded()) {
        m_shaderCache[path] = shader;
        LOG_INFO("Shader cached: " + path);
    }
    
    return shader;
}

void ResourceManager::ClearCache() {
    LOG_INFO("Clearing all resource caches");
    m_meshCache.clear();
    m_textureCache.clear();
    m_shaderCache.clear();
}

void ResourceManager::ClearMeshCache() {
    LOG_INFO("Clearing mesh cache");
    m_meshCache.clear();
}

void ResourceManager::ClearTextureCache() {
    LOG_INFO("Clearing texture cache");
    m_textureCache.clear();
}

void ResourceManager::ClearShaderCache() {
    LOG_INFO("Clearing shader cache");
    m_shaderCache.clear();
}

void ResourceManager::ReloadShader(const std::string& path) {
    LOG_INFO("Reloading shader: " + path);
    
    // Удаляем из кэша
    m_shaderCache.erase(path);
    
    // Загружаем заново
    LoadShader(path);
}

void ResourceManager::ReloadTexture(const std::string& path) {
    LOG_INFO("Reloading texture: " + path);
    
    // Удаляем из кэша
    m_textureCache.erase(path);
    
    // Загружаем заново
    LoadTexture(path);
}
