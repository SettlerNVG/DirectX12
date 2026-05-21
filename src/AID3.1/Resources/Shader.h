#pragma once

#include "Resource.h"
#include <d3d12.h>
#include <string>

// Класс шейдера
class Shader : public Resource {
public:
    Shader(const std::string& path);
    ~Shader();
    
    const std::string& GetVertexShaderCode() const { return m_vertexShaderCode; }
    const std::string& GetPixelShaderCode() const { return m_pixelShaderCode; }
    
    void SetVertexShaderCode(const std::string& code) { m_vertexShaderCode = code; }
    void SetPixelShaderCode(const std::string& code) { m_pixelShaderCode = code; }
    
    // GPU ресурсы
    ID3D12PipelineState* GetPipelineState() const { return m_pipelineState; }
    ID3D12RootSignature* GetRootSignature() const { return m_rootSignature; }
    
    void SetPipelineState(ID3D12PipelineState* pso) { m_pipelineState = pso; }
    void SetRootSignature(ID3D12RootSignature* rootSig) { m_rootSignature = rootSig; }
    
private:
    std::string m_vertexShaderCode;
    std::string m_pixelShaderCode;
    
    // GPU ресурсы
    ID3D12PipelineState* m_pipelineState;
    ID3D12RootSignature* m_rootSignature;
};
