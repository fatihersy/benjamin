#include <windows.h>
#include <print>

using sum_fn = int(*)(int, int);

int main() {
    auto dll = LoadLibraryA("gameplay.dll");
    auto sum = (sum_fn)GetProcAddress(dll, "gameplay_sum");

    std::print("Hello = {}\n", sum(2, 3));
}
