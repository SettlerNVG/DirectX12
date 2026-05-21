#pragma once

#include <string>
#include <vector>

// Структура описания ресурса в манифесте
struct ResourceEntry {
    std::string id;          // Уникальный идентификатор
    std::string type;        // "mesh", "texture", "shader"
    std::string path;        // Путь к файлу
    bool preload;            // Загружать при старте?
    
    ResourceEntry() : preload(false) {}
};

// Класс для работы с манифестом ресурсов (JSON)
class ResourceManifest {
public:
    // Загрузка манифеста из JSON файла
    static bool Load(const std::string& manifestPath, std::vector<ResourceEntry>& outEntries);
    
    // Сохранение манифеста в JSON файл
    static bool Save(const std::string& manifestPath, const std::vector<ResourceEntry>& entries);
    
    // Создание примера манифеста
    static void CreateExample(const std::string& outputPath);
};
