#include "wasteland1/assets.h"
#include "wasteland1/globals.h"

namespace wl {

Assets::Assets(const std::string &dir, const std::string &exePath) {
	colorFont = loadFont(shared::readFile(dir + "/COLORF.FNT"));
	sprites = loadSprites(shared::readFile(dir + "/IC0_9.WLF"));
	masks = loadMasks(shared::readFile(dir + "/MASKS.WLF"));
	for (const char *f : {"ALLHTDS1", "ALLHTDS2"})
		for (auto &set : loadTilesets(shared::readFile(dir + "/" + f))) tilesets.push_back(std::move(set));
	// TITLE.PIC: 288x128, rows XORed with the row above
	{
		const Bytes raw = shared::readFile(dir + "/TITLE.PIC");
		const int w = 288, h = 128;
		titlePic.assign(size_t(w) * h, 0);
		std::vector<int> last(size_t(w / 2), 0);
		for (int y = 0; y < h; ++y)
			for (int bx = 0; bx < w / 2; ++bx) {
				int b = raw.at(size_t(y) * size_t(w / 2) + size_t(bx));
				if (y > 0) b ^= last[size_t(bx)];
				last[size_t(bx)] = b;
				titlePic[size_t(y) * size_t(w) + size_t(bx) * 2] = uint8_t(b >> 4);
				titlePic[size_t(y) * size_t(w) + size_t(bx) * 2 + 1] = uint8_t(b & 15);
			}
	}
	exe = loadExePack(shared::readFile(exePath), 0x1000);
	game[0] = loadGameFile(dir + "/GAME1");
	game[1] = loadGameFile(dir + "/GAME2");
}

const uint8_t *Assets::monoGlyph(int code) const { return seg003(g::monoFont + uint32_t(code & 0x7F) * 8); }

} // namespace wl
