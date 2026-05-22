#pragma once

#include <DirectXMath.h>

// Компонент физического тела
struct Rigidbody {
    DirectX::XMFLOAT3 velocity;      // Текущая скорость
    DirectX::XMFLOAT3 acceleration;  // Ускорение (для будущего расширения)
    float mass;                       // Масса объекта
    bool useGravity;                  // Применять ли гравитацию
    bool isKinematic;                 // Кинематическое тело (не реагирует на физику, но может двигаться)
    float drag;                       // Сопротивление воздуха (0-1)
    float restitution;                // Коэффициент упругости (0-1)
    
    // Sleeping optimization fields
    bool isSleeping;                  // Объект спит (не обрабатывается физикой)
    float sleepTimer;                 // Время неподвижности
    bool isGrounded;                  // Объект касается земли (для стабилизации)
    float groundContactTime;          // Время контакта с землей
    
    Rigidbody()
        : velocity(0.0f, 0.0f, 0.0f)
        , acceleration(0.0f, 0.0f, 0.0f)
        , mass(1.0f)
        , useGravity(true)
        , isKinematic(false)
        , drag(0.01f)
        , restitution(0.3f)
        , isSleeping(false)
        , sleepTimer(0.0f)
        , isGrounded(false)
        , groundContactTime(0.0f) {
    }
    
    Rigidbody(float m, bool gravity = true)
        : velocity(0.0f, 0.0f, 0.0f)
        , acceleration(0.0f, 0.0f, 0.0f)
        , mass(m)
        , useGravity(gravity)
        , isKinematic(false)
        , drag(0.01f)
        , restitution(0.3f)
        , isSleeping(false)
        , sleepTimer(0.0f)
        , isGrounded(false)
        , groundContactTime(0.0f) {
    }
    
    // Добавить силу (F = ma)
    void AddForce(const DirectX::XMFLOAT3& force) {
        using namespace DirectX;
        XMVECTOR acc = XMLoadFloat3(&acceleration);
        XMVECTOR f = XMLoadFloat3(&force);
        acc = XMVectorAdd(acc, XMVectorScale(f, 1.0f / mass));
        XMStoreFloat3(&acceleration, acc);
    }
    
    // Добавить импульс (изменение скорости напрямую)
    void AddImpulse(const DirectX::XMFLOAT3& impulse) {
        using namespace DirectX;
        XMVECTOR vel = XMLoadFloat3(&velocity);
        XMVECTOR imp = XMLoadFloat3(&impulse);
        vel = XMVectorAdd(vel, XMVectorScale(imp, 1.0f / mass));
        XMStoreFloat3(&velocity, vel);
    }
};
