#include "wl/ui.h"
#include "wl/globals.h"

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
