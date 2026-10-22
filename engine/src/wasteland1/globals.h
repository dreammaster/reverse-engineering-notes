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
// offsets inside saved2 (IDB struct Saved2)
namespace s2 {
constexpr uint8_t membersInGroup = 5, activePartyGroup = 6, field7 = 7, field8 = 8, totalPartyGroups = 9;
}
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

// ---- character screens and item handling scratch (data segment, IDB names in comments)
constexpr uint16_t selectedEquipItem = 0xCCE7;   // _selectedEquipItem?
constexpr uint16_t selectedItemOffset = 0xCCE0;
constexpr uint16_t selectedWeaponQtyOffset = 0xCCDF;
constexpr uint16_t currentItemId = 0xCD42;

// ---- text engine state (data segment)
constexpr uint16_t numberBcd = 0x466E;           // 4 bytes: packed BCD result of the number conversion (IDB byte_2148E..21491)
constexpr uint16_t unk2147C = 0x465C;            // 5 byte random generator state
constexpr uint16_t textString = 0x4680;          // near pointer to the string being written (IDB _textString)
constexpr uint16_t itemQuantity = 0x4687;        // quantity shown by control code 0Fh / used by the 0Ah filter
constexpr uint16_t numberLeading = 0x467C;       // 0xFF until the first non-zero digit was written (IDB byte_2149C)
constexpr uint16_t numberDigitsLeft = 0x467E;    // digits still to write (IDB byte_2149E)
constexpr uint16_t writeNumberPrefix = 0x467D;   // character written instead of leading zeros
constexpr uint16_t tempTextX = 0x467F;           // field width of the number writers
constexpr uint16_t textPointer1 = 0x4692;        // pointer to the encoded string table in use
constexpr uint16_t textPtrTemp1 = 0x46A1;
constexpr uint16_t decodeTextCtr = 0x46BB;
constexpr uint16_t decodeTextCapitals = 0x46BC;
constexpr uint16_t decodeTextMask = 0x46BD;
constexpr uint16_t encodedTextNum = 0xB267;
constexpr uint16_t buttonMask1 = 0x7DF3;         // which hot keys are accepted now (bit mask, IDB word_24C13)
constexpr uint16_t buttonMask2 = 0x7DF5;         // second mask word (IDB word_24C15)
constexpr uint16_t buttonMask3 = 0x7DF7;
constexpr uint16_t prevMask1 = 0x8DD2;           // masks as last displayed (IDB word_25BF2/25BF4)
constexpr uint16_t prevMask2 = 0x8DD4;
constexpr uint16_t hotLabelCurrent = 0x8DD6;     // IDB byte_25BF6
constexpr uint16_t mouseCursorOn = 0x8DCB;       // IDB word_25BEB
constexpr uint16_t graphicsBusy = 0x8DD9;        // IDB byte_25BF9
constexpr uint16_t mouseEnabled = 0x8DDB;        // IDB byte_25BFB
constexpr uint16_t hintsDrawn = 0x9169;          // IDB byte_25F89
constexpr uint16_t macroSlot = 0xCA63;           // IDB byte_29883: macro being recorded
constexpr uint16_t macroPlaySlot = 0xCA64;       // IDB byte_29884
constexpr uint16_t macroPos = 0xCA62;            // IDB byte_29882
constexpr uint16_t hintPass = 0xCA6B;            // IDB byte_2988B
constexpr uint16_t hintMask = 0xCA69;            // IDB word_29889
constexpr uint16_t keyCaseDelta = 0xC058;        // IDB byte_28E78: 20h subtracts the case bit
constexpr uint16_t lastKey = 0x4667;             // IDB unk_21487: the key checkKeypress returned
constexpr uint16_t textSuppressed = 0x46F0;      // IDB byte_21510
constexpr uint16_t captureNextChar = 0x4703;     // IDB byte_21523
constexpr uint16_t charCapture = 0x8DDC;         // 25 bytes (IDB byte_25BFC)
constexpr uint16_t filterSexOn = 0x470C;         // IDB byte_2152C
constexpr uint16_t filterEOn = 0x471B;           // IDB byte_2153B
constexpr uint16_t filterQtyOn = 0x4702;         // IDB byte_21522
constexpr uint16_t filterSexState = 0xCDCA;      // IDB byte_29BEA
constexpr uint16_t filterEState = 0xCDC9;        // IDB byte_29BE9
constexpr uint16_t filterQtyState = 0xCDCB;      // IDB byte_29BEB
constexpr uint16_t itemSexFlag = 0x471A;         // IDB byte_2153A
constexpr uint16_t wrapColumn = 0xCDCC;          // IDB byte_29BEC
constexpr uint16_t historyLine = 0x8DF6;         // IDB byte_25C16
constexpr uint16_t historyShown = 0x8DF7;        // IDB byte_25C17
constexpr uint16_t historyRecording = 0x8DF8;    // IDB byte_25C18
constexpr uint16_t lineBuffer = 0x71CD;          // 51 bytes (IDB _saved.field_9C): the line being assembled
constexpr uint16_t moreCounter = 0x46FE;         // IDB byte_2151E
constexpr uint16_t pagingEnabled = 0x46FF;       // IDB byte_2151F
constexpr uint16_t textSpeedFlag = 0x46F7;       // IDB byte_21517: 0 = instant text
constexpr uint16_t rosterShown = 0x4707;         // IDB byte_21527
constexpr uint16_t rosterAnim = 0x4708;          // IDB byte_21528
constexpr uint16_t clearBottomFlag = 0x4719;     // IDB byte_21539
constexpr uint16_t rosterAnimOn = 0x4727;        // IDB byte_21547
constexpr uint16_t rosterLocOn = 0x46FB;         // IDB byte_2151B
constexpr uint16_t scrollSaveTick = 0xCD99;      // IDB byte_29BB9
constexpr uint16_t scrollDelay = 0xCD9A;         // IDB byte_29BBA
constexpr uint16_t scrollSteps = 0xCD98;         // IDB byte_29BB8
constexpr uint16_t scrollSavedSpeed = 0xCDE3;    // IDB byte_29C03
constexpr uint16_t scrollSpeedTables = 0xCDA4;   // 3 tables of 9 bytes (steps, tick delay, row delay)
constexpr uint16_t bcdPow = 0xC020;              // 8 x 3 byte powers of ten at C020/C028/C030, BCD increments at C038..C050
constexpr uint16_t textSpecialFn1 = 0xCD50;      // 18 near pointers: control characters 0..17 of writeCharacter2
constexpr uint16_t textSpecialFn2 = 0xCD74;      // same for writeCharacter (line buffered)
constexpr uint16_t hotspotLabels = 0xCBBD;       // 17 entries {x, y, length, ?, near ptr to a glyph string}
constexpr uint16_t scrollRowTableUnused = 0;

// ---- graphics work buffer (offsets in segment 2AE2h)
constexpr uint32_t titlePic = 0x0920;
constexpr uint32_t fontColor = 0xB4E0;           // COLORF.FNT: 172 glyphs x 32 bytes (four 8-byte planes)
constexpr uint32_t monoFont = 0xCA60;            // 128 glyphs x 8 bytes, 1 bit per pixel

} // namespace g
} // namespace wl
