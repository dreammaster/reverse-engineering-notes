#pragma once
// Offsets of the original program's globals: DS-relative for the data segment, segment-relative for the graphics
// work buffer (segment 2AE2h). Names follow the original IDB where it was meaningful and the notes in docs/wl-engine.md.
#include <cstdint>

namespace wl {
namespace g {

// ---- text / rectangle state (data segment)
constexpr uint16_t numberValue = 0x466B;         // 3 byte packed BCD scratch value (writeNumber)
constexpr uint16_t rectTextX = 0x4672;           // text cursor column (character cells)
constexpr uint16_t rectTextY = 0x4673;           // text cursor row (character cells)
constexpr uint16_t rectLeftCol = 0x4674;         // clip/clear rectangle: left column (IDB name _rectTop)
constexpr uint16_t rectRightCol = 0x4675;        // right column, inclusive (IDB _rectRight)
constexpr uint16_t rectTopLine = 0x4676;         // top pixel line (IDB _rectLeft)
constexpr uint16_t rectBottomLine = 0x4677;      // bottom pixel line, exclusive (IDB _rectBottom)
constexpr uint16_t inverseText = 0x4678;         // non-zero: characters are drawn inverted
constexpr uint16_t stringXPos = 0x8DD0;          // colour glyph cursor column
constexpr uint16_t stringYPos = 0x8DD1;          // colour glyph cursor row
constexpr uint16_t gfxLineOffsets = 0x8DF9;      // 200 words: video offset of every pixel line (40 * y)
constexpr uint16_t writeCharacterFn = 0xB265;    // near pointer used by writeString (writeCharacter / writeCharacter2)

// ---- graphics work buffer (offsets in segment 2AE2h)
constexpr uint32_t titlePic = 0x0920;
constexpr uint32_t fontColor = 0xB4E0;           // COLORF.FNT: 172 glyphs x 32 bytes (four 8-byte planes)
constexpr uint32_t monoFont = 0xCA60;            // 128 glyphs x 8 bytes, 1 bit per pixel

} // namespace g
} // namespace wl
