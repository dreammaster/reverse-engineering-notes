#include "wasteland1/ui.h"
#include "wasteland1/globals.h"

namespace wl {

namespace {

uint16_t lineOffset(const Machine &m, int line) { return m.d16(uint16_t(g::gfxLineOffsets + 2 * line)); }

void writeVideo(Machine &m, uint16_t off, uint8_t v) { m.ega.write(0xA0000u + off, v); }

void setMapMask(Machine &m, uint8_t mask) { m.ega.seqIdx = 2; m.ega.mapMask = mask; }

} // namespace

void writeCharacterAt(Machine &m, uint8_t ch) {
	const int x = m.d8(g::rectTextX), y = m.d8(g::rectTextY);
	if (x > m.d8(g::rectRightCol)) return;
	uint16_t di = uint16_t(lineOffset(m, y * 8) + x);
	const uint32_t src = kGfxLinear + g::monoFont + uint32_t(ch & 0x7F) * 8;
	setMapMask(m, 0x0F);
	const bool inverse = m.d8(g::inverseText) != 0;
	for (int row = 0; row < 8; ++row) {
		uint8_t b = m.mem[src + uint32_t(row)];
		if (inverse) b = uint8_t(~b);
		writeVideo(m, di, b);
		di = uint16_t(di + 40);
	}
}

void writeColorCharacter(Machine &m, uint8_t ch) {
	const int y = m.d8(g::stringYPos), x = m.d8(g::stringXPos);
	if (ch >= 24 && ch <= 51 && m.d8(0x722F) == 0) ch = uint8_t(ch + 28);   // _saved+0FEh: letters are shifted to the other colour set
	m.d8(g::stringXPos) = uint8_t(x + 1);
	const uint16_t base = uint16_t(lineOffset(m, y * 8) + x);
	const uint32_t src = kGfxLinear + g::fontColor + uint32_t(ch) * 32;
	for (int plane = 0; plane < 4; ++plane) {
		setMapMask(m, uint8_t(1 << plane));
		uint16_t di = base;
		for (int row = 0; row < 8; ++row) {
			writeVideo(m, di, m.mem[src + uint32_t(plane) * 8 + uint32_t(row)]);
			di = uint16_t(di + 40);
		}
	}
}

void clearRect(Machine &m) {
	setMapMask(m, 0x0F);
	const int left = m.d8(g::rectLeftCol), right = m.d8(g::rectRightCol), top = m.d8(g::rectTopLine), bottom = m.d8(g::rectBottomLine);
	int line = top;
	for (;;) {
		const uint16_t di = lineOffset(m, line);
		for (int col = right; col >= left && col >= 0; --col) writeVideo(m, uint16_t(di + col), 0);
		++line;
		if (line == 200 || line == bottom) break;
	}
	m.d8(g::rectTextX) = uint8_t(left);
	m.d8(g::rectTextY) = uint8_t(top / 8);
	m.d8(g::inverseText) = 0;
}

} // namespace wl

