// The game font -- 200.CHARSET / 400.CHARSET.
//
// Geometry confirmed by rendering: 16 px wide, 8 px tall, 16 bytes/glyph
// (2 bytes per row, MSB left), 512 glyphs per file.  Glyphs 0..255 are the
// text font (ASCII); 256..511 are a second bank (graphics / alt weight).
// On a 200-line screen the 1:2 pixel aspect makes a 16x8 cell read square.
#pragma once
#include "wiz/surface.h"

namespace wiz {

class Font {
public:
    static constexpr int kW = 16, kH = 8, kGlyphBytes = 16;

    bool load(std::vector<u8> charset);
    int glyphCount() const { return int(data_.size()) / kGlyphBytes; }

    void drawGlyph(Surface &s, int glyph, int x, int y, u8 fg, int bg = -1) const;
    void drawText(Surface &s, const std::string &t, int x, int y, u8 fg, int bg = -1) const;

private:
    std::vector<u8> data_;
};

} // namespace wiz
