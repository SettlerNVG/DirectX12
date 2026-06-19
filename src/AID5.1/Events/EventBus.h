#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <memory>
#include <algorithm>

// Базовый класс для всех событий
class Event {
public:
    virtual ~Event() = default;
};

// Обработчик событий
template<typename T>
using EventHandler = std::function<void(const T&)>;

// Глобальная шина событий
class EventBus {
public:
    static EventBus& GetInstance() {
        static EventBus instance;
        return instance;
    }
    
    // Подписаться на событие
    template<typename T>
    void Subscribe(EventHandler<T> handler) {
        std::type_index typeIndex(typeid(T));
        
        auto wrapper = [handler](const Event& e) {
            handler(static_cast<const T&>(e));
        };
        
        m_subscribers[typeIndex].push_back(wrapper);
    }
    
    // Опубликовать событие
    template<typename T>
    void Publish(const T& event) {
        std::type_index typeIndex(typeid(T));
        
        auto it = m_subscribers.find(typeIndex);
        if (it != m_subscribers.end()) {
            for (auto& handler : it->second) {
                handler(event);
            }
        }
    }
    
    // Очистить все подписки
    void Clear() {
        m_subscribers.clear();
    }
    
    // Очистить подписки на конкретный тип события
    template<typename T>
    void ClearSubscribers() {
        std::type_index typeIndex(typeid(T));
        m_subscribers.erase(typeIndex);
    }

private:
    EventBus() = default;
    ~EventBus() = default;
    
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    
    using EventCallback = std::function<void(const Event&)>;
    std::unordered_map<std::type_index, std::vector<EventCallback>> m_subscribers;
};

// Удобный макрос для подписки
#define SUBSCRIBE_TO_EVENT(EventType, Handler) \
    EventBus::GetInstance().Subscribe<EventType>(Handler)

// Удобный макрос для публикации
#define PUBLISH_EVENT(EventInstance) \
    EventBus::GetInstance().Publish(EventInstance)
