#ifndef ULTIMA6_ULTIMA6_TEXT_WINDOW_H
#define ULTIMA6_ULTIMA6_TEXT_WINDOW_H

#include "engine/framebuffer.h"
#include "files/tile_data.h"

#include <string>
#include <vector>

namespace Ultima6 {
namespace Ultima6 {

/**
 * One of the text windows of GAME.EXE (sub_17449 defines them in character cells of 8 x 8 pixels).
 * The play screen has the full-screen window, the party panel (columns 22-38, rows 1-12) and the
 * scroll (columns 22-38, rows 14-23: 17 x 10 characters at pixel 176,112) where the game prints.
 * Text is black-ish (colour 0x48) on the parchment (colour 0x31) of PAPER.BMP; words wrap, the
 * oldest line scrolls away.
 */
class TextWindow {
public:
	TextWindow(int col, int row, int cols, int rows, int fg = 0x48, int bg = 0x31)
		: _col(col), _row(row), _cols(cols), _rows(rows), _fg(fg), _bg(bg) {
		_lines.push_back("");
	}

	/** Prints text; '\n' starts a new line. */
	void print(const std::string &text);
	/** printf-style helper for one %s / %d free messages is not needed: callers format themselves. */
	void clear();
	/** Draws the window on the screen. */
	void draw(Framebuffer &screen, const Font &font) const;

	int cols() const {
		return _cols;
	}
	int rows() const {
		return _rows;
	}
	/** The visible lines, oldest first (the last one is the line being typed). */
	const std::vector<std::string> &lines() const {
		return _lines;
	}

private:
	void putChar(char c);
	void newLine();

	int _col, _row, _cols, _rows, _fg, _bg;
	std::vector<std::string> _lines; // at most _rows
};

} // namespace Ultima6
} // namespace Ultima6

#endif
