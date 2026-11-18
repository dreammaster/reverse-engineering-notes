/* A text window of the original (the Windows_* tables and Font_WriteLines / write_window_cr): a rectangle of the screen with a font, colours and a pen.
 *
 * Text is broken into words at space, tab, new line and after a hyphen, a word that does not fit on the rest of the line starts a new line, a word wider than a
 * whole line is cut anywhere, and when the pen reaches the bottom the window scrolls up by one line. The pen starts at (xCenter, yCenter) of the font, i.e. half a
 * character in from the window's top left corner. A window with paging switched on (setPause) writes "- MORE -" on its last line and waits (the handler) every
 * time a page of lines has been written. The original's line buffer for text arriving in pieces is not implemented.
 */
#ifndef LEGEND_TEXTWINDOW_H
#define LEGEND_TEXTWINDOW_H

#include <functional>

#include "common/types.h"
#include "legend/font.h"
#include "legend/surface.h"

namespace Legend {

class TextWindow {
public:
	TextWindow();

	/** Windows_AddWindow: (x1, y1)-(x2, y2) inclusive; the pen is placed with the font's margins. `background` < 0 = transparent text. */
	void setup(int x1, int y1, int x2, int y2, const Font *font, int foreground, int background);
	void setColors(int foreground, int background) { _fg = foreground; _bg = background; }

	/** Font_WriteLines: prints text with word wrap at the pen; returns the number of characters (like the original, used for the line count of text mode). */
	int print(Surface &s, const std::string &text);
	/** write_window_cr: new line (scrolls the window up at the bottom). */
	void newLine(Surface &s);
	/** Clears the window to the background colour and puts the pen at the start. */
	void clear(Surface &s);

	/** Moves the bottom edge (the window grows over what lies below it). */
	void setBottom(int y2) { _y2 = y2; }
	int bottom() const { return _y2; }
	/** True once the window had to scroll (the text did not fit). */
	bool scrolled() const { return _scrolled; }
	/** The pen back at the start of the window, without touching the pixels. */
	void home() { _xpos = _font->xCenter(); _ypos = _font->yCenter(); _scrolled = false; }

	/** Paging on (set_window_pause?): the first "- MORE -" comes when the pen reaches the last line, the next ones every page after; off with clearPause. */
	void setPause() { _pause = _ypos - _font->yCenter(); }
	void clearPause() { _pause = -1; }
	/** What waits for the player at "- MORE -" (the screen is shown, a click or key returns). */
	void setMoreHandler(const std::function<bool()> &handler) { _more = handler; }
	/** The handler returned false (the game is ending): nothing more is written until the window is set up again. */
	bool stopped() const { return _stopped; }

	/** The pen is at the start of a line (just after a new line), and its distance from the window top. */
	bool atLineStart() const { return _xpos == _font->xCenter(); }
	int penOffsetY() const { return _ypos; }

	int penX() const { return _left + _xpos; }
	int penY() const { return _y1 + _ypos; }

private:
	int wordWidth(const std::string &t, size_t from, size_t to) const;
	void writeRange(Surface &s, const std::string &t, size_t from, size_t to);

	const Font *_font;
	int _x1, _y1, _x2, _y2, _left, _fg, _bg, _xpos, _ypos;
	int _pause;                 // Windows_v2: pixels of text left before "- MORE -", -1 = no paging
	std::function<bool()> _more;
	bool _scrolled, _stopped;
};

} // namespace Legend

#endif
