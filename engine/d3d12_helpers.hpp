#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl.h>

#include <stdexcept>

// Helper function to enable the D3D12 debug layer.
inline void enable_debug_layer() {
#if defined(_DEBUG)
    Microsoft::WRL::ComPtr<ID3D12Debug> debug_controller;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)))) {
        debug_controller->EnableDebugLayer();
    }
#endif
}

// Helper function for acquiring the DXGI factory.
inline Microsoft::WRL::ComPtr<IDXGIFactory4> create_dxgi_factory() {
    UINT dxgi_factory_flags = 0;
#if defined(_DEBUG)
    dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    Microsoft::WRL::ComPtr<IDXGIFactory4> dxgi_factory;
    if (FAILED(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&dxgi_factory)))) {
        throw std::runtime_error("Failed to create DXGI Factory.");
    }
    return dxgi_factory;
}

// Helper function for creating the D3D12 device.
inline Microsoft::WRL::ComPtr<ID3D12Device> create_d3d_device(IDXGIFactory4* factory) {
    Microsoft::WRL::ComPtr<IDXGIAdapter1> hardware_adapter;
    factory->EnumAdapters1(0, &hardware_adapter);

    Microsoft::WRL::ComPtr<ID3D12Device> d3d_device;
    if (FAILED(D3D12CreateDevice(hardware_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&d3d_device)))) {
        throw std::runtime_error("Failed to create D3D12 Device.");
    }
    return d3d_device;
}
