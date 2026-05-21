#pragma once

// Базовый интерфейс для компонентов
// В C++ компоненты - это просто POD структуры
// Этот файл служит для документирования концепции
struct IComponent {
    virtual ~IComponent() = default;
};
