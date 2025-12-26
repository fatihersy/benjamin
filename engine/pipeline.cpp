#include "pipeline.hpp"
#include <stdexcept>
#include <d3dcompiler.h>
#include "d3dx12.h"

Pipeline::Pipeline(
    ID3D12Device* device,
    const std::wstring& shader_path,
    const std::vector<D3D12_INPUT_ELEMENT_DESC>& input_layout,
    const D3D12_ROOT_SIGNATURE_DESC& root_signature_desc
) {
    Microsoft::WRL::ComPtr<ID3DBlob> signature;
    Microsoft::WRL::ComPtr<ID3DBlob> error;
    if (FAILED(D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) {
        throw std::runtime_error("Failed to serialize root signature.");
    }
    if (FAILED(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&root_signature)))) {
        throw std::runtime_error("Failed to create root signature.");
    }

    Microsoft::WRL::ComPtr<ID3DBlob> vertex_shader;
    Microsoft::WRL::ComPtr<ID3DBlob> pixel_shader;
    UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    if (FAILED(D3DCompileFromFile(shader_path.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compile_flags, 0, &vertex_shader, &error))) {
        throw std::runtime_error("Failed to compile vertex shader.");
    }
    if (FAILED(D3DCompileFromFile(shader_path.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compile_flags, 0, &pixel_shader, &error))) {
        throw std::runtime_error("Failed to compile pixel shader.");
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
    pso_desc.InputLayout = { input_layout.data(), (UINT)input_layout.size() };
    pso_desc.pRootSignature = root_signature.Get();
    pso_desc.VS = { vertex_shader->GetBufferPointer(), vertex_shader->GetBufferSize() };
    pso_desc.PS = { pixel_shader->GetBufferPointer(), pixel_shader->GetBufferSize() };
    CD3DX12_RASTERIZER_DESC rasterizer_desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rasterizer_desc.CullMode = D3D12_CULL_MODE_NONE;
    pso_desc.RasterizerState = rasterizer_desc;
    pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pso_desc.DepthStencilState.DepthEnable = FALSE;
    pso_desc.DepthStencilState.StencilEnable = FALSE;
    pso_desc.SampleMask = UINT_MAX;
    pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso_desc.NumRenderTargets = 1;
    pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso_desc.SampleDesc.Count = 1;

    if (FAILED(device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pipeline_state)))) {
        throw std::runtime_error("Failed to create pipeline state.");
    }
}

Pipeline::~Pipeline() {}
