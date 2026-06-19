#include "EditorGUI.h"
#include "../ECS/World.h"
#include "../Physics/PhysicsSystem.h"
#include "../Systems/RenderSystem.h"
#include "../Systems/CameraSystem.h"
#include "../Input/InputManager.h"
#include "../Components/Transform.h"
#include "../Components/Tag.h"
#include "../Components/MeshRenderer.h"
#include "../Components/Rigidbody.h"
#include "../Components/Collider.h"
#include "../Components/Camera.h"
#include "../Resources/ResourceCatalog.h"
#include "../Utils/Logger.h"
#include "../Rendering/D3D12Adapter.h"

// Dear ImGui
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

// ImGuizmo для 3D манипуляторов
#include <ImGuizmo.h>

// DirectX 12
#include <d3d12.h>
#include <dxgi1_4.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <regex>
#include <sstream>

EditorGUI::EditorGUI()
    : m_selectedEntity(0)
    , m_hasSelectedEntity(false)
    , m_isPlayMode(false)
    , m_gizmoOperation(GizmoOperation::Translate)
    , m_gizmoMode(GizmoMode::World)
    , m_useSnap(false)
    , m_frameTime(0.0f)
    , m_fps(0.0f)
    , m_frameCount(0)
    , m_fpsTimer(0.0f)
    , m_showSceneHierarchy(true)
    , m_showInspector(true)
    , m_showViewport(true)
    , m_showStatistics(true)
    , m_showAssetBrowser(false)
    , m_showConsole(false)
    , m_showDemo(false)
    , m_viewportFocused(false)
    , m_viewportHovered(false)
    , m_viewportX(0.0f)
    , m_viewportY(0.0f)
    , m_viewportWidth(0.0f)
    , m_viewportHeight(0.0f)
    , m_renderedObjects(0)
    , m_activeCollisions(0)
    , m_estimatedResourceBytes(0)
    , m_resourceCount(0)
    , m_dx12BackendInitialized(false)
    , m_imguiSrvCpuStart{}
    , m_imguiSrvGpuStart{}
    , m_imguiSrvDescriptorSize(0)
    , m_initialized(false)
    , m_renderer(nullptr) {
    
    // Инициализируем snap значения
    m_snapValues[0] = 1.0f; // translate
    m_snapValues[1] = 15.0f; // rotate (degrees)
    m_snapValues[2] = 0.5f; // scale
}

EditorGUI::~EditorGUI() {
    if (m_initialized) {
        Shutdown();
    }
}

bool EditorGUI::Initialize(HWND hwnd, RenderAdapter* renderer) {
    LOG_INFO("Initializing EditorGUI with Dear ImGui...");
    
    m_renderer = renderer;
    
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    // Включаем docking и viewport
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    
    // Настраиваем стиль
    ImGui::StyleColorsDark();
    
    // Когда viewports включены, настраиваем стиль для окон
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    
    // Инициализируем платформенные бэкенды
    if (!ImGui_ImplWin32_Init(hwnd)) {
        LOG_ERROR("Failed to initialize ImGui Win32 backend");
        return false;
    }
    
    // Получаем DirectX 12 устройство от рендера
    auto* d3d12Adapter = dynamic_cast<D3D12Adapter*>(renderer);
    if (!d3d12Adapter) {
        LOG_ERROR("Renderer is not D3D12Adapter");
        ImGui_ImplWin32_Shutdown();
        return false;
    }
    
    // Инициализируем DirectX 12 backend (правильные параметры)
    ID3D12DescriptorHeap* imguiSrvHeap = d3d12Adapter->GetSRVHeap();
    m_imguiSrvCpuStart = imguiSrvHeap->GetCPUDescriptorHandleForHeapStart();
    m_imguiSrvGpuStart = imguiSrvHeap->GetGPUDescriptorHandleForHeapStart();
    m_imguiSrvDescriptorSize = d3d12Adapter->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_imguiSrvDescriptorUsed.assign(imguiSrvHeap->GetDesc().NumDescriptors, 0);

    ImGui_ImplDX12_InitInfo initInfo;
    initInfo.Device = d3d12Adapter->GetDevice();
    initInfo.CommandQueue = d3d12Adapter->GetCommandQueue();
    initInfo.NumFramesInFlight = D3D12Adapter::GetFrameCount();
    initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    initInfo.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    initInfo.SrvDescriptorHeap = imguiSrvHeap;
    initInfo.SrvDescriptorAllocFn = &EditorGUI::AllocateImGuiSrvDescriptor;
    initInfo.SrvDescriptorFreeFn = &EditorGUI::FreeImGuiSrvDescriptor;
    initInfo.UserData = this;

    if (!ImGui_ImplDX12_Init(&initInfo)) {
        LOG_ERROR("Failed to initialize ImGui DirectX 12 backend");
        ImGui_ImplWin32_Shutdown();
        return false;
    }
    m_dx12BackendInitialized = true;
    
    m_initialized = true;
    LOG_INFO("EditorGUI initialized successfully with Dear ImGui");
    return true;
}

void EditorGUI::Shutdown() {
    if (!m_initialized) return;
    
    LOG_INFO("Shutting down EditorGUI...");
    
    // Shutdown ImGui
    if (m_dx12BackendInitialized) {
        ImGui_ImplDX12_Shutdown();
        m_dx12BackendInitialized = false;
    }
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    m_imguiSrvDescriptorUsed.clear();
    
    m_initialized = false;
    LOG_INFO("EditorGUI shutdown complete");
}

void EditorGUI::AllocateImGuiSrvDescriptor(ImGui_ImplDX12_InitInfo* info,
                                           D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle,
                                           D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle) {
    if (!info || !info->UserData || !outCpuHandle || !outGpuHandle) {
        return;
    }

    auto* editor = static_cast<EditorGUI*>(info->UserData);
    for (size_t index = 0; index < editor->m_imguiSrvDescriptorUsed.size(); ++index) {
        if (editor->m_imguiSrvDescriptorUsed[index] != 0) {
            continue;
        }

        editor->m_imguiSrvDescriptorUsed[index] = 1;
        outCpuHandle->ptr = editor->m_imguiSrvCpuStart.ptr + index * editor->m_imguiSrvDescriptorSize;
        outGpuHandle->ptr = editor->m_imguiSrvGpuStart.ptr + index * editor->m_imguiSrvDescriptorSize;
        return;
    }

    LOG_ERROR("ImGui SRV descriptor heap exhausted");
    outCpuHandle->ptr = 0;
    outGpuHandle->ptr = 0;
}

