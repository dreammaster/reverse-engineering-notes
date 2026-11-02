#include "ultima6/text_window.h"

namespace Ultima6 {
namespace Ultima6 {

void TextWindow::newLine() {
	_lines.push_back("");
	if ((int)_lines.size() > _rows)
		_lines.erase(_lines.begin());
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
	if ((int)line.size() >= _cols) {
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
