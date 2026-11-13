#include "xanth/display.h"

#include <cstdio>
#include <cstdlib>

#include "legend/draw.h"
#include "xanth/logic/logic.h"

namespace Xanth {

using namespace Legend;

Display::Display(const Common::GameFiles &files) : _now(0), _files(files), _game(NULL), _screen(320, 200), _picNum(-1), _compass(0), _frameDrawn(false) {
	memset(_drawn, 0, sizeof(_drawn));
}

bool Display::init() {
	return _ms.load(_files);
}

void Display::redrawAll() {
	_ms.drawFrame(_screen);
	_ms.drawVerbs(_screen, _verbs);
	_text.setup(51, 128, 314, 151, &_ms.font10(), 15, 23);
	_compass = 0;
	_frameDrawn = true;
	updateCompass();
}

int Display::printText(const std::string &text) {
	if (!_frameDrawn)
		return 0;
	_text.print(_screen, text);
	return 0;
}

int Display::putChar(int ch) {
	if (_frameDrawn && ch == '\n')
		_text.newLine(_screen);
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
	int changed = 0;
	// pictures that were drawn and are not visible any more: exclusive-or them out again
	for (unsigned i = 0; i < count; i++) {
		if (_drawn[i] && !overlayVisible(table->records[i])) {
			changed = 1;
			PicImage ov;
			if (MainScreen::loadPicture(_files, table->records[i].pic, ov))
				MainScreen::xorOverlay(_room, _roomPixels, ov);
			_drawn[i] = false;
		}
	}
	// and the visible ones that are not drawn yet
	for (unsigned i = 0; i < count; i++) {
		if (!_drawn[i] && overlayVisible(table->records[i])) {
			PicImage ov;
			if (MainScreen::loadPicture(_files, table->records[i].pic, ov))
				MainScreen::xorOverlay(_room, _roomPixels, ov);
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
		_picNum = -1;
		popupWindowEvent();
		_game->call_object_handler(_game->state.v.current_room, 0x17);
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
	}
	return 0;
}

} // namespace Xanth
