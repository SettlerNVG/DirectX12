// nlohmann/json - v3.11.2 - JSON for Modern C++
// Это заглушка для компиляции. Скачайте полную версию с:
// https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp

#ifndef NLOHMANN_JSON_HPP
#define NLOHMANN_JSON_HPP

#include <string>
#include <map>
#include <vector>
#include <fstream>

namespace nlohmann {

// Упрощённая заглушка для json
class json {
public:
    json() {}
    json(const std::string&) {}
    
    // Операторы доступа
    json& operator[](const std::string& key) { return *this; }
    const json& operator[](const std::string& key) const { return *this; }
    json& operator[](size_t index) { return *this; }
    
    // Преобразования
    operator std::string() const { return ""; }
    operator int() const { return 0; }
    operator float() const { return 0.0f; }
    operator bool() const { return false; }
    
    std::string get_string() const { return ""; }
    
    // Проверки
    bool is_null() const { return true; }
    bool is_string() const { return false; }
    bool is_array() const { return false; }
    bool is_object() const { return false; }
    bool contains(const std::string&) const { return false; }
    
    size_t size() const { return 0; }
    bool empty() const { return true; }
    
    // Итераторы (заглушка)
    json* begin() { return this; }
    json* end() { return this; }
    const json* begin() const { return this; }
    const json* end() const { return this; }
    
    // Парсинг
    static json parse(const std::string&) { return json(); }
    static json parse(std::ifstream&) { return json(); }
    
    // Сериализация
    std::string dump(int indent = -1) const { return "{}"; }
};

} // namespace nlohmann

#endif // NLOHMANN_JSON_HPP

/*
ИНСТРУКЦИЯ ПО УСТАНОВКЕ ПОЛНОЙ ВЕРСИИ:

1. Скачайте json.hpp с GitHub:
   https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp

2. Замените этот файл скачанной версией (один заголовочный файл ~25000 строк)

3. Перекомпилируйте проект

ИСПОЛЬЗОВАНИЕ:

#include "json.hpp"
using json = nlohmann::json;

// Создание JSON
json j = {
    {"name", "John"},
    {"age", 30},
    {"items", {"sword", "shield"}}
};

// Парсинг из строки
json j2 = json::parse("{\"name\":\"Alice\"}");

// Парсинг из файла
std::ifstream file("config.json");
json j3 = json::parse(file);

// Доступ к данным
std::string name = j["name"];
int age = j["age"];

// Сериализация
std::string str = j.dump(4); // с отступами

ДОКУМЕНТАЦИЯ:
https://json.nlohmann.me/
*/
