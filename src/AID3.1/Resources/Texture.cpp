#include "Texture.h"

Texture::Texture(const std::string& path)
    : Resource(path)
    , m_width(0)
    , m_height(0)
    , m_channels(0)
    , m_gpuTexture(nullptr) {
    m_srvHandle = {};
}

Texture::~Texture() {
    // GPU ресурсы освобождаются через RenderAdapter
}
