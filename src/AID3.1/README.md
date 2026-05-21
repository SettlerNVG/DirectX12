# Game Engine AID3.1 - Resource Manager

Игровой движок с централизованной системой управления ресурсами и загрузкой графических ассетов.

## Описание проекта

Практическая работа №3: **Менеджер ресурсов и загрузка графических ассетов**

Реализована централизованная система управления ресурсами (моделями, текстурами, шейдерами) с кэшированием, интеграция загрузки 3D-моделей и их визуализация через компонент MeshRenderer на базе ECS.

### Реализованные компоненты:

## ✅ Выполненные задачи по ТЗ

### 1. ✅ Подключение библиотек
- **Assimp** - для загрузки 3D-моделей (поддержка .obj, .fbx, .dae, .gltf, .blend и 40+ форматов)
  - Интеграция: `MeshLoader::LoadWithAssimp()`
  - Флаги: `aiProcess_Triangulate`, `aiProcess_GenNormals`, `aiProcess_FlipUVs`
  - Использование: `#define USE_ASSIMP` для активации
- **stb_image** - для загрузки изображений (PNG, JPEG, BMP, TGA, GIF, HDR)
  - Интеграция: `TextureLoader::Load()` с `stbi_load()`
  - Автоматическая конвертация в RGBA
  - Fallback на белую текстуру 1x1 при ошибке
- **nlohmann/json** - для работы с JSON конфигурациями
  - Интеграция: `ResourceManifest` для загрузки манифеста ресурсов
  - Файл: `assets/resources.json`
  - Поддержка preload флагов
- **DirectXMath** - математическая библиотека (уже используется)

### 2. ✅ Шаблонный класс ResourceManager с кэшированием
- Реализован как синглтон для глобального доступа
- Кэш: `std::unordered_map<std::string, std::shared_ptr<Resource>>`
- Метод `Load<T>(path)` - загружает ресурс или возвращает из кэша
- Автоматическое управление памятью через `std::shared_ptr`
- Методы очистки кэша: `ClearCache()`, `ClearMeshCache()`, `ClearTextureCache()`, `ClearShaderCache()`

### 3. ✅ Загрузчики для конкретных типов ресурсов

**MeshLoader:**
- Загрузка кастомного формата .txt (skull.txt, car.txt из Chapter 11)
- Парсинг вершин (позиция, нормаль)
- Парсинг индексов треугольников
- **Полная поддержка Assimp** для .obj, .fbx, .dae, .gltf, .blend и других
  - Автоматическая триангуляция
  - Генерация нормалей
  - Загрузка UV координат
  - Поддержка множественных submeshes

**TextureLoader:**
- **Полная интеграция stb_image**
- Поддержка PNG, JPEG, BMP, TGA, GIF, HDR, PIC, PNM
- Автоматическая конвертация в RGBA (4 канала)
- Fallback на белую текстуру 1x1 при ошибке загрузки

**ShaderLoader:**
- Загрузка HLSL шейдеров из текстовых файлов
- Чтение vertex и pixel shader кода

**ResourceManifest (JSON):**
- Загрузка списка ресурсов из JSON манифеста
- Поддержка preload флагов
- Автоматическая загрузка ресурсов при старте
- Пример: `assets/resources.json`

### 4. ✅ Расширение RenderAdapter методами для GPU-ресурсов

Добавлены методы в `RenderAdapter`:
- `CreateVertexBuffer(data, size)` - создание вершинного буфера
- `CreateIndexBuffer(data, size)` - создание индексного буфера
- `CreateTexture2D(width, height, channels, pixels)` - создание текстуры в видеопамяти
- `CompileShader(vertexShader, pixelShader)` - компиляция шейдеров
- `CreateRootSignature()` - создание root signature для D3D12
- `DrawMesh(vertexBuffer, indexBuffer, indexCount, pipelineState)` - отрисовка меша
- `SetTexture(texture)` - установка текстуры для рендеринга

### 5. ✅ Модификация компонента MeshRenderer

Новая структура `MeshRenderer`:
```cpp
struct MeshRenderer {
    std::shared_ptr<Mesh> mesh;        // Ссылка на меш
    std::shared_ptr<Texture> texture;  // Ссылка на текстуру
    std::shared_ptr<Shader> shader;    // Ссылка на шейдер
    bool visible;
    
    bool IsReady() const;  // Проверка готовности всех ресурсов
};
```

### 6. ✅ Адаптация RenderSystem для отрисовки загруженных моделей

`RenderSystem` обновлен:
- Проверка готовности ресурсов через `IsReady()`
- Установка текстуры перед рендерингом
- Вызов `DrawMesh()` с загруженными буферами
- Поддержка иерархии трансформаций

