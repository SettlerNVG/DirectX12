#include "TextRenderer.h"
#include "../Utils/Logger.h"
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

TextRenderer::TextRenderer()
    : m_cbvDataBegin(nullptr) {
}

TextRenderer::~TextRenderer() {
    Shutdown();
}

bool TextRenderer::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {
    if (!CreateRootSignature(device)) return false;
    if (!CreatePipelineState(device)) return false;
    
    // Создание вершинного буфера для квадрата
    TextVertex quadVertices[] = {
        { XMFLOAT3(-0.5f,  0.5f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3( 0.5f,  0.5f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(-0.5f, -0.5f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3( 0.5f, -0.5f, 0.0f), XMFLOAT2(1.0f, 1.0f) }
    };
    
    const UINT vertexBufferSize = sizeof(quadVertices);
    
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = vertexBufferSize;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    
    if (FAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
        &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&m_vertexBuffer)))) {
        LOG_ERROR("Failed to create text vertex buffer");
        return false;
    }
    
    UINT8* pVertexDataBegin;
    D3D12_RANGE readRange = { 0, 0 };
    m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
    memcpy(pVertexDataBegin, quadVertices, vertexBufferSize);
    m_vertexBuffer->Unmap(0, nullptr);
    
    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = sizeof(TextVertex);
    m_vertexBufferView.SizeInBytes = vertexBufferSize;
    
    // Создание константного буфера
    UINT constantBufferSize = (sizeof(TextConstants) + 255) & ~255;
    resourceDesc.Width = constantBufferSize;
    
    if (FAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
        &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&m_constantBuffer)))) {
        LOG_ERROR("Failed to create text constant buffer");
        return false;
    }
    
    m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_cbvDataBegin));
    
    return true;
}

void TextRenderer::Shutdown() {
    if (m_constantBuffer && m_cbvDataBegin) {
        m_constantBuffer->Unmap(0, nullptr);
        m_cbvDataBegin = nullptr;
    }
}

bool TextRenderer::CreateRootSignature(ID3D12Device* device) {
    D3D12_ROOT_PARAMETER rootParameter = {};
    rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameter.Descriptor.ShaderRegister = 0;
    rootParameter.Descriptor.RegisterSpace = 0;
    
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 1;
    rootSigDesc.pParameters = &rootParameter;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    
    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    
    if (FAILED(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        &signature, &error))) {
        if (error) {
            LOG_ERROR("Failed to serialize text root signature: " +
                std::string((char*)error->GetBufferPointer()));
        }
        return false;
    }
    
    if (FAILED(device->CreateRootSignature(0, signature->GetBufferPointer(),
        signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)))) {
        LOG_ERROR("Failed to create text root signature");
        return false;
    }
    
    return true;
}

bool TextRenderer::CreatePipelineState(ID3D12Device* device) {
    const char* shaderCode = R"(
        cbuffer TextConstants : register(b0)
        {
            float4x4 gTransform;
            float4 gColor;
        };
        
        struct VertexIn
        {
            float3 PosL : POSITION;
            float2 TexCoord : TEXCOORD;
        };
        
        struct VertexOut
        {
            float4 PosH : SV_POSITION;
            float2 TexCoord : TEXCOORD;
        };
        
        VertexOut VS(VertexIn vin)
        {
            VertexOut vout;
            vout.PosH = mul(float4(vin.PosL, 1.0f), gTransform);
            vout.TexCoord = vin.TexCoord;
            return vout;
        }
        
        float4 PS(VertexOut pin) : SV_Target
        {
            return gColor;
        }
    )";
    
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    ComPtr<ID3DBlob> errors;
    
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    
    if (FAILED(D3DCompile(shaderCode, strlen(shaderCode), nullptr, nullptr, nullptr,
        "VS", "vs_5_0", compileFlags, 0, &vertexShader, &errors))) {
        if (errors) {
            LOG_ERROR("Text vertex shader compilation failed: " +
                std::string((char*)errors->GetBufferPointer()));
        }
        return false;
    }
    
    if (FAILED(D3DCompile(shaderCode, strlen(shaderCode), nullptr, nullptr, nullptr,
        "PS", "ps_5_0", compileFlags, 0, &pixelShader, &errors))) {
        if (errors) {
            LOG_ERROR("Text pixel shader compilation failed: " +
                std::string((char*)errors->GetBufferPointer()));
        }
        return false;
    }
    
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
    psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    
    // Включаем блендинг для прозрачности
    psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
    psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    
    if (FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)))) {
        LOG_ERROR("Failed to create text pipeline state");
        return false;
    }
    
    return true;
}

void TextRenderer::DrawRect(ID3D12GraphicsCommandList* commandList, float x, float y,
                            float width, float height, XMFLOAT4 color) {
    TextConstants constants;
    
    XMMATRIX scaleMatrix = XMMatrixScaling(width, height, 1.0f);
    XMMATRIX translationMatrix = XMMatrixTranslation(x, y, 0.0f);
    XMMATRIX transform = scaleMatrix * translationMatrix;
    
    XMStoreFloat4x4(&constants.transform, XMMatrixTranspose(transform));
    constants.color = color;
    
    memcpy(m_cbvDataBegin, &constants, sizeof(TextConstants));
    
    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    commandList->DrawInstanced(4, 1, 0, 0);
}

void TextRenderer::DrawText(ID3D12GraphicsCommandList* commandList, const std::string& text,
                            float x, float y, float scale, XMFLOAT4 color) {
    float charWidth = 0.05f * scale;
    float charHeight = 0.08f * scale;
    float spacing = charWidth * 1.2f;
    
    for (size_t i = 0; i < text.length(); i++) {
        float posX = x + i * spacing;
        DrawRect(commandList, posX, y, charWidth, charHeight, color);
    }
}
