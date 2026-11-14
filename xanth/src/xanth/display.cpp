#include "xanth/display.h"

#include <cstdio>
#include <cstdlib>

#include "legend/draw.h"
#include <algorithm>

#include "xanth/logic/logic.h"

namespace Xanth {

using namespace Legend;

static const int kTextBottom = 151, kSavedTop = 149, kSavedBottom = 196;

Display::Display(const Common::GameFiles &files) : _now(0), _files(files), _game(NULL), _screen(320, 200), _picNum(-1), _compass(0), _frameDrawn(false), _mouseX(160), _mouseY(100), _expanded(false), _input(NULL), _convPicW(0), _convPicH(0), _convBubbleRight(0), _convMenuBottom(0), _selectedVerb(-1), _inventoryCount(0), _inventoryScroll(0), _rgnFile(-1) {
	memset(_drawn, 0, sizeof(_drawn));
}

bool Display::init() {
	return _ms.load(_files);
}

void Display::redrawAll() {
	_ms.drawFrame(_screen);
	_ms.drawVerbs(_screen, _verbs);
	_expanded = false;
	_turnText.clear();
	_selectedVerb = -1;
	_text.setup(51, 128, 314, kTextBottom, &_ms.font10(), 15, 23);
	_compass = 0;
	_frameDrawn = true;
	updateCompass();
}

void Display::expandText(int edge) {
	if (_expanded)
		collapseText();
	_below.clear();
	for (int y = kSavedTop; y <= kSavedBottom; y++)
		for (int x = 50; x <= 316; x++)
			_below.push_back(_screen.get(x, y));
	_expanded = true;
	_text.setBottom(edge);
	_ms.restoreMarble(_screen, 50, edge, 316, kSavedBottom);              // the inventory below the grown window is gone until the next command
	fillArea(_screen, 51, 129, 314, edge - 1, 23);
	for (int x = 50; x <= 315; x++)                                       // the bottom edge of the grown window is lit like the one of a bevel
		lightenPixel(_screen, x, edge);
	// three pixels of the old frames that the original's window popup darkens / lights further (as seen in the oracle's screen)
	for (int i = 0; i < 3; i++) {
		darkenPixel(_screen, 50, 149);
		darkenPixel(_screen, 50, 150);
	}
	lightenPixel(_screen, 315, 150);
}

void Display::drawSlot(int picNum, int x, int y) {
	PicImage img;
	if (!MainScreen::loadPicture(_files, picNum ? picNum : 0x100, img))
		return;
	drawPicture(_screen, x + img.x, y + img.y, &img.pixels[0], img.width, img.height, (img.flags & 0x40) != 0);
}

void Display::drawInventory() {
	struct Entry {
		int pic, obj;
	};
	// the people with you (sub_1DD59): a portrait each
	std::vector<Entry> companions;
	switch (_game->state.v.companion_obj) {
	case 5:
		companions.push_back({0x181, 5});
		if (_game->state.v.flag_85318_cart)
			companions.push_back({0x186, 9});
		break;
	case 6:
		companions.push_back({0x182, 6});
		companions.push_back({0x183, 7});
		break;
	case 10:
		companions.push_back({0x184, 10});
		break;
	case 11:
		companions.push_back({0x185, 11});
		break;
	default:
		break;
	}
	// what you carry (sub_1DDDA): everything inside the player, with its picture number
	std::vector<Entry> items;
	_game->walkContents(_game->rt.player_obj, true, [&](int obj) {
		if (obj != _game->rt.player_obj && items.size() < 48) {
			int pic = _game->object_get_prop_a(obj);
			Entry e = {pic ? pic : 0x100, obj};
			items.push_back(e);
		}
		return false;
	});
	if (items.size() <= 14)
		_inventoryScroll = 0;
	_inventoryCount = (int)items.size();
	_cells.clear();
	// sub_25C55: cells 28 x 22 pixels in a window, left to right and top to bottom
	int x = 55, y = 152, n = 0;
	for (;;) {
		drawSlot(n < (int)companions.size() ? companions[n].pic : 0, x, y);
		if (n < (int)companions.size()) {
			Cell c = {x, y, companions[n].obj};
			_cells.push_back(c);
		}
		n++;
		x += 28;
		if (x > 82) {
			x = 55;
			y += 22;
			if (y > 195)
				break;
		}
	}
	x = 92, y = 152, n = _inventoryScroll;
	for (;;) {
		drawSlot(n < (int)items.size() ? items[n].pic : 0, x, y);
		if (n < (int)items.size()) {
			Cell c = {x, y, items[n].obj};
			_cells.push_back(c);
		}
		n++;
		x += 28;
		if (x > 287) {
			x = 92;
			y += 22;
			if (y > 195)
				break;
		}
	}
	PicImage arrows;
	if (MainScreen::loadPicture(_files, items.size() > 14 ? 0x103 : 0x104, arrows))
		drawPicture(_screen, 293 + arrows.x, 152 + arrows.y, &arrows.pixels[0], arrows.width, arrows.height, false);
}

void Display::collapseText() {
	if (!_expanded)
		return;
	size_t k = 0;
	for (int y = kSavedTop; y <= kSavedBottom; y++)
		for (int x = 50; x <= 316; x++)
			_screen.put(x, y, _below[k++]);
	_expanded = false;
	_text.setBottom(kTextBottom);
}

void Display::beginCommand() {
	if (!_frameDrawn)
		return;
	collapseText();
	fillArea(_screen, 51, 129, 314, 148, 23);
	_text.home();
	_turnText.clear();
}

void Display::pressButton(int button) {
	static const int kLeft[4] = {4, 25, 4, 25}, kBottom[4] = {181, 181, 195, 195};
	_screen.put(kLeft[button], kBottom[button], 4);
}

void Display::selectVerb(int index) {
	_selectedVerb = index;
	_ms.drawVerbs(_screen, _verbs, index);
}

int Display::verbAt(int x, int y) const {
	if (x < 2 || x > 49 || y < 4)
		return -1;
	int row = (y - 4) / 10;
	return row < (int)_verbs.size() ? row : -1;
}

// the small arrow cursor of the original (SMALL_ARROW_CURSOR): the usual DOS mouse masks, 16 words that are ANDed with the screen and 16 that are then XORed
static const uint16 kSmallArrow[32] = {
	0x3FFF, 0x1FFF, 0x0FFF, 0x07FF, 0x03FF, 0x01FF, 0x00FF, 0x00FF, 0x01FF, 0xE1FF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	0x0000, 0x4000, 0x6000, 0x7000, 0x7800, 0x7C00, 0x7E00, 0x5800, 0x0C00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000};

void Display::render(Surface &out) const {
	out = _screen;
	if (!_hover.empty() && _frameDrawn) {
		// the name of the object under the pointer: outline font 1, centred in the picture window, just above its bottom edge
		int w = _ms.font1().textWidthOutlined(_hover);
		_ms.font1().drawTextOutlined(out, 181 - w / 2, 114, _hover, 15, 0);   // (centre found from the oracle: 151 for "computer")
	}
	for (int row = 0; row < 16; row++) {
		for (int col = 0; col < 16; col++) {
			unsigned bit = 0x8000u >> col;
			byte old = out.get(_mouseX + col, _mouseY + row);
			byte v = (byte)((old & ((kSmallArrow[row] & bit) ? 0xFF : 0)) ^ ((kSmallArrow[16 + row] & bit) ? 0xFF : 0));
			if (_mouseX + col >= 0 && _mouseY + row >= 0)
				out.put(_mouseX + col, _mouseY + row, v);
		}
	}
}

// ------------------------------------------------------------------------------------------------------------------------------ conversations
// The scene of Conversation_Show: the picture in a frame at the top left on the marble, the replies in a window below it, the speech to its right.
void Display::conversationBegin(int picNum) {
	PicImage img;
	_ms.restoreMarble(_screen, 0, 0, 319, 199);
	_frameDrawn = true;
	if (!MainScreen::loadPicture(_files, picNum, img))
		return;
	byte pal[768];
	memcpy(pal, img.hasPalette ? img.palette : _screen.palette, 768);
	_ms.restoreSubPalettes(pal);
	_screen.setPalette(pal);
	_convPicW = img.width;
	_convPicH = img.height;
	_convBubbleRight = 0;
	_convMenuBottom = 0;
	bevelFrame(_screen, 3, 3, img.width + 4, img.height + 4);
	drawPicture(_screen, 4 + img.x, 4 + img.y, &img.pixels[0], img.width, img.height, false);
	_picNum = -1;
}

void Display::clearConversationBubble() {
	// a new line of speech: the speech to the right of the picture and everything below it go back to the marble
	if (_convBubbleRight)
		_ms.restoreMarble(_screen, _convPicW + 6, 4, 319, _convPicH + 6);
	if (_convMenuBottom)
		_ms.restoreMarble(_screen, 0, _convPicH + 6, 319, 199);
	_convBubbleRight = _convMenuBottom = 0;
}

void Display::conversationLine(const std::string &text, bool wait) {
	clearConversationBubble();
	const Font &font = _ms.font10();
	int x1 = _convPicW + 8, x2 = 315;
	int w = font.textWidth(text) + font.advance('M');
	if (w > 307 - _convPicW)
		w = 307 - _convPicW;
	Surface nothing(1, 1);
	TextWindow probe;
	probe.setup(x1, 0, x1 + w, 4000, &font, 15, 23);
	probe.print(nothing, text);
	int textHeight = probe.penY() + font.lineHeight() - font.yCenter();   // the pen's line plus the margin below
	int y1 = (_convPicH - textHeight) / 2 + 4;
	if (y1 < 4)
		y1 = 4;
	int y2 = y1 + textHeight;
	bevelFrame(_screen, x1 - 1, y1 - 1, x1 + w + 1, y2 + 1);
	fillArea(_screen, x1, y1, x1 + w, y2, 23);
	TextWindow bubble;
	bubble.setup(x1, y1, x1 + w, y2, &font, 15, -1);
	bubble.print(_screen, text);
	(void)x2;
	_convBubbleRight = x1 + w + 2;
	if (wait && _input) {
		for (;;) {
			InputEvent e = _input->wait();
			if (e.type == InputEvent::Click || e.type == InputEvent::Key || e.type == InputEvent::Quit)
				break;
		}
	}
}

int Display::conversationMenu(const std::vector<std::string> &options, int initial) {
	if (_convMenuBottom) {                      // (the speech to the right stays while the player chooses)
		_ms.restoreMarble(_screen, 0, _convPicH + 6, 319, 199);
		_convMenuBottom = 0;
	}
	const Font &font = _ms.font10();
	int y1 = _convPicH + 7;
	int maxW = 0;
	for (size_t i = 0; i < options.size(); i++) {
		int w = font.textWidth(options[i]) + font.advance('M');
		if (w > maxW)
			maxW = w;
	}
	if (maxW > 311)
		maxW = 311;
	// the height of every option: the lines it wraps to
	std::vector<int> top(options.size() + 1, 0);
	int h = font.yCenter();
	for (size_t i = 0; i < options.size(); i++) {
		Surface nothing(1, 1);
		TextWindow probe;
		probe.setup(4, 0, 4 + maxW, 4000, &font, 15, -1);
		probe.print(nothing, options[i]);
		top[i] = h;
		h += probe.penY() + font.lineHeight() - font.yCenter() - font.yCenter() + (int)0;
	}
	top[options.size()] = h;
	int y2 = y1 + h + font.yCenter();
	if (y2 > 199)
		y2 = 199;
	bevelFrame(_screen, 3, y1 - 1, 4 + maxW + 1, y2 + 1);
	fillArea(_screen, 4, y1, 4 + maxW, y2, 23);
	_convMenuBottom = y2 + 2;
	int sel = initial < 0 || initial >= (int)options.size() ? 0 : initial;
	auto draw = [&](int i, int color) {
		TextWindow line;
		line.setup(4, y1 + top[i] - font.yCenter(), 4 + maxW, y2, &font, color, -1);
		line.print(_screen, options[i]);
	};
	for (size_t i = 0; i < options.size(); i++)
		draw((int)i, (int)i == sel ? 16 : 15);
	if (!_input)
		return sel;
	for (;;) {
		InputEvent e = _input->wait();
		if (e.type == InputEvent::Quit)
			return sel;
		int hit = -1;
		if (e.type == InputEvent::Move || e.type == InputEvent::Click) {
			if (e.x >= 4 && e.x <= 4 + maxW) {
				for (size_t i = 0; i < options.size(); i++)
					if (e.y >= y1 + top[i] - font.yCenter() / 2 && e.y < y1 + top[i + 1] - font.yCenter() / 2 + (i + 1 == options.size() ? 99 : 0))
						hit = (int)i;
			}
		}
		if (e.type == InputEvent::Key) {
			int old = sel;
			if (e.key == 0x148 && sel > 0)
				sel--;
			else if (e.key == 0x150 && sel + 1 < (int)options.size())
				sel++;
			else if (e.key == 0x0D)
				return sel;
			if (sel != old) {
				fillArea(_screen, 4, y1, 4 + maxW, y2, 23);
				for (size_t i = 0; i < options.size(); i++)
					draw((int)i, (int)i == sel ? 16 : 15);
			}
		} else if (hit >= 0 && hit != sel) {
			sel = hit;
			fillArea(_screen, 4, y1, 4 + maxW, y2, 23);
			for (size_t i = 0; i < options.size(); i++)
				draw((int)i, (int)i == sel ? 16 : 15);
		}
		if (e.type == InputEvent::Click && hit >= 0)
			return hit;
	}
}

void Display::conversationEnd() {
	clearConversationBubble();
	_convPicW = _convPicH = 0;
}

int Display::waitTicksOrClick(int ticks) {
	// wait_ticks_or_click: a click or a key ends the wait; ticks (of about 55 ms) end it too, 0 waits for the player alone
	if (!_input)
		return 0;
	unsigned ms = (unsigned)ticks * 55u;
	for (;;) {
		InputEvent e = ticks ? _input->wait(ms) : _input->wait();
		if (e.type == InputEvent::Click || e.type == InputEvent::Key || e.type == InputEvent::Quit || e.type == InputEvent::Timeout)
			return e.type == InputEvent::Click ? 1 : 0;
	}
}

int Display::inventoryObjectAt(int x, int y) const {
	if (_expanded)
		return 0;
	for (size_t i = 0; i < _cells.size(); i++)
		if (x >= _cells[i].x && x <= _cells[i].x + 23 && y >= _cells[i].y && y <= _cells[i].y + 19)
			return _cells[i].obj;
	return 0;
}

bool Display::scrollInventory(int x, int y) {
	if (_expanded || _inventoryCount <= 14 || x < 293 || x > 314 || y < 152 || y > 195)
		return false;
	if (y <= 173) {
		_inventoryScroll -= 7;
		if (_inventoryScroll < 7)
			_inventoryScroll = 0;
	} else if (_inventoryScroll + 14 < _inventoryCount) {
		_inventoryScroll += 7;
	}
	drawInventory();
	return true;
}

int Display::popupMenu(const std::vector<std::string> &labels) {
	if (labels.empty())
		return -1;
	const Font &font = _ms.font1();
	int w = 0;
	for (size_t i = 0; i < labels.size(); i++)
		w = std::max(w, font.textWidth(labels[i]));
	int bw = w + 16, step = 16;
	int ww = bw + 10, wh = (int)labels.size() * step + 8;
	int x1 = 160 - ww / 2, y1 = 100 - wh / 2;
	Surface saved = _screen;
	_ms.restoreMarble(_screen, x1, y1, x1 + ww, y1 + wh);
	bevelFrame(_screen, x1 - 1, y1 - 1, x1 + ww + 1, y1 + wh + 1);
	std::vector<int> top(labels.size());
	auto draw = [&](size_t i, bool lit) {
		int bx = x1 + 5, by = y1 + 4 + (int)i * step;
		top[i] = by;
		buttonFrame(_screen, bx, by, bx + bw - 1, by + 11, true);
		font.drawText(_screen, bx + (bw - font.textWidth(labels[i])) / 2, by + 2, labels[i], lit ? 16 : 0);
	};
	for (size_t i = 0; i < labels.size(); i++)
		draw(i, false);
	int result = -1;
	if (_input) {
		size_t lit = labels.size();
		for (;;) {
			InputEvent e = _input->wait();
			if (e.type == InputEvent::Quit)
				break;
			int hit = -1;
			if ((e.type == InputEvent::Move || e.type == InputEvent::Click) && e.x >= x1 + 5 && e.x < x1 + 5 + bw)
				for (size_t i = 0; i < labels.size(); i++)
					if (e.y >= top[i] && e.y <= top[i] + 11)
						hit = (int)i;
			if (e.type == InputEvent::Key && e.key == 27)
				break;
			if (e.type == InputEvent::Click) {
				result = hit;
				break;
			}
			if (hit != (int)lit && e.type == InputEvent::Move) {
				if (lit < labels.size())
					draw(lit, false);
				lit = hit >= 0 ? (size_t)hit : labels.size();
				if (lit < labels.size())
					draw(lit, true);
			}
			_mouseX = e.x;
			_mouseY = e.y;
		}
	}
	_screen = saved;
	return result;
}

int Display::compassAt(int x, int y) const {
	// regionRefs of the original: the rectangles of the arrows, relative to the compass picture at (4, 129)
	static const signed char kRects[10][4] = {{15, 2, 27, 11}, {30, 3, 38, 11}, {30, 13, 39, 25}, {30, 27, 38, 35}, {15, 27, 27, 36},
	                                          {4, 27, 12, 35}, {3, 13, 12, 25}, {4, 3, 12, 11}, {16, 13, 26, 18}, {16, 20, 26, 25}};
	for (int i = 0; i < 10; i++) {
		if (x >= 4 + kRects[i][0] && x <= 4 + kRects[i][2] && y >= 129 + kRects[i][1] && y <= 129 + kRects[i][3])
			return (_compass & (1u << i)) ? i : -1;
	}
	return -1;
}

int Display::objectAt(int x, int y, bool orRoom) {
	if (x < 51 || x > 314 || y < 4 || y > 125)
		return 0;
	if (_picNum < 0)
		return orRoom ? _game->state.v.current_room : 0;
	if (_rgnFile != (_picNum >> 8)) {
		char name[16];
		snprintf(name, sizeof(name), "XANTH_%02d.RGN", _picNum >> 8);
		Common::Bytes d;
		_rgn = Legend::RgnFile();
		if (_files.load(name, d))
			_rgn.load(d);
		_rgnFile = _picNum >> 8;
	}
	size_t scene = _picNum & 255;
	if (scene >= _rgn.size())
		return orRoom ? _game->state.v.current_room : 0;
	// regions are tested from the last one added: what was added later lies on top (the computer on the desk)
	const std::vector<Legend::RgnFile::Region> &regions = _rgn.scene(scene).regions;
	for (size_t i = regions.size(); i-- > 0;)
		if (regions[i].contains(x - 51, y - 4))
			return regions[i].id;
	return orRoom ? _game->state.v.current_room : 0;
}

int Display::printText(const std::string &text) {
	if (!_frameDrawn)
		return 0;
	// how much room the text of this turn needs: the window grows by lines, from its edge at 133 + 8 * lines
	{
		Surface nothing(1, 1);
		TextWindow tall = _text;
		tall.home();
		tall.setBottom(4000);
		for (size_t i = 0; i < _turnText.size(); i++)
			tall.print(nothing, _turnText[i]);
		tall.print(nothing, text);
		const Font &font = _ms.font10();
		// the window ends below the last line (half a line below the pen when it just started a line); more than the two lines of the normal window makes it grow
		int edge = 128 + tall.penOffsetY() + (tall.atLineStart() ? font.yCenter() : font.lineHeight()) + 1;
		if (getenv("XANTH_TRACE_TEXT"))
			fprintf(stderr, "text edge %d (pen %d, start %d) for \"%s\"\n", edge, tall.penOffsetY(), tall.atLineStart() ? 1 : 0, text.c_str());
		if (edge <= kTextBottom)
			edge = kTextBottom;
		if (edge > 193)                      // (the window grows no further than this: longer text scrolls)
			edge = 193;
		if (edge > kTextBottom && (!_expanded || edge > _text.bottom())) {
			expandText(edge);
			_text.home();
			for (size_t i = 0; i < _turnText.size(); i++)      // the grown window starts again at the top with everything of this turn
				_text.print(_screen, _turnText[i]);
		}
	}
	_turnText.push_back(text);
	_text.print(_screen, text);
	return 0;
}

int Display::showTextBox(const std::string &text) {
	// a text box replaces what the text window showed (it grows to fit the box's text)
	if (_frameDrawn) {
		fillArea(_screen, 51, 129, 314, (_expanded ? _text.bottom() : kTextBottom) - 3, 23);
		_text.home();
		_turnText.clear();
	}
	return printText(text);
}

int Display::putChar(int ch) {
	if (!_frameDrawn)
		return 0;
	if (ch == '\n')
		_text.newLine(_screen);
	else if (ch >= 0x20 && ch < 0x7F)
		printText(std::string(1, (char)ch));          // wputchar: one character through the same word wrap
	return 0;
}

bool Display::loadRoom(int picNum) {
	PicImage img;
	if (!MainScreen::loadPicture(_files, picNum, img))
		return false;
	_room = img;
	_roomPixels = img.pixels;
	_picNum = picNum;
	memset(_drawn, 0, sizeof(_drawn));
	return true;
}

void Display::drawRoom() {
	if (_picNum >= 0)
		_ms.drawPictureWindow(_screen, _room, _roomPixels);
}

int Display::addAnimation(int picNum, int count, int step, unsigned start) {
	if (_anims.size() >= 5)
		return -1;
	Animation a;
	for (int i = 0; i < count; i++) {
		PicImage f;
		if (!MainScreen::loadPicture(_files, picNum + i, f))
			return -1;
		a.frames.push_back(f);
	}
	a.step = step;
	a.cur = 0;
	a.next = 0;
	a.start = start;
	_anims.push_back(a);
	return (int)_anims.size() - 1;
}

void Display::startAnimations() {
	for (size_t i = 0; i < _anims.size(); i++) {
		Animation &a = _anims[i];
		a.cur = a.step == -1 ? (int)a.frames.size() - 1 : 0;
		a.next = a.start + _now;
	}
}

void Display::drawFrame(const PicImage &f) {
	drawPicture(_screen, 51 + f.x, 4 + f.y, &f.pixels[0], f.width, f.height, (f.flags & 0x40) != 0);
}

void Display::advance(unsigned ms) {
	_now += ms;
	for (size_t i = 0; i < _anims.size(); i++) {
		Animation &a = _anims[i];
		if (a.cur < 0 || _now < a.next)
			continue;
		const PicImage &img = a.frames[a.cur];
		bool xorKind = (img.flags & 0x40) != 0;
		int delayFrame = a.cur;
		if (a.cur > 0 && a.step < 0 && xorKind)
			delayFrame--;
		const PicImage &timing = a.frames[delayFrame];
		a.next = _now + 50u * timing.delayBase + (unsigned)(_game->_rand() % (50 * timing.delayRandom + 1));
		drawFrame(img);
		if (a.step != 0) {
			a.cur += a.step;
			if (a.cur < 0 || a.cur >= (int)a.frames.size()) {
				int back = xorKind ? 1 : 2;           // looping: turn round without drawing the end frame twice (a delta picture)
				a.step = -a.step;
				a.cur += back * a.step;
			}
		} else {
			a.cur++;
			if (a.cur >= (int)a.frames.size())
				a.cur = 0;                        // update_animation(1): loop
		}
	}
}

void Display::popupWindowEvent() {
	int pic = sub_1DB4E(*_game, _game->state.v.current_room);
	if (pic != 0xFFFF && pic != _picNum)
		loadRoom(pic);
	_game->call_object_handler(_game->state.v.current_room, 0x16);
	drawRoom();
	startAnimations();
}

bool Display::overlayVisible(const OverlayRecord &r) {
	// overlay_visible?: the callback decides if there is one; otherwise the object must be inside its parent / have its attribute
	if (r.visible)
		return r.visible(*_game) != 0;
	if (r.object && !r.parent && !r.attr)
		return true;
	if (r.parent && !_game->object_parent_is(r.object, r.parent))
		return false;
	if (r.attr && !_game->object_has_attr(r.object, r.attr))
		return false;
	return true;
}

int Display::setupPicRegions(unsigned address, unsigned count) {
	const OverlayTable *table = NULL;
	for (unsigned i = 0; i < kOverlayTableCount; i++)
		if (kOverlayTables[i].address == address)
			table = &kOverlayTables[i];
	if (!table || _picNum < 0)
		return 0;
	if (count > table->count)
		count = table->count;
	if (getenv("XANTH_TRACE"))
		for (unsigned i = 0; i < count; i++)
			fprintf(stderr, "  overlay %X[%u] pic %X visible %d drawn %d\n", address, i, table->records[i].pic, overlayVisible(table->records[i]) ? 1 : 0, _drawn[i] ? 1 : 0);
	int changed = 0;
	// pictures that were drawn and are not visible any more: exclusive-or them out again
	for (unsigned i = 0; i < count; i++) {
		if (_drawn[i] && !overlayVisible(table->records[i])) {
			changed = 1;
			PicImage ov;
			if (MainScreen::loadPicture(_files, table->records[i].pic, ov)) {
				MainScreen::xorOverlay(_room, _roomPixels, ov);
				if (ov.hasPalette) {                 // opening a picture loads its palette too
					memcpy(_room.palette, ov.palette, 768);
					_room.hasPalette = true;
				}
			}
			_drawn[i] = false;
		}
	}
	// and the visible ones that are not drawn yet
	for (unsigned i = 0; i < count; i++) {
		if (!_drawn[i] && overlayVisible(table->records[i])) {
			PicImage ov;
			if (MainScreen::loadPicture(_files, table->records[i].pic, ov)) {
				MainScreen::xorOverlay(_room, _roomPixels, ov);
				if (ov.hasPalette) {
					memcpy(_room.palette, ov.palette, 768);
					_room.hasPalette = true;
				}
			}
			_drawn[i] = true;
			changed = 1;
		}
	}
	return changed;
}

void Display::updateCompass() {
	unsigned mask = (unsigned)sub_1DBDC(*_game) & 0x3FF;
	_ms.toggleCompassArrows(_screen, mask ^ _compass);
	_compass = mask;
}

int Display::other(const char *name, const intptr_t *args, int) {
	if (getenv("XANTH_TRACE"))
		fprintf(stderr, "other %s(%d, %d, %d)\n", name, (int)args[0], (int)args[1], (int)args[2]);
	if (!strcmp(name, "RoomItem_Draw")) {
		// 0x10: everything; 0x20: the compass and the lists; 0x40 (and each of the others, in the normal graphics mode): the room is loaded again and drawn
		if (args[0] == 0x10)
			redrawAll();
		else if (args[0] == 0x20)
			updateCompass();
		if ((args[0] == 0x10 || args[0] == 0x20) && !_expanded)
			drawInventory();
		_picNum = -1;
		popupWindowEvent();
		_game->call_object_handler(_game->state.v.current_room, 0x17);
	} else if (!strcmp(name, "game_over_screen")) {
		// a full-screen picture (the letter, the endings): no frame, the picture's own palette; the caller waits for a click and redraws the game
		PicImage img;
		if (MainScreen::loadPicture(_files, (int)args[0], img)) {
			_screen.clear(0);
			if (img.hasPalette)
				_screen.setPalette(img.palette);
			drawPicture(_screen, img.x, img.y, &img.pixels[0], img.width, img.height, false);
			_frameDrawn = false;
			_picNum = -1;
		}
	} else if (!strcmp(name, "sub_1EED9")) {
		const intptr_t redraw[5] = {0x10, 0, 0, 0, 0};
		other("RoomItem_Draw", redraw, 5);
	} else if (!strcmp(name, "popup_window_event")) {
		popupWindowEvent();
	} else if (!strcmp(name, "setup_pic_regions")) {
		return setupPicRegions((unsigned)args[0], (unsigned)args[1]);
	} else if (!strcmp(name, "sub_2B236")) {
		_ms.toggleCompassArrows(_screen, (unsigned)args[0] ^ _compass);
		_compass = (unsigned)args[0];
	} else if (!strcmp(name, "sub_2A5B4")) {
		_anims.clear();
	} else if (!strcmp(name, "sub_2A386")) {
		for (size_t i = 0; i < _anims.size(); i++) {
			_anims[i].start = 0;
			if (_anims[i].step != 0)
				_anims[i].step = 1;
		}
	} else if (!strcmp(name, "sub_2A333")) {
		startAnimations();
	} else if (!strcmp(name, "sub_2A207")) {
		return addAnimation((int)args[0], (int)args[1], (int8)args[2], (unsigned)args[3]);
	} else if (!strcmp(name, "clear_text_area")) {
		_text.clear(_screen);
		_turnText.clear();
	}
	return 0;
}

} // namespace Xanth