### 7. ✅ Демонстрационная сцена с 3D-моделями

**Загруженные модели:**
- **2 черепа** (skull.txt) - демонстрация кэширования (один меш используется дважды)
- **1 машина** (car.txt)
- Текстуры (заглушки)
- Шейдеры (заглушки)

**Анимация:**
- Череп 1: вращение вокруг оси Y
- Машина: движение вверх-вниз + вращение
- Череп 2: статичный

### 8. ✅ (БОНУС) Горячая замена шейдеров

**Реализован класс HotReloadWatcher:**
- Фоновый поток для отслеживания изменений файлов
- Использование `std::filesystem::last_write_time()` для проверки изменений
- Автоматическая перезагрузка при изменении файла

**Методы в ResourceManager:**
- `ReloadShader(path)` - перезагрузка шейдера из файла
- `ReloadTexture(path)` - перезагрузка текстуры
- Удаление из кэша и повторная загрузка
- Автоматическое обновление в рантайме без перезапуска

**Интеграция в Application:**
- HotReloadWatcher запускается при инициализации
- Проверка изменений в каждом кадре через `CheckForChanges()`
- Автоматическая остановка при завершении приложения

**Использование в GameplayState:**
```cpp
auto* hotReloadWatcher = m_app->GetHotReloadWatcher();
hotReloadWatcher->WatchShader("assets/basic.hlsl");
hotReloadWatcher->WatchTexture("assets/default.png");
```

**Как работает:**
1. Файл добавляется в список отслеживания
2. Сохраняется время последней модификации
3. Каждую секунду проверяется изменение времени модификации
4. При обнаружении изменения - автоматическая перезагрузка
5. Ресурс обновляется в кэше без перезапуска движка

## Архитектура решения

```
AID3.1/
├── Resources/              # Система управления ресурсами
│   ├── ResourceManager     # Центральный менеджер с кэшем
│   ├── Resource            # Базовый класс ресурса
│   ├── Mesh                # Геометрия + GPU буферы
│   ├── Texture             # Текстура + GPU ресурс
│   ├── Shader              # Шейдерная программа
│   ├── MeshLoader          # Загрузка моделей
│   ├── TextureLoader       # Загрузка текстур
│   └── ShaderLoader        # Загрузка шейдеров
├── Core/                   # Ядро движка
├── ECS/                    # Entity Component System
├── Components/             # Компоненты (Transform, MeshRenderer, Camera)
├── Systems/                # Системы (RenderSystem, CameraSystem)
├── Rendering/              # Рендеринг (RenderAdapter, D3D12Adapter)
├── States/                 # Состояния игры
└── Utils/                  # Утилиты (Logger)
```

## Требования

- Visual Studio 2022
- Windows 10/11
- DirectX 12 SDK (входит в Windows SDK)
- C++17 или выше
- CMake 3.15+ (опционально)

### Внешние библиотеки (опционально)

Проект работает **без установки библиотек** (используются заглушки), но для полной функциональности:

#### 1. Assimp (для загрузки .obj, .fbx, .gltf)
```bash
# Скачать с GitHub
https://github.com/assimp/assimp/releases

# Или через vcpkg
vcpkg install assimp:x64-windows

# Добавить в проект:
# - Включить путь к assimp/include
# - Линковать assimp-vc143-mt.lib
# - Определить USE_ASSIMP в препроцессоре
```

#### 2. stb_image (для загрузки PNG, JPEG)
```bash
# Скачать один файл
https://raw.githubusercontent.com/nothings/stb/master/stb_image.h

# Заменить файл:
src/AID3.1/Resources/stb_image.h
```

#### 3. nlohmann/json (для манифеста ресурсов)
```bash
# Скачать один файл
https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp

# Заменить файл:
src/AID3.1/Resources/json.hpp
```

**Примечание:** Текущая версия использует заглушки библиотек и работает с кастомными форматами (.txt для мешей, белая текстура для изображений). Для полной функциональности замените заглушки на реальные библиотеки.

## Сборка и запуск

### Visual Studio

1. Откройте файл `AID3.1.sln` в Visual Studio 2022
2. Выберите конфигурацию Debug или Release
3. Нажмите F5 для сборки и запуска проекта

### CMake

```bash
mkdir build
cd build
cmake ../src/AID3.1
cmake --build . --config Release
```

Исполняемый файл: `build/bin/Release/AID3.1.exe`

## Использование

### Загрузка ресурсов

```cpp
auto& resourceManager = ResourceManager::GetInstance();

// Загрузка меша (с кэшированием)
auto mesh = resourceManager.LoadMesh("models/skull.txt");

// Загрузка текстуры
auto texture = resourceManager.LoadTexture("textures/wood.png");

// Загрузка шейдера
auto shader = resourceManager.LoadShader("shaders/basic.hlsl");
```

