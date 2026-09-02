#include "wiz/surface.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace wiz {

const Color kDefaultPalette[16] = {
    {0, 0, 0},       {255, 255, 255}, {0, 170, 0},     {0, 170, 170},
    {170, 0, 0},     {170, 0, 170},   {170, 85, 0},    {170, 170, 170},
    {85, 85, 85},    {85, 85, 255},   {85, 255, 85},   {85, 255, 255},
    {255, 85, 85},   {255, 85, 255},  {255, 255, 85},  {255, 255, 255},
};

void Surface::fillRect(int x, int y, int w, int h, u8 c) {
    for (int yy = y; yy < y + h; ++yy)
        for (int xx = x; xx < x + w; ++xx) set(xx, yy, c);
}

void Surface::frameRect(int x, int y, int w, int h, u8 c) {
    hLine(x, y, w, c);
    hLine(x, y + h - 1, w, c);
    vLine(x, y, h, c);
    vLine(x + w - 1, y, h, c);
}

void Surface::hLine(int x, int y, int w, u8 c) {
    for (int i = 0; i < w; ++i) set(x + i, y, c);
}

void Surface::vLine(int x, int y, int h, u8 c) {
    for (int i = 0; i < h; ++i) set(x, y + i, c);
}

void Surface::line(int x0, int y0, int x1, int y1, u8 c) {
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        set(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void Surface::blit1bpp(const u8 *src, int sw, int sh, int dx, int dy, u8 fg, int bg) {
    int stride = (sw + 7) / 8;
    for (int y = 0; y < sh; ++y)
        for (int x = 0; x < sw; ++x) {
            bool on = (src[y * stride + x / 8] >> (7 - (x & 7))) & 1;
            if (on) set(dx + x, dy + y, fg);
            else if (bg >= 0) set(dx + x, dy + y, u8(bg));
        }
}

bool Surface::savePPM(const std::string &path, const Color *palette, int paletteLen) const {
    FILE *f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    std::fprintf(f, "P6\n%d %d\n255\n", w_, h_);
    for (u8 v : px_) {
        Color c = v < paletteLen ? palette[v] : Color{255, 0, 255};
        u8 rgb[3] = {c.r, c.g, c.b};
        std::fwrite(rgb, 1, 3, f);
    }
    std::fclose(f);
    return true;
}

} // namespace wiz
