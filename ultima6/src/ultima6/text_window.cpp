#include "ultima6/text_window.h"

namespace Ultima6 {
namespace Ultima6 {

void TextWindow::newLine() {
	_lines.push_back("");
	if ((int)_lines.size() > maxLines())
		_lines.erase(_lines.begin());
}

int TextWindow::lineWidth(const std::string &line) const {
	int w = 0;
	for (char c : line)
		w += _font->wouWidth[(uint8_t)c];
	return w;
}

// APPROX(42): not the original's exact behaviour, see docs/approximations.md
void TextWindow::putChar(char c) {
	if (c == '\b') {
		if (!_lines.back().empty())
			_lines.back().erase(_lines.back().size() - 1);
		return;
	}
	if (c == '\n') {
		newLine();
		return;
	}
	std::string &line = _lines.back();
	bool full = _font && _font->wou ? lineWidth(line) + _font->wouWidth[(uint8_t)c] > _cols * 8 : (int)line.size() >= _cols;
	if (full) {
		// wrap at the last space of the full line, as the game's word wrapping does
		size_t space = line.rfind(' ');
		std::string carry;
		if (c != ' ' && space != std::string::npos) {
			carry = line.substr(space + 1);
			line.erase(space);
		}
		newLine();
		_lines.back() = carry;
		if (c == ' ')
			return; // the space that caused the wrap is dropped
	}
	_lines.back() += c;
}

void TextWindow::print(const std::string &text) {
	for (char c : text)
		putChar(c);
}

void TextWindow::clear() {
	_lines.assign(1, "");
}

void TextWindow::draw(Framebuffer &screen, const Font &font) const {
	int x0 = _col * 8, y0 = _row * 8;
	screen.fillRect(x0, y0, _cols * 8, _rows * 8, _bg);
	if (font.wou) {
		for (size_t i = 0; i < _lines.size(); ++i) {
			int x = x0, y = y0 + (int)i * font.wouHeight;
			for (char ch : _lines[i]) {
				uint8_t c = (uint8_t)ch;
				int w = font.wouWidth[c];
				for (int r = 0; r < font.wouHeight; ++r)
					for (int b = 0; b < w; ++b)
						if (font.wouPixels[c][r * w + b] && x + b < Framebuffer::WIDTH && y + r < Framebuffer::HEIGHT)
							screen.pixels()[(y + r) * Framebuffer::WIDTH + x + b] = (uint8_t)_fg;
				x += w;
			}
		}
		return;
	}
	// the newest line sits on the last row once the window has filled up
	int first = (int)_lines.size() > _rows ? 0 : 0;
	for (size_t i = first; i < _lines.size(); ++i) {
		const std::string &line = _lines[i];
		for (size_t k = 0; k < line.size(); ++k) {
			const uint8_t *g = font.glyph[(uint8_t)line[k]];
			for (int r = 0; r < 8; ++r)
				for (int b = 0; b < 8; ++b)
					if (g[r] & (0x80 >> b))
						screen.pixels()[(y0 + (int)i * 8 + r) * Framebuffer::WIDTH + x0 + (int)k * 8 + b] = (uint8_t)_fg;
		}
	}
}

} // namespace Ultima6
} // namespace Ultima6
