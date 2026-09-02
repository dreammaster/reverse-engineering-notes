#include "wiz/font.h"

namespace wiz {

bool Font::load(std::vector<u8> charset) {
    if (charset.size() < kGlyphBytes * 128) return false;
    data_ = std::move(charset);
    return true;
}

void Font::drawGlyph(Surface &s, int glyph, int x, int y, u8 fg, int bg) const {
    if (glyph < 0 || glyph >= glyphCount()) return;
    s.blit1bpp(data_.data() + glyph * kGlyphBytes, kW, kH, x, y, fg, bg);
}

void Font::drawText(Surface &s, const std::string &t, int x, int y, u8 fg, int bg) const {
    for (unsigned char c : t) {
        drawGlyph(s, c, x, y, fg, bg);
        x += kW;
    }
}

} // namespace wiz