void EditorGUI::FreeImGuiSrvDescriptor(ImGui_ImplDX12_InitInfo* info,
                                       D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
                                       D3D12_GPU_DESCRIPTOR_HANDLE) {
    if (!info || !info->UserData) {
        return;
    }

    auto* editor = static_cast<EditorGUI*>(info->UserData);
    if (editor->m_imguiSrvDescriptorSize == 0 || cpuHandle.ptr < editor->m_imguiSrvCpuStart.ptr) {
        return;
    }

    const size_t index = static_cast<size_t>((cpuHandle.ptr - editor->m_imguiSrvCpuStart.ptr) / editor->m_imguiSrvDescriptorSize);
    if (index < editor->m_imguiSrvDescriptorUsed.size()) {
        editor->m_imguiSrvDescriptorUsed[index] = 0;
    }
}

void EditorGUI::Update(World* world, float deltaTime) {
    if (!m_initialized || !world) return;
    
    // Обновляем статистику FPS
    m_frameTime = deltaTime * 1000.0f; // в миллисекундах
    m_frameCount++;
    m_fpsTimer += deltaTime;
    
    if (m_fpsTimer >= 1.0f) {
        m_fps = static_cast<float>(m_frameCount) / m_fpsTimer;
        m_frameCount = 0;
        m_fpsTimer = 0.0f;
    }

    m_resourceCount = static_cast<int>(ResourceCatalog::GetInstance().GetAssetCount());
    m_estimatedResourceBytes = static_cast<size_t>(ResourceCatalog::GetInstance().GetTotalSizeBytes());
    
    // Start ImGui frame
    if (m_dx12BackendInitialized) {
        ImGui_ImplDX12_NewFrame();
    }
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    // Инициализируем ImGuizmo для текущего кадра
    ImGuizmo::BeginFrame();
    
    // Создаем dockspace
    CreateDockSpace();
    
    // Создаем главное меню
    CreateMainMenuBar(world);
    
    // Создаем панель инструментов
    CreateToolbar();
    
    // Создаем панели редактора
    if (m_showSceneHierarchy) {
        CreateSceneHierarchy(world);
    }
    
    if (m_showInspector) {
        CreateInspector(world);
    }
    
    if (m_showViewport) {
        CreateViewport(world);
    }
    
    if (m_showStatistics) {
        CreateStatistics(world, deltaTime);
    }
    
    if (m_showAssetBrowser) {
        CreateAssetBrowser();
    }
    
    if (m_showConsole) {
        CreateConsole();
    }
    
    // Показываем demo окно если нужно
    // Обрабатываем Gizmo
    HandleGizmo(world);
}

void EditorGUI::Render() {
    if (!m_initialized) return;
    
    // Render ImGui
    ImGui::Render();
    
    // Получаем command list от рендера для отрисовки ImGui
    auto* d3d12Adapter = dynamic_cast<D3D12Adapter*>(m_renderer);
    if (m_dx12BackendInitialized && d3d12Adapter && d3d12Adapter->GetCommandList()) {
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), d3d12Adapter->GetCommandList());
    }
    
    // Update and Render additional Platform Windows
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

bool EditorGUI::WantCaptureMouse() const {
    if (!m_initialized) return false;
    return ImGui::GetIO().WantCaptureMouse;
}

bool EditorGUI::WantCaptureKeyboard() const {
    if (!m_initialized) return false;
    return ImGui::GetIO().WantCaptureKeyboard;
}

