// A small PC under the lifted program: the software interrupts (DOS 21h, video 10h, keyboard 16h, mouse 33h, EMS 67h, timer 1Ah) and the I/O ports
// (PIT, PIC, VGA DAC / status, AdLib, ...) that the original code talks to.  Files come from the game folder (read-only, case-insensitive) plus an in-memory
// store for files the game creates (save games); input comes from queues the tests fill; the VGA palette written through the DAC ports is captured.
#include "host.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>

namespace {

Host &H() { return host(); }

std::string lower(std::string s) {
	for (auto &ch : s)
		ch = (char)tolower((unsigned char)ch);
	return s;
}

std::string baseName(const std::string &path) {
	size_t p = path.find_last_of("/\\:");
	return lower(p == std::string::npos ? path : path.substr(p + 1));
}

void setCF(Cpu &c, bool v) { c.CF = v; }

}   // namespace

// --- files --------------------------------------------------------------------------------------------------------------------------------------
void Machine::scanGameDir() {
	gameFiles.clear();
	DIR *d = opendir(gameDir.c_str());
	if (!d)
		return;
	while (dirent *e = readdir(d))
		gameFiles[lower(e->d_name)] = e->d_name;
	closedir(d);
}

bool Machine::load(const std::string &name, std::vector<uint8_t> &out) {
	std::string b = baseName(name);
	auto v = vfs.find(b);
	if (v != vfs.end()) {
		out = v->second;
		return true;
	}
	auto g = gameFiles.find(b);
	if (g == gameFiles.end())
		return false;
	try {
		out = Legend::readFile(gameDir + "/" + g->second);
	} catch (...) {
		return false;
	}
	return true;
}

int Machine::openFile(const std::string &name, bool create, bool writable) {
	OpenFile f;
	f.name = baseName(name);
	f.writable = writable || create;
	if (create) {
		vfs[f.name].clear();
	} else {
		std::string b = f.name;
		if (!vfs.count(b)) {
			// large game files (the 190 MB voice file) are read on demand, everything else is cached
			auto g = gameFiles.find(b);
			if (g == gameFiles.end())
				return -1;
			f.path = gameDir + "/" + g->second;
		}
	}
	openLog.push_back(f.name);
	int h = nextHandle++;
	if (!f.path.empty()) {
		f.fp = fopen(f.path.c_str(), "rb");
		if (!f.fp)
			return -1;
	}
	files[h] = f;
	return h;
}

int Machine::readFile(int h, uint8_t *buf, int count) {
	auto it = files.find(h);
	if (it == files.end())
		return -1;
	OpenFile &f = it->second;
	if (f.fp) {
		fseek(f.fp, (long)f.pos, SEEK_SET);
		size_t n = fread(buf, 1, (size_t)count, f.fp);
		f.pos += n;
		return (int)n;
	}
	std::vector<uint8_t> &data = vfs[f.name];
	if (f.pos >= data.size())
		return 0;
	size_t n = std::min<size_t>((size_t)count, data.size() - f.pos);
	memcpy(buf, data.data() + f.pos, n);
	f.pos += n;
	return (int)n;
}

int Machine::writeFile(int h, const uint8_t *buf, int count) {
	if (h == 1 || h == 2) {
		stdoutText.append((const char *)buf, (size_t)count);
		return count;
	}
	auto it = files.find(h);
	if (it == files.end() || !it->second.writable)
		return -1;
	OpenFile &f = it->second;
	std::vector<uint8_t> &data = vfs[f.name];
	if (f.pos + count > data.size())
		data.resize(f.pos + count);
	memcpy(data.data() + f.pos, buf, (size_t)count);
	f.pos += (size_t)count;
	return count;
}

long Machine::seekFile(int h, long off, int whence) {
	auto it = files.find(h);
	if (it == files.end())
		return -1;
	OpenFile &f = it->second;
	long size = 0;
	if (f.fp) {
		fseek(f.fp, 0, SEEK_END);
		size = ftell(f.fp);
	} else {
		size = (long)vfs[f.name].size();
	}
	long np = whence == 0 ? off : whence == 1 ? (long)f.pos + off : size + off;
	if (np < 0)
		return -1;
	f.pos = (size_t)np;
	return np;
}

