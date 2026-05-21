#pragma once

#include "../Resources/Mesh.h"
#include "../Resources/Texture.h"
#include "../Resources/Shader.h"
#include <memory>
#include <string>

// Компонент для отрисовки 3D-моделей с ресурсами
struct MeshRenderer {
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Texture> texture;
    std::shared_ptr<Shader> shader;
    bool visible;
    
    MeshRenderer()
        : mesh(nullptr)
        , texture(nullptr)
        , shader(nullptr)
        , visible(true) {
    }
    
    MeshRenderer(std::shared_ptr<Mesh> m, std::shared_ptr<Texture> t, std::shared_ptr<Shader> s)
        : mesh(m)
        , texture(t)
        , shader(s)
        , visible(true) {
    }
    
    // Проверка готовности к рендерингу
    bool IsReady() const {
        // Mesh и texture обязательны, shader опционален (используется дефолтный)
        return mesh && mesh->IsLoaded() && 
               texture && texture->IsLoaded();
    }
};