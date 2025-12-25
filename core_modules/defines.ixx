module;

#include <cstdint>

export module defines;

export {
    using i64 = int64_t;
    using u64 = uint64_t;
    using f64 = double;

    using i32 = int32_t;
    using u32 = uint32_t;
    using f32 = float;

    using i8 = int8_t;
    using u8 = uint8_t;
    using i16 = int16_t;
    using u16 = uint16_t;
};

export i32 sum(i32 a, i32 b) {
    return a + b;
}
