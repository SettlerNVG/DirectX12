#pragma once

#include <DirectXMath.h>

// Компонент камеры
struct Camera {
    float fov;           // Field of view в радианах
    float nearPlane;
    float farPlane;
    float aspectRatio;
    
    // Для FPS камеры
    float yaw;           // Поворот по Y (влево-вправо)
    float pitch;         // Поворот по X (вверх-вниз)
    float moveSpeed;
    float lookSpeed;
    
    bool isActive;       // Активная камера
    
    Camera()
        : fov(DirectX::XM_PIDIV4)
        , nearPlane(0.1f)
        , farPlane(100.0f)
        , aspectRatio(800.0f / 600.0f)
        , yaw(0.0f)
        , pitch(0.0f)
        , moveSpeed(3.0f)
        , lookSpeed(0.002f)
        , isActive(true) {
    }
    
    // Получить направление взгляда
    DirectX::XMVECTOR GetForward() const {
        using namespace DirectX;
        float cosYaw = cosf(yaw);
        float sinYaw = sinf(yaw);
        float cosPitch = cosf(pitch);
        float sinPitch = sinf(pitch);
        
        return XMVectorSet(
            cosYaw * cosPitch,
            sinPitch,
            sinYaw * cosPitch,
            0.0f
        );
    }
    
    // Получить направление вправо
    DirectX::XMVECTOR GetRight() const {
        using namespace DirectX;
        return XMVectorSet(cosf(yaw + XM_PIDIV2), 0.0f, sinf(yaw + XM_PIDIV2), 0.0f);
    }
    
    // Получить направление вверх
    DirectX::XMVECTOR GetUp() const {
        using namespace DirectX;
        return XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    }
};
