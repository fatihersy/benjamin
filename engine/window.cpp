#include "window.hpp"
#include "renderer.hpp"
#include <stdexcept>

Window::Window(UINT width, UINT height, const wchar_t* title) {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = window_proc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"d3d12_hello_triangle";
    RegisterClassW(&wc);

    hwnd = CreateWindowExW(
        0,
        L"d3d12_hello_triangle",
        title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr
    );

    if (!hwnd) {
        throw std::runtime_error("Failed to create window.");
    }

    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    ShowWindow(hwnd, SW_SHOW);

    renderer = std::make_unique<Renderer>(width, height, hwnd);
}

Window::~Window() {
    DestroyWindow(hwnd);
}

void Window::run() {
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            renderer->render();
        }
    }
}

LRESULT CALLBACK Window::window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    Window* window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
            if (wparam == VK_ESCAPE) {
                PostQuitMessage(0);
                return 0;
            }
            break;
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}