void EditorGUI::CreateMainMenuBar(World* world) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                NewScene(world);
            }
            if (ImGui::MenuItem("Open Scene", "Ctrl+O")) {
                LoadScene(world, "saved_scene.json");
            }
            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                SaveScene(world, "saved_scene.json");
            }
            if (ImGui::MenuItem("Save Scene As", "Ctrl+Shift+S")) {
                SaveScene(world, "saved_scene_as.json");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Import Asset")) {
                LOG_INFO("Import Asset requested");
            }
            if (ImGui::MenuItem("Export Scene")) {
                LOG_INFO("Export Scene requested");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                LOG_INFO("Exit requested");
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                LOG_INFO("Undo requested");
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                LOG_INFO("Redo requested");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "Ctrl+X")) {
                LOG_INFO("Cut requested");
            }
            if (ImGui::MenuItem("Copy", "Ctrl+C")) {
                LOG_INFO("Copy requested");
            }
            if (ImGui::MenuItem("Paste", "Ctrl+V")) {
                LOG_INFO("Paste requested");
            }
            if (ImGui::MenuItem("Delete", "Del")) {
                if (m_hasSelectedEntity && world && world->IsEntityValid(m_selectedEntity)) {
                    world->DestroyEntity(m_selectedEntity);
                    m_hasSelectedEntity = false;
                    m_selectedEntity = 0;
                    LOG_INFO("Deleted selected entity");
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Select All", "Ctrl+A")) {
                LOG_INFO("Select All requested");
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Scene Hierarchy", nullptr, &m_showSceneHierarchy);
            ImGui::MenuItem("Inspector", nullptr, &m_showInspector);
            ImGui::MenuItem("Viewport", nullptr, &m_showViewport);
            ImGui::MenuItem("Statistics", nullptr, &m_showStatistics);
            ImGui::MenuItem("Asset Browser", nullptr, &m_showAssetBrowser);
            ImGui::MenuItem("Console", nullptr, &m_showConsole);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("GameObject")) {
            if (ImGui::MenuItem("Create Empty")) {
                CreatePrimitive(world, static_cast<int>(PrimitiveType::Triangle), "Empty Entity");
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("3D Object")) {
                if (ImGui::MenuItem("Cube")) {
                    CreatePrimitive(world, static_cast<int>(PrimitiveType::Cube), "Cube");
                }
                if (ImGui::MenuItem("Sphere")) {
                    CreatePrimitive(world, static_cast<int>(PrimitiveType::Cube), "Sphere Proxy");
                }
                if (ImGui::MenuItem("Plane")) {
                    CreatePrimitive(world, static_cast<int>(PrimitiveType::Quad), "Plane");
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Light")) {
                if (ImGui::MenuItem("Directional Light")) {
                    LOG_INFO("Create Directional Light requested");
                }
                if (ImGui::MenuItem("Point Light")) {
                    LOG_INFO("Create Point Light requested");
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Documentation")) {
                LOG_INFO("Documentation requested");
            }
            if (ImGui::MenuItem("Shortcuts")) {
                LOG_INFO("Shortcuts requested");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("About AID5.1")) {
                LOG_INFO("About dialog requested");
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
}

void EditorGUI::CreateDockSpace() {
    // Создаем dockspace поверх главного viewport
    static bool dockspaceOpen = true;
    static bool opt_fullscreen_persistant = true;
    bool opt_fullscreen = opt_fullscreen_persistant;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    // Мы используем ImGuiWindowFlags_NoDocking чтобы сделать родительское окно не dockable
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen) {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    // Когда используем ImGuiDockNodeFlags_PassthruCentralNode, DockSpace будет рендерить фон
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    // Важно: обратите внимание, что мы продолжаем использовать ImGuiWindowFlags_NoMove даже когда opt_fullscreen == false.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
    ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    ImGui::End();
}

void EditorGUI::CreateSceneHierarchy(World* world) {
    if (ImGui::Begin("Scene Hierarchy", &m_showSceneHierarchy)) {
        // Получаем все сущности
        auto entities = world->GetAllEntities();
        
        for (uint32_t entity : entities) {
            auto* tag = world->GetComponent<Tag>(entity);
            std::string name = tag ? tag->name : ("Entity " + std::to_string(entity));
            
            bool isSelected = (m_hasSelectedEntity && m_selectedEntity == entity);
            
            if (ImGui::Selectable(name.c_str(), isSelected)) {
                m_selectedEntity = entity;
                m_hasSelectedEntity = true;
                LOG_INFO("Selected entity: " + name);
            }
        }
    }
    ImGui::End();
}

void EditorGUI::CreateInspector(World* world) {
    if (ImGui::Begin("Inspector", &m_showInspector)) {
        if (m_hasSelectedEntity && world->IsEntityValid(m_selectedEntity)) {
            ImGui::Text("Entity ID: %u", m_selectedEntity);
            ImGui::Separator();
            
            // Transform компонент
            if (auto* transform = world->GetComponent<Transform>(m_selectedEntity)) {
                if (ImGui::TreeNode("Transform")) {
                    float pos[3] = { transform->position.x, transform->position.y, transform->position.z };
                    if (ImGui::DragFloat3("Position", pos, 0.1f)) {
                        transform->position.x = pos[0];
                        transform->position.y = pos[1];
                        transform->position.z = pos[2];
                    }
                    
                    float rot[3] = {
                        DirectX::XMConvertToDegrees(transform->rotation.x),
                        DirectX::XMConvertToDegrees(transform->rotation.y),
                        DirectX::XMConvertToDegrees(transform->rotation.z)
                    };
                    if (ImGui::DragFloat3("Rotation", rot, 1.0f)) {
                        transform->rotation.x = DirectX::XMConvertToRadians(rot[0]);
                        transform->rotation.y = DirectX::XMConvertToRadians(rot[1]);
                        transform->rotation.z = DirectX::XMConvertToRadians(rot[2]);
                    }
                    
                    float scale[3] = { transform->scale.x, transform->scale.y, transform->scale.z };
                    if (ImGui::DragFloat3("Scale", scale, 0.1f)) {
                        transform->scale.x = scale[0];
                        transform->scale.y = scale[1];
                        transform->scale.z = scale[2];
                    }
                    
                    ImGui::TreePop();
                }
            }
            
            // Tag компонент
            if (auto* tag = world->GetComponent<Tag>(m_selectedEntity)) {
                if (ImGui::TreeNode("Tag")) {
                    char buffer[256];
                    strncpy_s(buffer, tag->name.c_str(), sizeof(buffer) - 1);
                    buffer[sizeof(buffer) - 1] = '\0';
                    
                    if (ImGui::InputText("Name", buffer, sizeof(buffer))) {
                        tag->name = buffer;
                    }
                    
                    ImGui::TreePop();
                }
            }
            
            // MeshRenderer компонент
            if (auto* meshRenderer = world->GetComponent<MeshRenderer>(m_selectedEntity)) {
                if (ImGui::TreeNode("Mesh Renderer")) {
                    float color[4] = { 
                        meshRenderer->color.x, 
                        meshRenderer->color.y, 
                        meshRenderer->color.z,
                        meshRenderer->color.w
                    };
                    if (ImGui::ColorEdit4("Color", color)) {
                        meshRenderer->color.x = color[0];
                        meshRenderer->color.y = color[1];
                        meshRenderer->color.z = color[2];
                        meshRenderer->color.w = color[3];
                    }
                    
                    ImGui::Checkbox("Visible", &meshRenderer->visible);
                    
                    // Primitive Type
                    const char* primitiveTypes[] = { "Triangle", "Quad", "Cube" };
                    int currentType = static_cast<int>(meshRenderer->primitiveType);
                    if (ImGui::Combo("Primitive Type", &currentType, primitiveTypes, 3)) {
                        meshRenderer->primitiveType = static_cast<PrimitiveType>(currentType);
                    }

                    auto drawAssetCombo = [](const char* label, AssetType type, std::string& selectedPath) {
                        auto assets = ResourceCatalog::GetInstance().GetAssetsByType(type);
                        std::string currentName = selectedPath.empty() ? "None" : selectedPath;
                        for (const AssetInfo* asset : assets) {
                            if (asset && asset->path == selectedPath) {
                                currentName = asset->name;
                                break;
                            }
                        }

                        if (ImGui::BeginCombo(label, currentName.c_str())) {
                            bool noneSelected = selectedPath.empty();
                            if (ImGui::Selectable("None", noneSelected)) {
                                selectedPath.clear();
                            }
                            if (noneSelected) {
                                ImGui::SetItemDefaultFocus();
                            }

                            for (const AssetInfo* asset : assets) {
                                if (!asset) {
                                    continue;
                                }
                                bool selected = (selectedPath == asset->path);
                                if (ImGui::Selectable(asset->name.c_str(), selected)) {
                                    selectedPath = asset->path;
                                }
                                if (ImGui::IsItemHovered()) {
                                    ImGui::SetTooltip("%s", asset->path.c_str());
                                }
                                if (selected) {
                                    ImGui::SetItemDefaultFocus();
                                }
                            }
                            ImGui::EndCombo();
                        }
                    };

                    drawAssetCombo("Mesh", AssetType::Mesh, meshRenderer->meshAssetPath);
                    drawAssetCombo("Texture", AssetType::Texture, meshRenderer->textureAssetPath);
                    drawAssetCombo("Shader", AssetType::Shader, meshRenderer->shaderAssetPath);
                    
                    ImGui::TreePop();
                }
            }
            
            // Rigidbody компонент
            if (auto* rigidbody = world->GetComponent<Rigidbody>(m_selectedEntity)) {
                if (ImGui::TreeNode("Rigidbody")) {
                    ImGui::DragFloat("Mass", &rigidbody->mass, 0.1f, 0.1f, 100.0f);
                    ImGui::Checkbox("Use Gravity", &rigidbody->useGravity);
                    ImGui::Checkbox("Is Kinematic", &rigidbody->isKinematic);
                    
                    ImGui::Separator();
                    ImGui::Text("Physics Properties:");
                    ImGui::DragFloat("Drag", &rigidbody->drag, 0.01f, 0.0f, 1.0f);
                    ImGui::DragFloat("Restitution", &rigidbody->restitution, 0.01f, 0.0f, 1.0f);
                    
                    ImGui::Separator();
                    ImGui::Text("Runtime Info:");
                    // Показываем скорость только для чтения
                    ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", 
                        rigidbody->velocity.x, rigidbody->velocity.y, rigidbody->velocity.z);
                    
                    ImGui::Checkbox("Is Sleeping", &rigidbody->isSleeping);
                    ImGui::Checkbox("Is Grounded", &rigidbody->isGrounded);
                    
                    if (ImGui::Button("Add Impulse Up")) {
                        rigidbody->AddImpulse({0.0f, 5.0f, 0.0f});
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Reset Velocity")) {
                        rigidbody->velocity = {0.0f, 0.0f, 0.0f};
                    }
                    
                    ImGui::TreePop();
                }
            }
            
            // Collider компонент
            if (auto* collider = world->GetComponent<Collider>(m_selectedEntity)) {
                if (ImGui::TreeNode("Collider")) {
                    ImGui::Checkbox("Is Trigger", &collider->isTrigger);
                    
                    // Тип коллайдера
                    const char* colliderTypes[] = { "Box", "Sphere" };
                    int currentType = static_cast<int>(collider->type);
                    if (ImGui::Combo("Collider Type", &currentType, colliderTypes, 2)) {
                        collider->type = static_cast<ColliderType>(currentType);
                    }
                    
                    if (collider->type == ColliderType::Box) {
                        float extents[3] = { collider->halfExtents.x, collider->halfExtents.y, collider->halfExtents.z };
                        if (ImGui::DragFloat3("Half Extents", extents, 0.1f)) {
                            collider->halfExtents.x = extents[0];
                            collider->halfExtents.y = extents[1];
                            collider->halfExtents.z = extents[2];
                        }
                    } else if (collider->type == ColliderType::Sphere) {
                        float radius = collider->halfExtents.x;
                        if (ImGui::DragFloat("Radius", &radius, 0.1f)) {
                            collider->halfExtents.x = radius;
                            collider->halfExtents.y = radius;
                            collider->halfExtents.z = radius;
                        }
                    }
                    
                    float offset[3] = { collider->offset.x, collider->offset.y, collider->offset.z };
                    if (ImGui::DragFloat3("Offset", offset, 0.1f)) {
                        collider->offset.x = offset[0];
                        collider->offset.y = offset[1];
                        collider->offset.z = offset[2];
                    }
                    
                    ImGui::TreePop();
                }
            }
        } else {
            ImGui::Text("No entity selected");
        }
    }
    ImGui::End();
}


void EditorGUI::CreateViewport(World* world) {
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.02f, 0.025f, 0.08f));
    if (ImGui::Begin("Viewport", &m_showViewport, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        m_viewportFocused = ImGui::IsWindowFocused();
        m_viewportHovered = ImGui::IsWindowHovered();

        ImVec2 viewportPos = ImGui::GetCursorScreenPos();
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        viewportSize.x = (std::max)(viewportSize.x, 1.0f);
        viewportSize.y = (std::max)(viewportSize.y, 1.0f);

        m_viewportX = viewportPos.x;
        m_viewportY = viewportPos.y;
        m_viewportWidth = viewportSize.x;
        m_viewportHeight = viewportSize.y;

        ImGui::InvisibleButton("SceneViewportCanvas", viewportSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImU32 borderColor = IM_COL32(90, 130, 190, m_viewportFocused ? 220 : 120);
        drawList->AddRect(viewportPos, ImVec2(viewportPos.x + viewportSize.x, viewportPos.y + viewportSize.y), borderColor);
        drawList->AddText(ImVec2(viewportPos.x + 10.0f, viewportPos.y + 8.0f), IM_COL32(220, 230, 240, 210), m_isPlayMode ? "PLAY" : "EDIT");

        if (m_hasSelectedEntity && world && world->IsEntityValid(m_selectedEntity)) {
            DrawGizmo(world, m_selectedEntity);
        }
    }
    ImGui::End();
    ImGui::PopStyleColor();
}

void EditorGUI::CreateStatistics(World* world, float deltaTime) {
    if (ImGui::Begin("Statistics", &m_showStatistics)) {
        ImGui::Text("Performance");
        ImGui::Separator();
        ImGui::Text("FPS: %.1f", m_fps);
        ImGui::Text("Frame Time: %.2f ms", m_frameTime);
        
        ImGui::Text("");
        ImGui::Text("Scene");
        ImGui::Separator();
        
        auto entities = world->GetAllEntities();
        ImGui::Text("Total Entities: %zu", entities.size());
        
        // Подсчитываем объекты с MeshRenderer
        int renderableObjects = 0;
        for (uint32_t entity : entities) {
            if (world->GetComponent<MeshRenderer>(entity)) {
                renderableObjects++;
            }
        }
        ImGui::Text("Renderable Objects: %d", renderableObjects);
        ImGui::Text("Rendered Last Frame: %d", m_renderedObjects);
        
        // Подсчитываем физические объекты
        int physicsObjects = 0;
        for (uint32_t entity : entities) {
            if (world->GetComponent<Rigidbody>(entity)) {
                physicsObjects++;
            }
        }
        ImGui::Text("Physics Objects: %d", physicsObjects);
        ImGui::Text("Active Collisions: %d", m_activeCollisions);
        ImGui::Text("Resources: %d", m_resourceCount);
        ImGui::Text("Estimated Resource Memory: %.2f MB", static_cast<double>(m_estimatedResourceBytes) / (1024.0 * 1024.0));
    }
    ImGui::End();
}


void EditorGUI::CreateAssetBrowser() {
    if (ImGui::Begin("Asset Browser", &m_showAssetBrowser)) {
        auto& catalog = ResourceCatalog::GetInstance();
        ImGui::Text("Indexed Assets: %zu", catalog.GetAssetCount());
        ImGui::SameLine();
        if (ImGui::Button("Refresh")) {
            catalog.Refresh();
        }
        ImGui::Separator();

        auto drawAssetGroup = [](const char* title, AssetType type) {
            auto assets = ResourceCatalog::GetInstance().GetAssetsByType(type);
            std::string nodeTitle = std::string(title) + " (" + std::to_string(assets.size()) + ")";
            if (ImGui::TreeNode(nodeTitle.c_str())) {
                for (const AssetInfo* asset : assets) {
                    if (!asset) {
                        continue;
                    }
                    ImGui::Selectable(asset->name.c_str());
                    ImGui::SameLine();
                    ImGui::TextDisabled("%.2f KB", static_cast<double>(asset->sizeBytes) / 1024.0);
                    if (ImGui::IsItemHovered() || ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                        ImGui::SetTooltip("%s", asset->path.c_str());
                    }
                }
                ImGui::TreePop();
            }
        };

        drawAssetGroup("Meshes", AssetType::Mesh);
        drawAssetGroup("Textures", AssetType::Texture);
        drawAssetGroup("Shaders", AssetType::Shader);
        drawAssetGroup("Materials", AssetType::Material);
    }
    ImGui::End();
}


void EditorGUI::CreateConsole() {
    if (ImGui::Begin("Console", &m_showConsole)) {
        static bool showInfo = true;
        static bool showWarning = true;
        static bool showError = true;
        static bool showDebug = true;

        ImGui::Checkbox("Info", &showInfo);
        ImGui::SameLine();
        ImGui::Checkbox("Warning", &showWarning);
        ImGui::SameLine();
        ImGui::Checkbox("Error", &showError);
        ImGui::SameLine();
        ImGui::Checkbox("Debug", &showDebug);
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            Logger::GetInstance().ClearEntries();
        }

        ImGui::Separator();
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);

        const auto entries = Logger::GetInstance().GetRecentEntries();
        for (const auto& entry : entries) {
            bool visible = false;
            ImVec4 color(0.82f, 0.84f, 0.88f, 1.0f);
            switch (entry.level) {
                case LogLevel::INFO:
                    visible = showInfo;
                    color = ImVec4(0.72f, 0.95f, 0.72f, 1.0f);
                    break;
                case LogLevel::WARNING:
                    visible = showWarning;
                    color = ImVec4(1.0f, 0.88f, 0.45f, 1.0f);
                    break;
                case LogLevel::ERROR_LOG:
                    visible = showError;
                    color = ImVec4(1.0f, 0.45f, 0.45f, 1.0f);
                    break;
                case LogLevel::DEBUG:
                    visible = showDebug;
                    color = ImVec4(0.55f, 0.8f, 1.0f, 1.0f);
                    break;
            }

            if (visible) {
                ImGui::TextColored(color, "%s", entry.message.c_str());
            }
        }

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();

        static char inputBuffer[256] = "";
        if (ImGui::InputText("Command", inputBuffer, sizeof(inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
            LOG_INFO("Console command: " + std::string(inputBuffer));
            inputBuffer[0] = '\0';
        }
    }
    ImGui::End();
}

void EditorGUI::CreateToolbar() {
    if (ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        // Кнопки Play/Stop
        if (m_isPlayMode) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("■ Stop")) {
                m_isPlayMode = false;
                LOG_INFO("Switched to Edit mode");
            }
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
            if (ImGui::Button("▶ Play")) {
                m_isPlayMode = true;
                LOG_INFO("Switched to Play mode");
            }
            ImGui::PopStyleColor();
        }
        
        ImGui::SameLine();
        ImGui::Text("Mode: %s", m_isPlayMode ? "Play" : "Edit");
        
        ImGui::Separator();
        
        // Gizmo операции
        ImGui::Text("Gizmo:");
        ImGui::SameLine();
        
        if (ImGui::RadioButton("Translate", m_gizmoOperation == GizmoOperation::Translate)) {
            m_gizmoOperation = GizmoOperation::Translate;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", m_gizmoOperation == GizmoOperation::Rotate)) {
            m_gizmoOperation = GizmoOperation::Rotate;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale", m_gizmoOperation == GizmoOperation::Scale)) {
            m_gizmoOperation = GizmoOperation::Scale;
        }
        
        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();
        
        // Gizmo режим
        if (ImGui::RadioButton("World", m_gizmoMode == GizmoMode::World)) {
            m_gizmoMode = GizmoMode::World;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Local", m_gizmoMode == GizmoMode::Local)) {
            m_gizmoMode = GizmoMode::Local;
        }
        
        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();
        
        // Snap настройки
        ImGui::Checkbox("Snap", &m_useSnap);
        if (m_useSnap) {
            ImGui::SameLine();
            switch (m_gizmoOperation) {
                case GizmoOperation::Translate:
                    ImGui::DragFloat("##snap", &m_snapValues[0], 0.1f, 0.1f, 10.0f, "%.1f");
                    break;
                case GizmoOperation::Rotate:
                    ImGui::DragFloat("##snap", &m_snapValues[1], 1.0f, 1.0f, 90.0f, "%.0f°");
                    break;
                case GizmoOperation::Scale:
                    ImGui::DragFloat("##snap", &m_snapValues[2], 0.05f, 0.05f, 2.0f, "%.2f");
                    break;
            }
        }
    }
    ImGui::End();
}

void EditorGUI::HandleGizmo(World* world) {
    // Обрабатываем горячие клавиши для Gizmo
    if (ImGui::IsKeyPressed(ImGuiKey_Q)) {
        m_gizmoOperation = GizmoOperation::Translate;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_W)) {
        m_gizmoOperation = GizmoOperation::Rotate;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_E)) {
        m_gizmoOperation = GizmoOperation::Scale;
    }
    
    // Переключение между World/Local режимами
    if (ImGui::IsKeyPressed(ImGuiKey_R)) {
        m_gizmoMode = (m_gizmoMode == GizmoMode::World) ? GizmoMode::Local : GizmoMode::World;
    }
}


