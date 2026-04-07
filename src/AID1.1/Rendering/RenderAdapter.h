#pragma once

#include <Windows.h>

// Интерфейс адаптера рендеринга
class RenderAdapter {
public:
    virtual ~RenderAdapter() = default;
    
    virtual bool Initialize(HWND hwnd, int width, int height) = 0;
    virtual void Shutdown() = 0;
    
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    
    virtual void Clear(float r, float g, float b, float a) = 0;
    virtual void DrawTriangle() = 0;
    virtual void DrawQuad() = 0;
    
    virtual void SetObjectTransform(float x, float y, float scale, float rotation) = 0;
    
    // UI методы
    virtual void DrawUIRect(float x, float y, float width, float height, float r, float g, float b, float a) = 0;
};
