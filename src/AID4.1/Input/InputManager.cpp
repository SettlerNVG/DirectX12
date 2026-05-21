#include "InputManager.h"

using namespace DirectX;

InputManager::InputManager()
    : m_hwnd(nullptr)
    , m_mousePosition(0.0f, 0.0f)
    , m_prevMousePosition(0.0f, 0.0f)
    , m_mouseDelta(0.0f, 0.0f)
    , m_mouseWheelDelta(0.0f)
    , m_cursorVisible(true) {
}

void InputManager::Initialize(HWND hwnd) {
    m_hwnd = hwnd;
    
    // Получаем начальную позицию мыши
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(m_hwnd, &pt);
    m_mousePosition = XMFLOAT2(static_cast<float>(pt.x), static_cast<float>(pt.y));
    m_prevMousePosition = m_mousePosition;
}

void InputManager::Update() {
    // Сохраняем предыдущие состояния
    m_prevKeyStates = m_keyStates;
    m_prevMousePosition = m_mousePosition;
    
    // Обновляем состояния всех отслеживаемых клавиш
    for (auto& pair : m_keyStates) {
        pair.second = (GetAsyncKeyState(static_cast<int>(pair.first)) & 0x8000) != 0;
    }
    
    // Обновляем позицию мыши
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(m_hwnd, &pt);
    
    m_mousePosition = XMFLOAT2(static_cast<float>(pt.x), static_cast<float>(pt.y));
    m_mouseDelta = XMFLOAT2(
        m_mousePosition.x - m_prevMousePosition.x,
        m_mousePosition.y - m_prevMousePosition.y
    );
    
    // Сброс колеса мыши (обновляется через WM_MOUSEWHEEL)
    m_mouseWheelDelta = 0.0f;
}

bool InputManager::IsKeyPressed(KeyCode key) const {
    auto it = m_keyStates.find(key);
    if (it != m_keyStates.end()) {
        return it->second;
    }
    
    // Если клавиша не отслеживается, проверяем напрямую
    return (GetAsyncKeyState(static_cast<int>(key)) & 0x8000) != 0;
}

bool InputManager::IsKeyDown(KeyCode key) const {
    auto it = m_keyStates.find(key);
    auto prevIt = m_prevKeyStates.find(key);
    
    bool current = (it != m_keyStates.end()) ? it->second : false;
    bool previous = (prevIt != m_prevKeyStates.end()) ? prevIt->second : false;
    
    return current && !previous;
}

bool InputManager::IsKeyUp(KeyCode key) const {
    auto it = m_keyStates.find(key);
    auto prevIt = m_prevKeyStates.find(key);
    
    bool current = (it != m_keyStates.end()) ? it->second : false;
    bool previous = (prevIt != m_prevKeyStates.end()) ? prevIt->second : false;
    
    return !current && previous;
}

bool InputManager::IsMouseButtonPressed(KeyCode button) const {
    return IsKeyPressed(button);
}

bool InputManager::IsMouseButtonDown(KeyCode button) const {
    return IsKeyDown(button);
}

bool InputManager::IsMouseButtonUp(KeyCode button) const {
    return IsKeyUp(button);
}

void InputManager::ShowCursor(bool show) {
    m_cursorVisible = show;
    ::ShowCursor(show ? TRUE : FALSE);
}

void InputManager::SetCursorPosition(int x, int y) {
    POINT pt = { x, y };
    ClientToScreen(m_hwnd, &pt);
    ::SetCursorPos(pt.x, pt.y);
}

void InputManager::CenterCursor() {
    if (!m_hwnd) return;
    
    RECT rect;
    GetClientRect(m_hwnd, &rect);
    int centerX = (rect.right - rect.left) / 2;
    int centerY = (rect.bottom - rect.top) / 2;
    
    SetCursorPosition(centerX, centerY);
}

void InputManager::BindAction(const std::string& actionName, KeyCode key) {
    m_actionBindings[actionName] = key;
    
    // Добавляем клавишу в отслеживаемые
    m_keyStates[key] = false;
    m_prevKeyStates[key] = false;
}

bool InputManager::IsActionActive(const std::string& actionName) const {
    auto it = m_actionBindings.find(actionName);
    if (it != m_actionBindings.end()) {
        return IsKeyPressed(it->second);
    }
    return false;
}

bool InputManager::IsActionTriggered(const std::string& actionName) const {
    auto it = m_actionBindings.find(actionName);
    if (it != m_actionBindings.end()) {
        return IsKeyDown(it->second);
    }
    return false;
}

void InputManager::ClearBindings() {
    m_actionBindings.clear();
}
