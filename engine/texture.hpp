#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <string>

class Texture {
public:
    Texture(ID3D12Device* device, ID3D12GraphicsCommandList* command_list, const std::string& file_path);
    ~Texture();

    ID3D12Resource* get_resource() const { return resource.Get(); }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    Microsoft::WRL::ComPtr<ID3D12Resource> upload_heap;
};
