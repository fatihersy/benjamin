#include "renderer.hpp"
#include "d3dx12.h"
#include <stdexcept>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

struct Vertex {
    XMFLOAT3 position;
    XMFLOAT2 uv;
};

Renderer::Renderer(UINT width, UINT height, HWND hwnd)
    : width(width), height(height), hwnd(hwnd), frame_index(0), fence_value(0), rotation_angle(0.0f) {

    viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    scissor_rect = CD3DX12_RECT(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));

    init_pipeline();
    load_assets();

    ID3D12CommandList* command_lists[] = { command_list.Get() };
    command_queue->ExecuteCommandLists(1, command_lists);
    command_queue->Signal(fence.Get(), 1);
    fence_value = 1;
    if (fence->GetCompletedValue() < 1) {
        fence->SetEventOnCompletion(1, fence_event);
        WaitForSingleObject(fence_event, INFINITE);
    }

    camera = std::make_unique<Camera>(XM_PIDIV2, static_cast<float>(width) / height, 0.1f, 100.0f, 5.0f);
    camera_buffer = std::make_unique<Buffer>(device.Get(), 256, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
}

Renderer::~Renderer() {
    wait_for_previous_frame();
    CloseHandle(fence_event);
}

void Renderer::init_pipeline() {
    UINT dxgi_factory_flags = 0;

#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debug_controller;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)))) {
        debug_controller->EnableDebugLayer();
        dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&factory)))) {
        throw std::runtime_error("Failed to create DXGI factory.");
    }

    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)))) {
        throw std::runtime_error("Failed to create D3D12 device.");
    }

    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    if (FAILED(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue)))) {
        throw std::runtime_error("Failed to create command queue.");
    }

    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
    swap_chain_desc.BufferCount = frame_count;
    swap_chain_desc.Width = width;
    swap_chain_desc.Height = height;
    swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_desc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swap_chain1;
    if (FAILED(factory->CreateSwapChainForHwnd(
        command_queue.Get(),
        hwnd,
        &swap_chain_desc,
        nullptr,
        nullptr,
        &swap_chain1
    ))) {
        throw std::runtime_error("Failed to create swap chain.");
    }
    swap_chain1.As(&swap_chain);
    frame_index = swap_chain->GetCurrentBackBufferIndex();

    D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
    rtv_heap_desc.NumDescriptors = frame_count;
    rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&rtv_heap)))) {
        throw std::runtime_error("Failed to create RTV heap.");
    }
    rtv_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc = {};
    dsv_heap_desc.NumDescriptors = 1;
    dsv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(device->CreateDescriptorHeap(&dsv_heap_desc, IID_PPV_ARGS(&dsv_heap)))) {
        throw std::runtime_error("Failed to create DSV heap.");
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(rtv_heap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < frame_count; i++) {
        if (FAILED(swap_chain->GetBuffer(i, IID_PPV_ARGS(&render_targets[i])))) {
            throw std::runtime_error("Failed to get swap chain buffer.");
        }
        device->CreateRenderTargetView(render_targets[i].Get(), nullptr, rtv_handle);
        rtv_handle.Offset(1, rtv_descriptor_size);
    }

    create_depth_buffer();

    if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator)))) {
        throw std::runtime_error("Failed to create command allocator.");
    }
}

