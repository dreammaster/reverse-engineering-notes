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

// ---- party / characters (data segment)
constexpr uint16_t saved2 = 0x464E;              // live copy of the savegame bytes 78h..85h (see docs/wl-engine.md)
constexpr uint16_t itemDetailsPtr = 0x4665;      // pointer to the record found by getItemDetails*
constexpr uint16_t itemDetailsBasePtr = 0x4694;
constexpr uint16_t itemDetailsRecSize = 0x469B;
constexpr uint16_t locationDetailsPtr = 0x46AE;  // pointer to the action data being executed (IDB _locationDetailsPtr?)
constexpr uint16_t locationsPtr = 0x46B0;        // decoded map block of the current location
constexpr uint16_t currentCharacter = 0x46B5;    // pointer to the 0x100 byte record of the "current" member
constexpr uint16_t partyIndexesPtr = 0x46B7;     // pointer to the 14 byte party record of the active group
constexpr uint16_t savedLocationDetailsPtr = 0x46C6;
constexpr uint16_t currentCharacterSex = 0x470B;
constexpr uint16_t saved = 0x7131;               // the 0x800 byte savegame image: 4 party records, ..., 7 character records at +100h*n
constexpr uint16_t itemDetails = 0x7A31;         // 8 byte header, then one 8 byte record per item id
constexpr uint16_t partiesOffsets = 0xB239;      // 4 bytes: offsets of the party records inside `saved`
constexpr uint16_t itemDetails2 = 0xBA20;        // skill id -> {unused, attribute offset} (2 bytes each)
constexpr uint16_t conditionThresholds = 0xCCCD; // 5 signed bytes
constexpr uint16_t ammunitionItems = 0xCD00;     // item ids that use ammunition, FFh terminated

// ---- graphics work buffer (offsets in segment 2AE2h)
constexpr uint32_t titlePic = 0x0920;
constexpr uint32_t fontColor = 0xB4E0;           // COLORF.FNT: 172 glyphs x 32 bytes (four 8-byte planes)
constexpr uint32_t monoFont = 0xCA60;            // 128 glyphs x 8 bytes, 1 bit per pixel

} // namespace g
} // namespace wl