void EditorGUI::DrawGizmo(World* world, uint32_t entity) {
    if (!world || !m_hasSelectedEntity || !world->IsEntityValid(entity) || !HasViewportRect()) {
        return;
    }

    auto* transform = world->GetComponent<Transform>(entity);
    if (!transform) {
        return;
    }

    auto cameraEntities = world->GetEntitiesWith<Transform, Camera>();
    Transform* cameraTransform = nullptr;
    Camera* camera = nullptr;
    for (Entity cameraEntity : cameraEntities) {
        auto* candidateCamera = world->GetComponent<Camera>(cameraEntity);
        if (candidateCamera && candidateCamera->isActive) {
            camera = candidateCamera;
            cameraTransform = world->GetComponent<Transform>(cameraEntity);
            break;
        }
    }

    if (!camera || !cameraTransform) {
        return;
    }

    using namespace DirectX;
    XMVECTOR eye = XMLoadFloat3(&cameraTransform->position);
    XMVECTOR at = XMVectorAdd(eye, camera->GetForward());
    XMMATRIX viewMatrix = XMMatrixLookAtLH(eye, at, camera->GetUp());
    XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(camera->fov, GetViewportAspectRatio(), camera->nearPlane, camera->farPlane);
    XMMATRIX objectMatrix = transform->GetMatrix();

    XMFLOAT4X4 view;
    XMFLOAT4X4 projection;
    XMFLOAT4X4 object;
    XMStoreFloat4x4(&view, viewMatrix);
    XMStoreFloat4x4(&projection, projectionMatrix);
    XMStoreFloat4x4(&object, objectMatrix);

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
    ImGuizmo::SetRect(m_viewportX, m_viewportY, m_viewportWidth, m_viewportHeight);

    ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
    switch (m_gizmoOperation) {
        case GizmoOperation::Translate: operation = ImGuizmo::TRANSLATE; break;
        case GizmoOperation::Rotate: operation = ImGuizmo::ROTATE; break;
        case GizmoOperation::Scale: operation = ImGuizmo::SCALE; break;
    }

    ImGuizmo::MODE mode = (m_gizmoMode == GizmoMode::World) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
    float snap[3] = { m_snapValues[0], m_snapValues[0], m_snapValues[0] };
    if (m_gizmoOperation == GizmoOperation::Rotate) {
        snap[0] = snap[1] = snap[2] = m_snapValues[1];
    } else if (m_gizmoOperation == GizmoOperation::Scale) {
        snap[0] = snap[1] = snap[2] = m_snapValues[2];
    }

    if (ImGuizmo::Manipulate(
            reinterpret_cast<float*>(&view),
            reinterpret_cast<float*>(&projection),
            operation,
            mode,
            reinterpret_cast<float*>(&object),
            nullptr,
            m_useSnap ? snap : nullptr)) {
        float translation[3];
        float rotationDegrees[3];
        float scale[3];
        ImGuizmo::DecomposeMatrixToComponents(reinterpret_cast<float*>(&object), translation, rotationDegrees, scale);

        transform->position = XMFLOAT3(translation[0], translation[1], translation[2]);
        transform->rotation = XMFLOAT3(
            XMConvertToRadians(rotationDegrees[0]),
            XMConvertToRadians(rotationDegrees[1]),
            XMConvertToRadians(rotationDegrees[2]));
        transform->scale = XMFLOAT3(scale[0], scale[1], scale[2]);
    }
}