void Renderer::load_assets() {
    D3D12_DESCRIPTOR_RANGE srv_range = {};
    srv_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srv_range.NumDescriptors = 1;
    srv_range.BaseShaderRegister = 0;
    srv_range.RegisterSpace = 0;
    srv_range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER root_params[2] = {};
    root_params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    root_params[0].Descriptor.ShaderRegister = 0;
    root_params[0].Descriptor.RegisterSpace = 0;
    root_params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    root_params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_params[1].DescriptorTable.NumDescriptorRanges = 1;
    root_params[1].DescriptorTable.pDescriptorRanges = &srv_range;
    root_params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC sampler_desc = {};
    sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler_desc.MipLODBias = 0;
    sampler_desc.MaxAnisotropy = 1;
    sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    sampler_desc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    sampler_desc.MinLOD = 0.0f;
    sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
    sampler_desc.ShaderRegister = 0;
    sampler_desc.RegisterSpace = 0;
    sampler_desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
    root_signature_desc.NumParameters = 2;
    root_signature_desc.pParameters = root_params;
    root_signature_desc.NumStaticSamplers = 1;
    root_signature_desc.pStaticSamplers = &sampler_desc;
    root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    pipeline = std::make_unique<Pipeline>(device.Get(), L"C:\\Users\\supre\\Repository\\Repositories\\benjamin\\engine\\shaders.hlsl", input_layout, root_signature_desc);

    if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator.Get(), pipeline->get_pipeline_state(), IID_PPV_ARGS(&command_list)))) {
        throw std::runtime_error("Failed to create command list.");
    }

    Vertex cube_vertices[] = {
        // Front face (z = 0.5)
        { { -0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f } },
        { {  0.5f, -0.5f,  0.5f }, { 1.0f, 1.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f } },
        { { -0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f } },
        // Back face (z = -0.5)
        { {  0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f } },
        { { -0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f } },
        { { -0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f } },
        { {  0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f } },
        // Left face (x = -0.5)
        { { -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f } },
        { { -0.5f, -0.5f,  0.5f }, { 1.0f, 1.0f } },
        { { -0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f } },
        { { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f } },
        // Right face (x = 0.5)
        { {  0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f } },
        { {  0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f } },
        { {  0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f } },
        // Top face (y = 0.5)
        { { -0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 1.0f, 1.0f } },
        { {  0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f } },
        { { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f } },
        // Bottom face (y = -0.5)
        { { -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f } },
        { {  0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f } },
        { {  0.5f, -0.5f,  0.5f }, { 1.0f, 0.0f } },
        { { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f } }
    };

    UINT16 cube_indices[] = {
        // Front
        0, 2, 1, 0, 3, 2,
        // Back
        4, 6, 5, 4, 7, 6,
        // Left
        8, 10, 9, 8, 11, 10,
        // Right
        12, 14, 13, 12, 15, 14,
        // Top
        16, 18, 17, 16, 19, 18,
        // Bottom
        20, 22, 21, 20, 23, 22
    };

    index_count = _countof(cube_indices);
    const UINT vertex_buffer_size = sizeof(cube_vertices);
    const UINT index_buffer_size = sizeof(cube_indices);

    D3D12_HEAP_PROPERTIES heap_props_default = {};
    heap_props_default.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC buffer_desc = {};
    buffer_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer_desc.Width = vertex_buffer_size;
    buffer_desc.Height = 1;
    buffer_desc.DepthOrArraySize = 1;
    buffer_desc.MipLevels = 1;
    buffer_desc.SampleDesc.Count = 1;
    buffer_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    if (FAILED(device->CreateCommittedResource(&heap_props_default, D3D12_HEAP_FLAG_NONE, &buffer_desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&vertex_buffer)))) {
        throw std::runtime_error("Failed to create vertex buffer.");
    }

    D3D12_HEAP_PROPERTIES heap_props_upload = {};
    heap_props_upload.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC upload_buffer_desc = {};
    upload_buffer_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    upload_buffer_desc.Width = vertex_buffer_size;
    upload_buffer_desc.Height = 1;
    upload_buffer_desc.DepthOrArraySize = 1;
    upload_buffer_desc.MipLevels = 1;
    upload_buffer_desc.SampleDesc.Count = 1;
    upload_buffer_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    if (FAILED(device->CreateCommittedResource(&heap_props_upload, D3D12_HEAP_FLAG_NONE, &upload_buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertex_upload_heap)))) {
        throw std::runtime_error("Failed to create vertex upload heap.");
    }

    UINT8* p_vertex_data_begin;
    CD3DX12_RANGE read_range(0, 0);
    if (FAILED(vertex_upload_heap->Map(0, &read_range, reinterpret_cast<void**>(&p_vertex_data_begin)))) {
        throw std::runtime_error("Failed to map vertex upload heap.");
    }
    memcpy(p_vertex_data_begin, cube_vertices, sizeof(cube_vertices));
    vertex_upload_heap->Unmap(0, nullptr);

    // Create index buffer
    D3D12_RESOURCE_DESC index_buffer_desc = buffer_desc;
    index_buffer_desc.Width = index_buffer_size;

    if (FAILED(device->CreateCommittedResource(&heap_props_default, D3D12_HEAP_FLAG_NONE, &index_buffer_desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&index_buffer)))) {
        throw std::runtime_error("Failed to create index buffer.");
    }

    D3D12_RESOURCE_DESC index_upload_desc = upload_buffer_desc;
    index_upload_desc.Width = index_buffer_size;

    if (FAILED(device->CreateCommittedResource(&heap_props_upload, D3D12_HEAP_FLAG_NONE, &index_upload_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&index_upload_heap)))) {
        throw std::runtime_error("Failed to create index upload heap.");
    }

    UINT8* p_index_data_begin;
    if (FAILED(index_upload_heap->Map(0, &read_range, reinterpret_cast<void**>(&p_index_data_begin)))) {
        throw std::runtime_error("Failed to map index upload heap.");
    }
    memcpy(p_index_data_begin, cube_indices, sizeof(cube_indices));
    index_upload_heap->Unmap(0, nullptr);

    command_list->CopyBufferRegion(vertex_buffer.Get(), 0, vertex_upload_heap.Get(), 0, vertex_buffer_size);
    command_list->CopyBufferRegion(index_buffer.Get(), 0, index_upload_heap.Get(), 0, index_buffer_size);

    D3D12_RESOURCE_BARRIER barriers[2] = {};
    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.pResource = vertex_buffer.Get();
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.pResource = index_buffer.Get();
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;
    barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    command_list->ResourceBarrier(2, barriers);

    // Create SRV heap
    D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc = {};
    srv_heap_desc.NumDescriptors = 1;
    srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(device->CreateDescriptorHeap(&srv_heap_desc, IID_PPV_ARGS(&srv_heap)))) {
        throw std::runtime_error("Failed to create SRV heap.");
    }

    // Load texture
    cube_texture = std::make_unique<Texture>(device.Get(), command_list.Get(), "C:/Users/supre/Repository/Repositories/benjamin/assets/grass.png");

    // Create SRV for texture
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(cube_texture->get_resource(), &srv_desc, srv_heap->GetCPUDescriptorHandleForHeapStart());

    command_list->Close();

    vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
    vertex_buffer_view.StrideInBytes = sizeof(Vertex);
    vertex_buffer_view.SizeInBytes = vertex_buffer_size;

    index_buffer_view.BufferLocation = index_buffer->GetGPUVirtualAddress();
    index_buffer_view.SizeInBytes = index_buffer_size;
    index_buffer_view.Format = DXGI_FORMAT_R16_UINT;

    if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)))) {
        throw std::runtime_error("Failed to create fence.");
    }
    fence_value = 0;
    fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void Renderer::populate_command_list() {
    if (FAILED(command_allocator->Reset())) {
        throw std::runtime_error("Failed to reset command allocator.");
    }
    if (FAILED(command_list->Reset(command_allocator.Get(), pipeline->get_pipeline_state()))) {
        throw std::runtime_error("Failed to reset command list.");
    }

    // Update matrices
    camera->update();
    rotation_angle += 0.01f;

    DirectX::XMMATRIX model = XMMatrixRotationY(rotation_angle);
    DirectX::XMMATRIX view = camera->get_view_matrix();
    DirectX::XMMATRIX proj = camera->get_projection_matrix();

    void* mapped = camera_buffer->map();
    memcpy(mapped, &model, sizeof(XMMATRIX));
    memcpy((char*)mapped + sizeof(XMMATRIX), &view, sizeof(XMMATRIX));
    memcpy((char*)mapped + 2 * sizeof(XMMATRIX), &proj, sizeof(XMMATRIX));
    camera_buffer->unmap();

    command_list->SetGraphicsRootSignature(pipeline->get_root_signature());

    ID3D12DescriptorHeap* heaps[] = { srv_heap.Get() };
    command_list->SetDescriptorHeaps(1, heaps);

    command_list->SetGraphicsRootConstantBufferView(0, camera_buffer->get_resource()->GetGPUVirtualAddress());
    command_list->SetGraphicsRootDescriptorTable(1, srv_heap->GetGPUDescriptorHandleForHeapStart());

    command_list->RSSetViewports(1, &viewport);
    command_list->RSSetScissorRects(1, &scissor_rect);

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(render_targets[frame_index].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    command_list->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(rtv_heap->GetCPUDescriptorHandleForHeapStart(), frame_index, rtv_descriptor_size);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsv_handle(dsv_heap->GetCPUDescriptorHandleForHeapStart());
    command_list->OMSetRenderTargets(1, &rtv_handle, FALSE, &dsv_handle);

    const float clear_color[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    command_list->ClearRenderTargetView(rtv_handle, clear_color, 0, nullptr);
    command_list->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->IASetVertexBuffers(0, 1, &vertex_buffer_view);
    command_list->IASetIndexBuffer(&index_buffer_view);
    command_list->DrawIndexedInstanced(index_count, 1, 0, 0, 0);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(render_targets[frame_index].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    command_list->ResourceBarrier(1, &barrier);

    if (FAILED(command_list->Close())) {
        throw std::runtime_error("Failed to close command list.");
    }
}

void Renderer::render() {
    populate_command_list();
    ID3D12CommandList* command_lists[] = { command_list.Get() };
    command_queue->ExecuteCommandLists(_countof(command_lists), command_lists);

    if (FAILED(swap_chain->Present(1, 0))) {
        throw std::runtime_error("Failed to present swap chain.");
    }

    wait_for_previous_frame();
}

void Renderer::wait_for_previous_frame() {
    const UINT64 current_fence_value = fence_value;
    if (FAILED(command_queue->Signal(fence.Get(), current_fence_value))) {
        throw std::runtime_error("Failed to signal fence.");
    }
    fence_value++;

    if (fence->GetCompletedValue() < current_fence_value) {
        if (FAILED(fence->SetEventOnCompletion(current_fence_value, fence_event))) {
            throw std::runtime_error("Failed to set fence event.");
        }
        WaitForSingleObject(fence_event, INFINITE);
    }

    frame_index = swap_chain->GetCurrentBackBufferIndex();
}

void Renderer::resize(UINT new_width, UINT new_height) {
    if (new_width == 0 || new_height == 0) return;

    wait_for_previous_frame();

    for (UINT i = 0; i < frame_count; i++) {
        render_targets[i].Reset();
    }

    if (FAILED(swap_chain->ResizeBuffers(frame_count, new_width, new_height, DXGI_FORMAT_R8G8B8A8_UNORM, 0))) {
        throw std::runtime_error("Failed to resize swap chain buffers.");
    }

    frame_index = swap_chain->GetCurrentBackBufferIndex();

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(rtv_heap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < frame_count; i++) {
        if (FAILED(swap_chain->GetBuffer(i, IID_PPV_ARGS(&render_targets[i])))) {
            throw std::runtime_error("Failed to get swap chain buffer.");
        }
        device->CreateRenderTargetView(render_targets[i].Get(), nullptr, rtv_handle);
        rtv_handle.Offset(1, rtv_descriptor_size);
    }

    width = new_width;
    height = new_height;
    viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    scissor_rect = CD3DX12_RECT(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));

    depth_stencil_buffer.Reset();
    create_depth_buffer();

    camera->set_aspect_ratio(static_cast<float>(width) / height);
}

void Renderer::create_depth_buffer() {
    D3D12_RESOURCE_DESC depth_desc = {};
    depth_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depth_desc.Width = width;
    depth_desc.Height = height;
    depth_desc.DepthOrArraySize = 1;
    depth_desc.MipLevels = 1;
    depth_desc.Format = DXGI_FORMAT_D32_FLOAT;
    depth_desc.SampleDesc.Count = 1;
    depth_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clear_value = {};
    clear_value.Format = DXGI_FORMAT_D32_FLOAT;
    clear_value.DepthStencil.Depth = 1.0f;
    clear_value.DepthStencil.Stencil = 0;

    D3D12_HEAP_PROPERTIES heap_props = {};
    heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;

    if (FAILED(device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &depth_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value, IID_PPV_ARGS(&depth_stencil_buffer)))) {
        throw std::runtime_error("Failed to create depth stencil buffer.");
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
    dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
    dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    device->CreateDepthStencilView(depth_stencil_buffer.Get(), &dsv_desc, dsv_heap->GetCPUDescriptorHandleForHeapStart());
}