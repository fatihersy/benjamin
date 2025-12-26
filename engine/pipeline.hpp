#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <vector>

class Pipeline {
public:
    Pipeline(
        ID3D12Device* device,
        const std::wstring& shader_path,
        const std::vector<D3D12_INPUT_ELEMENT_DESC>& input_layout,
        const D3D12_ROOT_SIGNATURE_DESC& root_signature_desc
    );
    ~Pipeline();

    ID3D12RootSignature* get_root_signature() const { return root_signature.Get(); }
    ID3D12PipelineState* get_pipeline_state() const { return pipeline_state.Get(); }

private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_state;
};
