#include "ultima6/ui.h"

namespace Ultima6 {
namespace Ultima6 {
namespace Ui {

void drawTile(Framebuffer &screen, const World &world, int tile, int x, int y) {
	if (tile < 0 || tile >= TileSet::COUNT)
		return;
	screen.blit(x, y, world.tiles.pixels[tile], 16, 16, 16, world.tiles.transparent(tile) ? TileSet::TRANSPARENT_PIXEL : -1);
}

void drawText(Framebuffer &screen, const Font &font, int x, int y, const std::string &text, int fg, int bg) {
	if (font.wou) {
		for (size_t k = 0; k < text.size(); ++k) {
			uint8_t c = (uint8_t)text[k];
			int w = font.wouWidth[c], h = font.wouHeight;
			for (int r = 0; r < h; ++r)
				for (int b = 0; b < w; ++b) {
					int px = x + b, py = y + r;
					if (px < 0 || py < 0 || px >= Framebuffer::WIDTH || py >= Framebuffer::HEIGHT)
						continue;
					if (font.wouPixels[c][r * w + b])
						screen.pixels()[py * Framebuffer::WIDTH + px] = (uint8_t)fg;
					else if (bg >= 0)
						screen.pixels()[py * Framebuffer::WIDTH + px] = (uint8_t)bg;
				}
			x += w;
		}
		return;
	}
	for (size_t k = 0; k < text.size(); ++k) {
		const uint8_t *g = font.glyph[(uint8_t)text[k]];
		for (int r = 0; r < 8; ++r) {
			for (int b = 0; b < 8; ++b) {
				int px = x + (int)k * 8 + b, py = y + r;
				if (px < 0 || py < 0 || px >= Framebuffer::WIDTH || py >= Framebuffer::HEIGHT)
					continue;
				if (g[r] & (0x80 >> b))
					screen.pixels()[py * Framebuffer::WIDTH + px] = (uint8_t)fg;
				else if (bg >= 0)
					screen.pixels()[py * Framebuffer::WIDTH + px] = (uint8_t)bg;
			}
		}
	}
}

void restorePaper(Framebuffer &screen, const Picture &paper, int x, int y, int w, int h) {
	screen.blit(x, y, &paper.pixels[y * paper.width + x], w, h, paper.width);
}

} // namespace Ui
} // namespace Ultima6
} // namespace Ultima6
