import defines;

extern "C" __declspec(dllexport)
int gameplay_sum(int a, int b) {
    return sum(a, b);
}