void Machine::closeFile(int h) {
	auto it = files.find(h);
	if (it == files.end())
		return;
	if (it->second.fp)
		fclose(it->second.fp);
	files.erase(it);
}

// --- DOS memory blocks (int 21h 48h / 49h / 4Ah) -------------------------------------------------------------------------------------------
void Machine::initArena() {
	blocks.clear();
	blocks.push_back({0x9500, 0xB00, false});     // free paragraphs behind the data segment's stack
	blocks.push_back({0xB000, 0x3000, false});    // video text memory / option ROM area: unused here, handed out as extra conventional memory
}

int Machine::allocBlock(int paras) {
	for (auto &b : blocks)
		if (!b.used && b.paras >= paras) {
			if (b.paras > paras)
				blocks.insert(blocks.begin() + (&b - &blocks[0]) + 1, Block{(uint16_t)(b.seg + paras), (uint16_t)(b.paras - paras), false});
			b.paras = (uint16_t)paras;
			b.used = true;
			return b.seg;
		}
	return -1;
}

int Machine::largestFree() const {
	int best = 0;
	for (auto &b : blocks)
		if (!b.used)
			best = std::max<int>(best, b.paras);
	return best;
}

bool Machine::freeBlock(int seg) {
	for (size_t i = 0; i < blocks.size(); i++)
		if (blocks[i].used && blocks[i].seg == seg) {
			blocks[i].used = false;
			// merge neighbours
			for (size_t k = 0; k + 1 < blocks.size();) {
				if (!blocks[k].used && !blocks[k + 1].used && blocks[k].seg + blocks[k].paras == blocks[k + 1].seg) {
					blocks[k].paras = (uint16_t)(blocks[k].paras + blocks[k + 1].paras);
					blocks.erase(blocks.begin() + k + 1);
				} else {
					k++;
				}
			}
			return true;
		}
	return false;
}

bool Machine::resizeBlock(int seg, int paras) {
	for (size_t i = 0; i < blocks.size(); i++)
		if (blocks[i].used && blocks[i].seg == seg) {
			if (paras <= blocks[i].paras) {
				if (paras < blocks[i].paras) {
					Block rest{(uint16_t)(seg + paras), (uint16_t)(blocks[i].paras - paras), false};
					blocks[i].paras = (uint16_t)paras;
					blocks.insert(blocks.begin() + i + 1, rest);
				}
				return true;
			}
			if (i + 1 < blocks.size() && !blocks[i + 1].used && blocks[i].paras + blocks[i + 1].paras >= paras) {
				int extra = paras - blocks[i].paras;
				blocks[i + 1].seg = (uint16_t)(blocks[i + 1].seg + extra);
				blocks[i + 1].paras = (uint16_t)(blocks[i + 1].paras - extra);
				blocks[i].paras = (uint16_t)paras;
				if (!blocks[i + 1].paras)
					blocks.erase(blocks.begin() + i + 1);
				return true;
			}
			return false;
		}
	return false;
}

// --- EMS ----------------------------------------------------------------------------------------------------------------------------------
void Machine::emsSync(Cpu &c) {
	for (int p = 0; p < 4; p++)
		if (emsMap[p].handle > 0 && ems.count(emsMap[p].handle) && emsMap[p].page < (int)ems[emsMap[p].handle].size())
			memcpy(ems[emsMap[p].handle][emsMap[p].page].data(), c.mem + 0xE0000 + p * 0x4000, 0x4000);
}

