#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <memory>
#include <vector>
#include "pipeline.hpp"
#include "camera.hpp"
#include "buffer.hpp"

class Renderer {
public:
    Renderer(UINT width, UINT height, HWND hwnd);
    ~Renderer();

    void render();

private:
    void init_pipeline();
    void load_assets();
    void populate_command_list();
    void wait_for_previous_frame();

    static const UINT frame_count = 2;

    UINT width;
    UINT height;
    HWND hwnd;

    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> swap_chain;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_heap;
    UINT rtv_descriptor_size;
    Microsoft::WRL::ComPtr<ID3D12Resource> render_targets[frame_count];
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    UINT64 fence_value;
    HANDLE fence_event;
    UINT frame_index;

    D3D12_VIEWPORT viewport;
    D3D12_RECT scissor_rect;

    std::unique_ptr<Pipeline> pipeline;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertex_buffer;
    D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;

    std::unique_ptr<Camera> camera;
    std::unique_ptr<Buffer> camera_buffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertex_upload_heap;
};