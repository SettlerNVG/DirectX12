#pragma once

#include <d3d12.h>
#include <Windows.h>

class ImGuiHelper {
public:
    static bool Initialize(HWND hwnd, ID3D12Device* device, int numFrames,
                          DXGI_FORMAT rtvFormat, ID3D12DescriptorHeap* srvHeap);
    static void Shutdown();
    
    static void NewFrame();
    static void Render(ID3D12GraphicsCommandList* commandList);
    
    static void ProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