void EditorGUI::SetRuntimeStats(int renderedObjects, int activeCollisions) {
    m_renderedObjects = renderedObjects;
    m_activeCollisions = activeCollisions;
}

void EditorGUI::CreatePrimitive(World* world, int primitiveType, const char* name) {
    if (!world) {
        return;
    }

    Entity entity = world->CreateEntity();

    Transform transform;
    transform.position = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
    transform.scale = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
    world->AddComponent<Transform>(entity, transform);

    MeshRenderer mesh;
    mesh.primitiveType = static_cast<PrimitiveType>(primitiveType);
    mesh.color = DirectX::XMFLOAT4(0.35f, 0.72f, 0.95f, 1.0f);
    world->AddComponent<MeshRenderer>(entity, mesh);

    Tag tag;
    tag.name = name ? name : "Entity";
    world->AddComponent<Tag>(entity, tag);

    Collider collider;
    collider.type = ColliderType::Box;
    collider.halfExtents = DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f);
    world->AddComponent<Collider>(entity, collider);

    m_selectedEntity = entity;
    m_hasSelectedEntity = true;
    LOG_INFO("Created entity: " + tag.name);
}

static void WriteVec3(std::ostream& out, const DirectX::XMFLOAT3& value) {
    out << "[" << value.x << "," << value.y << "," << value.z << "]";
}

