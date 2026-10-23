#include "shared/Machine16.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace Yendor1 {
namespace Shared {

namespace {

// The files the parent hands down: the handle number is the offset of the field in the parameter block that carries it
struct Inherited {
	uint16_t handle;
	const char *path;
	bool inWorkDir;
};

const Inherited kInherited[] = {
	{0x5C, "CURRGAME/PARTY.DAT", true}, {0x5E, "FILE00.16", false}, {0x60, "FILE01.16", false}, {0x62, "FILE02.16", false},
	{0x64, "FILE03.16", false}, {0x66, "FILE04.16", false}, {0x68, "FILE05.16", false}, {0x6A, "FILE06.16", false},
	{0x70, "WORLD.DAT", false}, {0x72, "TOWN.DAT", false}, {0x74, "MINE.DAT", false}, {0x76, "CURRGAME/NPC.DAT", true},
	{0x78, "CURRGAME/AREA.DAT", true}, {0x7A, "CURRGAME/BATPIC.DAT", true}};

std::string joinPath(const std::string &dir, const std::string &name) {
	if (dir.empty())
		return name;
	const char last = dir[dir.size() - 1];
	return dir + ((last == '/' || last == '\\') ? "" : "/") + name;
}

} // namespace

Machine16::Machine16(Runtime &rtIn, Launcher *launcherIn, const Params &p)
	: rt(rtIn), launcher(launcherIn), params(p), lastExitCode(0), _cursorX(0), _cursorY(0), _dummy(0), _polls(0), _pollsAtCall(0),
	  _speakerDivisor(0) {
	memset(_stack, 0, sizeof(_stack));
	memset(_psp, 0, sizeof(_psp));
	memset(_parentPsp, 0, sizeof(_parentPsp));
	memset(_jft, 0, sizeof(_jft));
	memset(_ctxExtra, 0, sizeof(_ctxExtra));
	r.ds = r.es = kSegPsp;
	r.ss = kSegStack;
	r.sp = 0xFFF0;
	memcpy(_psp + 0x80, p.raw, 128);
	_psp[0x16] = (uint8_t)kSegParentPsp;
	_psp[0x17] = (uint8_t)(kSegParentPsp >> 8);
	_parentPsp[0x36] = (uint8_t)kSegJft;
	_parentPsp[0x37] = (uint8_t)(kSegJft >> 8);

	// the context block maps onto the driver's context
	for (int i = 0; i < 0x1000; i++)
		_ctxMap[i] = &_ctxExtra[i];
	Vid::Context &c = rt.video.ctx;
	auto map = [this](uint16_t off, void *member, int size) {
		for (int i = 0; i < size; i++)
			_ctxMap[off + i] = (uint8_t *)member + i;
	};
	map(0x00, &c.tileSlots, 2); map(0x08, &c.delayStep, 2); map(0x0A, &c.delayWipe, 2); map(0x0C, &c.delayCpu, 2);
	map(0x0E, &c.cpuClass, 2); map(0x10, &c.imgRows, 2); map(0x12, &c.imgBytes, 2); map(0x14, &c.imgMask, 1);
	map(0x1A, &c.x1, 2); map(0x1C, &c.y1, 2); map(0x1E, &c.x2, 2); map(0x20, &c.y2, 2); map(0x22, &c.x3, 2); map(0x24, &c.y3, 2);
	map(0x2A, &c.textCol, 1); map(0x2B, &c.textRow, 1); map(0x2C, &c.textChar, 1); map(0x2E, &c.textFg, 1); map(0x2F, &c.textBg, 1);
	map(0x30, &c.colour, 1); map(0x31, &c.drawPage, 1); map(0x32, &c.destPage, 1); map(0x33, &c.videoKind, 1);
	map(0x34, &c.keyColour, 1); map(0x35, &c.fontSelect, 1); map(0x36, &c.iconBarState, 1);
	map(0x64, c.colourMap, 16);
	map(0x676, &c.paletteCommand, 1); map(0x678, &c.viewHalfW, 1); map(0x679, &c.viewHalfH, 1);
	map(0x67A, &c.hotMarginX, 2); map(0x67C, &c.hotMarginY, 2); map(0x67E, &c.viewOffX, 2); map(0x680, &c.viewOffY, 2);
	map(0x912, &c.flags912, 1);
	map(0x952, &c.mousePresent, 1); map(0x953, &c.mouseFlags, 1); map(0x955, &c.cursorHide, 1); map(0x956, &c.cursorShape, 1);
	map(0x957, &c.cursorPage, 1); map(0x958, &c.cursorMode, 1);
	map(0x95A, &c.clipX1, 2); map(0x95C, &c.clipX2, 2); map(0x95E, &c.clipY1, 2); map(0x960, &c.clipY2, 2);
	map(0x962, &c.curX, 2); map(0x964, &c.curY, 2); map(0x966, &c.mouseX, 2); map(0x968, &c.mouseY, 2);
	map(0x96A, &c.hotX, 2); map(0x96C, &c.hotY, 2);

	initTileStore();
}

Machine16::~Machine16() {
	for (size_t i = 0; i < _files.size(); i++)
		if (_files[i])
			fclose(_files[i]);
}

bool Machine16::loadProgram(const std::string &exeName, uint32_t dataSegmentImageOffset) {
	Data::ExeImage exe;
	if (!exe.load(joinPath(rt.dataDir(), exeName).c_str(), dataSegmentImageOffset))
		return false;
	return ds.load(exe);
}

uint8_t &Machine16::ctxByte(uint16_t off) {
	return *_ctxMap[off & 0xFFF];
}

// The driver's tile store as VID16.DRV leaves it for the programs: ES:0 slots, ES:2 the segment of the slot table (6 bytes per slot:
// flag, file, tile index, segment of the slot's 512-byte buffer), ES:6 the next slot to try. Programs (tileOp in their own code) search
// and fill it themselves, so it lives in the program's memory; every slot starts unused, as after tileOp's "release" (flag 0, file FFh,
// index FFFFh).
void Machine16::initTileStore() {
	const unsigned slots = 200, perBlock = 128;
	const uint16_t table = (uint16_t)(kFirstBlock + _blocks.size() * 0x1000);
	_blocks.push_back(std::vector<uint8_t>(slots * 6, 0));
	std::vector<uint16_t> bufferSeg;
	for (unsigned i = 0; i < slots; i++) {
		if (i % perBlock == 0)
			_blocks.push_back(std::vector<uint8_t>(((slots - i) < perBlock ? (slots - i) : perBlock) * 512, 0));
		bufferSeg.push_back((uint16_t)(kFirstBlock + (_blocks.size() - 1) * 0x1000 + (i % perBlock) * 32));
	}
	for (unsigned i = 0; i < slots; i++) {
		uint8_t *e = &_blocks[table == 0 ? 0 : (size_t)(table - kFirstBlock) / 0x1000][i * 6];
		e[0] = 0;
		e[1] = 0xFF;
		e[2] = e[3] = 0xFF;
		e[4] = (uint8_t)bufferSeg[i];
		e[5] = (uint8_t)(bufferSeg[i] >> 8);
	}
	_tileTableSeg = table;
	_tileSlots = slots;
	rt.video.ctx.tileSlots = (uint16_t)slots;
	_ctxExtra[2] = (uint8_t)table;
	_ctxExtra[3] = (uint8_t)(table >> 8);
	_ctxExtra[4] = (uint8_t)bufferSeg[0];
	_ctxExtra[5] = (uint8_t)(bufferSeg[0] >> 8);
	_ctxExtra[6] = _ctxExtra[7] = 0;
}

uint8_t &Machine16::b(uint16_t seg, uint16_t off) {
	switch (seg) {
	case kSegData: return ds.b(off);
	case kSegCtx: return ctxByte(off);
	case kSegStack: return _stack[off];
	case kSegPsp: return _psp[off & 0xFF];
	case kSegParentPsp: return _parentPsp[off & 0xFF];
	case kSegJft: return _jft[off & 0xFF];
	default:
		if (seg >= kFirstBlock) {
			const size_t index = (size_t)(seg - kFirstBlock) / 0x1000;
			if (index < _blocks.size() && !_blocks[index].empty()) {
				const size_t at = ((size_t)(seg - kFirstBlock) % 0x1000) * 16 + off;
				if (at < _blocks[index].size())
					return _blocks[index][at];
			}
		}
		_dummy = 0;
		return _dummy;
	}
}

uint8_t *Machine16::blockPtr(uint16_t seg) {
	if (seg == kSegData)
		return ds.p(0);
	if (seg >= kFirstBlock) {
		const size_t index = (size_t)(seg - kFirstBlock) / 0x1000;
		if (index < _blocks.size() && !_blocks[index].empty())
			return &_blocks[index][0];
	}
	return 0;
}

uint8_t *Machine16::dosPtr(uint16_t seg, uint16_t off, uint32_t length) {
	(void)length;
	return &b(seg, off);
}

std::string Machine16::asciz(uint16_t seg, uint16_t off) {
	std::string s;
	for (int i = 0; i < 128; i++) {
		const char c = (char)b(seg, (uint16_t)(off + i));
		if (!c)
			break;
		s += c == '\\' ? '/' : c;
	}
	return s;
}

// ---------------------------------------------------------------------------------------------
// string instructions
// ---------------------------------------------------------------------------------------------

void Machine16::movs(int size) {
	if (size == 1)
		b(r.es, r.di) = b(r.ds, r.si);
	else
		ww(r.es, r.di, rw(r.ds, r.si));
	const int step = r.df ? -size : size;
	r.si = (uint16_t)(r.si + step);
	r.di = (uint16_t)(r.di + step);
}

void Machine16::stos(int size) {
	if (size == 1)
		b(r.es, r.di) = r.al;
	else
		ww(r.es, r.di, r.ax);
	r.di = (uint16_t)(r.di + (r.df ? -size : size));
}

uint16_t Machine16::lods(int size) {
	uint16_t v = size == 1 ? b(r.ds, r.si) : rw(r.ds, r.si);
	r.si = (uint16_t)(r.si + (r.df ? -size : size));
	return v;
}

void Machine16::scas(int size) {
	if (size == 1)
		alu::sub<uint8_t>(r, r.al, b(r.es, r.di));
	else
		alu::sub<uint16_t>(r, r.ax, rw(r.es, r.di));
	r.di = (uint16_t)(r.di + (r.df ? -size : size));
}

void Machine16::cmps(int size) {
	if (size == 1)
		alu::sub<uint8_t>(r, b(r.ds, r.si), b(r.es, r.di));
	else
		alu::sub<uint16_t>(r, rw(r.ds, r.si), rw(r.es, r.di));
	const int step = r.df ? -size : size;
	r.si = (uint16_t)(r.si + step);
	r.di = (uint16_t)(r.di + step);
}

// ---------------------------------------------------------------------------------------------
// interrupts
// ---------------------------------------------------------------------------------------------

void Machine16::intr(uint8_t n) {
	switch (n) {
	case 0x62: int62(); break;
	case 0x21: int21(); break;
	case 0x1A: int1A(); break;
	case 0x10: int10(); break;
	case 0x33: int33(); break;
	default: break;
	}
}

// the image pointer the program keeps at ES:16h (segment) and ES:18h (offset) becomes the driver's image buffer
void Machine16::bridgeImage() {
	Vid::Context &c = rt.video.ctx;
	const uint16_t seg = (uint16_t)(_ctxExtra[0x16] | (_ctxExtra[0x17] << 8));
	const uint16_t off = (uint16_t)(_ctxExtra[0x18] | (_ctxExtra[0x19] << 8));
	uint8_t *base = &b(seg, 0);
	size_t size = 65536;
	if (seg != kSegData && seg >= kFirstBlock) {
		const size_t index = (size_t)(seg - kFirstBlock) / 0x1000;
		if (index < _blocks.size())
			size = _blocks[index].size();
	} else if (seg == kSegPsp || seg == kSegParentPsp || seg == kSegJft) {
		size = 256;
	} else if (seg != kSegData) {
		size = 0;
	}
	c.image.set(base, (uint32_t)size, off);
}

void Machine16::unbridgeImage() {
	const uint16_t off = (uint16_t)rt.video.ctx.image.offset;
	_ctxExtra[0x18] = (uint8_t)off;
	_ctxExtra[0x19] = (uint8_t)(off >> 8);
}

void Machine16::int62() {
	_pollsAtCall = _polls;
	bridgeImage();
	rt.video.call(r.ah, r.al);
	unbridgeImage();
}

void Machine16::int1A() {
	if (r.ah == 0) {
		const uint32_t t = rt.system.ticks();
		r.dx = (uint16_t)t;
		r.cx = (uint16_t)(t >> 16);
		r.al = 0;
	}
}

void Machine16::int10() {
	// the DOS text screen is not modelled
}

void Machine16::int33() {
	if (r.ax == 3) {
		r.bx = 0;
		r.cx = (uint16_t)rt.video.ctx.mouseX;
		r.dx = (uint16_t)rt.video.ctx.mouseY;
	}
}

// ---------------------------------------------------------------------------------------------
// DOS
// ---------------------------------------------------------------------------------------------

std::string Machine16::resolvePath(const std::string &dosName) const {
	return dosName;
}

FILE *Machine16::handleFile(uint16_t h) {
	if (h >= 0x100)
		return h < _files.size() ? _files[h] : 0;
	for (size_t k = 0; k < sizeof(kInherited) / sizeof(kInherited[0]); k++) {
		if (kInherited[k].handle != h)
			continue;
		if (_files.size() <= (size_t)0x80)
			_files.resize(0x100, 0);
		FILE *&slot = _files[h];                      // inherited handles live in the low slots of the same table
		if (!slot) {
			const std::string path = joinPath(kInherited[k].inWorkDir ? rt.workDir() : rt.dataDir(), kInherited[k].path);
			slot = fopen(path.c_str(), "r+b");
			if (!slot)
				slot = fopen(path.c_str(), "rb");
		}
		return slot;
	}
	return 0;
}

uint16_t Machine16::openFile(const std::string &name) {
	const std::string inWork = joinPath(rt.workDir(), name), inData = joinPath(rt.dataDir(), name);
	FILE *f = fopen(inWork.c_str(), "r+b");
	if (!f)
		f = fopen(inData.c_str(), "r+b");
	if (!f)
		f = fopen(inWork.c_str(), "rb");
	if (!f)
		f = fopen(inData.c_str(), "rb");
	if (!f)
		return 0;
	if (_files.size() < 0x100)
		_files.resize(0x100, 0);
	for (size_t i = 0x100; i < _files.size(); i++)
		if (!_files[i]) {
			_files[i] = f;
			return (uint16_t)i;
		}
	_files.push_back(f);
	return (uint16_t)(_files.size() - 1);
}

void Machine16::execChild() {
	const std::string name = asciz(r.ds, r.dx);
	const uint16_t blockSeg = r.es, blockOff = r.bx;
	const uint16_t tailOff = rw(blockSeg, (uint16_t)(blockOff + 2)), tailSeg = rw(blockSeg, (uint16_t)(blockOff + 4));
	Params child;
	for (int i = 0; i < 128; i++)
		child.raw[i] = b(tailSeg, (uint16_t)(tailOff + i));
	int program = -1;
	size_t at = 0;
	while (at < name.size() && !isdigit((unsigned char)name[at]))
		at++;
	if (at < name.size())
		program = atoi(name.c_str() + at);
	lastExitCode = 0xFF;
	r.cf = false;
	if (launcher && program >= 0) {
		const int code = launcher->run(program, child, rt);
		lastExitCode = code < 0 ? 0xFF : code;
	}
}

void Machine16::int21() {
	switch (r.ah) {
	case 0x06:                                           // direct console input (DL = FFh)
		if (!rt.system.pump())
			throw ProgramExit(0xFD);                     // the platform wants to stop
		if (r.dl == 0xFF) {
			const int k = rt.system.pollKeyFor(_polls - _pollsAtCall);
			_polls++;
			if (k < 0) {
				r.zf = true;
				r.al = 0;
			} else {
				r.zf = false;
				r.al = (uint8_t)k;
			}
		}
		break;
	case 0x0C:                                           // flush the keyboard buffer
		rt.system.flushKeys();
		break;
	case 0x2C:
		r.dx = rt.system.dosClock();
		r.cx = 0;
		break;
	case 0x30:
		r.ax = 0x0005;
		break;
	case 0x48: {                                         // allocate memory
		const uint32_t bytes = (uint32_t)r.bx * 16;
		_blocks.push_back(std::vector<uint8_t>(bytes < 16 ? 16 : (bytes > 0x10000 ? 0x10000 : bytes), 0));
		r.ax = (uint16_t)(kFirstBlock + (_blocks.size() - 1) * 0x1000);
		r.cf = false;
		break;
	}
	case 0x49:                                           // free memory
	case 0x4A:
		r.cf = false;
		break;
	case 0x4B: execChild(); break;
	case 0x4C: throw ProgramExit(r.al);
	case 0x4D: r.ax = (uint16_t)lastExitCode; break;
	case 0x51:
	case 0x62:
		r.bx = kSegPsp;
		break;
	case 0x3D: {                                         // open
		const uint16_t h = openFile(asciz(r.ds, r.dx));
		if (h) {
			r.ax = h;
			r.cf = false;
		} else {
			r.ax = 2;
			r.cf = true;
		}
		break;
	}
	case 0x3E: {                                         // close
		FILE *f = handleFile(r.bx);
		if (f && r.bx >= 0x100) {
			fclose(f);
			_files[r.bx] = 0;
		}
		r.cf = false;
		break;
	}
	case 0x3F: {                                         // read
		FILE *f = handleFile(r.bx);
		uint16_t got = 0;
		if (f) {
			std::vector<uint8_t> tmp(r.cx);
			got = (uint16_t)fread(&tmp[0], 1, r.cx, f);
			for (uint16_t i = 0; i < got; i++)
				b(r.ds, (uint16_t)(r.dx + i)) = tmp[i];
		}
		r.ax = got;
		r.cf = f == 0;
		break;
	}
	case 0x40: {                                         // write (zero bytes: cut the file at the current position)
		FILE *f = handleFile(r.bx);
		if (!f) {
			r.ax = 5;
			r.cf = true;
			break;
		}
		if (r.cx == 0) {
			fflush(f);
#ifdef _WIN32
			_chsize(_fileno(f), (long)ftell(f));
#else
			if (ftruncate(fileno(f), ftell(f)) != 0) {
			}
#endif
			r.ax = 0;
		} else {
			std::vector<uint8_t> tmp(r.cx);
			for (uint16_t i = 0; i < r.cx; i++)
				tmp[i] = b(r.ds, (uint16_t)(r.dx + i));
			r.ax = (uint16_t)fwrite(&tmp[0], 1, r.cx, f);
			fflush(f);
		}
		r.cf = false;
		break;
	}
	case 0x42: {                                         // lseek
		FILE *f = handleFile(r.bx);
		if (!f) {
			r.cf = true;
			break;
		}
		const long off = (long)(((uint32_t)r.cx << 16) | r.dx);
		fseek(f, off, r.al == 0 ? SEEK_SET : (r.al == 1 ? SEEK_CUR : SEEK_END));
		const long pos = ftell(f);
		r.ax = (uint16_t)pos;
		r.dx = (uint16_t)((uint32_t)pos >> 16);
		r.cf = false;
		break;
	}
	default:
		break;
	}
}

// ---------------------------------------------------------------------------------------------
// ports: the speaker
// ---------------------------------------------------------------------------------------------

void Machine16::out8(uint16_t port, uint8_t v) {
	static uint8_t portB = 0;
	static int timerByte = 0;
	switch (port) {
	case 0x42:
		if (timerByte == 0)
			_speakerDivisor = v;
		else
			_speakerDivisor = (uint16_t)(_speakerDivisor | (v << 8));
		timerByte ^= 1;
		break;
	case 0x61:
		portB = v;
		if ((v & 3) == 3 && _speakerDivisor)
			rt.system.speaker((uint16_t)(1193180u / _speakerDivisor));
		else
			rt.system.speaker(0);
		break;
	default:
		break;
	}
}

uint8_t Machine16::in8(uint16_t port) {
	static uint8_t toggle = 0;
	if (port == 0x3DA) {
		toggle ^= 8;                                     // vertical retrace bit, alternating
		return toggle;
	}
	return 0;
}

// ---------------------------------------------------------------------------------------------
// the palette helper
// ---------------------------------------------------------------------------------------------

void Machine16::paletteCommand() {
	rt.sendPaletteCommand(r.al);
}

void Machine16::unsupported(const char *what) {
	fprintf(stderr, "Machine16: untranslated code reached: %s\n", what);
	throw ProgramExit(0xFE);
}

} // namespace Shared
} // namespace Yendor1
