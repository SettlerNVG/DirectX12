#include "ImGuiHelper.h"
#include "../Utils/Logger.h"

// Заглушка - ImGui требует дополнительных библиотек
// Вместо этого используем простой текстовый вывод через Windows GDI

bool ImGuiHelper::Initialize(HWND hwnd, ID3D12Device* device, int numFrames,
                             DXGI_FORMAT rtvFormat, ID3D12DescriptorHeap* srvHeap) {
    LOG_INFO("ImGui Helper initialized (stub)");
    return true;
}

void ImGuiHelper::Shutdown() {
    LOG_INFO("ImGui Helper shutdown");
}

void ImGuiHelper::NewFrame() {
    // Stub
}

void ImGuiHelper::Render(ID3D12GraphicsCommandList* commandList) {
    // Stub
}

void ImGuiHelper::ProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Stub
}