static void WriteVec4(std::ostream& out, const DirectX::XMFLOAT4& value) {
    out << "[" << value.x << "," << value.y << "," << value.z << "," << value.w << "]";
}

static std::string EscapeJsonString(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    for (char ch : value) {
        if (ch == '\\' || ch == '"') {
            result.push_back('\\');
        }
        result.push_back(ch);
    }
    return result;
}

void EditorGUI::SaveScene(World* world, const char* path) {
    if (!world || !path) {
        return;
    }

    std::ofstream file(path);
    if (!file) {
        LOG_ERROR(std::string("Failed to save scene: ") + path);
        return;
    }

    file << "{\n  \"scene\": \"AID5.1 Editor Scene\",\n  \"entities\": [\n";
    const auto& entities = world->GetAllEntities();
    bool firstEntity = true;
    for (Entity entity : entities) {
        if (!firstEntity) {
            file << ",\n";
        }
        firstEntity = false;

        file << "    {\n";
        file << "      \"id\": " << entity;

        if (auto* tag = world->GetComponent<Tag>(entity)) {
            file << ",\n      \"tag\": \"" << EscapeJsonString(tag->name) << "\"";
        }
        if (auto* transform = world->GetComponent<Transform>(entity)) {
            file << ",\n      \"transform\": { \"position\": ";
            WriteVec3(file, transform->position);
            file << ", \"rotation\": ";
            WriteVec3(file, transform->rotation);
            file << ", \"scale\": ";
            WriteVec3(file, transform->scale);
            file << " }";
        }
        if (auto* mesh = world->GetComponent<MeshRenderer>(entity)) {
            file << ",\n      \"meshRenderer\": { \"primitiveType\": " << static_cast<int>(mesh->primitiveType)
                 << ", \"visible\": " << (mesh->visible ? "true" : "false")
                 << ", \"meshAssetPath\": \"" << EscapeJsonString(mesh->meshAssetPath) << "\""
                 << ", \"textureAssetPath\": \"" << EscapeJsonString(mesh->textureAssetPath) << "\""
                 << ", \"shaderAssetPath\": \"" << EscapeJsonString(mesh->shaderAssetPath) << "\""
                 << ", \"color\": ";
            WriteVec4(file, mesh->color);
            file << " }";
        }
        if (auto* rigidbody = world->GetComponent<Rigidbody>(entity)) {
            file << ",\n      \"rigidbody\": { \"mass\": " << rigidbody->mass
                 << ", \"useGravity\": " << (rigidbody->useGravity ? "true" : "false")
                 << ", \"isKinematic\": " << (rigidbody->isKinematic ? "true" : "false")
                 << ", \"velocity\": ";
            WriteVec3(file, rigidbody->velocity);
            file << " }";
        }
        if (auto* collider = world->GetComponent<Collider>(entity)) {
            file << ",\n      \"collider\": { \"type\": " << static_cast<int>(collider->type)
                 << ", \"isTrigger\": " << (collider->isTrigger ? "true" : "false")
                 << ", \"halfExtents\": ";
            WriteVec3(file, collider->halfExtents);
            file << ", \"offset\": ";
            WriteVec3(file, collider->offset);
            file << " }";
        }
        if (auto* camera = world->GetComponent<Camera>(entity)) {
            file << ",\n      \"camera\": { \"fov\": " << camera->fov
                 << ", \"nearPlane\": " << camera->nearPlane
                 << ", \"farPlane\": " << camera->farPlane
                 << ", \"aspectRatio\": " << camera->aspectRatio
                 << ", \"yaw\": " << camera->yaw
                 << ", \"pitch\": " << camera->pitch
                 << ", \"isActive\": " << (camera->isActive ? "true" : "false")
                 << " }";
        }

        file << "\n    }";
    }
    file << "\n  ]\n}\n";
    LOG_INFO(std::string("Scene saved to ") + path);
}

