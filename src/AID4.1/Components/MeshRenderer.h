#pragma once

#include <DirectXMath.h>

// Типы примитивов для отрисовки
enum class PrimitiveType {
    Triangle,
    Quad,
    Cube
};

// Компонент для отрисовки меша
struct MeshRenderer {
    PrimitiveType primitiveType;
    DirectX::XMFLOAT4 color;  // RGBA цвет
    bool visible;
    
    MeshRenderer()
        : primitiveType(PrimitiveType::Triangle)
        , color(1.0f, 1.0f, 1.0f, 1.0f)
        , visible(true) {
    }
    
    MeshRenderer(PrimitiveType type, const DirectX::XMFLOAT4& col)
        : primitiveType(type)
        , color(col)
        , visible(true) {
    }
};
