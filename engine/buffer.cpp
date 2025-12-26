#include "buffer.hpp"
#include <stdexcept>

Buffer::Buffer(ID3D12Device* device, UINT size, D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_STATES initial_state) :
    mapped_data(nullptr)
{
    D3D12_HEAP_PROPERTIES heap_props = {};
    heap_props.Type = heap_type;

    D3D12_RESOURCE_DESC buffer_desc = {};
    buffer_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer_desc.Width = size;
    buffer_desc.Height = 1;
    buffer_desc.DepthOrArraySize = 1;
    buffer_desc.MipLevels = 1;
    buffer_desc.SampleDesc.Count = 1;
    buffer_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    if (FAILED(device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &buffer_desc, initial_state, nullptr, IID_PPV_ARGS(&resource)))) {
        throw std::runtime_error("Failed to create buffer.");
    }
}

Buffer::~Buffer() {
    if (mapped_data) {
        unmap();
    }
}

void* Buffer::map() {
    if (!mapped_data) {
        D3D12_RANGE read_range = {};
        if (FAILED(resource->Map(0, &read_range, &mapped_data))) {
            throw std::runtime_error("Failed to map buffer.");
        }
    }
    return mapped_data;
}

void Buffer::unmap() {
    if (mapped_data) {
        resource->Unmap(0, nullptr);
        mapped_data = nullptr;
    }
}
