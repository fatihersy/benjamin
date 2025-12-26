#include "texture.hpp"
#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "d3dx12.h"

Texture::Texture(ID3D12Device* device, ID3D12GraphicsCommandList* command_list, const std::string& file_path) {
    int tex_width, tex_height, channels;
    stbi_uc* pixels = stbi_load(file_path.c_str(), &tex_width, &tex_height, &channels, 4);
    if (!pixels) {
        throw std::runtime_error("Failed to load texture file: " + file_path);
    }

    D3D12_HEAP_PROPERTIES heap_props = {};
    heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC texture_desc = {};
    texture_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texture_desc.Width = tex_width;
    texture_desc.Height = tex_height;
    texture_desc.DepthOrArraySize = 1;
    texture_desc.MipLevels = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    if (FAILED(device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &texture_desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resource)))) {
        stbi_image_free(pixels);
        throw std::runtime_error("Failed to create texture resource.");
    }

    UINT64 row_pitch = (tex_width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
    UINT64 upload_buffer_size = row_pitch * tex_height;

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

    UINT8* mapped_data;
    CD3DX12_RANGE read_range(0, 0);
    if (FAILED(upload_heap->Map(0, &read_range, reinterpret_cast<void**>(&mapped_data)))) {
        stbi_image_free(pixels);
        throw std::runtime_error("Failed to map upload heap.");
    }

    for (int y = 0; y < tex_height; y++) {
        memcpy(mapped_data + y * row_pitch, pixels + y * tex_width * 4, tex_width * 4);
    }
    upload_heap->Unmap(0, nullptr);

    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource = resource.Get();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = upload_heap.Get();
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint.Offset = 0;
    src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    src.PlacedFootprint.Footprint.Width = tex_width;
    src.PlacedFootprint.Footprint.Height = tex_height;
    src.PlacedFootprint.Footprint.Depth = 1;
    src.PlacedFootprint.Footprint.RowPitch = static_cast<UINT>(row_pitch);

    command_list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

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
