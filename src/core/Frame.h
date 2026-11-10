#pragma once
#include <cstdint>
#include <vector>
namespace at {
// A CPU image: 8-bit RGBA, straight (non-premultiplied) alpha, row-major, no padding.
struct Frame {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> rgba;
    bool empty() const { return width <= 0 || height <= 0 || rgba.empty(); }
    void allocate(int w, int h) {
        width = w; height = h;
        rgba.assign(static_cast<size_t>(w) * h * 4, 0);
    }
};
}
