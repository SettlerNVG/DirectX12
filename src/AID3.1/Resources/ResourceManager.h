#pragma once

#include "Mesh.h"
#include "Texture.h"
#include "Shader.h"
#include <memory>
#include <unordered_map>
#include <string>

// Forward declaration
class RenderAdapter;

// Синглтон менеджер ресурсов с кэшированием
class ResourceManager {
public:
    static ResourceManager& GetInstance() {
        static ResourceManager instance;
        return instance;
    }
    
    // Запрет копирования
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    
    // Загрузка ресурсов (с кэшированием)
    std::shared_ptr<Mesh> LoadMesh(const std::string& path);
    std::shared_ptr<Texture> LoadTexture(const std::string& path);
    std::shared_ptr<Shader> LoadShader(const std::string& path);
    
    // Очистка кэша
    void ClearCache();
    void ClearMeshCache();
    void ClearTextureCache();
    void ClearShaderCache();
    
    // Перезагрузка ресурса (для горячей замены)
    void ReloadShader(const std::string& path);
    void ReloadTexture(const std::string& path);
    
    // Установка RenderAdapter для загрузки на GPU
    void SetRenderAdapter(class RenderAdapter* adapter) { m_renderAdapter = adapter; }
    
private:
    ResourceManager() : m_renderAdapter(nullptr) {}
    ~ResourceManager() { ClearCache(); }
    
    // Кэши ресурсов
    std::unordered_map<std::string, std::shared_ptr<Mesh>> m_meshCache;
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_textureCache;
    std::unordered_map<std::string, std::shared_ptr<Shader>> m_shaderCache;
    
    class RenderAdapter* m_renderAdapter;
};
