#pragma once

#include <Windows.h>
#include <DirectXMath.h>

// Интерфейс адаптера рендеринга (расширенный для ECS)
class RenderAdapter {
public:
    virtual ~RenderAdapter() = default;
    
    virtual bool Initialize(HWND hwnd, int width, int height) = 0;
    virtual void Shutdown() = 0;
    
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    
    virtual void Clear(float r, float g, float b, float a) = 0;
    
    // Новые методы для ECS
    virtual void SetModelMatrix(const DirectX::XMMATRIX& model) = 0;
    virtual void SetViewMatrix(const DirectX::XMMATRIX& view) = 0;
    virtual void SetProjectionMatrix(const DirectX::XMMATRIX& projection) = 0;
    virtual void SetColor(float r, float g, float b, float a) = 0;
    virtual void SetViewportRect(float x, float y, float width, float height) = 0;
    virtual void ResetViewportRect() = 0;
    
    // Отрисовка примитивов
    virtual void DrawTriangle() = 0;
    virtual void DrawQuad() = 0;
    virtual void DrawCube() = 0;
    
    // Старые методы для совместимости
    virtual void SetObjectTransform(float x, float y, float scale, float rotation) = 0;
    
    // UI методы
    virtual void DrawUIRect(float x, float y, float width, float height, float r, float g, float b, float a) = 0;
};
