#include "fod/screen.h"
#include "fod/lzss.h"
#include <algorithm>
#include <cstdio>

namespace fod {

Image Screen::toImage() const {
	Image img(W, H);
	const Rgb *pal = masterPalette();
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x) img.set(x, y, pal[_px[size_t(y) * W + x] & 15]);
	return img;
}

Assets::Assets(const std::string &dir)
    : tiles(lzssDecode(readFile(dir + "/TILES"))), borders(readFile(dir + "/BORDERS")) {
	Bytes f = readFile(dir + "/FONT");
	font.assign(f.begin() + 2, f.end());
}

void drawTile(Screen &s, const Assets &a, int x, int y, int tile) {
	if (tile < 0 || size_t(tile + 1) * 128 > a.tiles.size()) return;
	const uint8_t *t = &a.tiles[size_t(tile) * 128];
	for (int row = 0; row < 16; ++row)
		for (int col = 0; col < 8; ++col) {
			s.put(x + col * 2, y + row, t[row * 8 + col] >> 4);
			s.put(x + col * 2 + 1, y + row, t[row * 8 + col] & 15);
		}
}

void drawGlyph(Screen &s, const Assets &a, int x, int y, int ch, bool inverse) {
	if (ch < 0 || size_t(ch + 1) * 32 > a.font.size()) return;
	const uint8_t *g = &a.font[size_t(ch) * 32];
	for (int row = 0; row < 8; ++row)
		for (int col = 0; col < 4; ++col) {
			uint8_t v = g[row * 4 + col];
			if (inverse) v ^= 0xFF;
			s.put(x + col * 2, y + row, v >> 4);
			s.put(x + col * 2 + 1, y + row, v & 15);
		}
}

void drawText(Screen &s, const Assets &a, int col, int row, const std::string &text, bool inverse) {
	for (size_t i = 0; i < text.size(); ++i) drawGlyph(s, a, (col + int(i)) * 8, row * 8, uint8_t(text[i]), inverse);
}

void drawBorders(Screen &s, const Assets &a, int variant) {
	if (a.borders.size() < size_t(variant + 1) * 1000) return;
	for (int r = 0; r < 25; ++r)
		for (int c = 0; c < 40; ++c) {
			const uint8_t ch = a.borders[size_t(variant) * 1000 + r * 40 + c];
			if (ch) drawGlyph(s, a, c * 8, r * 8, ch);
		}
}

// Overlay entity id -> tile number in TILES (KEH table at dseg:9E1A holds paragraph offsets; /8 = tile).
static int overlayTile(int id) {
	static const uint16_t kPara[32] = {0,      0x1E68, 0x1E60, 0x1F38, 0x1E48, 0x1E50, 0x1E58, 0x1EC8, 0x1ED0, 0x1ED8, 0x1EE0,
	                                   0x1EE8, 0x1EF0, 0x1EF8, 0x1F00, 0x1F08, 0x1F10, 0x1F18, 0x1F20, 0x1F28, 0x1F30};
	return id > 0 && id < 21 ? kPara[id] / 8 : -1;
}

static void drawOverlay(Screen &s, const Assets &a, int x, int y, int id) {
	const int tile = overlayTile(id);
	if (tile < 0 || size_t(tile + 1) * 128 > a.tiles.size()) return;
	const uint8_t *t = &a.tiles[size_t(tile) * 128];
	for (int row = 0; row < 16; ++row)
		for (int col = 0; col < 8; ++col) {
			const uint8_t v = t[row * 8 + col];
			if (v >> 4) s.put(x + col * 2, y + row, v >> 4);
			if (v & 15) s.put(x + col * 2 + 1, y + row, v & 15);
		}
}

void drawMapView(Screen &s, const Assets &a, const World &w) {
	if (!w.map) return;
	const int px = w.disk.mapX, py = w.disk.mapY;
	const int ox = px - 9, oy = py - 4;
	const int defaultTile = w.map->flags() & 0x7FF;
	int overlay[9][19] = {};
	auto mark = [&](int x, int y, int id, int radius) {
		if (std::abs(x - px) > radius || std::abs(y - py) > radius) return;
		const int tx = x - ox, ty = y - oy;
		if (tx >= 0 && tx < 19 && ty >= 0 && ty < 9) overlay[ty][tx] = id;
	};
	for (const auto &m : w.map->monsters())
		if (m.raw[0x5D] > 0x3C && m.x() >= 0) mark(m.x(), m.y(), m.raw[0x65], m.raw[0x5F]);
	for (const auto &g : w.map->groups())
		if (g.raw[9] & 0xF) mark(g.x(), g.y(), g.raw[0xC], g.raw[5]);
	for (int ty = 0; ty < 9; ++ty)
		for (int tx = 0; tx < 19; ++tx) {
			const int mx = ox + tx, my = oy + ty;
			int tile = defaultTile;
			int ov = overlay[ty][tx];
			if (mx >= 0 && my >= 0 && mx < w.map->width() && my < w.map->height()) {
				const MapCell &c = w.map->cell(mx, my);
				tile = c.tileIndex();
				if (c.flags & 0x40) ov = 3;
				else if (c.flags & 0x80) ov = 2;
			}
			drawTile(s, a, 8 + tx * 16, 24 + ty * 16, tile);
			if (ov) drawOverlay(s, a, 8 + tx * 16, 24 + ty * 16, ov);
		}
	drawOverlay(s, a, 8 + 9 * 16, 24 + 4 * 16, 1);   // the party
}

void drawPartyNames(Screen &s, const Assets &a, const World &w) {
	for (int i = 0; i < 5; ++i) {
		const int x1 = i == 0 ? 1 : 1 + 7 * i;
		std::string name = i < w.partyCount() ? w.party(i).name() : "";
		if (name.size() > 6) name.resize(6);
		const int pad = (6 - int(name.size())) / 2;
		drawText(s, a, x1 + pad, 1, name, i < w.partyCount() && w.party(i).u8(0x58) != 0);
	}
}

void drawClock(Screen &s, const Assets &a, const World &w) {
	static const char *days[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	drawText(s, a, 1, 22, days[w.disk.weekday % 7]);
	char buf[16];
	std::snprintf(buf, sizeof buf, "%2d:%02d", w.disk.hours, w.disk.minutes);
	drawText(s, a, 0, 23, buf);
}

void renderMainScreen(Screen &s, const Assets &a, const World &w, const std::vector<std::string> &messages) {
	s.clear();
	drawBorders(s, a, 0);
	drawMapView(s, a, w);
	drawPartyNames(s, a, w);
	drawClock(s, a, w);
	// message window: (7,22)-(39,24) = 3 rows of 33 chars
	const size_t first = messages.size() > 3 ? messages.size() - 3 : 0;
	for (size_t i = first; i < messages.size(); ++i) {
		std::string m = messages[i];
		if (m.size() > 33) m.resize(33);
		drawText(s, a, 7, 22 + int(i - first), m);
	}
}

} // namespace fod
