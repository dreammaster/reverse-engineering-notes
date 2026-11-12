#include "legend/textwindow.h"

namespace Legend {

TextWindow::TextWindow() : _font(NULL), _x1(0), _y1(0), _x2(0), _y2(0), _left(0), _fg(15), _bg(-1), _xpos(0), _ypos(0) {
}

void TextWindow::setup(int x1, int y1, int x2, int y2, const Font *font, int foreground, int background) {
	_font = font;
	_x1 = _left = x1;
	_y1 = y1;
	_x2 = x2;
	_y2 = y2;
	_fg = foreground;
	_bg = background;
	_xpos = font->xCenter();
	_ypos = font->yCenter();
}

void TextWindow::clear(Surface &s) {
	s.fillRect(Common::Rect((int16)_x1, (int16)_y1, (int16)(_x2 + 1), (int16)(_y2 + 1)), (byte)(_bg < 0 ? 0 : _bg));
	_xpos = _font->xCenter();
	_ypos = _font->yCenter();
}

int TextWindow::wordWidth(const std::string &t, size_t from, size_t to) const {
	int w = 0;
	for (size_t i = from; i < to; i++)
		w += _font->advance((byte)t[i]);
	return w;
}

void TextWindow::writeRange(Surface &s, const std::string &t, size_t from, size_t to) {
	int x = penX(), y = penY();
	for (size_t i = from; i < to; i++)
		x += _font->drawChar(s, x, y, (byte)t[i], (byte)_fg, _bg);
}

void TextWindow::newLine(Surface &s) {
	int lineHeight = _font->lineHeight();
	if (_y2 - _y1 - 2 * lineHeight >= _ypos) {
		_ypos += lineHeight;
	} else {
		// scroll_up?: the lines below the first move up by one line, the last line is cleared
		int top = _y1 + _font->yCenter(), bottom = _y2 - _font->yCenter(), left = _x1 + _font->xCenter(), right = _x2 - 1;
		for (int y = top; y + lineHeight <= bottom; y++)
			for (int x = left; x <= right; x++)
				s.put(x, y, s.get(x, y + lineHeight));
		int fill = _bg < 0 ? 0 : _bg;
		for (int y = bottom - lineHeight + 1; y <= bottom; y++)
			for (int x = left; x <= right; x++)
				s.put(x, y, (byte)fill);
	}
	_xpos = _font->xCenter();
}

int TextWindow::print(Surface &s, const std::string &t) {
	const int xCenter = _font->xCenter();
	const int spaceWidth = _font->advance(' ');
	int count = 0;
	size_t msg = 0;
	while (msg < t.size()) {
		size_t end = msg;
		while (end < t.size() && t[end] != ' ' && t[end] != '\t' && t[end] != '\n' && t[end] != '-')
			end++;
		if (end < t.size() && t[end] == '-')
			end++;
		int w = wordWidth(t, msg, end);
		int avail = _x2 - _left - xCenter;
		if (avail - xCenter < w) {
			// wider than a whole line: as many characters as fit on the rest of this one
			avail -= _xpos;
			w = 0;
			end = msg;
			while (end < t.size() && _font->advance((byte)t[end]) + w <= avail) {
				w += _font->advance((byte)t[end]);
				end++;
			}
		} else {
			avail -= _xpos;
			if (avail < w)
				newLine(s);
		}
		if (end == msg && msg < t.size() && t[msg] != ' ' && t[msg] != '\t' && t[msg] != '\n') {
			// not even one character fits on the rest of the line: continue on the next one (always taking a character, so that the loop ends)
			newLine(s);
			end = msg + 1;
			w = _font->advance((byte)t[msg]);
		}
		writeRange(s, t, msg, end);
		_xpos += w;
		count += (int)(end - msg);
		if (end < t.size() && t[end] == ' ') {
			msg = end + 1;
			if (_x2 - _xpos - _left - xCenter < spaceWidth) {
				newLine(s);
			} else {
				writeRange(s, " ", 0, 1);
				_xpos += spaceWidth;
				count++;
			}
		} else if (end < t.size() && t[end] == '\n') {
			msg = end + 1;
			newLine(s);
		} else if (end < t.size() && t[end] == '\t') {
			msg = end + 1; // tab stops are not implemented
		} else {
			msg = end;
		}
	}
	return count;
}

} // namespace Legend
