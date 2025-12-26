#include "renderer.hpp"
#include "d3dx12.h"
#include <stdexcept>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

struct Vertex {
    XMFLOAT3 position;
    XMFLOAT4 color;
};

Renderer::Renderer(UINT width, UINT height, HWND hwnd)
    : width(width), height(height), hwnd(hwnd), frame_index(0), fence_value(0) {
    
    viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    scissor_rect = CD3DX12_RECT(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));

    init_pipeline();
    load_assets();
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

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(rtv_heap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < frame_count; i++) {
        if (FAILED(swap_chain->GetBuffer(i, IID_PPV_ARGS(&render_targets[i])))) {
            throw std::runtime_error("Failed to get swap chain buffer.");
        }
        device->CreateRenderTargetView(render_targets[i].Get(), nullptr, rtv_handle);
        rtv_handle.Offset(1, rtv_descriptor_size);
    }

    if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator)))) {
        throw std::runtime_error("Failed to create command allocator.");
    }
}

void Renderer::load_assets() {
    D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
    root_signature_desc.NumParameters = 0;
    root_signature_desc.pParameters = nullptr;
    root_signature_desc.NumStaticSamplers = 0;
    root_signature_desc.pStaticSamplers = nullptr;
    root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    pipeline = std::make_unique<Pipeline>(device.Get(), L"shaders.hlsl", input_layout, root_signature_desc);

    if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator.Get(), pipeline->get_pipeline_state(), IID_PPV_ARGS(&command_list)))) {
        throw std::runtime_error("Failed to create command list.");
    }
    command_list->Close();

    Vertex triangle_vertices[] = {
        { { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
    };

    const UINT vertex_buffer_size = sizeof(triangle_vertices);

    auto heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(vertex_buffer_size);

    if (FAILED(device->CreateCommittedResource(
        &heap_props,
        D3D12_HEAP_FLAG_NONE,
        &buffer_desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vertex_buffer)))) {
        throw std::runtime_error("Failed to create vertex buffer.");
    }

    UINT8* p_vertex_data_begin;
    CD3DX12_RANGE read_range(0, 0);
    if (FAILED(vertex_buffer->Map(0, &read_range, reinterpret_cast<void**>(&p_vertex_data_begin)))) {
        throw std::runtime_error("Failed to map vertex buffer.");
    }
    memcpy(p_vertex_data_begin, triangle_vertices, sizeof(triangle_vertices));
    vertex_buffer->Unmap(0, nullptr);

    vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
    vertex_buffer_view.StrideInBytes = sizeof(Vertex);
    vertex_buffer_view.SizeInBytes = vertex_buffer_size;

    if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)))) {
        throw std::runtime_error("Failed to create fence.");
    }
    fence_value = 1;
    fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void Renderer::populate_command_list() {
    if (FAILED(command_allocator->Reset())) {
        throw std::runtime_error("Failed to reset command allocator.");
    }
    if (FAILED(command_list->Reset(command_allocator.Get(), pipeline->get_pipeline_state()))) {
        throw std::runtime_error("Failed to reset command list.");
    }

    command_list->SetGraphicsRootSignature(pipeline->get_root_signature());
    command_list->RSSetViewports(1, &viewport);
    command_list->RSSetScissorRects(1, &scissor_rect);

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(render_targets[frame_index].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    command_list->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(rtv_heap->GetCPUDescriptorHandleForHeapStart(), frame_index, rtv_descriptor_size);
    command_list->OMSetRenderTargets(1, &rtv_handle, FALSE, nullptr);

    const float clear_color[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    command_list->ClearRenderTargetView(rtv_handle, clear_color, 0, nullptr);
    command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->IASetVertexBuffers(0, 1, &vertex_buffer_view);
    command_list->DrawInstanced(3, 1, 0, 0);

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