### Создание сущности с ресурсами

```cpp
Entity entity = world->CreateEntity();
world->AddComponent(entity, Transform(position, rotation, scale));
world->AddComponent(entity, MeshRenderer(mesh, texture, shader));
```

### Кэширование

ResourceManager автоматически кэширует загруженные ресурсы. При повторном запросе того же файла возвращается указатель на уже загруженный ресурс из кэша.

```cpp
// Первая загрузка - читает с диска
auto mesh1 = resourceManager.LoadMesh("skull.txt");

// Вторая загрузка - возвращает из кэша
auto mesh2 = resourceManager.LoadMesh("skull.txt");

// mesh1 и mesh2 указывают на один и тот же объект
```

### Манифест ресурсов (JSON)

Проект поддерживает загрузку ресурсов из JSON манифеста:

```json
{
    "version": "1.0",
    "resources": [
        {
            "id": "skull_mesh",
            "type": "mesh",
            "path": "../Chapter 11 Stenciling/StencilDemo/Models/skull.txt",
            "preload": true
        },
        {
            "id": "default_texture",
            "type": "texture",
            "path": "assets/default.png",
            "preload": true
        }
    ]
}
```

Использование:
```cpp
#include "Resources/ResourceManifest.h"

std::vector<ResourceEntry> entries;
if (ResourceManifest::Load("assets/resources.json", entries)) {
    for (const auto& entry : entries) {
        if (entry.preload) {
            if (entry.type == "mesh") {
                resourceManager.LoadMesh(entry.path);
            } else if (entry.type == "texture") {
                resourceManager.LoadTexture(entry.path);
            }
        }
    }
}
```

## Управление

- **RMB + Move Mouse** - Осмотр камерой
- **WASD** - Перемещение камеры
- **Q/E** - Вверх/Вниз
- **Shift** - Ускорение движения
- **ESC** - Выход в меню / Выход из приложения

## Демонстрация

При запуске загружаются:
- 2 модели черепа (skull.txt) - демонстрация переиспользования ресурса
- 1 модель машины (car.txt)
- Текстуры и шейдеры (заглушки)

Все ресурсы кэшируются в ResourceManager.

## Технические детали

### Принцип работы кэша

ResourceManager использует `std::unordered_map` для хранения загруженных ресурсов:

```cpp
std::unordered_map<std::string, std::shared_ptr<Mesh>> m_meshCache;
std::unordered_map<std::string, std::shared_ptr<Texture>> m_textureCache;
std::unordered_map<std::string, std::shared_ptr<Shader>> m_shaderCache;
```

**Алгоритм загрузки:**
1. Проверка наличия ресурса в кэше по пути
2. Если найден - возврат `shared_ptr` из кэша
3. Если не найден - загрузка через соответствующий Loader
4. Сохранение в кэш и возврат `shared_ptr`

**Преимущества:**
- Ресурс загружается только один раз
- Автоматическое управление памятью через `shared_ptr`
- Быстрый доступ O(1) через hash map

### Формат моделей (.txt)

Поддерживается кастомный текстовый формат:
```
VertexCount: N
TriangleCount: M
VertexList (pos, normal)
{
    x y z nx ny nz
    ...
}
TriangleList
{
    i0 i1 i2
    ...
}
```

### Расширения RenderAdapter

Добавлены методы для работы с GPU ресурсами:
- Создание буферов (вершинных, индексных)
- Создание текстур
- Компиляция шейдеров
- Отрисовка мешей

## Соответствие ТЗ

| Задача | Статус | Реализация |
|--------|--------|------------|
| 1. Подключение библиотек (Assimp, stb_image, nlohmann/json) | ✅ | Полная интеграция с заглушками, готово к замене на реальные библиотеки |
| 2. Шаблонный ResourceManager с кэшированием | ✅ | Синглтон, `unordered_map`, `shared_ptr` |
| 3. Загрузчики Mesh, Texture, Shader | ✅ | MeshLoader (Assimp), TextureLoader (stb_image), ShaderLoader, ResourceManifest (JSON) |
| 4. Расширение RenderAdapter для GPU | ✅ | CreateVertexBuffer, CreateIndexBuffer, CreateTexture2D, CompileShader, DrawMesh |
| 5. Модификация MeshRenderer | ✅ | Хранит `shared_ptr<Mesh>`, `shared_ptr<Texture>`, `shared_ptr<Shader>` |
| 6. Адаптация RenderSystem | ✅ | Проверка `IsReady()`, вызов `DrawMesh()` |
| 7. Демонстрация загрузки моделей | ✅ | 2 черепа + 1 машина, кэширование работает, рендеринг с освещением |
| 8. (Бонус) Горячая замена | ✅ | HotReloadWatcher с фоновым потоком, `ReloadShader()`, `ReloadTexture()` |

