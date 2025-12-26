#pragma once

#include <d3d12.h>
#include <wrl.h>

class Buffer {
public:
    Buffer(ID3D12Device* device, UINT size, D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_STATES initial_state);
    ~Buffer();

    ID3D12Resource* get_resource() const { return resource.Get(); }
    void* map();
    void unmap();

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    void* mapped_data;
};