// ---------------------------------------------------------------------------------------------------------------------
// engine painter
namespace wl {

void Ui::colorChar(uint8_t glyph) {
	if (glyph >= 24 && glyph <= 51 && buttonLettersAlt) glyph = uint8_t(glyph + 28);
	if (glyph < _a.colorFont.size()) _v.colorGlyph(cx, cy, _a.colorFont[glyph]);
	++cx;
}

void Ui::monoChar(int x, int y, uint8_t ascii, bool inverse) {
	_v.monoGlyph(x, y, _a.monoGlyph(ascii >= 0x20 ? ascii - 0x20 : 0), inverse);
}

void Ui::clearRectCells(int leftCol, int rightCol, int topLine, int bottomLine) {
	for (int line = topLine; line < bottomLine && line < Video::H; ++line)
		for (int col = leftCol; col <= rightCol; ++col)
			for (int x = 0; x < 8; ++x) _v.put(col * 8 + x, line, 0);
}

void Ui::box(int x0, int y0, int x1, int y1) {
	cx = x0; cy = y0;
	colorChar(0x0E);
	while (cx != x1) colorChar(0x12);
	colorChar(0x0F);
	++cy;
	do {
		cx = x0; colorChar(0x0D);
		cx = x1; colorChar(0x13);
		++cy;
	} while (cy != y1);
	cx = x0; colorChar(0x10);
	while (cx != x1) colorChar(0x12);
	colorChar(0x11);
}

void Ui::mainFrame() {
	box(0, 0, 37, 17);
	cy = 17; cx = 0; colorChar(0x5E);
	cx = 37; colorChar(0x5F); colorChar(0x12); colorChar(0x0F);
	for (cy = 18; cy != 24; ++cy) {
		cx = 0; colorChar(0x0D);
		cx = 39; colorChar(0x13);
	}
}

void Ui::titleLetters() {
	static const char *letters = "bcdefghijklmdenopq";
	cx = 38; cy = 0;
	for (const char *p = letters; *p; p += 2) {
		colorChar(uint8_t(p[0]));
		colorChar(uint8_t(p[1]));
		cx = 38; ++cy;
	}
}

void Ui::clockAndGauge(int hours, int minutes, int radLevel, bool hasDetector, bool drawClock) {
	if (drawClock) {
		cx = 28; cy = 0;
		colorChar(0x0A);
		colorChar(uint8_t(hours / 10)); colorChar(uint8_t(hours % 10));
		colorChar(0x0C);
		colorChar(uint8_t(minutes / 10)); colorChar(uint8_t(minutes % 10));
		colorChar(0x0B);
	}
	int g = hasDetector ? 0x7E : 0x7A;
	if (!hasDetector) radLevel = 0xFF;
	cx = 38; cy = 16;
	colorChar(uint8_t(g)); colorChar(uint8_t(g + 1)); g += 2;
	cx = 38; --cy;
	colorChar(uint8_t(g)); colorChar(uint8_t(g + 1)); g += 2;
	cx = 38; --cy;
	int odd = 0;
	const int level = radLevel >> 3;
	bool fill = true;
	if (level < 9) {
		const int n = 9 - level;
		odd = n & 1;
		int pieces = n >> 1;
		while (pieces > 0) {
			if (cy > 12) { colorChar(0x8A); colorChar(0x8B); } else { colorChar(0x86); colorChar(0x87); }
			cx = 38; --cy;
			--pieces;
		}
		if (cy == 0x0A) fill = false;
		else if (odd) {
			if (cy > 12) { colorChar(0x84); colorChar(0x85); } else { colorChar(0x88); colorChar(0x89); }
			odd = 0;
			cx = 38; --cy;
		}
	}
	if (fill) {
		while (cy != 0x0A) {
			colorChar(0x82); colorChar(0x83);
			cx = 38; --cy;
		}
	}
	int top = odd ? 0x76 : 0x72;
	colorChar(uint8_t(top)); colorChar(uint8_t(top + 1));
	cx = 38; --cy;
	colorChar(uint8_t(top + 2)); colorChar(uint8_t(top + 3));
}

void Ui::buttonsRow(const std::string &text, int selected) {
	const int xStart = 19 - int((text.size() + 1) / 2);
	cx = 0; cy = 24;
	if (xStart == 0) colorChar(0x5C);
	else {
		colorChar(0x10);
		while (cx != xStart) colorChar(0x12);
		colorChar(0x17);
	}
	auto letter = [](char c) -> uint8_t { return c == ' ' ? 49 : uint8_t((uint8_t(c) & 0xDF) + 0xD7); };
	for (char c : text) colorChar(letter(c));
	colorChar(0x28);
	while (cx != 39) colorChar(0x12);
	colorChar(0x11);
	if (selected >= 0) {   // highlight: the selected word is redrawn with the plain colour set
		int word = 0;
		size_t start = 0;
		for (size_t i = 0; i <= text.size(); ++i)
			if (i == text.size() || text[i] == ' ') {
				if (word == selected) {
					cx = xStart + 1 + int(start); cy = 24;
					const bool saved = buttonLettersAlt;
					buttonLettersAlt = false;
					for (size_t k = start; k < i; ++k) colorChar(letter(text[k]));
					colorChar(i == text.size() ? 0x28 : 0x31);
					buttonLettersAlt = saved;
					break;
				}
				++word;
				start = i + 1;
			}
	}
}

} // namespace wl
