#include "texture.hpp"
#include <stdexcept>
#include "stb_image.h"
#include "d3dx12.h"

Texture::Texture(ID3D12Device* device, ID3D12GraphicsCommandList* command_list, const std::string& file_path) {
    int width, height, channels;
    stbi_uc* pixels = stbi_load(file_path.c_str(), &width, &height, &channels, 4);
    if (!pixels) {
        throw std::runtime_error("Failed to load texture file.");
    }

    D3D12_HEAP_PROPERTIES heap_props = {};
    heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC texture_desc = {};
    texture_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texture_desc.Width = width;
    texture_desc.Height = height;
    texture_desc.DepthOrArraySize = 1;
    texture_desc.MipLevels = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    if (FAILED(device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &texture_desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resource)))) {
        stbi_image_free(pixels);
        throw std::runtime_error("Failed to create texture resource.");
    }

    const UINT64 upload_buffer_size = GetRequiredIntermediateSize(resource.Get(), 0, 1);

    D3D12_HEAP_PROPERTIES upload_heap_props = {};
    upload_heap_props.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC upload_buffer_desc = {};
    upload_buffer_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    upload_buffer_desc.Width = upload_buffer_size;
    upload_buffer_desc.Height = 1;
    upload_buffer_desc.DepthOrArraySize = 1;
    upload_buffer_desc.MipLevels = 1;
    upload_buffer_desc.SampleDesc.Count = 1;
    upload_buffer_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    if (FAILED(device->CreateCommittedResource(&upload_heap_props, D3D12_HEAP_FLAG_NONE, &upload_buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload_heap)))) {
        stbi_image_free(pixels);
        throw std::runtime_error("Failed to create texture upload heap.");
    }

    D3D12_SUBRESOURCE_DATA texture_data = {};
    texture_data.pData = pixels;
    texture_data.RowPitch = width * 4;
    texture_data.SlicePitch = texture_data.RowPitch * height;

    UpdateSubresources(command_list, resource.Get(), upload_heap.Get(), 0, 0, 1, &texture_data);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    command_list->ResourceBarrier(1, &barrier);

    stbi_image_free(pixels);
}

Texture::~Texture() {}
