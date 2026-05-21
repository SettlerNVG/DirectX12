#include "Shader.h"

Shader::Shader(const std::string& path)
    : Resource(path)
    , m_pipelineState(nullptr)
    , m_rootSignature(nullptr) {
}

Shader::~Shader() {
    // GPU ресурсы освобождаются через RenderAdapter
}
