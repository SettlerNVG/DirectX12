#pragma once

#include "Mesh.h"
#include <memory>
#include <string>

// Загрузчик мешей
class MeshLoader {
public:
    // Загрузка меша из файла
    static std::shared_ptr<Mesh> Load(const std::string& path);
    
private:
    // Загрузка кастомного формата .txt (skull.txt, car.txt)
    static std::shared_ptr<Mesh> LoadCustomFormat(const std::string& path);
    
    // Загрузка через Assimp (.obj, .fbx, и т.д.)
    static std::shared_ptr<Mesh> LoadWithAssimp(const std::string& path);
};
