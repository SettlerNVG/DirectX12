#include "HotReloadWatcher.h"
#include "ResourceManager.h"
#include "../Utils/Logger.h"
#include <chrono>

HotReloadWatcher::HotReloadWatcher()
    : m_running(false) {
}

HotReloadWatcher::~HotReloadWatcher() {
    Stop();
}

void HotReloadWatcher::Start() {
    if (m_running) return;
    
    LOG_INFO("Starting Hot Reload Watcher");
    m_running = true;
    m_watchThread = std::thread(&HotReloadWatcher::WatchThreadFunc, this);
}

void HotReloadWatcher::Stop() {
    if (!m_running) return;
    
    LOG_INFO("Stopping Hot Reload Watcher");
    m_running = false;
    
    if (m_watchThread.joinable()) {
        m_watchThread.join();
    }
}

void HotReloadWatcher::WatchShader(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        LOG_WARNING("Cannot watch shader - file does not exist: " + path);
        return;
    }
    
    FileInfo info;
    info.lastWriteTime = std::filesystem::last_write_time(path);
    info.type = FileInfo::Shader;
    
    m_watchedFiles[path] = info;
    LOG_INFO("Watching shader for hot reload: " + path);
}

void HotReloadWatcher::WatchTexture(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        LOG_WARNING("Cannot watch texture - file does not exist: " + path);
        return;
    }
    
    FileInfo info;
    info.lastWriteTime = std::filesystem::last_write_time(path);
    info.type = FileInfo::Texture;
    
    m_watchedFiles[path] = info;
    LOG_INFO("Watching texture for hot reload: " + path);
}

void HotReloadWatcher::CheckForChanges() {
    for (auto& [path, info] : m_watchedFiles) {
        if (!std::filesystem::exists(path)) {
            continue;
        }
        
        auto currentWriteTime = std::filesystem::last_write_time(path);
        
        if (currentWriteTime != info.lastWriteTime) {
            LOG_INFO("File changed detected: " + path);
            ReloadFile(path, info.type);
            info.lastWriteTime = currentWriteTime;
        }
    }
}

void HotReloadWatcher::WatchThreadFunc() {
    LOG_INFO("Hot Reload watch thread started");
    
    while (m_running) {
        // Проверяем изменения каждую секунду
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Проверка выполняется в главном потоке через CheckForChanges()
    }
    
    LOG_INFO("Hot Reload watch thread stopped");
}

void HotReloadWatcher::ReloadFile(const std::string& path, FileInfo::Type type) {
    auto& resourceManager = ResourceManager::GetInstance();
    
    switch (type) {
        case FileInfo::Shader:
            LOG_INFO("Hot reloading shader: " + path);
            resourceManager.ReloadShader(path);
            break;
            
        case FileInfo::Texture:
            LOG_INFO("Hot reloading texture: " + path);
            resourceManager.ReloadTexture(path);
            break;
    }
}