static std::string UnescapeJsonString(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    bool escaped = false;
    for (char ch : value) {
        if (escaped) {
            result.push_back(ch);
            escaped = false;
        } else if (ch == '\\') {
            escaped = true;
        } else {
            result.push_back(ch);
        }
    }
    return result;
}

static bool ExtractStringField(const std::string& object, const std::string& key, std::string& out) {
    std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    std::smatch match;
    if (std::regex_search(object, match, pattern)) {
        out = UnescapeJsonString(match[1].str());
        return true;
    }
    return false;
}

static bool ExtractIntField(const std::string& object, const std::string& key, int& out) {
    std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*(-?\\d+)");
    std::smatch match;
    if (std::regex_search(object, match, pattern)) {
        out = std::stoi(match[1].str());
        return true;
    }
    return false;
}

static bool ExtractFloatField(const std::string& object, const std::string& key, float& out) {
    std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*(-?\\d+(?:\\.\\d+)?)");
    std::smatch match;
    if (std::regex_search(object, match, pattern)) {
        out = std::stof(match[1].str());
        return true;
    }
    return false;
}

static bool ExtractBoolField(const std::string& object, const std::string& key, bool& out) {
    std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*(true|false)");
    std::smatch match;
    if (std::regex_search(object, match, pattern)) {
        out = (match[1].str() == "true");
        return true;
    }
    return false;
}

static bool ExtractVec3Field(const std::string& object, const std::string& key, DirectX::XMFLOAT3& out) {
    std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\[\\s*(-?\\d+(?:\\.\\d+)?)\\s*,\\s*(-?\\d+(?:\\.\\d+)?)\\s*,\\s*(-?\\d+(?:\\.\\d+)?)\\s*\\]");
    std::smatch match;
    if (std::regex_search(object, match, pattern)) {
        out = DirectX::XMFLOAT3(std::stof(match[1].str()), std::stof(match[2].str()), std::stof(match[3].str()));
        return true;
    }
    return false;
}

static bool ExtractVec4Field(const std::string& object, const std::string& key, DirectX::XMFLOAT4& out) {
    std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\[\\s*(-?\\d+(?:\\.\\d+)?)\\s*,\\s*(-?\\d+(?:\\.\\d+)?)\\s*,\\s*(-?\\d+(?:\\.\\d+)?)\\s*,\\s*(-?\\d+(?:\\.\\d+)?)\\s*\\]");
    std::smatch match;
    if (std::regex_search(object, match, pattern)) {
        out = DirectX::XMFLOAT4(std::stof(match[1].str()), std::stof(match[2].str()), std::stof(match[3].str()), std::stof(match[4].str()));
        return true;
    }
    return false;
}

static std::vector<std::string> ExtractEntityObjects(const std::string& text) {
    std::vector<std::string> objects;
    size_t entitiesPos = text.find("\"entities\"");
    if (entitiesPos == std::string::npos) {
        return objects;
    }

    size_t arrayStart = text.find('[', entitiesPos);
    if (arrayStart == std::string::npos) {
        return objects;
    }

    int depth = 0;
    size_t objectStart = std::string::npos;
    for (size_t i = arrayStart + 1; i < text.size(); ++i) {
        char ch = text[i];
        if (ch == '{') {
            if (depth == 0) {
                objectStart = i;
            }
            depth++;
        } else if (ch == '}') {
            depth--;
            if (depth == 0 && objectStart != std::string::npos) {
                objects.push_back(text.substr(objectStart, i - objectStart + 1));
                objectStart = std::string::npos;
            }
        } else if (ch == ']' && depth == 0) {
            break;
        }
    }
    return objects;
}

