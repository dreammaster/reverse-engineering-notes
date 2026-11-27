#include "ultima6/se_title.h"

#include "files/lib_n.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

namespace Ultima6 {
namespace Ultima6 {

namespace {

// where the pieces lie in the megabyte of the interpreter
const uint16_t IMAGE_SEG = 0x1000; // SAVAGE.EXE as IDA loads it (the listing's addresses are the linear ones)
const uint16_t DS_SEG = 0x1DAA;    // dseg
const uint16_t RETURN_SEG = 0x3E00;
const uint16_t STUB_SEG = 0x3F00; // the resource locator the drivers call
const uint16_t TM_SEG = 0x4000;
const uint16_t SUPER_SEG = 0x5000;
const uint16_t FRONT_SEG = 0x6000; // the screen (the VGA's 0xA000)
const uint16_t BACK_SEG = 0x7000;
const uint16_t FIRST_ITEM_SEG = 0x8000;
const uint16_t ROWS_FRONT = 0x5000, ROWS_BACK = 0x5200; // in the data segment: the offset of each row of a screen
const uint16_t HEAP = 0x6000;
const uint32_t DATA_FLAT = 0x1DAA0; // dseg's linear address: a variable of the listing named word_XXXXX is at ds:XXXXX - DATA_FLAT
const double RETRACE_MS = 14.268;
const double COUNTS_PER_MS = 2386.364; // the timer chip counts down by two at 1.193 MHz

inline uint16_t ds(uint32_t flat) {
	return (uint16_t)(flat - DATA_FLAT);
}

// the far pointers the drivers' tables fill (sub_1D46C, sub_1D60A): the table's word n goes to the variable, its segment behind it
const uint32_t TM_VARS[26] = {0, 0x1FAC8, 0x1FAC0, 0x1FAC4, 0x20676, 0x20624, 0x20628, 0x20666, 0x20FD0, 0x205DA, 0x20FD4, 0x2065A, 0x20FB6, 0x2062C, 0x205D4, 0x20602, 0x20FDA, 0x20FCC,
							  0x20630, 0x20F8E, 0x20648, 0x20652, 0x20F9E, 0, 0, 0x2065E};
const uint32_t SUPER_VARS[6] = {0, 0x2067C, 0x20644, 0x2060A, 0x205D0, 0x20634};

enum Routine {
	LOAD_ITEM = 1,
	FREE,
	MUSIC_START,
	MUSIC_STOP,
	KEY_AVAILABLE,
	WAIT_RETRACE,
	DELAY_COUNTS,
	SPEED,
	SOUND,
	SOUND_FREE,
	CALLOC,
	FREE_HEAP
};

} // namespace

SeTitle::SeTitle(const std::string &dir, bool music, uint32_t seed) : _dir(dir), _music(music), _seed(seed), _cpu(new Cpu8086()), _nextSeg(FIRST_ITEM_SEG), _heap(HEAP) {
	std::memset(_dac, 0, sizeof _dac);
	_ok = loadExe();
	if (_ok)
		setup();
}

uint16_t SeTitle::dsw(uint16_t off) const {
	return _cpu->read16(DS_SEG, off);
}

uint8_t SeTitle::dsb(uint16_t off) const {
	return _cpu->read8(DS_SEG, off);
}

void SeTitle::setDsw(uint16_t off, uint16_t v) {
	_cpu->write16(DS_SEG, off, v);
}

bool SeTitle::loadExe() {
	Bytes exe;
	if (!readFile(joinPath(_dir, "SAVAGE.EXE"), exe) || exe.size() < 0x40 || exe[0] != 'M' || exe[1] != 'Z')
		return false;
	auto w = [&](size_t o) { return (uint32_t)(exe[o] | (exe[o + 1] << 8)); };
	size_t header = w(8) * 16, relocs = w(6), table = w(0x18);
	size_t total = (w(4) - 1) * 512 + w(2);
	if (w(2) == 0)
		total = w(4) * 512;
	if (header >= exe.size() || total > exe.size() || total <= header)
		return false;
	size_t size = total - header;
	Cpu8086 &c = *_cpu;
	if (Cpu8086::linear(IMAGE_SEG, 0) + size > 0x3E000)
		return false;
	std::memcpy(c.memory() + Cpu8086::linear(IMAGE_SEG, 0), &exe[header], size);
	for (size_t i = 0; i < relocs; ++i) {
		size_t at = table + 4 * i;
		if (at + 4 > exe.size())
			return false;
		uint16_t off = (uint16_t)w(at), seg = (uint16_t)w(at + 2);
		uint16_t v = c.read16((uint16_t)(seg + IMAGE_SEG), off);
		c.write16((uint16_t)(seg + IMAGE_SEG), off, (uint16_t)(v + IMAGE_SEG));
	}
	return true;
}

// a driver (TM.DRV, SUPERTM.DRV): the block for the VGA (item 0) goes to its segment, its table of entries goes into the variables, and the resource locator is put where the game puts it
bool SeTitle::loadDriver(const char *name, uint16_t seg, const uint16_t *) {
	Bytes raw, block;
	LibN lib;
	if (!readFile(joinPath(_dir, name), raw) || !lib.load(raw, true) || !lib.get(0, block) || block.size() < 0x40 || block.size() > 0xFFF0)
		return false;
	std::memcpy(_cpu->memory() + Cpu8086::linear(seg, 0), block.data(), block.size());
	return true;
}

void SeTitle::setup() {
	Cpu8086 &c = *_cpu;
	uint8_t *m = c.memory();
	// the program's data: the file's, and zero from ds:2814 to ds:4344 (start does that)
	std::memset(m + Cpu8086::linear(DS_SEG, 0x2814), 0, 0x4344 - 0x2814);
	if (!loadDriver("TM.DRV", TM_SEG, nullptr) || !loadDriver("SUPERTM.DRV", SUPER_SEG, nullptr)) {
		_ok = false;
		return;
	}
	for (int n = 1; n < 26; ++n)
		if (TM_VARS[n]) {
			setDsw(ds(TM_VARS[n]), c.read16(TM_SEG, (uint16_t)(2 * n)));
			setDsw((uint16_t)(ds(TM_VARS[n]) + 2), TM_SEG);
		}
	for (int n = 1; n < 6; ++n) {
		setDsw(ds(SUPER_VARS[n]), c.read16(SUPER_SEG, (uint16_t)(2 * n)));
		setDsw((uint16_t)(ds(SUPER_VARS[n]) + 2), SUPER_SEG);
	}
	// the locator: a handle is the address of the item, as it is in memory
	const uint8_t stub[] = {0x55, 0x8B, 0xEC, 0x8B, 0x46, 0x06, 0x8B, 0x56, 0x08, 0x5D, 0xCB}; // push bp / mov bp, sp / mov ax, [bp+6] / mov dx, [bp+8] / pop bp / retf
	std::memcpy(m + Cpu8086::linear(STUB_SEG, 0), stub, sizeof stub);
	// the sound driver's entries (dword_2063E, the sound effects, and the rest of its table) do nothing here: a retf
	m[Cpu8086::linear(STUB_SEG, 0x20)] = 0xCB;
	for (uint32_t v : {0x20F96, 0x20F9A, 0x202B4, 0x2060E, 0x2064E, 0x20672, 0x20680, 0x20F84, 0x205CA}) {
		setDsw(ds(v), 0x20);
		setDsw((uint16_t)(ds(v) + 2), STUB_SEG);
	}
	// dword_2063E (a sound effect: volume, note, timbre) is a routine that takes its three arguments off the stack: retf 6
	m[Cpu8086::linear(STUB_SEG, 0x30)] = 0xCA;
	m[Cpu8086::linear(STUB_SEG, 0x31)] = 0x06;
	m[Cpu8086::linear(STUB_SEG, 0x32)] = 0x00;
	setDsw(ds(0x2063E), 0x30);
	setDsw((uint16_t)(ds(0x2063E) + 2), STUB_SEG);
	c.write16(TM_SEG, 0x2E, 0);
	c.write16(TM_SEG, 0x30, STUB_SEG);
	c.write16(SUPER_SEG, 0xC, 0);
	c.write16(SUPER_SEG, 0xE, STUB_SEG);
	// the screens: the front (ds:2B1A) and the back (ds:2B52): the segment of the pixels, the table of the rows' offsets, the clipping rectangle
	struct Screen {
		uint16_t at, seg, rows;
	} screens[2] = {{0x2B1A, FRONT_SEG, ROWS_FRONT}, {0x2B52, BACK_SEG, ROWS_BACK}};
	for (const Screen &s : screens) {
		for (int y = 0; y <= 200; ++y)
			setDsw((uint16_t)(s.rows + 2 * y), (uint16_t)(y * 320));
		setDsw(s.at + 0, s.seg);
		setDsw(s.at + 2, s.rows);
		setDsw(s.at + 4, 0);
		setDsw(s.at + 6, 0);
		setDsw(s.at + 8, 319);
		setDsw(s.at + 0xA, 199);
	}
	// what main has set when it calls the title
	setDsw(ds(0x1FAEA), 0x13); // the video mode: VGA
	setDsw(ds(0x20608), 0);
	setDsw(ds(0x20FDE), _music ? 1 : 0);
	setDsw(ds(0x1FB6A), _music ? 2 : 0);
	setDsw(ds(0x20642), 2);
	setDsw(ds(0x1FF26), (uint16_t)((_seed * 2654435761u) | 1)); // the random generator's seed (random_seed)
	// the colours: palette 2 of SAVAGE.PAL (set_palette 2 is what main does first)
	Bytes praw, pal;
	LibN pl;
	if (readFile(joinPath(_dir, "SAVAGE.PAL"), praw) && pl.load(praw, true) && pl.get(0, pal) && pal.size() >= 3 * 768)
		for (int i = 0; i < 256; ++i)
			for (int k = 0; k < 3; ++k)
				_dac[i][k] = (uint8_t)(pal[2 * 768 + i * 3 + k] >> 2);
	c.onOut = [this](uint16_t port, uint16_t value, bool wide) {
		auto one = [&](uint16_t p, uint8_t v) {
			if (p == 0x3C7) {
				_dacRead = v;
				_dacReadPhase = 0;
			} else if (p == 0x3C8) {
				_dacWrite = v;
				_dacPhase = 0;
			} else if (p == 0x3C9) {
				_dac[_dacWrite & 255][_dacPhase] = v & 63;
				if (++_dacPhase == 3) {
					_dacPhase = 0;
					++_dacWrite;
				}
			}
		};
		one(port, (uint8_t)value);
		if (wide)
			one((uint16_t)(port + 1), (uint8_t)(value >> 8));
	};
	c.onIn = [this](uint16_t port, bool) -> uint16_t {
		if (port == 0x3C9) {
			uint8_t v = _dac[_dacRead & 255][_dacReadPhase];
			if (++_dacReadPhase == 3) {
				_dacReadPhase = 0;
				++_dacRead;
			}
			return v;
		}
		return 0;
	};
	// the routines the host does (their addresses are the listing's)
	_hle[0x1389B] = LOAD_ITEM;
	_hle[0x1BD76] = FREE;
	_hle[0x1A83F] = MUSIC_START;
	_hle[0x1A2B8] = MUSIC_STOP;
	_hle[0x11BFD] = KEY_AVAILABLE;
	_hle[0x17D78] = WAIT_RETRACE;
	_hle[0x1CFD6] = DELAY_COUNTS;
	_hle[0x1CA2E] = SPEED;
	_hle[0x1931A] = SOUND;
	_hle[0x196E7] = SOUND_FREE;
	_hle[0x1026F] = CALLOC;
	_hle[0x131F0] = FREE_HEAP;
	c.onStep = [this](const Cpu8086 &cpu) {
		uint32_t lin = Cpu8086::linear(cpu.cs, cpu.ip);
		auto it = _hle.find(lin);
		if (it != _hle.end())
			hle(const_cast<Cpu8086 &>(cpu));
	};
	// the call of title_sequence as main makes it: the music's handle (a token: the host plays the song), the video mode, the front and the back screen; it returns to the sentinel
	uint32_t target = 0x16FCD;
	uint16_t cs = 0, ip = 0;
	for (uint32_t a = 0x10000; a < 0x22000 && !cs; ++a) {
		if (m[a] != 0x9A)
			continue;
		uint16_t off = (uint16_t)(m[a + 1] | (m[a + 2] << 8)), seg = (uint16_t)(m[a + 3] | (m[a + 4] << 8));
		if ((((uint32_t)seg << 4) + off) == target) {
			cs = seg;
			ip = off;
		}
	}
	if (!cs) { // main calls it through `push cs; call` or the far call is a different form: the routine's segment is the one of the call's neighbours
		cs = (uint16_t)(target >> 4);
		ip = (uint16_t)(target & 15);
	}
	c.ss = DS_SEG;
	c.ds = DS_SEG;
	c.es = DS_SEG;
	c.sp = 0xFFF0;
	c.push16(_music ? 0 : 0);
	c.push16(_music ? 1 : 0);
	c.push16(0);
	c.push16(0x2B1A);
	c.push16(0x2B52);
	c.push16(RETURN_SEG);
	c.push16(0);
	c.cs = cs;
	c.ip = ip;
	_wakeAt = 0;
}

uint16_t SeTitle::arg(Cpu8086 &c, int n) const {
	return c.read16(c.ss, (uint16_t)(c.sp + 4 + 2 * n));
}

// the return of a far routine whose caller takes its arguments off: ip and cs come off the stack
void SeTitle::ret(Cpu8086 &c, uint16_t ax, uint16_t dx) {
	c.ax = ax;
	c.dx = dx;
	c.ip = c.pop16();
	c.cs = c.pop16();
}

int SeTitle::itemSegment(const std::string &file, int item) {
	std::string key = file + ":" + std::to_string(item);
	auto it = _items.find(key);
	if (it != _items.end())
		return it->second;
	Bytes raw, data;
	LibN lib;
	std::string name = file;
	for (char &ch : name)
		ch = (char)std::toupper((unsigned char)ch);
	if (!readFile(joinPath(_dir, name), raw) || !lib.load(raw, false) || !lib.get((size_t)item + 1, data) || data.size() > 0xFFF0 || (uint32_t)_nextSeg + (data.size() >> 4) + 2 > 0xF000)
		return 0;
	int seg = _nextSeg;
	std::memcpy(_cpu->memory() + Cpu8086::linear((uint16_t)seg, 0), data.data(), data.size());
	_nextSeg = (uint16_t)(_nextSeg + (data.size() >> 4) + 2);
	_items[key] = seg;
	return seg;
}

void SeTitle::hle(Cpu8086 &c) {
	int r = _hle[Cpu8086::linear(c.cs, c.ip)];
	switch (r) {
	case LOAD_ITEM: { // load_lzc_item(name, item): the handle (segment : offset 0) of the item, 0 when it is not there
		std::string name;
		for (uint16_t o = arg(c, 0); dsb(o); ++o)
			name += (char)dsb(o);
		int seg = itemSegment(name, (int16_t)arg(c, 1));
		ret(c, 0, (uint16_t)seg);
		break;
	}
	case FREE:
	case SOUND_FREE:
	case FREE_HEAP:
		ret(c, 0);
		break;
	case MUSIC_START:
		_musicStart = true;
		ret(c, 0);
		break;
	case MUSIC_STOP:
		_musicStop = (int16_t)arg(c, 0);
		ret(c, 0);
		break;
	case KEY_AVAILABLE:
		ret(c, _key ? 0xFFFF : 0);
		if (_music && !_key) { // a wait for the song's cue polls the keyboard without end: each look takes a little of the clock, so that the time can reach the cue
			_wakeAt += 0.1;
			c.stopRequested = true;
		}
		break;
	case WAIT_RETRACE:
		ret(c, 0);
		_wakeAt += RETRACE_MS;
		c.stopRequested = true;
		break;
	case DELAY_COUNTS: { // (low, high) counts of the timer chip
		uint32_t counts = (uint32_t)arg(c, 0) | ((uint32_t)arg(c, 1) << 16);
		ret(c, 0);
		_wakeAt += counts / COUNTS_PER_MS;
		c.stopRequested = true;
		break;
	}
	case SPEED: // the machine's speed measure: a fast one
		ret(c, 64);
		break;
	case SOUND:
		ret(c, 0, 0);
		break;
	case CALLOC: { // calloc(n, size) of the near heap
		uint32_t bytes = (uint32_t)arg(c, 0) * arg(c, 1);
		uint16_t at = _heap;
		if ((uint32_t)_heap + bytes > 0xE000) {
			ret(c, 0);
			break;
		}
		std::memset(c.memory() + Cpu8086::linear(DS_SEG, at), 0, bytes);
		_heap = (uint16_t)(_heap + ((bytes + 1) & ~1u));
		ret(c, at);
		break;
	}
	}
}

void SeTitle::cue(int n) {
	_cues = std::max(_cues, n);
	setDsw(ds(0x1FB6E), (uint16_t)_cues);
}

bool SeTitle::takeMusicStart() {
	bool b = _musicStart;
	_musicStart = false;
	return b;
}

int SeTitle::takeMusicStop() {
	int n = _musicStop;
	_musicStop = -1;
	return n;
}

const uint8_t *SeTitle::pixels() const {
	return _cpu->memory() + Cpu8086::linear(FRONT_SEG, 0);
}

// the code runs until it waits (wait_retrace, delay_counts) or ends
void SeTitle::runSlice() {
	Cpu8086 &c = *_cpu;
	bool ok = c.run(Cpu8086::linear(RETURN_SEG, 0), 50000000);
	_steps += c.steps();
	if (!ok) {
		_error = c.error() ? c.error() : "stopped";
		char buf[64];
		std::snprintf(buf, sizeof buf, " at %04X:%04X", c.cs, c.ip);
		_error += buf;
		_finished = true;
	} else if (Cpu8086::linear(c.cs, c.ip) == Cpu8086::linear(RETURN_SEG, 0)) {
		_finished = true;
	}
}

void SeTitle::tick(unsigned ms) {
	if (_finished || !_ok)
		return;
	_clock += ms;
	while (!_finished && _clock >= _wakeAt)
		runSlice();
}

} // namespace Ultima6
} // namespace Ultima6
