#include "ultima6/ending.h"

#include <algorithm>
#include <cstring>

namespace Ultima6 {
namespace Ultima6 {

#include "ultima6/ending_text.inc"

static const size_t PAGE_COUNT = sizeof(ENDING_PAGES) / sizeof(ENDING_PAGES[0]);

static std::vector<std::string> wrap(const std::string &s, size_t cols) {
	std::vector<std::string> out;
	size_t pos = 0;
	while (pos < s.size()) {
		while (pos < s.size() && s[pos] == ' ')
			++pos;
		size_t end = pos + cols;
		if (end >= s.size()) {
			end = s.size();
		} else {
			size_t sp = s.rfind(' ', end);
			if (sp != std::string::npos && sp > pos)
				end = sp;
		}
		out.push_back(s.substr(pos, end - pos));
		pos = end;
	}
	return out;
}

Ending::Ending(const std::string &dir, int years, int months, int days) : _years(years), _months(months), _days(days) {
	std::memset(_palette, 0, sizeof _palette);
	Bytes d;
	if (!readFile(joinPath(dir, "U6.CH"), d) || !_font.load(d))
		return;
	if (!readFile(joinPath(dir, "END.SHP"), d) || !loadShapeFile(d, _shapes) || _shapes.size() < 13)
		return;
	// ENDPAL.LBM: an IFF ILBM whose CMAP chunk holds 256 colours of 3 bytes, 8 bits each (the game uses the top 6)
	if (!readFile(joinPath(dir, "ENDPAL.LBM"), d))
		return;
	size_t i = 0;
	for (; i + 8 < d.size(); ++i)
		if (!std::memcmp(&d[i], "CMAP", 4))
			break;
	if (i + 8 + 768 > d.size())
		return;
	for (int c = 0; c < 256; ++c)
		for (int k = 0; k < 3; ++k)
			_palette[c][k] = (uint8_t)(d[i + 8 + c * 3 + k] >> 2);
	// the pages wrapped to the parchment: 37 columns, 7 lines a screen
	for (size_t p = 0; p < PAGE_COUNT; ++p) {
		std::string all;
		for (int i = 0; ENDING_PAGES[p][i]; ++i)
			all += (all.empty() || all.back() == ' ' || ENDING_PAGES[p][i][0] == ' ' ? "" : " ") + std::string(ENDING_PAGES[p][i]);
		std::vector<std::string> lines = wrap(all, 37);
		for (size_t l = 0; l < lines.size(); l += 7) {
			Screen sc;
			sc.scene = (int)p;
			sc.lines.assign(lines.begin() + l, lines.begin() + std::min(lines.size(), l + 7));
			_screens.push_back(sc);
		}
	}
	_ok = true;
}

// number_to_words (END.EXE 12D1C)
std::string Ending::numberToWords(int n) {
	static const char *const ONES[20] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten",
										 "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"};
	static const char *const TENS[10] = {"", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"};
	if (n < 20)
		return ONES[n < 0 ? 0 : n];
	if (n < 100)
		return std::string(TENS[n / 10]) + (n % 10 ? std::string("-") + ONES[n % 10] : "");
	return std::to_string(n);
}

std::string Ending::timePlayed(int years, int months, int days) {
	auto unit = [](int n, const char *name) { return numberToWords(n) + " " + name + (n == 1 ? "" : "s"); };
	return "in " + unit(years, "year") + ", " + unit(months, "month") + ",\nand " + unit(days, "day") + ".";
}

static void text(Framebuffer &screen, const Font &font, int x, int y, const std::string &s, int colour) {
	for (size_t k = 0; k < s.size(); ++k) {
		const uint8_t *g = font.glyph[(uint8_t)s[k]];
		for (int r = 0; r < 8; ++r)
			for (int b = 0; b < 8; ++b) {
				int px = x + (int)k * 8 + b, py = y + r;
				if ((g[r] & (0x80 >> b)) && px >= 0 && py >= 0 && px < Framebuffer::WIDTH && py < Framebuffer::HEIGHT)
					screen.pixels()[py * Framebuffer::WIDTH + px] = (uint8_t)colour;
			}
	}
}

static void blitShape(Framebuffer &screen, const Shape &s, int x, int y, int clipRight = 320) {
	for (int j = 0; j < s.height; ++j)
		for (int i = 0; i < s.width; ++i) {
			int px = x - s.hotX + i, py = y - s.hotY + j;
			uint8_t v = s.pixels[j * s.width + i];
			if (v != Shape::TRANSPARENT && px >= 0 && py >= 0 && px < clipRight && py < Framebuffer::HEIGHT)
				screen.pixels()[py * Framebuffer::WIDTH + px] = v;
		}
}

// APPROX(50): which picture goes with which page and where it stands are guesses; the original's scene scripting (show_ending_text, END.EXE 10BFA) is not traced
void Ending::render(Framebuffer &screen) {
	screen.setPalette(_palette);
	screen.clear(0);
	int brightest = 15, best = -1;
	for (int c = 1; c < 255; ++c) {
		int sum = _palette[c][0] + _palette[c][1] + _palette[c][2];
		if (sum > best) {
			best = sum;
			brightest = c;
		}
	}
	if (_page >= _screens.size()) {
		// the credits
		text(screen, _font, 100, 24, "CONGRATULATIONS", brightest);
		int y = 52;
		std::vector<std::string> lines = wrap("Thou hast completed Ultima VI: The False Prophet,", 38);
		std::string t = timePlayed(_years, _months, _days);
		size_t pos = 0;
		while (pos <= t.size()) {
			size_t nl = t.find('\n', pos);
			lines.push_back(t.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos));
			if (nl == std::string::npos)
				break;
			pos = nl + 1;
		}
		lines.push_back("");
		for (const std::string &l : wrap("Report thy feat unto Lord British at Origin Systems!", 38))
			lines.push_back(l);
		for (const std::string &l : lines) {
			text(screen, _font, 8, y, l, brightest);
			y += 10;
		}
		text(screen, _font, 8, 140, "\"The path to victory is marked with", brightest);
		text(screen, _font, 8, 150, "the laws of Virtue, not the commands", brightest);
		text(screen, _font, 8, 160, "of DOS.\"", brightest);
		text(screen, _font, 160, 176, "-- Lord British", brightest);
		return;
	}
	const Screen &sc = _screens[_page];
	int scene = sc.scene;
	blitShape(screen, _shapes[7], 0, 0);             // the Codex chamber
	if (scene < 6) {
		blitShape(screen, _shapes[3], 119, 0);       // the red gate
		blitShape(screen, _shapes[8], 0, 0, 160);    // Lord British and Nystul
	}
	if (scene >= 3 && scene < 6)
		blitShape(screen, _shapes[2], 128, 20);      // the Codex shimmers on the wall
	if (scene >= 6)
		blitShape(screen, _shapes[8], 0, 0);         // and King Draxinusom
	if (scene == 2 || scene == 8)
		blitShape(screen, _shapes[9], 0, 0);         // the lenses change hands
	blitShape(screen, _shapes[10], 1, 134);
	int y = 140;
	for (const std::string &l : sc.lines) {
		text(screen, _font, 12, y, l, 0);
		y += 8;
	}
}

bool Ending::handleKey(int key) {
	if (key == KEY_QUIT)
		return false;
	if (key == KEY_NONE)
		return true;
	if (_page > _screens.size())
		return false;
	++_page;
	return _page <= _screens.size();
}

} // namespace Ultima6
} // namespace Ultima6