void EditorGUI::NewScene(World* world) {
    if (!world) {
        return;
    }

    world->Clear();
    m_hasSelectedEntity = false;
    m_selectedEntity = 0;

    Entity cameraEntity = world->CreateEntity();
    Transform cameraTransform;
    cameraTransform.position = DirectX::XMFLOAT3(0.0f, 5.0f, 10.0f);
    cameraTransform.rotation = DirectX::XMFLOAT3(DirectX::XMConvertToRadians(-20.0f), 0.0f, 0.0f);
    world->AddComponent<Transform>(cameraEntity, cameraTransform);

    Camera camera;
    camera.fov = DirectX::XM_PIDIV4;
    camera.farPlane = 1000.0f;
    camera.yaw = -DirectX::XM_PIDIV2;
    camera.pitch = DirectX::XMConvertToRadians(-20.0f);
    camera.isActive = true;
    world->AddComponent<Camera>(cameraEntity, camera);

    Tag cameraTag;
    cameraTag.name = "Editor Camera";
    world->AddComponent<Tag>(cameraEntity, cameraTag);

    Entity groundEntity = world->CreateEntity();
    Transform groundTransform;
    groundTransform.position = DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f);
    groundTransform.scale = DirectX::XMFLOAT3(20.0f, 1.0f, 20.0f);
    world->AddComponent<Transform>(groundEntity, groundTransform);

    MeshRenderer groundMesh;
    groundMesh.primitiveType = PrimitiveType::Cube;
    groundMesh.color = DirectX::XMFLOAT4(0.45f, 0.5f, 0.55f, 1.0f);
    world->AddComponent<MeshRenderer>(groundEntity, groundMesh);

    Collider groundCollider = Collider::CreateBox(10.0f, 0.5f, 10.0f);
    world->AddComponent<Collider>(groundEntity, groundCollider);

    Tag groundTag;
    groundTag.name = "Ground";
    world->AddComponent<Tag>(groundEntity, groundTag);

    LOG_INFO("New scene created");
}

void EditorGUI::LoadScene(World* world, const char* path) {
    if (!world || !path) {
        return;
    }

    std::ifstream file(path);
    if (!file) {
        LOG_ERROR(std::string("Failed to open scene: ") + path);
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    const auto objects = ExtractEntityObjects(buffer.str());

    world->Clear();
    m_hasSelectedEntity = false;
    m_selectedEntity = 0;

    bool hasCamera = false;
    for (const auto& object : objects) {
        Entity entity = world->CreateEntity();

        std::string tagName;
        if (ExtractStringField(object, "tag", tagName)) {
            Tag tag;
            tag.name = tagName;
            world->AddComponent<Tag>(entity, tag);
        }

        if (object.find("\"transform\"") != std::string::npos) {
            Transform transform;
            ExtractVec3Field(object, "position", transform.position);
            ExtractVec3Field(object, "rotation", transform.rotation);
            ExtractVec3Field(object, "scale", transform.scale);
            world->AddComponent<Transform>(entity, transform);
        }

        if (object.find("\"meshRenderer\"") != std::string::npos) {
            MeshRenderer mesh;
            int primitive = static_cast<int>(mesh.primitiveType);
            ExtractIntField(object, "primitiveType", primitive);
            mesh.primitiveType = static_cast<PrimitiveType>((std::max)(0, (std::min)(2, primitive)));
            ExtractBoolField(object, "visible", mesh.visible);
            ExtractVec4Field(object, "color", mesh.color);
            ExtractStringField(object, "meshAssetPath", mesh.meshAssetPath);
            ExtractStringField(object, "textureAssetPath", mesh.textureAssetPath);
            ExtractStringField(object, "shaderAssetPath", mesh.shaderAssetPath);
            world->AddComponent<MeshRenderer>(entity, mesh);
        }

        if (object.find("\"rigidbody\"") != std::string::npos) {
            Rigidbody rigidbody;
            ExtractFloatField(object, "mass", rigidbody.mass);
            ExtractBoolField(object, "useGravity", rigidbody.useGravity);
            ExtractBoolField(object, "isKinematic", rigidbody.isKinematic);
            ExtractVec3Field(object, "velocity", rigidbody.velocity);
            world->AddComponent<Rigidbody>(entity, rigidbody);
        }

        if (object.find("\"collider\"") != std::string::npos) {
            Collider collider;
            int colliderType = static_cast<int>(collider.type);
            ExtractIntField(object, "type", colliderType);
            collider.type = static_cast<ColliderType>((std::max)(0, (std::min)(1, colliderType)));
            ExtractBoolField(object, "isTrigger", collider.isTrigger);
            ExtractVec3Field(object, "halfExtents", collider.halfExtents);
            ExtractVec3Field(object, "offset", collider.offset);
            world->AddComponent<Collider>(entity, collider);
        }

        if (object.find("\"camera\"") != std::string::npos || tagName.find("Camera") != std::string::npos) {
            Camera camera;
            camera.yaw = -DirectX::XM_PIDIV2;
            camera.pitch = DirectX::XMConvertToRadians(-20.0f);
            ExtractFloatField(object, "fov", camera.fov);
            ExtractFloatField(object, "nearPlane", camera.nearPlane);
            ExtractFloatField(object, "farPlane", camera.farPlane);
            ExtractFloatField(object, "aspectRatio", camera.aspectRatio);
            ExtractFloatField(object, "yaw", camera.yaw);
            ExtractFloatField(object, "pitch", camera.pitch);
            ExtractBoolField(object, "isActive", camera.isActive);
            world->AddComponent<Camera>(entity, camera);
            hasCamera = true;
        }
    }

    if (!hasCamera) {
        Entity cameraEntity = world->CreateEntity();
        Transform cameraTransform;
        cameraTransform.position = DirectX::XMFLOAT3(0.0f, 5.0f, 10.0f);
        world->AddComponent<Transform>(cameraEntity, cameraTransform);
        Camera camera;
        camera.yaw = -DirectX::XM_PIDIV2;
        camera.pitch = DirectX::XMConvertToRadians(-20.0f);
        world->AddComponent<Camera>(cameraEntity, camera);
        Tag tag;
        tag.name = "Editor Camera";
        world->AddComponent<Tag>(cameraEntity, tag);
    }

    LOG_INFO(std::string("Scene loaded from ") + path + " with " + std::to_string(world->GetAllEntities().size()) + " entities");
}
