#include "ultima6/se_front_end.h"

#include "files/data_file.h"
#include "files/lib_n.h"
#include "files/world_data.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>

namespace Ultima6 {
namespace Ultima6 {

#include "ultima6/se_front_data.inc"

namespace {

const int DISSOLVE_MS = 350; // how long the dissolve of the driver takes on the original's machine (it is as fast as the processor: APPROX 105)
const int OPENING_WAIT1 = 1500, OPENING_WAIT2 = 1500; // (wait_ms (sub_13B22)(1000) of the original without music: three waits of 500 ms)
const int FRAME_MS = 14; // a vertical retrace of the VGA (70.086 a second), 14.268 ms: the waits count them

uint32_t get32(const Bytes &b, size_t o) {
	return (uint32_t)b[o] | ((uint32_t)b[o + 1] << 8) | ((uint32_t)b[o + 2] << 16) | ((uint32_t)b[o + 3] << 24);
}

// the ink of the glyphs of SAVAGE.FNT (value 0) and what is behind it (0x48) are mapped to the colours of the text object, as the driver's character routine does
const int PROMPT_FG = 1; // the arrow of wait_key_prompt (sub_14FB6) is drawn with the colour 1 (sub_14FB6 sets byte_2061C)

} // namespace

// The offsets of creation_write_savegame (sub_15A7C) are those of the file; the engine takes the spin-offs' extra array (0x14F1 to 0x15F0) out, so what lies behind it is 0x100 nearer: 0x18F3 ... is talkFlags[2] ...,
// 0x1D13 ... the globals from 0x22, 0x1D71 / 0x1D72 the globals 0x80 / 0x81. The clock (globals 2 to 7) is set to day 1, month 1, year 0 as GAME.EXE does when it first starts (sub_14746 at 0x1502D).
void SeNewCharacter::applyTo(ObjList &al) const {
	std::memset(al.partyNames[0], 0, sizeof al.partyNames[0]);
	std::memcpy(al.partyNames[0], name.c_str(), std::min<size_t>(name.size(), 13));
	al.strength[1] = (uint8_t)strength;
	al.dexterity[1] = (uint8_t)dexterity;
	al.intelligence[1] = (uint8_t)intelligence;
	al.hp[1] = (uint8_t)(strength * 2 + intelligence);
	if (al.globals.size() >= 0x82) {
		static const uint8_t TIME[6] = {0, 8, 1, 1, 0, 0};
		for (int k = 0; k < 6; ++k)
			al.globals[0x22 + k] = TIME[k];
		al.globals[4] = 1;
		al.globals[5] = 1;
		al.globals[6] = 0;
		al.globals[7] = 0;
		al.globals[0x80] = 0;
		al.globals[0x81] = 1; // a character was made (sub_15CBC reads it)
	}
	for (int a : {2, 4, 5, 7, 8, 9, 0x17})
		al.talkFlags[a] |= 1;
}

SeFrontEnd::SeFrontEnd(const std::string &dir, uint32_t seed) : _dir(dir), _seed(seed), _rng((seed * 2654435761u) | 1) {
	std::memset(_pal, 0, sizeof _pal);
	std::memset(_font.glyph, 0, sizeof _font.glyph);
	Bytes raw, data;
	LibN lib;
	if (!readFile(joinPath(dir, "SAVAGE.PAL"), raw) || !lib.load(raw, true) || !lib.get(0, data) || data.size() < 3 * 768)
		return;
	for (int p = 0; p < 3; ++p)
		for (int c = 0; c < 256; ++c)
			for (int k = 0; k < 3; ++k)
				_pal[p][c][k] = (uint8_t)(data[p * 768 + c * 3 + k] >> 2); // 8 bits a colour, the screen takes 6
	Bytes fraw, fdata;
	LibN fl;
	if (!readFile(joinPath(dir, "SAVAGE.FNT"), fraw) || !fl.load(fraw, true) || !fl.get(0, fdata) || !_font.loadWou(fdata))
		return;
	_front.assign(Framebuffer::WIDTH * Framebuffer::HEIGHT, 0);
	_back.assign(Framebuffer::WIDTH * Framebuffer::HEIGHT, 0);
	_ok = true;
	showMenu(true);
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------------
// The machine: pictures, palettes, the text window

int SeFrontEnd::load(const char *file, int item) {
	for (size_t i = 0; i < _handles.size(); ++i)
		if (_handles[i].item == item && _handles[i].file == file)
			return (int)i;
	Handle h;
	h.file = file;
	h.item = item;
	_handles.push_back(h);
	return (int)_handles.size() - 1;
}

// The pictures of the *.LZC: item n of the original is item n + 1 of the archive; it holds a size and a table of u32 (from the start of the item) that point at the shapes
const std::vector<Shape> &SeFrontEnd::shapes(int handle) {
	static const std::vector<Shape> none;
	if (handle < 0 || handle >= (int)_handles.size())
		return none;
	Handle &h = _handles[handle];
	if (!h.loaded) {
		h.loaded = true;
		Bytes raw, item;
		LibN lib;
		if (readFile(joinPath(_dir, h.file), raw) && lib.load(raw, false) && lib.get((size_t)h.item + 1, item) && item.size() > 8) {
			size_t first = get32(item, 4), n = first / 4 - 1;
			for (size_t k = 0; k < n && first <= item.size(); ++k) {
				size_t a = get32(item, 4 + 4 * k), e = k + 1 < n ? get32(item, 4 + 4 * (k + 1)) : item.size();
				Shape s;
				if (a >= e || e > item.size() || !s.load(&item[a], e - a))
					s = Shape();
				h.shapes.push_back(s);
			}
		}
	}
	return h.shapes;
}

void SeFrontEnd::draw(std::vector<uint8_t> &buf, int handle, int frame, int x, int y) {
	const std::vector<Shape> &list = shapes(handle);
	if (frame < 0 || frame >= (int)list.size())
		return;
	const Shape &s = list[frame];
	int left = x - s.hotX, top = y - s.hotY;
	for (int r = 0; r < s.height; ++r) {
		int sy = top + r;
		if (sy < 0 || sy >= Framebuffer::HEIGHT)
			continue;
		for (int c = 0; c < s.width; ++c) {
			int sx = left + c;
			uint8_t p = s.pixels[r * s.width + c];
			if (sx < 0 || sx >= Framebuffer::WIDTH || p == Shape::TRANSPARENT)
				continue;
			buf[sy * Framebuffer::WIDTH + sx] = p;
		}
	}
}

// the driver's entry that fills the rectangle a shape covers (the shadow of the creation's panel, SAVAGE.EXE's dword_20FD4)
void SeFrontEnd::shadow(std::vector<uint8_t> &buf, int handle, int frame, int x, int y, int colour) {
	const std::vector<Shape> &list = shapes(handle);
	if (frame < 0 || frame >= (int)list.size())
		return;
	const Shape &s = list[frame];
	int left = x - s.hotX, top = y - s.hotY;
	for (int r = 0; r < s.height; ++r)
		for (int c = 0; c < s.width; ++c) {
			int sx = left + c, sy = top + r;
			if (sx >= 0 && sx < Framebuffer::WIDTH && sy >= 0 && sy < Framebuffer::HEIGHT)
				buf[sy * Framebuffer::WIDTH + sx] = (uint8_t)colour;
		}
}

void SeFrontEnd::setPalette(int n) {
	_palIdx = n;
}

void SeFrontEnd::clearBuffer(std::vector<uint8_t> &buf, int colour) {
	std::fill(buf.begin(), buf.end(), (uint8_t)colour);
}

// text_reset (sub_13CE2)'s window and cursor (the rectangle is the one of the text object's bitmap: word_205E2 ...)
void SeFrontEnd::window(int l, int t, int r, int b) {
	_wl = l;
	_wt = t;
	_wr = r;
	_wb = b;
	_tx = l;
	_ty = t;
	_fg = 0;
	_bg = 0x48;
	_rows.clear();
}

void SeFrontEnd::clearWindow(int colour) {
	for (int y = _wt; y <= _wb; ++y)
		for (int x = _wl; x <= _wr; ++x)
			if (x >= 0 && x < Framebuffer::WIDTH && y >= 0 && y < Framebuffer::HEIGHT)
				_front[y * Framebuffer::WIDTH + x] = (uint8_t)colour;
	_tx = _wl;
	_ty = _wt;
	_fg = 0;
	_bg = 0x48;
	_rows.clear();
}

void SeFrontEnd::drawGlyph(unsigned char c, int x, int y, int fg, int bg) {
	int w = _font.wouWidth[c], h = _font.wouHeight;
	for (int r = 0; r < h; ++r)
		for (int b = 0; b < w; ++b) {
			int px = x + b, py = y + r;
			if (px < 0 || py < 0 || px >= Framebuffer::WIDTH || py >= Framebuffer::HEIGHT)
				continue;
			_front[py * Framebuffer::WIDTH + px] = (uint8_t)(_font.wouPixels[c][r * w + b] ? fg : bg);
		}
}

int SeFrontEnd::textWidth(const std::string &t) const {
	int w = 0;
	for (unsigned char c : t)
		w += _font.wouWidth[c];
	return w;
}

// text_newline (sub_178B9): the next line; a line that would not fit scrolls the window up (sub_17B9A)
void SeFrontEnd::newline() {
	_tx = _wl;
	int h = _font.wouHeight;
	if (_ty + h < _wb)
		_ty += h;
	else
		scroll();
}

void SeFrontEnd::scroll() {
	int h = _font.wouHeight;
	for (int row = _wt + h; row <= _wb; ++row)
		for (int x = _wl; x <= _wr; ++x)
			_front[(row - h) * Framebuffer::WIDTH + x] = _front[row * Framebuffer::WIDTH + x];
	for (int row = _wb - h + 1; row <= _wb; ++row)
		for (int x = _wl; x <= _wr; ++x)
			_front[row * Framebuffer::WIDTH + x] = (uint8_t)_bg;
	std::map<int, std::string> moved;
	for (auto &kv : _rows)
		if (kv.first - h >= _wt)
			moved[kv.first - h] = kv.second;
	_rows.swap(moved);
}

// text_put_char (sub_17856): a character: a carriage return or a line feed act, one that does not fit starts a new line, one below the window is not drawn
void SeFrontEnd::putChar(unsigned char c) {
	if (c == 0x0D) {
		_tx = _wl;
		return;
	}
	if (c == 0x0A) {
		newline();
		return;
	}
	int w = _font.wouWidth[c], h = _font.wouHeight;
	while (_tx + w - 1 > _wr)
		newline();
	if (_ty + h > _wb)
		return;
	drawGlyph(c, _tx, _ty, _fg, _bg);
	_rows[_ty] += (char)c;
	_tx += w;
	if (_tx > _wr)
		_tx = _wr;
}

// text_paragraph (sub_17575): the text of a paragraph is put line by line: a line ends at the last space that fits (a word that is too long is broken by characters), at a line feed, or at the end
void SeFrontEnd::printBuffer(const std::string &buf) {
	size_t pos = 0, n = buf.size();
	auto at = [&](size_t i) -> unsigned char { return i < n ? (unsigned char)buf[i] : 0; };
	for (;;) {
		size_t start = pos, si = start;
		if (at(si) != 0) { // the end of the first word
			for (;;) {
				unsigned char c = at(si);
				++si;
				if (c == 0 || c == 0x0A)
					break;
				if (c == ' ') {
					--si;
					break;
				}
			}
		}
		size_t brk = si;
		int cx = _tx;
		si = start;
		bool ended = false;
		for (;;) {
			unsigned char c = at(si);
			if (c == 0 || c == 0x0A) {
				brk = si + 1;
				ended = true;
				break;
			}
			if (c == ' ')
				brk = si;
			++si;
			cx += _font.wouWidth[c];
			if (cx >= _wr)
				break;
		}
		(void)ended;
		for (size_t i = start; i < brk; ++i) {
			unsigned char c = at(i);
			if (c == 0)
				break;
			putChar(c);
		}
		unsigned char last = brk > 0 ? at(brk - 1) : 0;
		pos = brk;
		if (last == 0)
			return;
		if (last != 0x0A) {
			newline();
			while (at(pos) == ' ')
				++pos;
		}
	}
}

// text_printf (sub_17B32): a printf of the text window: %s puts the argument in, %P prints what was collected (a paragraph), the rest is collected
void SeFrontEnd::say(const std::string &format, const std::string &arg) {
	_buffer.clear();
	for (size_t i = 0; i < format.size(); ++i) {
		char c = format[i];
		if (c == '%' && i + 1 < format.size()) {
			char f = format[++i];
			if (f == 's')
				_buffer += arg;
			else if (f == 'P') {
				printBuffer(_buffer);
				_buffer.clear();
			} else {
				_buffer += f;
			}
		} else {
			_buffer += c;
		}
	}
}

// text_print_direct (sub_17B10): the characters are put at once
void SeFrontEnd::printDirect(const std::string &text) {
	for (unsigned char c : text)
		putChar(c);
}

std::vector<std::string> SeFrontEnd::textLines() const {
	std::vector<std::string> out;
	for (const auto &kv : _rows)
		out.push_back(kv.second);
	return out;
}

int SeFrontEnd::random(int lo, int hi) { // random_range (sub_1D257) (a multiplicative generator: x = 65539 * x, the high word)
	if (lo > hi)
		std::swap(lo, hi);
	_rng *= 65539u;
	return (int)((_rng >> 16) % (unsigned)(hi - lo + 1)) + lo;
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------------
// The program of a scene

SeFrontEnd::Op SeFrontEnd::call(std::function<void()> fn) const {
	Op o;
	o.kind = Op::CALL;
	o.fn = fn;
	return o;
}

SeFrontEnd::Op SeFrontEnd::wait(int ms, bool interruptible) const {
	Op o;
	o.kind = Op::WAIT_MS;
	o.a = ms;
	o.interruptible = interruptible;
	return o;
}

SeFrontEnd::Op SeFrontEnd::frames(int n, bool interruptible) const {
	return wait(n * FRAME_MS, interruptible);
}

SeFrontEnd::Op SeFrontEnd::fade(bool in) const {
	Op o;
	o.kind = in ? Op::FADE_IN : Op::FADE_OUT;
	return o;
}

SeFrontEnd::Op SeFrontEnd::dissolve() const {
	Op o;
	o.kind = Op::DISSOLVE;
	return o;
}

SeFrontEnd::Op SeFrontEnd::title() const {
	Op o;
	o.kind = Op::TITLE;
	return o;
}

SeFrontEnd::Op SeFrontEnd::waitFor(std::function<bool()> cond) const {
	Op o;
	o.kind = Op::WAIT_COND;
	o.until = cond;
	o.interruptible = true;
	return o;
}

// the song `song` has reached its cue `cue` (SAVAGE.EXE's word_1FB6E >= cue)
bool SeFrontEnd::cueReached(int song, int cue) const {
	return _music && _music->current() == song && _music->marker() >= cue;
}

SeFrontEnd::Op SeFrontEnd::prompt() const {
	Op o;
	o.kind = Op::PROMPT;
	return o;
}

SeFrontEnd::Op SeFrontEnd::input(std::function<bool(int)> fn) const {
	Op o;
	o.kind = Op::INPUT;
	o.keyFn = fn;
	return o;
}

void SeFrontEnd::start(Seq seq, int scene) {
	_state = SCENE;
	_scene = scene;
	_ops.clear();
	for (Op &o : seq)
		_ops.push_back(o);
	_fade = 0;
	_black = false;
	_blink = false;
	_blinkClock = 0;
	_typing = false;
	run();
}

void SeFrontEnd::pushFront(Seq seq) {
	for (size_t i = seq.size(); i-- > 0;)
		_ops.push_front(seq[i]);
}

void SeFrontEnd::pushBack(Op op) {
	_ops.push_back(op);
}

// The colours that the picture uses fade to or from black in as many steps as the greatest of their values, one step for each retrace (sub_18CA6 in, fade_out (sub_18964) out;
// the same accumulating rule as Martian Dreams' sub_1FF0B moves each component)
void SeFrontEnd::startFade(bool in) {
	if (in)
		_front = _back;
	bool used[256] = {};
	for (uint8_t p : _front)
		used[p] = true;
	int most = 0;
	for (int c = 0; c < 256; ++c)
		if (used[c])
			for (int k = 0; k < 3; ++k)
				most = std::max(most, (int)_pal[_palIdx][c][k]);
	_fade = in ? 1 : -1;
	_black = false;
	_fadeSteps = most;
	_fadeTotal = _fadeLeft = ((most + (in ? 0 : 1)) * 14268) / 1000;
	if (_fadeLeft <= 0) {
		_fadeLeft = 1;
		_fadeTotal = 1;
	}
}

// The driver's dissolve (TM.DRV's entry at 0x4BF6, SAVAGE.EXE's dword_20F9E): the picture of the back screen is copied to the front one pixel by pixel in the order of a 17-bit shift register
// (a Galois register with the taps 0x12000, started at 0x1FFFF; the low 9 bits are x, the 8 above them y, those outside the 320 x 200 are passed over), so that it seems to condense out of noise
void SeFrontEnd::startDissolve() {
	if (_dissolveOrder.empty()) {
		uint32_t state = 0x1FFFF;
		for (int n = 0; n < Framebuffer::WIDTH * Framebuffer::HEIGHT - 1;) {
			state = (state >> 1) ^ ((state & 1) ? 0x12000u : 0u);
			uint32_t x = state & 0x1FF, y = state >> 9;
			if (x >= (uint32_t)Framebuffer::WIDTH || y >= (uint32_t)Framebuffer::HEIGHT)
				continue;
			_dissolveOrder.push_back(y * Framebuffer::WIDTH + x);
			++n;
		}
		_dissolveOrder.push_back(0); // the last pixel is the corner
	}
	_dissolveDone = 0;
	_dissolveLeft = DISSOLVE_MS;
}

void SeFrontEnd::stepDissolve(int ms) {
	_dissolveLeft -= ms;
	int upto = _dissolveLeft <= 0 ? (int)_dissolveOrder.size() : (int)((long long)_dissolveOrder.size() * (DISSOLVE_MS - _dissolveLeft) / DISSOLVE_MS);
	for (; _dissolveDone < upto; ++_dissolveDone)
		_front[_dissolveOrder[_dissolveDone]] = _back[_dissolveOrder[_dissolveDone]];
}

void SeFrontEnd::run() {
	while (_state == SCENE && !_ops.empty()) {
		Op &o = _ops.front();
		switch (o.kind) {
		case Op::CALL: {
			std::function<void()> fn = o.fn;
			_ops.pop_front();
			if (fn)
				fn();
			continue;
		}
		case Op::WAIT_MS:
		case Op::WAIT_FRAMES:
			if (o.a > 0 && !(o.until && o.until()))
				return;
			_ops.pop_front();
			continue;
		case Op::FADE_IN:
		case Op::FADE_OUT:
			if (_fade == 0 && o.a == 0) {
				o.a = 1;
				startFade(o.kind == Op::FADE_IN);
				return;
			}
			if (_fade != 0)
				return;
			_ops.pop_front();
			continue;
		case Op::DISSOLVE:
			if (o.a == 0) {
				o.a = 1;
				startDissolve();
			}
			if (_dissolveLeft > 0)
				return;
			_ops.pop_front();
			continue;
		case Op::WAIT_COND:
			if (o.until && !o.until())
				return;
			_ops.pop_front();
			continue;
		case Op::TITLE:
			if (o.a == 0) {
				o.a = 1;
				_title.reset(new SeTitle(_dir, _music != nullptr, _seed));
				if (!_title->ok())
					_title.reset();
			}
			if (_title && !_title->finished())
				return;
			_title.reset();
			_ops.pop_front();
			continue;
		case Op::PROMPT:
		case Op::INPUT:
			return;
		}
	}
}

void SeFrontEnd::tick(unsigned ms) {
	if (_state != SCENE)
		return;
	double left = ms;
	_blinkClock += ms;
	while (left > 0 && _state == SCENE && !_ops.empty()) {
		Op &o = _ops.front();
		if (o.kind == Op::WAIT_MS || o.kind == Op::WAIT_FRAMES) {
			int step = (int)std::min<double>(o.a, left);
			if (step <= 0)
				step = 1;
			o.a -= step;
			left -= step;
		} else if (o.kind == Op::WAIT_COND) {
			left = 0;
		} else if (o.kind == Op::TITLE && _title) {
			if (_music && _music->current() == 0x13)
				_title->cue(_music->marker());
			_title->tick((unsigned)left);
			if (_music) {
				if (_title->takeMusicStart())
					_music->play(0x13); // the title's song (sub_1A83F)
				int stop = _title->takeMusicStop();
				if (stop >= 0)
					_music->stop(stop);
			}
			left = 0;
		} else if (o.kind == Op::DISSOLVE && _dissolveLeft > 0) {
			int step = (int)std::min<double>(_dissolveLeft, left);
			if (step <= 0)
				step = 1;
			stepDissolve(step);
			left -= step;
		} else if ((o.kind == Op::FADE_IN || o.kind == Op::FADE_OUT) && _fade != 0) {
			int step = (int)std::min<double>(_fadeLeft, left);
			if (step <= 0)
				step = 1;
			_fadeLeft -= step;
			left -= step;
			if (_fadeLeft <= 0) {
				if (_fade < 0)
					_black = true;
				_fade = 0;
			}
		} else {
			left = 0;
		}
		run();
	}
	if (_state == SCENE && !_ops.empty())
		run();
}

// A key: it goes to the operation that waits for one
bool SeFrontEnd::handleKey(int key) {
	if (key == KEY_QUIT) {
		_state = QUIT;
		return false;
	}
	if (_state == MENU) {
		int count = choices();
		int choice = -1;
		if (key == KEY_DOWN) {
			_selected = (_selected + 1) % count;
			menuDraw(_front, _selected, false);
		} else if (key == KEY_UP) {
			_selected = (_selected + count - 1) % count;
			menuDraw(_front, _selected, false);
		} else if (key == KEY_ENTER || key == KEY_SPACE) {
			choice = _selected;
		} else if (key == 'c' || key == 'C') {
			choice = 1;
		} else if (key == 's' || key == 'S' || key == 't' || key == 'T') {
			choice = 0;
		} else if (key == 'a' || key == 'A') {
			choice = 2;
		} else if ((key == 'j' || key == 'J') && count == 4) {
			choice = 3;
		} else if (key == 0x11) { // control-Q
			_state = QUIT;
			return false;
		}
		if (choice >= 0)
			menuChoose(choice);
		return _state != QUIT && _state != DONE;
	}
	if (_state != SCENE)
		return false;
	if (_ops.empty())
		return true;
	Op &o = _ops.front();
	switch (o.kind) {
	case Op::PROMPT:
		_lastKey = key;
		_ops.pop_front();
		run();
		break;
	case Op::INPUT: {
		Op op = o;
		_ops.pop_front();
		if (!op.keyFn(key))
			_ops.push_front(op);
		run();
		break;
	}
	case Op::TITLE:
		if (_title)
			_title->key(); // a key ends the title and the opening goes on (SAVAGE.EXE's main)
		break;
	case Op::WAIT_COND:
	case Op::WAIT_MS:
	case Op::WAIT_FRAMES:
		if (o.interruptible)
			abort();
		break;
	default:
		break;
	}
	return _state != QUIT;
}

// abort_to_menu (sub_13BFF)(0): a key ends a scene that goes by itself: the picture fades out and the menu is begun again
void SeFrontEnd::abort() {
	_ops.clear();
	if (_music)
		_music->stop(200); // abort_to_menu (sub_13BFF): sub_1A2B8(200), the song fades out
	quitToMenu(false);
}

// the menu as menu_loop (sub_14A37) begins it: `first` (its argument 1) leaves the screen and the palette as they are and puts the menu on at once; else it clears, sets palette 0 and fades in
void SeFrontEnd::quitToMenu(bool first) {
	Seq s;
	if (!first) {
		s.push_back(fade(false));
		s.push_back(call([this]() {
			clearBuffer(_front, 0);
			_black = false;
		}));
	}
	s.push_back(call([this]() {
		clearBuffer(_front, 0);
		_fade = 0;
		_black = false;
		setPalette(0);
		_selected = 0;
		clearBuffer(_back, 0);
		menuDraw(_back, _selected, false);
	}));
	s.push_back(fade(true));
	s.push_back(call([this]() {
		_state = MENU;
		_scene = -1;
	}));
	start(s, -1);
}

void SeFrontEnd::setSaveState(bool characterMade, bool inProgress) {
	_characterMade = characterMade;
	_inProgress = inProgress;
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------------
// The menu

void SeFrontEnd::menuDraw(std::vector<uint8_t> &buf, int selected, bool pressed) {
	if (_menuHandle < 0)
		_menuHandle = load("CREATE.LZC", 0);
	draw(buf, _menuHandle, 0, 0, 0);
	for (int i = 0; i < choices(); ++i) {
		int frame = SE_MENU_FRAME[i];
		if (i == selected)
			frame = pressed ? (frame - 1) / 2 + 11 : frame + 1;
		draw(buf, _menuHandle, frame, 0x55, SE_MENU_Y[i]);
	}
}

void SeFrontEnd::showMenu(bool first) {
	_state = MENU;
	_ops.clear();
	_fade = 0;
	_black = false;
	_scene = -1;
	_selected = 0;
	if (first) {
		clearBuffer(_back, 0);
		menuDraw(_back, _selected, false);
		_front = _back;
	} else {
		quitToMenu(false);
	}
}

// the choice made: the chosen picture flashes (sub_14A04), then the scene runs (sub_14C45's cases)
void SeFrontEnd::menuChoose(int choice) {
	_selected = choice;
	menuDraw(_front, choice, true);
	Seq s;
	switch (choice) {
	case 0:
		buildIntroduction(s);
		start(s, INTRODUCTION);
		break;
	case 1:
		buildCreation(s);
		start(s, CREATION);
		break;
	case 2:
		buildAcknowledgements(s);
		start(s, ACKNOWLEDGEMENTS);
		break;
	case 3:
		_state = DONE;
		break;
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------------
// The scenes

// fade_out_clear (sub_13C84): the screen fades out, is cleared and the palette set again
#define FADE_OUT_CLEAR(S)                      \
	S.push_back(fade(false));                  \
	S.push_back(call([this]() {                \
		clearBuffer(_front, 0);                \
		_fade = 0;                             \
		_black = false;                        \
	}));

// the menu comes back after a scene (sub_14C45's cases 0 and 1: palette 0, the menu with the choice, faded in)
#define BACK_TO_MENU(S, SEL)                   \
	S.push_back(call([this]() {                \
		setPalette(0);                         \
		clearBuffer(_back, 0);                 \
		_selected = SEL;                       \
		menuDraw(_back, _selected, false);     \
	}));                                       \
	S.push_back(fade(true));                   \
	S.push_back(call([this]() {                \
		_state = MENU;                         \
		_scene = -1;                           \
	}));

// intro_pages (sub_15D3B): the Introduction: ten pages, each a picture (CREATE.LZC item 5, frame page + 1) at (120, 45) and its text in the box at the bottom
void SeFrontEnd::buildIntroduction(Seq &s) {
	FADE_OUT_CLEAR(s)
	auto handle = std::make_shared<int>(-1);
	s.push_back(call([this, handle]() {
		*handle = load("CREATE.LZC", 5);
		setPalette(1);
		clearBuffer(_front, 0);
		clearBuffer(_back, 0);
		draw(_back, *handle, 0, 0, 0);
		window(0x19, 0x88, 0x125, 0xB3);
	}));
	s.push_back(fade(true));
	for (int page = 0; page < 10; ++page) {
		s.push_back(call([this, handle, page]() {
			clearWindow(0x48);
			draw(_front, *handle, page + 1, 0x78, 0x2D);
			say("%s%P", SE_PAGES[page]);
		}));
		s.push_back(prompt());
		s.push_back(call([this]() {
			if (_lastKey == KEY_ESC)
				leaveScene(1);
		}));
	}
	FADE_OUT_CLEAR(s)
	BACK_TO_MENU(s, 1)
}

// acknowledgements (sub_14CE9): the Acknowledgements: thirteen pictures of CREATE.LZC item 6 one under the other scroll up over the menu's picture inside a window (x 100 to 244, y 100 to 199), four rows a frame
void SeFrontEnd::buildAcknowledgements(Seq &s) {
	auto handle = std::make_shared<int>(-1);
	auto total = std::make_shared<int>(0);
	s.push_back(call([this, handle, total]() {
		*handle = load("CREATE.LZC", 6);
		_menuHandle = load("CREATE.LZC", 0);
		clearBuffer(_back, 0);
		draw(_back, _menuHandle, 0, 0, 0);
		_front = _back;
		const std::vector<Shape> &list = shapes(*handle);
		*total = 0;
		for (const Shape &sh : list)
			*total += sh.height - 1 - sh.hotY; // sub_17530(3, ...): the bottom of a shape drawn at y 0
	}));
	// the scroll: one picture of the original's loop for each frame
	int maxFrames = 700;
	auto offset = std::make_shared<int>(0xC5);
	for (int f = 0; f < maxFrames; ++f) {
		s.push_back(call([this, handle, total, offset]() {
			if (*offset + *total <= 4)
				return;
			draw(_back, _menuHandle, 0, 0, 0);
			int di = 0;
			const std::vector<Shape> &list = shapes(*handle);
			std::vector<uint8_t> saved = _back;
			for (size_t i = 0; i < list.size(); ++i) {
				draw(_back, *handle, (int)i, 100, *offset + di);
				di += list[i].height - 1 - list[i].hotY;
			}
			// the clipping rectangle: only inside it did the shapes show
			for (int y = 0; y < Framebuffer::HEIGHT; ++y)
				for (int x = 0; x < Framebuffer::WIDTH; ++x)
					if (x < 100 || x > 244 || y < 100)
						_back[y * Framebuffer::WIDTH + x] = saved[y * Framebuffer::WIDTH + x];
			_front = _back;
			*offset -= 4;
		}));
		s.push_back(frames(1));
	}
	s.push_back(call([this]() {
		clearBuffer(_back, 0);
		menuDraw(_back, _selected, false);
	}));
	s.push_back(dissolve());
	s.push_back(call([this]() {
		if (_music)
			_music->stop(60);
		_state = MENU;
		_scene = -1;
	}));
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------------
// The creation of a character (sub_150AC)

void SeFrontEnd::buildCreation(Seq &s) {
	FADE_OUT_CLEAR(s)
	_making = SeNewCharacter();
	_making.strength = 16 + random(0, 2);
	_making.dexterity = 16 + random(0, 2);
	_making.intelligence = 16 + random(0, 2);
	_name.clear();
	s.push_back(call([this]() {
		setPalette(1);
		clearBuffer(_front, 0);
		clearBuffer(_back, 0);
		_h1 = load("CREATE.LZC", 1);
		_h2 = load("CREATE.LZC", 2);
		_h3 = load("CREATE.LZC", 3);
		_h4 = load("CREATE.LZC", 4);
		window(0xB1, 0x20, 0x132, 0xA2);
		draw(_back, _h1, 2, 0, 0);
		draw(_back, _h1, 0, 8, 0x1C);
		draw(_back, _h1, 1, 8, 0x1C);
		shadow(_back, _h4, 2, 0xAF, 0x1E, 0);
		draw(_back, _h4, 2, 0xAA, 0x19);
	}));
	s.push_back(fade(true));
	s.push_back(call([this]() { _rows.clear(); }));
	Seq body;
	if (_inProgress) {
		body.push_back(call([this]() { say(SE_CREATE_TEXT[8]); }));
		body.push_back(prompt());
		FADE_OUT_CLEAR(body)
		BACK_TO_MENU(body, 1)
		for (Op &o : body)
			s.push_back(o);
		return;
	}
	// the three pages Intanya speaks (sub_154BD), the third ends asking for the name
	static const int PAGES[3][2] = {{0, 1}, {2, 3}, {4, 5}};
	for (int p = 0; p < 3; ++p) {
		s.push_back(call([this, p]() {
			clearWindow(0x48);
			say(SE_CREATE_TEXT[PAGES[p][0]]);
			say(SE_CREATE_TEXT[PAGES[p][1]]);
		}));
		Op pr = prompt();
		s.push_back(pr);
		s.push_back(call([this]() {
			if (_lastKey == KEY_ESC)
				creationEnd();
		}));
	}
	s.push_back(call([this]() {
		clearWindow(0x48);
		say(SE_CREATE_TEXT[6]);
		beginName();
	}));
}

// the escape of a prompt: the rest of the scene is dropped and the creation is left (sub_150AC's end)
void SeFrontEnd::creationEnd() {
	clearWindow(0x48);
	leaveScene(1);
}

// the scene is left at once: the picture fades out and the menu comes back with the choice selected
void SeFrontEnd::leaveScene(int selection) {
	_ops.clear();
	Seq s;
	FADE_OUT_CLEAR(s)
	_leaveSelection = selection;
	s.push_back(call([this]() {
		setPalette(0);
		clearBuffer(_back, 0);
		_selected = _leaveSelection;
		menuDraw(_back, _selected, false);
	}));
	s.push_back(fade(true));
	s.push_back(call([this]() {
		_state = MENU;
		_scene = -1;
	}));
	pushFront(s);
	run();
}

// creation_name (sub_1539C): the name (up to 13 characters of letters, digits, space, '.', '-' and the apostrophe) is typed after the question; Enter ends it, Escape or an empty name leaves
void SeFrontEnd::beginName() {
	_name.clear();
	_typing = true;
	_nameX = _tx;
	_nameY = _ty;
	Seq s;
	s.push_back(input([this](int key) {
		if (key == KEY_ENTER) {
			_typing = false;
			if (_name.empty()) {
				creationEnd();
			} else {
				_making.name = _name;
				_nextQuestion = random(0, 2);
				_question = 0;
				askQuestion(_nextQuestion, 0);
			}
			return true;
		}
		if (key == KEY_ESC) {
			_typing = false;
			creationEnd();
			return true;
		}
		if (key == KEY_BACKSPACE || key == KEY_LEFT) {
			if (!_name.empty()) {
				// creation_name (sub_1539C): the line from the cursor to the right edge is cleared, the cursor goes back by the width of the last character
				for (int y = _nameY; y < _nameY + 8; ++y)
					for (int x = _tx; x <= _wr; ++x)
						_front[y * Framebuffer::WIDTH + x] = (uint8_t)_bg;
				_tx -= _font.wouWidth[(unsigned char)_name.back()];
				_name.pop_back();
				for (int y = _nameY; y < _nameY + 8; ++y)
					for (int x = _tx; x < _tx + 8 && x <= _wr; ++x)
						_front[y * Framebuffer::WIDTH + x] = (uint8_t)_bg;
			}
			return false;
		}
		if (key >= 0x20 && key <= 0x7A && _name.size() < 13) {
			char c = (char)key;
			bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '.' || c == '-' || c == '\'';
			if (ok) {
				// the blinking cursor is wiped, the character put where it stood
				for (int y = _nameY; y < _nameY + 8; ++y)
					for (int x = _tx; x < _tx + 8 && x <= _wr; ++x)
						_front[y * Framebuffer::WIDTH + x] = (uint8_t)_bg;
				drawGlyph((unsigned char)c, _tx, _nameY, PROMPT_FG, _bg); // (get_key_cursor (sub_14E44) draws what is typed with the colour 1: blue)
				_tx += _font.wouWidth[(unsigned char)c];
				_name += c;
			}
		}
		return false;
	}));
	pushFront(s);
}

// creation_question (sub_155A0): a question of the table, asked by the box: its lines, then A or B; the answer raises a stat and the card of it is shown
void SeFrontEnd::askQuestion(int index, int number) {
	_question = index;
	const SeQuestion &q = SE_QUESTIONS[index];
	Seq s;
	s.push_back(call([this, index]() {
		const SeQuestion &q = SE_QUESTIONS[index];
		clearWindow(0x48);
		say("%s%P", q.line[0]);
		if (q.line[1]) {
			say("\n%s%P", q.line[1]);
			if (q.line[3]) {
				_questionStage = 1;
			} else {
				say("\n%s%P", q.line[2]);
				_questionStage = 0;
			}
		} else {
			say("\n%s%P", q.line[2]);
			if (q.line[3])
				say("\n%s%P", q.line[3]);
			_questionStage = 0;
		}
	}));
	if (q.line[1] && q.line[3]) {
		s.push_back(prompt());
		s.push_back(call([this, index]() {
			const SeQuestion &q = SE_QUESTIONS[index];
			clearWindow(0x48);
			say("%s%P", q.line[2]);
			say("\n%s%P", q.line[3]);
		}));
	}
	s.push_back(call([this]() { printDirect("\n"); }));
	s.push_back(input([this, index, number](int key) {
		int k = key;
		if (k >= 'a' && k <= 'z')
			k -= 32;
		if (k == KEY_ESC) {
			creationEnd();
			return true;
		}
		if (k != 'A' && k != 'B')
			return false;
		drawGlyph((unsigned char)k, _tx, _ty, PROMPT_FG, _bg);
		const SeQuestion &q = SE_QUESTIONS[index];
		int effect = k == 'A' ? q.a : q.b;
		// creation_raise_stat (sub_157B5): the stat grows by 3 to 5; the card shown is 0 for intelligence, 2 for dexterity, 1 for strength
		int card = 0;
		if (effect == 0) {
			_making.intelligence += random(0, 2) + 3;
			card = 0;
		} else if (effect == 1) {
			_making.dexterity += random(0, 2) + 3;
			card = 2;
		} else {
			_making.strength += random(0, 2) + 3;
			card = 1;
		}
		Seq a;
		cardAnimation(a, card);
		// the next question (sub_155A0's end)
		int next = effect;
		if (effect == 0)
			next = number == 0 ? 2 : (index == 0 ? 4 : 3);
		else if (effect == 1)
			next = number == 0 ? 0 : (index == 2 ? 4 : 5);
		else if (effect == 2)
			next = number == 0 ? 1 : (index == 0 ? 5 : 3);
		if (number >= 2) {
			a.push_back(call([this]() { finishCreation(); }));
		} else {
			a.push_back(call([this, next, number]() { askQuestion(next, number + 1); }));
		}
		pushFront(a);
		return true;
	}));
	pushFront(s);
	run();
}

// creation_write_savegame (sub_15A7C): the character is put into the saved game and the menu offers Journey Onward
void SeFrontEnd::finishCreation() {
	_character = _making;
	_hasCharacter = true;
	_characterMade = true;
	_inProgress = false;
	clearWindow(0x48);
	leaveScene(1);
}

// creation_card (sub_15818): the picture of the card: Intanya's potion (CREATE.LZC item 2, six frames), the card turning (item 3, frames 0 to 16) over the stat's card (frame 17 + card), the potion again backwards
void SeFrontEnd::cardAnimation(Seq &s, int card) {
	auto compose = [this](int potion, int cardFrame, int overlay) {
		draw(_back, _h1, 0, 8, 0x1C);
		if (potion >= 0)
			draw(_back, _h2, potion, 8, 0x1C);
		if (cardFrame >= 0)
			draw(_back, _h3, cardFrame, 8, 0x1C);
		if (overlay >= 0)
			draw(_back, _h3, overlay, 8, 0x1C);
		draw(_back, _h1, 1, 8, 0x1C);
		_front = _back;
	};
	s.push_back(call([this]() { _front = _back; }));
	for (int i = 0; i < 6; ++i) {
		s.push_back(call([compose, i]() { compose(i, -1, -1); }));
		s.push_back(frames(1, false));
		s.push_back(wait(i == 0 || i == 5 ? 40 : (i == 1 || i == 4 ? 30 : 20), false));
	}
	for (int i = 0; i < 17; ++i) {
		s.push_back(call([compose, i, card]() { compose(5, card + 0x11, i); }));
		s.push_back(frames(1, false));
		s.push_back(wait(i < 5 ? 24 - 6 * i : 15, false));
	}
	for (int i = 5; i >= 0; --i) {
		s.push_back(call([compose, i]() { compose(i, -1, -1); }));
		s.push_back(frames(1, false));
		s.push_back(wait(i == 0 || i == 5 ? 35 : (i == 1 || i == 4 ? 25 : 15), false));
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------------
// The ending (sub_15E82)

void SeFrontEnd::startEnding(int days, int months, int years, char who) {
	_endDays = days;
	_endMonths = months;
	_endYears = years;
	_endWho = who;
	Seq s;
	buildEnding(s);
	start(s, ENDING);
}

void SeFrontEnd::buildEnding(Seq &s) {
	auto handle = std::make_shared<int>(-1);
	s.push_back(call([this, handle]() {
		*handle = load("ENDGAME.LZC", 0);
		clearBuffer(_front, 0);
		setPalette(1);
		clearBuffer(_back, 0);
		draw(_back, *handle, 0, 0, 0);
		window(0x19, 0x88, 0x125, 0xB3);
	}));
	s.push_back(fade(true));
	auto page = [&](int text) {
		s.push_back(call([this, text]() {
			clearWindow(0x48);
			say("%s%P", SE_END_TEXT[text]);
		}));
		s.push_back(prompt());
	};
	// a picture put over the screen at once (the original waits for a retrace and copies the back screen)
	auto picture = [&](int frame) {
		s.push_back(call([this, handle, frame]() {
			clearBuffer(_back, 0);
			draw(_back, *handle, frame, 0, 0);
			_front = _back;
		}));
	};
	page(0);
	page(1);
	page(2);
	picture(1);
	page(3);
	page(4);
	if (_endWho == 'A') {
		picture(2);
		page(5);
		page(6);
		page(7);
	} else if (_endWho == 'T') {
		picture(3);
		page(8);
		page(9);
	}
	picture(4);
	page(10);
	page(11);
	page(12);
	page(13);
	FADE_OUT_CLEAR(s)
	s.push_back(call([this, handle]() {
		setPalette(1);
		clearBuffer(_back, 0);
		draw(_back, *handle, 1, 0, 0);
		window(0x19, 0x88, 0x125, 0xB3);
	}));
	s.push_back(fade(true));
	s.push_back(call([this]() {
		clearWindow(0x48);
		say("%s%P", SE_END_TEXT[14]);
		// the time played: months, days and years as it was given (sub_15E52: "one year", "3 months")
		auto unit = [](int n, const char *name) { return n == 1 ? std::string("one ") + name : std::to_string(n) + " " + name + "s"; };
		int d = _endDays, b = _endMonths, y = _endYears;
		int si = 0;
		if (d && b && y) {
			printDirect(unit(b, "month"));
			printDirect(",\n");
			printDirect(unit(d, "day"));
			printDirect(" and ");
			printDirect(unit(y, "year"));
		} else {
			if (b) {
				printDirect(unit(b, "month"));
				si = 1;
			}
			if (d) {
				if (si == 1) {
					printDirect("\nand ");
					si = 2;
				}
				printDirect(unit(d, "day"));
				if (si < 2)
					si = 1;
			}
			if (y) {
				if (si == 1) {
					printDirect("\nand ");
					si = 2;
				}
				printDirect(unit(y, "year"));
			}
		}
		printDirect(si == 2 ? ". " : ".\n");
		say("%s%P", SE_END_TEXT[15]);
	}));
	s.push_back(prompt());
	s.push_back(call([this]() {
		_state = DONE;
		_scene = -1;
	}));
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------------
// The opening (sub_141DE)

void SeFrontEnd::startOpening() {
	Seq s;
	buildOpening(s);
	start(s, OPENING);
}

// opening_sequence (sub_141DE): "Lord British presents..." (INTRO.LZC item 0), the savannah (item 1) with the panel (item 2) that rises over it, and the scroll of the savannah's layers (each moves at its
// own speed) with the figures of item 2 that flash by. The original waits for the cues of the music where this waits by the clock (the times are estimated from a run of the original: APPROX 105).
void SeFrontEnd::buildOpening(Seq &s) {
	s.push_back(title());
	auto h = std::make_shared<std::array<int, 3>>();
	s.push_back(call([this, h]() {
		clearBuffer(_front, 0);
		clearBuffer(_back, 0);
		(*h)[0] = load("INTRO.LZC", 0);
		(*h)[1] = load("INTRO.LZC", 1);
		(*h)[2] = load("INTRO.LZC", 2);
		setPalette(0);
		draw(_back, (*h)[0], 0, 0, 0);
		if (_music)
			_music->play(0x12); // sub_1A83F: the song of the opening
	}));
	s.push_back(fade(true));
	s.push_back(call([this, h]() { draw(_back, (*h)[0], 1, 0, 0); }));
	if (_music)
		s.push_back(waitFor([this]() { return cueReached(0x12, 1); })); // (word_1FB6E >= 1)
	else
		s.push_back(wait(OPENING_WAIT1));
	FADE_OUT_CLEAR(s)
	s.push_back(call([this]() { setPalette(0); }));
	s.push_back(fade(true));
	if (_music)
		s.push_back(waitFor([this]() { return cueReached(0x12, 2); }));
	else
		s.push_back(wait(OPENING_WAIT2));
	FADE_OUT_CLEAR(s)
	// the savannah: layers 0, 2 and 5 of item 1
	s.push_back(call([this, h]() {
		setPalette(0);
		draw(_back, (*h)[1], 0, 0, 0);
		draw(_back, (*h)[1], 2, 0, 0);
		draw(_back, (*h)[1], 5, 0, 0);
	}));
	s.push_back(fade(true));
	// opening_panel (sub_14724): the panel (item 2, frame 0) rises from below (four rows a frame), the thing of item 1's frames 8 to 12 turns on and off, then a pause of seven short waits
	auto scene = [this, h](int panelY, int extra) {
		return call([this, h, panelY, extra]() {
			draw(_back, (*h)[1], 0, 0, 0);
			draw(_back, (*h)[1], 2, 0, 0);
			draw(_back, (*h)[2], 0, 0, panelY);
			draw(_back, (*h)[1], 5, 0, 0);
			if (extra >= 0)
				draw(_back, (*h)[1], extra + 8, 0, 0);
			_front = _back;
		});
	};
	for (int y = 0xC8; y >= 0; y -= 4) {
		s.push_back(scene(y, -1));
		s.push_back(frames(1));
	}
	for (int i = 0; i < 5; ++i) {
		s.push_back(scene(0, i));
		s.push_back(frames(1));
	}
	for (int i = 4; i >= 0; --i) {
		s.push_back(scene(0, i));
		s.push_back(frames(1));
	}
	for (int i = 0; i < 7; ++i) { // (each wait is skipped once the song has reached its cue 3)
		Op w = wait(300);
		w.until = [this]() { return cueReached(0x12, 3); };
		s.push_back(w);
	}
	s.push_back(call([this, h]() {
		clearBuffer(_back, 0);
		draw(_back, (*h)[1], 0, 0, 0);
		draw(_back, (*h)[1], 2, 0, 0);
		draw(_back, (*h)[1], 5, 0, 0);
	}));
	s.push_back(dissolve());
	// the scroll: 213 frames; the sky at one speed, the hills at two rows less a frame, the foreground at three; the figures of item 2 (frame |x| / 35 + 1) show for 15 of each 35
	for (int k = 0; k < 213; ++k) {
		s.push_back(call([this, h, k]() {
			int si = -k, var4 = -2 * k, di = -3 * k;
			int frame = k / 35 + 1, rest = k % 35;
			draw(_back, (*h)[1], 0, si, 0);
			draw(_back, (*h)[1], 1, si + 0x140, 0);
			draw(_back, (*h)[1], 2, var4, 0);
			draw(_back, (*h)[1], 3, var4 + 0x140, 0);
			draw(_back, (*h)[1], 4, var4 + 0x280, 0);
			draw(_back, (*h)[1], 5, di, 0);
			draw(_back, (*h)[1], 6, di + 0x140, 0);
			draw(_back, (*h)[1], 7, di + 0x280, 0);
			if (rest < 15)
				draw(_back, (*h)[2], frame, 0, 0);
			_front = _back;
		}));
		s.push_back(frames(1));
	}
	s.push_back(call([this]() {
		setPalette(0);
		clearBuffer(_back, 0);
		_selected = 0;
		menuDraw(_back, _selected, false);
	}));
	s.push_back(dissolve());
	s.push_back(call([this]() {
		_state = MENU;
		_scene = -1;
	}));
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------------

void SeFrontEnd::render(Framebuffer &screen) const {
	if (_title && _state == SCENE && !_ops.empty() && _ops.front().kind == Op::TITLE) {
		screen.setPalette(_title->palette());
		screen.clear(0);
		screen.blit(0, 0, _title->pixels(), Framebuffer::WIDTH, Framebuffer::HEIGHT, Framebuffer::WIDTH);
		return;
	}
	uint8_t pal[256][3];
	int step = 0;
	if (_fade != 0)
		step = std::min(_fadeSteps, (_fadeTotal - _fadeLeft) * 1000 / 14268);
	for (int c = 0; c < 256; ++c)
		for (int k = 0; k < 3; ++k) {
			int v = _pal[_palIdx][c][k];
			if (_black) {
				v = 0;
			} else if (_fade != 0) {
				int acc = _fadeSteps / 2, moved = 0;
				for (int t = 0; t < step; ++t) {
					acc += v;
					if (acc > _fadeSteps) {
						acc -= _fadeSteps;
						++moved;
					}
				}
				v = _fade > 0 ? moved : v - moved;
			}
			pal[c][k] = (uint8_t)v;
		}
	screen.setPalette(pal);
	screen.clear(0);
	screen.blit(0, 0, &_front[0], Framebuffer::WIDTH, Framebuffer::HEIGHT, Framebuffer::WIDTH);
	// the blinking arrow of wait_key_prompt (sub_14FB6) (every 100 ms) and the underscore of the name
	bool on = ((long long)(_blinkClock / 100) & 1) == 0;
	if (_state == SCENE && !_ops.empty() && on) {
		auto glyph = [&](unsigned char c, int x, int y, int fg) {
			int w = _font.wouWidth[c], h = _font.wouHeight;
			for (int r = 0; r < h; ++r)
				for (int b = 0; b < w; ++b) {
					int px = x + b, py = y + r;
					if (px >= 0 && py >= 0 && px < Framebuffer::WIDTH && py < Framebuffer::HEIGHT)
						screen.pixels()[py * Framebuffer::WIDTH + px] = (uint8_t)(_font.wouPixels[c][r * w + b] ? fg : _bg);
				}
		};
		if (_ops.front().kind == Op::PROMPT)
			glyph(']', _tx, _ty, PROMPT_FG);
		else if (_ops.front().kind == Op::INPUT)
			glyph('_', _tx, _ty, PROMPT_FG);
	}
}

} // namespace Ultima6
} // namespace Ultima6
