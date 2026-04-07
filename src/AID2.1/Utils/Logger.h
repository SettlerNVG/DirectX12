#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <mutex>
#include <ctime>

enum class LogLevel {
    INFO,
    WARNING,
    ERROR_LOG,
    DEBUG
};

class Logger {
public:
    static Logger& GetInstance();
    
    void Log(LogLevel level, const std::string& message);
    void SetLogFile(const std::string& filename);
    void EnableConsoleOutput(bool enable);

private:
    Logger();
    ~Logger();
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    std::string GetTimeStamp();
    std::string LogLevelToString(LogLevel level);
    
    std::ofstream m_logFile;
    std::mutex m_mutex;
    bool m_consoleOutput;
};

// Макросы для удобного логирования
#define LOG_INFO(msg) Logger::GetInstance().Log(LogLevel::INFO, msg)
#define LOG_WARNING(msg) Logger::GetInstance().Log(LogLevel::WARNING, msg)
#define LOG_ERROR(msg) Logger::GetInstance().Log(LogLevel::ERROR_LOG, msg)
#define LOG_DEBUG(msg) Logger::GetInstance().Log(LogLevel::DEBUG, msg)
