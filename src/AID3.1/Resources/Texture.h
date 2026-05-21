#pragma once

#include "Resource.h"
#include <d3d12.h>
#include <vector>

// Класс текстуры
class Texture : public Resource {
public:
    Texture(const std::string& path);
    ~Texture();
    
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    int GetChannels() const { return m_channels; }
    
    const std::vector<unsigned char>& GetPixelData() const { return m_pixelData; }
    
    void SetDimensions(int width, int height, int channels) {
        m_width = width;
        m_height = height;
        m_channels = channels;
    }
    
    void SetPixelData(const std::vector<unsigned char>& data) { m_pixelData = data; }
    
    // GPU ресурсы
    ID3D12Resource* GetGPUTexture() const { return m_gpuTexture; }
    void SetGPUTexture(ID3D12Resource* texture) { m_gpuTexture = texture; }
    
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHandle() const { return m_srvHandle; }
    void SetSRVHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) { m_srvHandle = handle; }
    
private:
    int m_width;
    int m_height;
    int m_channels;
    std::vector<unsigned char> m_pixelData;
    
    // GPU ресурсы
    ID3D12Resource* m_gpuTexture;
    D3D12_CPU_DESCRIPTOR_HANDLE m_srvHandle;
};