void Machine::emsInt(Cpu &c) {
	int ah = c.ax.h;
	c.ax.h = 0;      // success unless changed
	switch (ah) {
	case 0x40: break;                                         // status
	case 0x41: c.bx.x = 0xE000; break;                        // page frame segment
	case 0x42: c.bx.x = (uint16_t)(2048 - emsUsed); c.dx.x = 2048; break;   // unallocated / total 16 KB pages (32 MB)
	case 0x43: {                                              // allocate pages: BX = count -> DX = handle
		int n = c.bx.x;
		if (n <= 0) { c.ax.h = 0x89; break; }
		if (emsUsed + n > 2048) { c.ax.h = 0x87; break; }
		int h = nextEms++;
		ems[h].assign((size_t)n, std::vector<uint8_t>(0x4000, 0));
		emsUsed += n;
		c.dx.x = (uint16_t)h;
		break;
	}
	case 0x44: {                                              // map page: AL = physical, BX = logical, DX = handle
		int ph = c.ax.l, lg = c.bx.x, h = c.dx.x;
		if (ph > 3) { c.ax.h = 0x8B; break; }
		emsSync(c);
		if (lg == 0xFFFF) { emsMap[ph] = {0, 0}; break; }
		if (!ems.count(h)) { c.ax.h = 0x83; break; }
		if (lg >= (int)ems[h].size()) { c.ax.h = 0x8A; break; }
		emsMap[ph] = {h, lg};
		memcpy(c.mem + 0xE0000 + ph * 0x4000, ems[h][lg].data(), 0x4000);
		break;
	}
	case 0x45: {                                              // release handle
		emsSync(c);
		auto it = ems.find(c.dx.x);
		if (it == ems.end()) { c.ax.h = 0x83; break; }
		emsUsed -= (int)it->second.size();
		for (int p = 0; p < 4; p++)
			if (emsMap[p].handle == c.dx.x)
				emsMap[p] = {0, 0};
		ems.erase(it);
		break;
	}
	case 0x46: c.ax.l = 0x40; break;                          // version 4.0
	case 0x4B: c.bx.x = (uint16_t)ems.size(); break;          // number of open handles
	case 0x4C: {                                              // pages owned by a handle
		auto it = ems.find(c.dx.x);
		if (it == ems.end()) { c.ax.h = 0x83; break; }
		c.bx.x = (uint16_t)it->second.size();
		break;
	}
	default: c.ax.h = 0x84; break;                            // undefined function
	}
}

// --- mouse (int 33h) and keyboard (int 16h) ------------------------------------------------------------------------------------------------
void Machine::mouseInt(Cpu &c) {
	switch (c.ax.x) {
	case 0: c.ax.x = 0xFFFF; c.bx.x = 2; mouseVisible = 0; break;                       // reset / driver installed, two buttons
	case 1: mouseVisible++; break;
	case 2: mouseVisible--; break;
	case 3: pollMouse(); c.bx.x = (uint16_t)mouseButtons; c.cx.x = (uint16_t)(mouseX * 2); c.dx.x = (uint16_t)mouseY; break;   // x in 640-wide virtual pixels
	case 4: mouseX = c.cx.x / 2; mouseY = c.dx.x; break;
	case 5: case 6: {                                                                    // button press / release info
		bool press = c.ax.x == 5;
		int b = c.bx.x & 1;
		c.ax.x = (uint16_t)mouseButtons;
		c.bx.x = (uint16_t)(press ? pressCount[b] : releaseCount[b]);
		c.cx.x = (uint16_t)(mouseX * 2);
		c.dx.x = (uint16_t)mouseY;
		if (press) pressCount[b] = 0; else releaseCount[b] = 0;
		break;
	}
	default: break;                                                                      // ranges, cursor shape, sensitivity, handler: accepted
	}
}

void Machine::pollMouse() {
	if (!mouseScript.empty()) {
		MouseEvent e = mouseScript.front();
		mouseScript.pop_front();
		mouseX = e.x;
		mouseY = e.y;
		if (e.buttons && !mouseButtons)
			pressCount[0]++;
		if (!e.buttons && mouseButtons)
			releaseCount[0]++;
		mouseButtons = e.buttons;
	}
}

