#include "window.hpp"
#include <stdexcept>

int main() {
    try {
        Window window(800, 600, L"DirectX 12 Hello Triangle");
        window.run();
    } catch (const std::exception& e) {
        MessageBoxA(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    return 0;
}



