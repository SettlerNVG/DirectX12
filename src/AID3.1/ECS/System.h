#pragma once

class World;

// Базовый класс для всех систем
class System {
public:
    virtual ~System() = default;
    
    virtual void Update(World* world, float deltaTime) = 0;
    
protected:
    System() = default;
};