void Machine::keyInt(Cpu &c) {
	switch (c.ax.h) {
	case 0: case 0x10:      // read key
		if (keys.empty()) { c.ax.x = 0; break; }
		c.ax.x = keys.front();
		keys.pop_front();
		break;
	case 1: case 0x11:      // key available?
		c.ZF = keys.empty();
		if (!keys.empty()) c.ax.x = keys.front();
		break;
	case 2: case 0x12: c.ax.l = 0; break;
	default: break;
	}
}

// --- DOS ----------------------------------------------------------------------------------------------------------------------------------
void Machine::dosInt(Cpu &c) {
	Host &h = H();
	int ah = c.ax.h;
	auto fileName = [&]() { return h.str(c.ds, c.dx.x, 200); };
	setCF(c, false);
	if (getenv("HOST_TRACE_DOS")) {
		fprintf(stderr, "int21 ah=%02X al=%02X bx=%04X cx=%04X dx=%04X ds=%04X sp=%04X", ah, c.ax.l, c.bx.x, c.cx.x, c.dx.x, c.ds, c.sp);
		if (ah == 0x3C || ah == 0x3D || ah == 0x41 || ah == 0x43 || ah == 0x4E) fprintf(stderr, " '%s'", fileName().c_str());
		fprintf(stderr, "\n");
	}
	switch (ah) {
	case 0x02: stdoutText.push_back((char)c.dx.l); break;
	case 0x06: if (c.dx.l != 0xFF) stdoutText.push_back((char)c.dx.l); else { c.ZF = true; c.ax.l = 0; } break;
	case 0x07: case 0x08: case 0x01: c.ax.l = 0x0D; break;
	case 0x09: { std::string s; for (int i = 0; i < 400; i++) { char ch = (char)c.rd8(c.ds, (uint16_t)(c.dx.x + i)); if (ch == '$') break; s.push_back(ch); } stdoutText += s; break; }
	case 0x0B: c.ax.l = 0; break;
	case 0x0E: c.ax.l = 3; break;                                  // select disk: three drives
	case 0x19: c.ax.l = 2; break;                                  // current drive C:
	case 0x1A: dta = ((uint32_t)c.ds << 16) | c.dx.x; break;
	case 0x25: c.wr16(0, (uint16_t)(c.ax.l * 4), c.dx.x); c.wr16(0, (uint16_t)(c.ax.l * 4 + 2), c.ds); break;     // the interrupt vector table lives at 0000:0000
	case 0x2F: c.es = (uint16_t)(dta >> 16); c.bx.x = (uint16_t)dta; break;
	case 0x2A: c.cx.x = 1994; c.dx.h = 4; c.dx.l = 18; c.ax.l = 1; break;      // date: 1994-04-18 (the release date of the data files)
	case 0x2C: {                                                    // time: advances on every call so that wait loops end
		clockCs += 3;
		unsigned t = clockCs;
		c.cx.h = (uint8_t)((12 + t / 360000) % 24); c.cx.l = (uint8_t)((t / 6000) % 60);
		c.dx.h = (uint8_t)((t / 100) % 60); c.dx.l = (uint8_t)(t % 100);
		break;
	}
	case 0x30: c.ax.x = 0x0005; c.bx.x = 0; c.cx.x = 0; break;     // DOS 5.0
	case 0x33: if (c.ax.l == 0) c.dx.l = 0; break;
	case 0x35: c.bx.x = c.rd16(0, (uint16_t)(c.ax.l * 4)); c.es = c.rd16(0, (uint16_t)(c.ax.l * 4 + 2)); break;
	case 0x3B: case 0x39: case 0x3A: break;
	case 0x3C: {                                                    // create
		int f = openFile(fileName(), true, true);
		if (f < 0) { setCF(c, true); c.ax.x = 5; } else c.ax.x = (uint16_t)f;
		break;
	}
	case 0x3D: {                                                    // open: AL = access mode
		int f = openFile(fileName(), false, (c.ax.l & 3) != 0);
		if (f < 0) { setCF(c, true); c.ax.x = 2; } else c.ax.x = (uint16_t)f;
		break;
	}
	case 0x3E: closeFile(c.bx.x); break;
	case 0x3F: {                                                    // read BX handle, CX count -> DS:DX
		std::vector<uint8_t> buf(c.cx.x);
		int n = c.bx.x == 0 ? 0 : readFile(c.bx.x, buf.data(), c.cx.x);
		if (n < 0) { setCF(c, true); c.ax.x = 6; break; }
		for (int i = 0; i < n; i++)
			c.wr8(c.ds, (uint16_t)(c.dx.x + i), buf[(size_t)i]);
		c.ax.x = (uint16_t)n;
		if (getenv("HOST_TRACE_DOS") && n > 0) fprintf(stderr, "   read ds=%04X dx=%04X first=%02X %02X %02X %02X\n", c.ds, c.dx.x, buf[0], n > 1 ? buf[1] : 0, n > 2 ? buf[2] : 0, n > 3 ? buf[3] : 0);
		break;
	}
	case 0x40: {                                                    // write
		std::vector<uint8_t> buf(c.cx.x);
		for (int i = 0; i < c.cx.x; i++)
			buf[(size_t)i] = c.rd8(c.ds, (uint16_t)(c.dx.x + i));
		int n = writeFile(c.bx.x, buf.data(), c.cx.x);
		if (n < 0) { setCF(c, true); c.ax.x = 5; } else c.ax.x = (uint16_t)n;
		break;
	}
	case 0x41: { std::string b = baseName(fileName()); if (vfs.erase(b)) break; setCF(c, true); c.ax.x = gameFiles.count(b) ? 5 : 2; break; }
	case 0x42: {                                                    // lseek AL = method, CX:DX = offset -> DX:AX
		long r = seekFile(c.bx.x, (long)(int32_t)(((uint32_t)c.cx.x << 16) | c.dx.x), c.ax.l);
		if (r < 0) { setCF(c, true); c.ax.x = 6; break; }
		c.dx.x = (uint16_t)((uint32_t)r >> 16);
		c.ax.x = (uint16_t)r;
		break;
	}
	case 0x43: { std::string b = baseName(fileName()); if (c.ax.l == 0) { if (vfs.count(b) || gameFiles.count(b)) c.cx.x = 0x20; else { setCF(c, true); c.ax.x = 2; } } break; }
	case 0x44:
		if (c.ax.l == 0) c.dx.x = c.bx.x < 5 ? 0x80D3 : 0x0000;      // device info: console for the standard handles, disk file otherwise
		else if (c.ax.l == 7 || c.ax.l == 6) c.ax.l = 0xFF;
		break;
	case 0x45: { auto it = files.find(c.bx.x); if (it == files.end()) { setCF(c, true); c.ax.x = 6; } else { int nh = nextHandle++; files[nh] = it->second; c.ax.x = (uint16_t)nh; } break; }
	case 0x47: c.wr8(c.ds, c.si, 0); break;                          // current directory: root
	case 0x48: {
		int seg = allocBlock(c.bx.x);
		if (seg < 0) { setCF(c, true); c.ax.x = 8; c.bx.x = (uint16_t)largestFree(); } else c.ax.x = (uint16_t)seg;
		break;
	}
	case 0x49: if (!freeBlock(c.es)) { setCF(c, true); c.ax.x = 9; } break;
	case 0x4A: if (!resizeBlock(c.es, c.bx.x)) { setCF(c, true); c.ax.x = 8; c.bx.x = (uint16_t)largestFree(); } break;
	case 0x4C: throw GameExit{c.ax.l};
	case 0x4E: case 0x4F: setCF(c, true); c.ax.x = 18; break;        // find first / next: no more files
	case 0x57: if (c.ax.l == 0) { c.cx.x = 0x6000; c.dx.x = 0x2492; } break;
	case 0x62: c.bx.x = 0x0800; break;                               // PSP segment
	default: break;
	}
	if (getenv("HOST_TRACE_DOS"))
		fprintf(stderr, "   -> ax=%04X dx=%04X cf=%d\n", c.ax.x, c.dx.x, c.CF);
}

