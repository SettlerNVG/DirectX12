#pragma once

#include <chrono>

class Timer {
public:
    Timer();
    
    void Start();
    void Tick();
    
    float GetDeltaTime() const { return m_deltaTime; }
    float GetTotalTime() const { return m_totalTime; }
    int GetFPS() const { return m_fps; }

private:
    std::chrono::high_resolution_clock::time_point m_startTime;
    std::chrono::high_resolution_clock::time_point m_lastFrameTime;
    
    float m_deltaTime;
    float m_totalTime;
    
    int m_fps;
    int m_frameCount;
    float m_fpsTimer;
};
