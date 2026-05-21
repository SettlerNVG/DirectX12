#pragma once

#include <string>
#include <unordered_map>
#include <filesystem>
#include <thread>
#include <atomic>

// Класс для отслеживания изменений файлов и горячей замены ресурсов
class HotReloadWatcher {
public:
    HotReloadWatcher();
    ~HotReloadWatcher();
    
    // Запуск/остановка отслеживания
    void Start();
    void Stop();
    
    // Добавление файлов для отслеживания
    void WatchShader(const std::string& path);
    void WatchTexture(const std::string& path);
    
    // Проверка изменений (вызывается из главного потока)
    void CheckForChanges();

private:
    struct FileInfo {
        std::filesystem::file_time_type lastWriteTime;
        enum Type { Shader, Texture } type;
    };
    
    std::unordered_map<std::string, FileInfo> m_watchedFiles;
    std::atomic<bool> m_running;
    std::thread m_watchThread;
    
    void WatchThreadFunc();
    void ReloadFile(const std::string& path, FileInfo::Type type);
};