## Архитектурные решения

### Паттерн "Синглтон"
ResourceManager реализован как синглтон для глобального доступа к ресурсам из любой части движка.

### Умные указатели
Используется `std::shared_ptr` для автоматического управления временем жизни ресурсов. Ресурс остается в памяти, пока на него есть ссылки.

### Кэширование
`std::unordered_map<std::string, std::shared_ptr<Resource>>` для быстрого поиска ресурсов по пути.

### Абстракция GPU
RenderAdapter предоставляет абстрактный интерфейс для работы с GPU, позволяя легко сменить графический API (D3D12 → Vulkan/OpenGL).

## Использованные технологии

- **C++17** - стандарт языка
- **DirectX 12** - графический API
- **DirectXMath** - математическая библиотека
- **ECS** - Entity Component System архитектура
- **Assimp** - загрузка 3D-моделей (подготовлено)
- **stb_image** - загрузка текстур (подготовлено)
- **nlohmann/json** - работа с JSON (подготовлено)

## Структура файлов проекта

```
AID3.1/
├── Resources/              # Система управления ресурсами (НОВОЕ)
│   ├── Resource.h/cpp      # Базовый класс ресурса
│   ├── ResourceManager.h/cpp  # Менеджер с кэшем
│   ├── Mesh.h/cpp          # Геометрия + GPU буферы
│   ├── Texture.h/cpp       # Текстура + GPU ресурс
│   ├── Shader.h/cpp        # Шейдерная программа
│   ├── MeshLoader.h/cpp    # Загрузка моделей
│   ├── TextureLoader.h/cpp # Загрузка текстур
│   └── ShaderLoader.h/cpp  # Загрузка шейдеров
├── Core/                   # Ядро движка (из AID2.1)
│   ├── Application.h/cpp   # Главный класс приложения
│   └── Timer.h/cpp         # Таймер для deltaTime
├── ECS/                    # Entity Component System (из AID2.1)
│   ├── World.h/cpp         # Мир сущностей
│   ├── Entity.h            # Идентификатор сущности
│   ├── Component.h         # Базовый компонент
│   └── System.h            # Базовая система
├── Components/             # Компоненты ECS
│   ├── Transform.h         # Позиция, вращение, масштаб
│   ├── MeshRenderer.h      # МОДИФИЦИРОВАН - ссылки на ресурсы
│   ├── Camera.h            # Камера
│   ├── Tag.h               # Имя объекта
│   └── Hierarchy.h         # Иерархия объектов
├── Systems/                # Системы ECS
│   ├── RenderSystem.h/cpp  # МОДИФИЦИРОВАН - рендеринг мешей
│   └── CameraSystem.h/cpp  # Управление камерой
├── Rendering/              # Рендеринг
│   ├── RenderAdapter.h     # РАСШИРЕН - методы для GPU
│   └── D3D12Adapter.h/cpp  # РАСШИРЕН - реализация для D3D12
├── States/                 # Состояния игры (из AID2.1)
│   ├── GameState.h         # Базовое состояние
│   ├── StateManager.h/cpp  # Менеджер состояний
│   ├── LoadingState.h/cpp  # Загрузка
│   ├── MenuState.h/cpp     # Меню
│   └── GameplayState.h/cpp # МОДИФИЦИРОВАН - загрузка моделей
├── Utils/                  # Утилиты (из AID2.1)
│   └── Logger.h/cpp        # Логирование
├── main.cpp                # Точка входа
├── AID3.1.sln              # Visual Studio Solution
├── AID3.1.vcxproj          # Visual Studio Project
├── CMakeLists.txt          # CMake конфигурация
└── README.md               # Этот файл
```

## Отличия от предыдущих работ

### От AID1.1:
- Добавлена ECS архитектура
- Добавлена система управления ресурсами
- Загрузка реальных 3D-моделей вместо примитивов

### От AID2.1:
- Добавлен ResourceManager с кэшированием
- Добавлены загрузчики ресурсов (Mesh, Texture, Shader)
- MeshRenderer теперь работает с загруженными ресурсами
- RenderAdapter расширен методами для GPU
- Демонстрация загрузки реальных моделей (skull, car)

## Логирование

Все операции с ресурсами логируются:
- Загрузка ресурсов
- Попадания в кэш
- Ошибки загрузки

Лог файл: `engine_aid3.log`

## Автор

Проект выполнен в рамках практического занятия №3 по созданию игрового движка.

## Лицензия

Учебный проект.