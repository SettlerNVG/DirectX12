#include "ResourceCatalog.h"
#include "../Utils/Logger.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <unordered_set>

namespace fs = std::filesystem;

ResourceCatalog& ResourceCatalog::GetInstance() {
    static ResourceCatalog instance;
    return instance;
}

void ResourceCatalog::SetProjectRoot(const std::string& root) {
    m_projectRoot = root;
}

void ResourceCatalog::Refresh() {
    m_assets.clear();
    m_totalSizeBytes = 0;

    if (m_projectRoot.empty()) {
        m_projectRoot = ".";
    }

    const std::vector<std::string> roots = {
        m_projectRoot + "\\Assets",
        m_projectRoot + "\\Textures",
        m_projectRoot + "\\Shaders",
        m_projectRoot + "\\Models",
        m_projectRoot + "\\src\\Textures",
        m_projectRoot + "\\src\\Chapter 11 Stenciling\\StencilDemo\\Models",
        m_projectRoot + "\\src\\Chapter 17 Picking\\Picking\\Models",
        m_projectRoot + "\\src\\Chapter 23 Character Animation\\SkinnedMesh\\Models",
        m_projectRoot + "\\..\\Textures",
        m_projectRoot + "\\..\\Chapter 11 Stenciling\\StencilDemo\\Models",
        m_projectRoot + "\\..\\Chapter 17 Picking\\Picking\\Models",
        m_projectRoot + "\\..\\Chapter 23 Character Animation\\SkinnedMesh\\Models"
    };

    for (const auto& root : roots) {
        ScanDirectory(root);
    }

    std::sort(m_assets.begin(), m_assets.end(), [](const AssetInfo& a, const AssetInfo& b) {
        if (a.type != b.type) {
            return static_cast<int>(a.type) < static_cast<int>(b.type);
        }
        return a.name < b.name;
    });

    LOG_INFO("ResourceCatalog refreshed: " + std::to_string(m_assets.size()) + " assets indexed");
}

std::vector<const AssetInfo*> ResourceCatalog::GetAssetsByType(AssetType type) const {
    std::vector<const AssetInfo*> result;
    for (const auto& asset : m_assets) {
        if (asset.type == type) {
            result.push_back(&asset);
        }
    }
    return result;
}

void ResourceCatalog::ScanDirectory(const std::string& directory) {
    std::error_code ec;
    if (!fs::exists(directory, ec) || !fs::is_directory(directory, ec)) {
        return;
    }

    static const std::unordered_set<std::string> ignoredExtensions = {
        ".obj", ".pdb", ".ilk", ".idb", ".tlog", ".log", ".exe", ".lib", ".dll"
    };

    for (const auto& entry : fs::recursive_directory_iterator(directory, fs::directory_options::skip_permission_denied, ec)) {
        if (ec || !entry.is_regular_file(ec)) {
            continue;
        }

        const fs::path path = entry.path();
        std::string extension = path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });

        if (ignoredExtensions.find(extension) != ignoredExtensions.end()) {
            continue;
        }

        AssetType type = ClassifyExtension(extension);
        if (type == AssetType::Other) {
            continue;
        }

        uintmax_t size = entry.file_size(ec);
        if (ec) {
            size = 0;
            ec.clear();
        }

        AssetInfo info;
        info.type = type;
        info.name = path.filename().string();
        info.path = path.string();
        info.sizeBytes = size;

        m_totalSizeBytes += size;
        m_assets.push_back(std::move(info));
    }
}

AssetType ResourceCatalog::ClassifyExtension(const std::string& extension) const {
    if (extension == ".txt" || extension == ".m3d" || extension == ".fbx" || extension == ".gltf" || extension == ".glb") {
        return AssetType::Mesh;
    }
    if (extension == ".dds" || extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp" || extension == ".tga") {
        return AssetType::Texture;
    }
    if (extension == ".hlsl" || extension == ".fx" || extension == ".hlsli") {
        return AssetType::Shader;
    }
    if (extension == ".mat") {
        return AssetType::Material;
    }
    return AssetType::Other;
}
