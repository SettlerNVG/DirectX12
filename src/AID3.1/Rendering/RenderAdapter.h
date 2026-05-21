#pragma once

#include <Windows.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include <vector>
#include <string>

// Структура вершины для передачи в RenderAdapter
struct Vertex;

// Расширенный интерфейс адаптера рендеринга для работы с ресурсами
class RenderAdapter {
public:
    virtual ~RenderAdapter() = default;
    
    virtual bool Initialize(HWND hwnd, int width, int height) = 0;
    virtual void Shutdown() = 0;
    
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    
    virtual void Clear(float r, float g, float b, float a) = 0;
    
    // Матрицы трансформации
    virtual void SetModelMatrix(const DirectX::XMMATRIX& model) = 0;
    virtual void SetViewMatrix(const DirectX::XMMATRIX& view) = 0;
    virtual void SetProjectionMatrix(const DirectX::XMMATRIX& projection) = 0;
    virtual void SetColor(float r, float g, float b, float a) = 0;
    
    // НОВЫЕ МЕТОДЫ ДЛЯ РЕСУРСОВ
    
    // Создание GPU буферов
    virtual ID3D12Resource* CreateVertexBuffer(const void* data, size_t size) = 0;
    virtual ID3D12Resource* CreateIndexBuffer(const void* data, size_t size) = 0;
    
    // Создание текстур
    virtual ID3D12Resource* CreateTexture2D(int width, int height, int channels, const void* pixels) = 0;
    
    // Создание шейдеров
    virtual ID3D12PipelineState* CompileShader(const std::string& vertexShader, const std::string& pixelShader) = 0;
    virtual ID3D12RootSignature* CreateRootSignature() = 0;
    
    // Отрисовка мешей
    virtual void DrawMesh(ID3D12Resource* vertexBuffer, ID3D12Resource* indexBuffer, 
                         size_t indexCount, ID3D12PipelineState* pipelineState) = 0;
    
    // Установка текстуры
    virtual void SetTexture(ID3D12Resource* texture) = 0;
    
    // Старые методы для совместимости
    virtual void DrawTriangle() = 0;
    virtual void DrawQuad() = 0;
    virtual void DrawCube() = 0;
    virtual void SetObjectTransform(float x, float y, float scale, float rotation) = 0;
    
    // UI методы
    virtual void DrawUIRect(float x, float y, float width, float height, float r, float g, float b, float a) = 0;
};