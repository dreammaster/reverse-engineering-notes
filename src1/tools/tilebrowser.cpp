/* tilebrowser <gameDir> <fontDir> [scale]
 *
 * Interactive check of the engine's graphics path: opens a window, draws pages of the
 * FILE0n.16 tiles through the video driver (putImage) and lets you browse.
 *   Left/Right: previous/next file    Up/Down or PageUp/PageDown: page    P: next palette record
 *   Esc or close the window to quit. The mouse cursor is the driver's (shape 1). */

#include "data/Palette.h"
#include "data/Tiles.h"
#include "platform/SdlHost.h"
#include "vid/Fonts.h"
#include "vid/Video.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>

using namespace Yendor1;

static void text(Vid::Video &v, int col, int row, const char *s, int fg, int bg) {
	for (; *s; s++, col++) {
		v.ctx.textCol = (Shared::byte)col;
		v.ctx.textRow = (Shared::byte)row;
		v.ctx.textChar = (Shared::byte)*s;
		v.ctx.textFg = (Shared::byte)fg;
		v.ctx.textBg = (Shared::byte)bg;
		v.drawChar();
	}
}

int main(int argc, char **argv) {
	if (argc < 3) {
		fprintf(stderr, "usage: %s <gameDir> <fontDir> [scale]\n", argv[0]);
		return 2;
	}
	const int scale = argc > 3 ? atoi(argv[3]) : 2;
	Data::TileFile tiles[Data::kTileFileCount];
	Data::PaletteFile pals[Data::kTileFileCount];
	for (int i = 0; i < Data::kTileFileCount; i++) {
		if (!tiles[i].load(argv[1], i) || !pals[i].load(argv[1], i)) {
			fprintf(stderr, "cannot load FILE%02d from %s\n", i, argv[1]);
			return 1;
		}
	}
	Vid::Fonts fonts;
	fonts.load(argv[2]);

	Vid::Video v;
	fonts.attach(v);
	v.init(true);
	v.setGraphicsMode();

	Platform::SdlHost host;
	if (!host.open(v, scale, "Yendor1 tile browser")) {
		fprintf(stderr, "cannot open the window: %s\n", SDL_GetError());
		return 1;
	}
	SDL_StartTextInput();

	// a plain arrow cursor in slot 1 so the driver cursor is visible
	Shared::byte *slot = v.cursorStore() + 1 * Vid::kCursorSlotBytes;
	slot[0] = 16; slot[1] = 0; slot[2] = 2; slot[3] = 0; slot[4] = 0xFF;
	for (int r = 0; r < 12; r++) {
		const int w = r < 6 ? r + 1 : 12 - r;
		const Shared::byte bits = (Shared::byte)(0xFF << (8 - (w > 8 ? 8 : w)));
		for (int p = 0; p < 3; p++)            // planes 2,1,0 -> colour 7 (white); plane 3 stays empty
			slot[5 + (1 + p) * 32 + r * 2] = bits;
	}
	v.ctx.mousePresent = 0xFF;
	v.ctx.clipX1 = 0; v.ctx.clipX2 = 639; v.ctx.clipY1 = 0; v.ctx.clipY2 = 349;
	v.ctx.mouseX = 320; v.ctx.mouseY = 175;
	v.ctx.drawPage = 0;

	int file = 0, page = 0, palRec = 0;
	const int perPage = 19 * 9;               // 19 columns x 9 rows of 32x32 tiles (leaves a text line)
	bool dirty = true;
	while (host.pump()) {
		int k;
		while ((k = host.pollKey()) != -1) {
			if (k == 0x1B) return 0;
			if (k == 0) {
				const int sc = host.pollKey();
				if (sc == 0x4D) { file = (file + 1) % Data::kTileFileCount; page = 0; palRec = 0; }
				if (sc == 0x4B) { file = (file + Data::kTileFileCount - 1) % Data::kTileFileCount; page = 0; palRec = 0; }
				if (sc == 0x50 || sc == 0x51) page++;
				if (sc == 0x48 || sc == 0x49) page = page > 0 ? page - 1 : 0;
			} else if (k == 'p' || k == 'P') {
				palRec++;
			}
			dirty = true;
		}
		if (dirty) {
			dirty = false;
			const int maxPage = (int)((tiles[file].count() + perPage - 1) / perPage) - 1;
			if (page > maxPage) page = maxPage;
			if (palRec >= (int)pals[file].count()) palRec = 0;
			v.setPaletteRegisters(pals[file].record((size_t)palRec));
			v.mouseHide();
			v.ctx.drawPage = 0;
			v.ctx.colour = 0;
			v.ctx.x1 = 0; v.ctx.y1 = 0; v.ctx.x2 = 639; v.ctx.y2 = 349;
			v.fillRect();
			for (int i = 0; i < perPage; i++) {
				const size_t idx = (size_t)page * perPage + i;
				const Shared::byte *raw = tiles[file].raw(idx);
				if (!raw) break;
				v.ctx.image.set(const_cast<Shared::byte *>(raw), Data::kTileBytes, 0);
				v.ctx.x1 = (Shared::uint16)((i % 19) * 32 + 16);
				v.ctx.y1 = (Shared::uint16)((i / 19) * 32 + 4);
				v.putImage();
			}
			char line[96];
			snprintf(line, sizeof(line), "FILE%02d.16  page %d/%d  tiles %d..  palette %d/%d   [</>] file  [Up/Down] page  [P] palette",
				file, page + 1, maxPage + 1, page * perPage, palRec + 1, (int)pals[file].count());
			text(v, 0, 24, line, 15, 0);
			v.copyScreen0To1AndShow();          // page 0 -> page 1, then show page 1
			v.mouseShow();
		}
		SDL_Delay(5);
	}
	return 0;
}
