#pragma once

#include <Windows.h>
#include <unordered_map>
#include <string>
#include <DirectXMath.h>

// Коды клавиш (расширенный набор)
enum class KeyCode {
    // Буквы
    A = 'A', B = 'B', C = 'C', D = 'D', E = 'E', F = 'F', G = 'G', H = 'H',
    I = 'I', J = 'J', K = 'K', L = 'L', M = 'M', N = 'N', O = 'O', P = 'P',
    Q = 'Q', R = 'R', S = 'S', T = 'T', U = 'U', V = 'V', W = 'W', X = 'X',
    Y = 'Y', Z = 'Z',
    
    // Цифры
    Num0 = '0', Num1 = '1', Num2 = '2', Num3 = '3', Num4 = '4',
    Num5 = '5', Num6 = '6', Num7 = '7', Num8 = '8', Num9 = '9',
    
    // Специальные клавиши
    Space = VK_SPACE,
    Enter = VK_RETURN,
    Escape = VK_ESCAPE,
    Tab = VK_TAB,
    Backspace = VK_BACK,
    
    // Модификаторы
    Shift = VK_SHIFT,
    Control = VK_CONTROL,
    Alt = VK_MENU,
    
    // Стрелки
    Left = VK_LEFT,
    Right = VK_RIGHT,
    Up = VK_UP,
    Down = VK_DOWN,
    
    // Функциональные клавиши
    F1 = VK_F1, F2 = VK_F2, F3 = VK_F3, F4 = VK_F4,
    F5 = VK_F5, F6 = VK_F6, F7 = VK_F7, F8 = VK_F8,
    F9 = VK_F9, F10 = VK_F10, F11 = VK_F11, F12 = VK_F12,
    
    // Мышь
    MouseLeft = VK_LBUTTON,
    MouseRight = VK_RBUTTON,
    MouseMiddle = VK_MBUTTON
};

// Менеджер ввода (Singleton)
class InputManager {
public:
    static InputManager& GetInstance() {
        static InputManager instance;
        return instance;
    }
    
    // Инициализация
    void Initialize(HWND hwnd);
    
    // Обновление состояния (вызывать каждый кадр)
    void Update();
    
    // Проверка состояния клавиш
    bool IsKeyPressed(KeyCode key) const;
    bool IsKeyDown(KeyCode key) const;      // Нажата в этом кадре
    bool IsKeyUp(KeyCode key) const;        // Отпущена в этом кадре
    
    // Проверка состояния мыши
    bool IsMouseButtonPressed(KeyCode button) const;
    bool IsMouseButtonDown(KeyCode button) const;
    bool IsMouseButtonUp(KeyCode button) const;
    
    // Позиция мыши
    DirectX::XMFLOAT2 GetMousePosition() const { return m_mousePosition; }
    DirectX::XMFLOAT2 GetMouseDelta() const { return m_mouseDelta; }
    float GetMouseWheelDelta() const { return m_mouseWheelDelta; }
    
    // Управление курсором
    void ShowCursor(bool show);
    void SetCursorPosition(int x, int y);
    void CenterCursor();
    
    // Привязка действий к клавишам
    void BindAction(const std::string& actionName, KeyCode key);
    bool IsActionActive(const std::string& actionName) const;
    bool IsActionTriggered(const std::string& actionName) const;
    
    // Очистка привязок
    void ClearBindings();
    
    // Получить HWND
    HWND GetWindowHandle() const { return m_hwnd; }

private:
    InputManager();
    ~InputManager() = default;
    
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;
    
    HWND m_hwnd;
    
    // Состояния клавиш
    std::unordered_map<KeyCode, bool> m_keyStates;
    std::unordered_map<KeyCode, bool> m_prevKeyStates;
    
    // Состояния мыши
    DirectX::XMFLOAT2 m_mousePosition;
    DirectX::XMFLOAT2 m_prevMousePosition;
    DirectX::XMFLOAT2 m_mouseDelta;
    float m_mouseWheelDelta;
    
    // Привязки действий
    std::unordered_map<std::string, KeyCode> m_actionBindings;
    
    bool m_cursorVisible;
};
