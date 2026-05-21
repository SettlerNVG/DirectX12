#pragma once

#include "Texture.h"
#include <memory>
#include <string>

// Загрузчик текстур
class TextureLoader {
public:
    // Загрузка текстуры из файла (PNG, JPEG, BMP и т.д.)
    static std::shared_ptr<Texture> Load(const std::string& path);
};
