#include "ShaderLoader.h"
#include "../Utils/Logger.h"
#include <fstream>
#include <sstream>

std::shared_ptr<Shader> ShaderLoader::Load(const std::string& path) {
    LOG_INFO("Loading shader: " + path);
    
    auto shader = std::make_shared<Shader>(path);
    
    // Читаем файл шейдера
    std::string shaderCode = ReadFile(path);
    if (shaderCode.empty()) {
        LOG_ERROR("Failed to read shader file: " + path);
        return nullptr;
    }
    
    // Для простоты, пока используем один файл для VS и PS
    // В реальном проекте можно разделить на .vs.hlsl и .ps.hlsl
    shader->SetVertexShaderCode(shaderCode);
    shader->SetPixelShaderCode(shaderCode);
    shader->SetLoaded(true);
    
    LOG_INFO("Shader loaded successfully: " + path);
    
    return shader;
}

std::string ShaderLoader::ReadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}
