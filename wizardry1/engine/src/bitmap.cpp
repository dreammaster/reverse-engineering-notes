#include "wiz/bitmap.h"

namespace wiz {

const Color kCgaPalette[4] = {
    {0, 0, 0}, {0, 170, 0}, {85, 255, 85}, {255, 255, 255},
};

Surface loadCga2bpp(Bytes data, int widthPx, int heightPx) {
    Surface s(widthPx, heightPx);
    int bytesPerRow = widthPx / 4;
    for (int y = 0; y < heightPx; ++y)
        for (int bx = 0; bx < bytesPerRow; ++bx) {
            size_t off = size_t(y) * bytesPerRow + bx;
            u8 b = off < data.size() ? data[off] : 0;
            for (int p = 0; p < 4; ++p)
                s.set(bx * 4 + p, y, (b >> (6 - 2 * p)) & 3);
        }
    return s;
}

} // namespace wiz
