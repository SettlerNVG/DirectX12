#pragma once

#include "Shader.h"
#include <memory>
#include <string>

// Загрузчик шейдеров
class ShaderLoader {
public:
    // Загрузка шейдера из файла (HLSL)
    static std::shared_ptr<Shader> Load(const std::string& path);
    
private:
    // Чтение текстового файла
    static std::string ReadFile(const std::string& path);
};
