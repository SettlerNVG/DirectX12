#include "MeshLoader.h"
#include "../Utils/Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>

// Assimp includes
#ifdef USE_ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#endif

std::shared_ptr<Mesh> MeshLoader::Load(const std::string& path) {
    LOG_INFO("Loading mesh: " + path);
    
    // Определяем формат по расширению
    std::string extension = path.substr(path.find_last_of('.') + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension == "txt") {
        return LoadCustomFormat(path);
    }
#ifdef USE_ASSIMP
    else if (extension == "obj" || extension == "fbx" || extension == "dae" || 
             extension == "gltf" || extension == "glb" || extension == "blend") {
        return LoadWithAssimp(path);
    }
#endif
    else {
        LOG_ERROR("Unsupported mesh format: " + extension);
        return nullptr;
    }
}

std::shared_ptr<Mesh> MeshLoader::LoadCustomFormat(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open mesh file: " + path);
        return nullptr;
    }
    
    auto mesh = std::make_shared<Mesh>(path);
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    std::string line;
    int vertexCount = 0;
    int triangleCount = 0;
    bool readingVertices = false;
    bool readingIndices = false;
    
    while (std::getline(file, line)) {
        // Пропускаем пустые строки
        if (line.empty()) continue;
        
        // Читаем количество вершин
        if (line.find("VertexCount:") != std::string::npos) {
            sscanf_s(line.c_str(), "VertexCount: %d", &vertexCount);
            vertices.reserve(vertexCount);
            continue;
        }
        
        // Читаем количество треугольников
        if (line.find("TriangleCount:") != std::string::npos) {
            sscanf_s(line.c_str(), "TriangleCount: %d", &triangleCount);
            indices.reserve(triangleCount * 3);
            continue;
        }
        
        // Начало списка вершин
        if (line.find("VertexList") != std::string::npos) {
            readingVertices = true;
            readingIndices = false;
            continue;
        }
        
        // Начало списка индексов
        if (line.find("TriangleList") != std::string::npos) {
            readingVertices = false;
            readingIndices = true;
            continue;
        }
        
        // Пропускаем фигурные скобки
        if (line.find("{") != std::string::npos || line.find("}") != std::string::npos) {
            continue;
        }
        
        // Читаем вершины (pos.x pos.y pos.z norm.x norm.y norm.z)
        if (readingVertices) {
            Vertex vertex;
            if (sscanf_s(line.c_str(), "%f %f %f %f %f %f",
                &vertex.position.x, &vertex.position.y, &vertex.position.z,
                &vertex.normal.x, &vertex.normal.y, &vertex.normal.z) == 6) {
                vertex.uv = DirectX::XMFLOAT2(0.0f, 0.0f); // UV по умолчанию
                vertices.push_back(vertex);
            }
        }
        
        // Читаем индексы (i0 i1 i2)
        if (readingIndices) {
            uint32_t i0, i1, i2;
            if (sscanf_s(line.c_str(), "%u %u %u", &i0, &i1, &i2) == 3) {
                indices.push_back(i0);
                indices.push_back(i1);
                indices.push_back(i2);
            }
        }
    }
    
    file.close();
    
    mesh->SetVertices(vertices);
    mesh->SetIndices(indices);
    mesh->SetLoaded(true);
    
    LOG_INFO("Mesh loaded successfully: " + std::to_string(vertices.size()) + " vertices, " + 
             std::to_string(indices.size() / 3) + " triangles");
    
    return mesh;
}

std::shared_ptr<Mesh> MeshLoader::LoadWithAssimp(const std::string& path) {
#ifdef USE_ASSIMP
    LOG_INFO("Loading mesh with Assimp: " + path);
    
    Assimp::Importer importer;
    
    // Настройки импорта
    unsigned int flags = 
        aiProcess_Triangulate |           // Триангуляция всех полигонов
        aiProcess_GenNormals |            // Генерация нормалей если их нет
        aiProcess_CalcTangentSpace |      // Вычисление тангентов и битангентов
        aiProcess_JoinIdenticalVertices | // Объединение одинаковых вершин
        aiProcess_SortByPType |           // Сортировка по типу примитива
        aiProcess_FlipUVs;                // Переворот UV координат (для DirectX)
    
    const aiScene* scene = importer.ReadFile(path, flags);
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        LOG_ERROR("Assimp failed to load mesh: " + std::string(importer.GetErrorString()));
        return nullptr;
    }
    
    auto mesh = std::make_shared<Mesh>(path);
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    // Обрабатываем все меши в сцене
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* aiMesh = scene->mMeshes[i];
        
        unsigned int baseVertex = static_cast<unsigned int>(vertices.size());
        
        // Загружаем вершины
        for (unsigned int v = 0; v < aiMesh->mNumVertices; v++) {
            Vertex vertex;
            
            // Позиция
            vertex.position.x = aiMesh->mVertices[v].x;
            vertex.position.y = aiMesh->mVertices[v].y;
            vertex.position.z = aiMesh->mVertices[v].z;
            
            // Нормаль
            if (aiMesh->HasNormals()) {
                vertex.normal.x = aiMesh->mNormals[v].x;
                vertex.normal.y = aiMesh->mNormals[v].y;
                vertex.normal.z = aiMesh->mNormals[v].z;
            } else {
                vertex.normal = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
            }
            
            // UV координаты (первый набор)
            if (aiMesh->HasTextureCoords(0)) {
                vertex.uv.x = aiMesh->mTextureCoords[0][v].x;
                vertex.uv.y = aiMesh->mTextureCoords[0][v].y;
            } else {
                vertex.uv = DirectX::XMFLOAT2(0.0f, 0.0f);
            }
            
            vertices.push_back(vertex);
        }
        
        // Загружаем индексы
        for (unsigned int f = 0; f < aiMesh->mNumFaces; f++) {
            aiFace face = aiMesh->mFaces[f];
            for (unsigned int idx = 0; idx < face.mNumIndices; idx++) {
                indices.push_back(baseVertex + face.mIndices[idx]);
            }
        }
    }
    
    mesh->SetVertices(vertices);
    mesh->SetIndices(indices);
    mesh->SetLoaded(true);
    
    LOG_INFO("Assimp mesh loaded: " + std::to_string(vertices.size()) + " vertices, " + 
             std::to_string(indices.size() / 3) + " triangles from " + 
             std::to_string(scene->mNumMeshes) + " submeshes");
    
    return mesh;
#else
    LOG_WARNING("Assimp not enabled. Define USE_ASSIMP to use Assimp loader.");
    return nullptr;
#endif
}
