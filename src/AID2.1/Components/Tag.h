#pragma once

#include <string>

// Компонент имени/тега объекта
struct Tag {
    std::string name;
    
    Tag() : name("Unnamed") {}
    Tag(const std::string& n) : name(n) {}
};
