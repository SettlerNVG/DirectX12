#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class AssetType {
    Mesh,
    Texture,
    Shader,
    Material,
    Other
};

struct AssetInfo {
    AssetType type;
    std::string name;
    std::string path;
    uintmax_t sizeBytes;
};

class ResourceCatalog {
public:
    static ResourceCatalog& GetInstance();

    void SetProjectRoot(const std::string& root);
    void Refresh();

    const std::vector<AssetInfo>& GetAssets() const { return m_assets; }
    std::vector<const AssetInfo*> GetAssetsByType(AssetType type) const;

    size_t GetAssetCount() const { return m_assets.size(); }
    uintmax_t GetTotalSizeBytes() const { return m_totalSizeBytes; }

private:
    ResourceCatalog() = default;

    void ScanDirectory(const std::string& directory);
    AssetType ClassifyExtension(const std::string& extension) const;

    std::string m_projectRoot;
    std::vector<AssetInfo> m_assets;
    uintmax_t m_totalSizeBytes = 0;
};
