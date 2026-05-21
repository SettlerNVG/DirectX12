#pragma once

#include <string>

// Базовый класс для всех ресурсов
class Resource {
public:
    Resource(const std::string& path) : m_path(path), m_loaded(false) {}
    virtual ~Resource() = default;
    
    const std::string& GetPath() const { return m_path; }
    bool IsLoaded() const { return m_loaded; }
    
    // Setter для загрузчиков
    void SetLoaded(bool loaded) { m_loaded = loaded; }
    
protected:
    std::string m_path;
    bool m_loaded;
};
