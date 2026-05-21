#include "ResourceManifest.h"
#include "json.hpp"
#include "../Utils/Logger.h"
#include <fstream>

using json = nlohmann::json;

bool ResourceManifest::Load(const std::string& manifestPath, std::vector<ResourceEntry>& outEntries) {
    LOG_INFO("Loading resource manifest: " + manifestPath);
    
    std::ifstream file(manifestPath);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open manifest file: " + manifestPath);
        return false;
    }
    
    try {
        json j = json::parse(file);
        
        if (!j.contains("resources") || !j["resources"].is_array()) {
            LOG_ERROR("Invalid manifest format: missing 'resources' array");
            return false;
        }
        
        outEntries.clear();
        
        for (const auto& item : j["resources"]) {
            ResourceEntry entry;
            
            if (item.contains("id")) entry.id = item["id"];
            if (item.contains("type")) entry.type = item["type"];
            if (item.contains("path")) entry.path = item["path"];
            if (item.contains("preload")) entry.preload = item["preload"];
            
            outEntries.push_back(entry);
        }
        
        LOG_INFO("Loaded " + std::to_string(outEntries.size()) + " resource entries from manifest");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to parse manifest JSON: " + std::string(e.what()));
        return false;
    }
}

bool ResourceManifest::Save(const std::string& manifestPath, const std::vector<ResourceEntry>& entries) {
    LOG_INFO("Saving resource manifest: " + manifestPath);
    
    json j;
    j["version"] = "1.0";
    j["resources"] = json::array();
    
    for (const auto& entry : entries) {
        json item;
        item["id"] = entry.id;
        item["type"] = entry.type;
        item["path"] = entry.path;
        item["preload"] = entry.preload;
        
        j["resources"].push_back(item);
    }
    
    std::ofstream file(manifestPath);
    if (!file.is_open()) {
        LOG_ERROR("Failed to create manifest file: " + manifestPath);
        return false;
    }
    
    file << j.dump(4); // С отступами для читаемости
    file.close();
    
    LOG_INFO("Manifest saved successfully");
    return true;
}

void ResourceManifest::CreateExample(const std::string& outputPath) {
    LOG_INFO("Creating example manifest: " + outputPath);
    
    std::vector<ResourceEntry> examples;
    
    // Пример 1: Меш черепа
    ResourceEntry skull;
    skull.id = "skull_mesh";
    skull.type = "mesh";
    skull.path = "../Chapter 11 Stenciling/StencilDemo/Models/skull.txt";
    skull.preload = true;
    examples.push_back(skull);
    
    // Пример 2: Меш машины
    ResourceEntry car;
    car.id = "car_mesh";
    car.type = "mesh";
    car.path = "../Chapter 11 Stenciling/StencilDemo/Models/car.txt";
    car.preload = true;
    examples.push_back(car);
    
    // Пример 3: Текстура
    ResourceEntry texture;
    texture.id = "default_texture";
    texture.type = "texture";
    texture.path = "assets/default.png";
    texture.preload = true;
    examples.push_back(texture);
    
    // Пример 4: Шейдер
    ResourceEntry shader;
    shader.id = "basic_shader";
    shader.type = "shader";
    shader.path = "assets/basic.hlsl";
    shader.preload = false;
    examples.push_back(shader);
    
    Save(outputPath, examples);
}
