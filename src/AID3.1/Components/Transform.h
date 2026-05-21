#pragma once

#include <DirectXMath.h>

// Компонент трансформации объекта
struct Transform {
    DirectX::XMFLOAT3 position;  // Позиция в пространстве
    DirectX::XMFLOAT3 rotation;  // Вращение (углы Эйлера в радианах)
    DirectX::XMFLOAT3 scale;     // Масштаб
    
    Transform()
        : position(0.0f, 0.0f, 0.0f)
        , rotation(0.0f, 0.0f, 0.0f)
        , scale(1.0f, 1.0f, 1.0f) {
    }
    
    Transform(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& rot, const DirectX::XMFLOAT3& scl)
        : position(pos)
        , rotation(rot)
        , scale(scl) {
    }
    
    // Получить матрицу трансформации
    DirectX::XMMATRIX GetMatrix() const {
        using namespace DirectX;
        
        XMMATRIX scaleMatrix = XMMatrixScaling(scale.x, scale.y, scale.z);
        XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
        XMMATRIX translationMatrix = XMMatrixTranslation(position.x, position.y, position.z);
        
        return scaleMatrix * rotationMatrix * translationMatrix;
    }
};