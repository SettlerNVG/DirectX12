#include "Logger.h"
#include <sstream>
#include <iomanip>
#include <Windows.h>

Logger& Logger::GetInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : m_consoleOutput(true) {
    SetLogFile("engine_aid3.log");
}

Logger::~Logger() {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

void Logger::SetLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
    
    m_logFile.open(filename, std::ios::app);
    if (!m_logFile.is_open()) {
        std::cerr << "Failed to open log file: " << filename << std::endl;
    }
}

void Logger::EnableConsoleOutput(bool enable) {
    m_consoleOutput = enable;
}

void Logger::Log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string timestamp = GetTimeStamp();
    std::string levelStr = LogLevelToString(level);
    std::string logMessage = "[" + timestamp + "] [" + levelStr + "] " + message + "\n";
    
    if (m_consoleOutput) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsole != INVALID_HANDLE_VALUE) {
            WORD color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
            
            switch (level) {
                case LogLevel::INFO:
                    color = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                    break;
                case LogLevel::WARNING:
                    color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                    break;
                case LogLevel::ERROR_LOG:
                    color = FOREGROUND_RED | FOREGROUND_INTENSITY;
                    break;
                case LogLevel::DEBUG:
                    color = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                    break;
            }
            
            SetConsoleTextAttribute(hConsole, color);
            
            DWORD written;
            WriteConsoleA(hConsole, logMessage.c_str(), (DWORD)logMessage.length(), &written, nullptr);
            
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }
    }
    
    if (m_logFile.is_open()) {
        m_logFile << logMessage;
        m_logFile.flush();
    }
}

std::string Logger::GetTimeStamp() {
    time_t now = time(nullptr);
    tm timeInfo;
    localtime_s(&timeInfo, &now);
    
    std::ostringstream oss;
    oss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string Logger::LogLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR_LOG: return "ERROR";
        case LogLevel::DEBUG: return "DEBUG";
        default: return "UNKNOWN";
    }
}