void Machine::intr(Cpu &c, int number) {
	if (getenv("HOST_TRACE_INT") && number != 0x21)
		fprintf(stderr, "int %02Xh ax=%04X bx=%04X cx=%04X dx=%04X\n", number, c.ax.x, c.bx.x, c.cx.x, c.dx.x);
	switch (number) {
	case 0x10:
		switch (c.ax.h) {
		case 0x00: videoMode = c.ax.l & 0x7F; break;
		case 0x0F: c.ax.l = (uint8_t)videoMode; c.ax.h = (uint8_t)(videoMode <= 3 ? 80 : 40); c.bx.h = 0; break;
		case 0x1A: if (c.ax.l == 0) { c.ax.l = 0x1A; c.bx.x = 0x0008; } break;     // VGA display combination
		case 0x12: if (c.bx.l == 0x10) { c.bx.x = 0x0003; } break;                  // EGA/VGA information
		default: break;
		}
		break;
	case 0x16: keyInt(c); break;
	case 0x1A: if (c.ax.h == 0) { clockCs += 5; unsigned t = clockCs * 182 / 1000; c.cx.x = (uint16_t)(t >> 16); c.dx.x = (uint16_t)t; c.ax.l = 0; } break;
	case 0x21: dosInt(c); break;
	case 0x33: mouseInt(c); break;
	case 0x67: emsInt(c); break;
	default: break;      // 08h/1Bh/1Ch/23h/24h ... are only hooked, never raised here
	}
}

