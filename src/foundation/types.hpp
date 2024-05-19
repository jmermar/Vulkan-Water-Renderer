#pragma once
#include <cstdint>
#include <vector>
struct Size {
    uint32_t w{}, h{};
};

struct Point {
    int32_t x{}, y{};
};

struct Region2D {
    uint32_t x{}, y{};
    uint32_t w{}, h{};
};

struct Rect {
    int32_t x{}, y{};
    uint32_t w{}, h{};
};

template <typename T>
using Ref = T*;

struct ImageData {
    Size size;
    std::vector<uint8_t> data;
};
