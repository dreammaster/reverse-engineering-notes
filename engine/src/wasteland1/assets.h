#pragma once
// Static game resources: graphics files plus the tables that live inside WL.EXE.
#include "wasteland1/exe.h"
#include "wasteland1/gamefile.h"
#include "wasteland1/graphics.h"

namespace wl {

class Assets {
public:
	// gameDir holds GAME1/GAME2/ALLHTDS*/..., exePath is the (EXEPACK compressed) WL.EXE.
	Assets(const std::string &gameDir, const std::string &exePath);

	std::vector<Pic> colorFont;                  // COLORF.FNT, 172 glyphs
	std::vector<Pic> sprites;                    // IC0_9.WLF
	std::vector<Bytes> masks;                    // MASKS.WLF
	std::vector<std::vector<Pic>> tilesets;      // ALLHTDS1 (4 sets) followed by ALLHTDS2 (5 sets)
	Bytes titlePic;                              // TITLE.PIC decoded to 288x128 4bpp indices
	ExeImage exe;                                // unpacked program image, loaded at segment 1000h
	GameFile game[2];

	const uint8_t *monoGlyph(int code) const;    // 8 bytes, code 0..127 (space = 0)
	// Bytes of the data segment (DS = 1CE2h) / graphics work segment (2AE2h) of the unpacked program
	const uint8_t *dseg(uint16_t off) const { return &exe.image[0xCE20 + off]; }
	const uint8_t *seg003(uint32_t off) const { return &exe.image[0x1AE20 + off]; }
};

} // namespace wl
