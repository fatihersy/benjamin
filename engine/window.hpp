#pragma once

#include <windows.h>
#include <memory>
#include "renderer.hpp"

class Window {
public:
    Window(UINT width, UINT height, const wchar_t* title);
    ~Window();

    void run();

private:
    static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    HWND hwnd;
    std::unique_ptr<Renderer> renderer;
};
