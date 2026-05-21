#include "Timer.h"

Timer::Timer() 
    : m_deltaTime(0.0f)
    , m_totalTime(0.0f)
    , m_fps(0)
    , m_frameCount(0)
    , m_fpsTimer(0.0f) {
}

void Timer::Start() {
    m_startTime = std::chrono::high_resolution_clock::now();
    m_lastFrameTime = m_startTime;
    m_totalTime = 0.0f;
    m_deltaTime = 0.0f;
}

void Timer::Tick() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<float> deltaTimeDuration = currentTime - m_lastFrameTime;
    m_deltaTime = deltaTimeDuration.count();
    
    std::chrono::duration<float> totalTimeDuration = currentTime - m_startTime;
    m_totalTime = totalTimeDuration.count();
    
    m_lastFrameTime = currentTime;
    
    // Подсчет FPS
    m_frameCount++;
    m_fpsTimer += m_deltaTime;
    
    if (m_fpsTimer >= 1.0f) {
        m_fps = m_frameCount;
        m_frameCount = 0;
        m_fpsTimer = 0.0f;
    }
}