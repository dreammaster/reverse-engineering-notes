// A simple 8-bit indexed framebuffer + drawing primitives.  No dependencies;
// a Platform backend turns it into pixels on screen.
#pragma once
#include "wiz/types.h"

namespace wiz {

struct Color { u8 r = 0, g = 0, b = 0; };

class Surface {
public:
    Surface() = default;
    Surface(int w, int h) { resize(w, h); }

    void resize(int w, int h) { w_ = w; h_ = h; px_.assign(size_t(w) * h, 0); }
    int width() const { return w_; }
    int height() const { return h_; }
    const u8 *data() const { return px_.data(); }

    u8 get(int x, int y) const { return in(x, y) ? px_[idx(x, y)] : 0; }
    void set(int x, int y, u8 c) { if (in(x, y)) px_[idx(x, y)] = c; }

    void fill(u8 c) { for (u8 &p : px_) p = c; }
    void fillRect(int x, int y, int w, int h, u8 c);
    void frameRect(int x, int y, int w, int h, u8 c);
    void hLine(int x, int y, int w, u8 c);
    void vLine(int x, int y, int h, u8 c);
    void line(int x0, int y0, int x1, int y1, u8 c);   // Bresenham

    // Blit a 1-bpp source (row-padded to whole bytes, MSB first).  Set bits
    // become colour `fg`; clear bits are left alone unless `bg >= 0`.
    void blit1bpp(const u8 *src, int sw, int sh, int dx, int dy, u8 fg, int bg = -1);

    // Netpbm P6 dump for headless verification.
    bool savePPM(const std::string &path, const Color *palette, int paletteLen) const;

private:
    bool in(int x, int y) const { return x >= 0 && y >= 0 && x < w_ && y < h_; }
    size_t idx(int x, int y) const { return size_t(y) * w_ + x; }

    int w_ = 0, h_ = 0;
    std::vector<u8> px_;
};

// A small default palette (index 0 = black, 1 = white, then a few EGA-ish).
extern const Color kDefaultPalette[16];

} // namespace wiz
