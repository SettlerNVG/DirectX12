#include "TextureLoader.h"
#include "../Utils/Logger.h"

// stb_image integration
#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8
#include "stb_image.h"

std::shared_ptr<Texture> TextureLoader::Load(const std::string& path) {
    LOG_INFO("Loading texture: " + path);
    
    auto texture = std::make_shared<Texture>(path);
    
    // Загружаем изображение через stb_image
    int width, height, channels;
    stbi_set_flip_vertically_on_load(false); // DirectX не требует переворота
    
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4); // Принудительно RGBA
    
    if (!data) {
        const char* reason = stbi_failure_reason();
        LOG_ERROR("Failed to load texture with stb_image: " + path +
                  (reason ? " - " + std::string(reason) : ""));
        return nullptr;
    }
    
    // Успешная загрузка
    texture->SetDimensions(width, height, 4); // Всегда RGBA
    
    // Копируем данные
    std::vector<unsigned char> pixels(data, data + (width * height * 4));
    texture->SetPixelData(pixels);
    
    // Освобождаем память stb_image
    stbi_image_free(data);
    
    texture->SetLoaded(true);
    
    LOG_INFO("Texture loaded successfully: " + path + " (" + 
             std::to_string(width) + "x" + std::to_string(height) + ", " + 
             std::to_string(channels) + " channels)");
    
    return texture;
}