// --- ports ----------------------------------------------------------------------------------------------------------------------------------
uint16_t Machine::in(Cpu &, uint16_t port, int bits) {
	uint16_t full = bits == 16 ? 0xFFFF : 0xFF;
	switch (port) {
	case 0x40: case 0x41: case 0x42: pitCount = (uint16_t)(pitCount - 0x91); return (uint16_t)((pitCount >> ((pitFlip ^= 1) ? 0 : 8)) & 0xFF);
	case 0x21: return picMask;
	case 0x61: return port61;
	case 0x3DA: retrace ^= 9; return retrace;                       // vertical retrace and display-enable bits toggle: wait loops end
	case 0x3C9: { uint8_t v = palette[dacRead]; dacSub++; if (dacSub == 3) { dacSub = 0; dacRead = (dacRead + 3) & 0x2FF; } return v; }
	case 0x388: return 0;                                           // AdLib status: timers not running
	case 0x201: return 0xFF;                                        // joystick: absent
	case 0x60: return 0;
	case 0x20: return 0;
	default: return full;                                           // empty bus: SoundBlaster / MPU-401 / other cards are not present
	}
}

void Machine::out(Cpu &, uint16_t port, uint16_t value, int) {
	switch (port) {
	case 0x21: picMask = (uint8_t)value; break;
	case 0x61: port61 = (uint8_t)value; break;
	case 0x3C7: dacRead = (value & 0xFF) * 3; dacSub = 0; break;
	case 0x3C8: dacWrite = (value & 0xFF) * 3; dacSub = 0; break;
	case 0x3C9:
		if (dacWrite + dacSub < 768) palette[dacWrite + dacSub] = (uint8_t)(value & 0x3F);
		if (++dacSub == 3) { dacSub = 0; dacWrite += 3; }
		paletteWrites++;
		break;
	case 0x42: case 0x40: pitCount = (uint16_t)value; break;
	default: break;
	}
}

void Machine::install(Cpu &c) {
	c.onInt = [](Cpu &cc, int n) { H().machine.intr(cc, n); };
	c.onIn = [](Cpu &cc, uint16_t p, int b) { return H().machine.in(cc, p, b); };
	c.onOut = [](Cpu &cc, uint16_t p, uint16_t v, int b) { H().machine.out(cc, p, v, b); };
}
