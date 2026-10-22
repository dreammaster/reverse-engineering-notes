#include "Machine.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#include <windows.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

namespace Yemu {

static const uint16_t kHleSeg = 0xF000;
static const uint16_t kReturnMarker = 0x00FF;

// ---------------------------------------------------------------------------------------------
// construction
// ---------------------------------------------------------------------------------------------

Machine::Machine(const std::string &rootDir)
	: cpu(mem, this), maxInstr(0), quit(false), currentPsp(0), ctxSegment(0), _root(rootDir),
	  _dtaSeg(0), _dtaOff(0x80), _lastExit(0), _lastExitType(0), _childDone(false), _skipIret(false),
	  _childExit(0), _childExitType(0), _findPos(0), _findAttr(0),
	  _mouseSeg(0), _mouseOff(0), _mouseMask(0), _mickX(0), _mickY(0), _mouseX(320), _mouseY(175), _buttons(0),
	  _nextTick(0), _scriptPos(0) {
	memset(mem, 0, sizeof(mem));
	_press[0] = _press[1] = 0;
	static const uint8_t stubs[] = {0x10, 0x16, 0x1A, 0x1C, 0x20, 0x21, 0x33};
	for (size_t i = 0; i < sizeof(stubs); i++) {
		mem[stubs[i] * 4 + 0] = stubs[i];
		mem[stubs[i] * 4 + 1] = 0;
		mem[stubs[i] * 4 + 2] = 0x00;
		mem[stubs[i] * 4 + 3] = 0xF0;
	}
	Block b;
	b.start = 0x0800;
	b.size = 0xA000 - 0x0800;
	b.owner = 0;
	b.used = false;
	_blocks.push_back(b);
	// standard handles
	for (int i = 0; i < 5; i++) {
		SftEntry e;
		e.f = 0;
		e.refs = 1000;
		e.console = true;
		_sft.push_back(e);
	}
}

Machine::~Machine() {
	for (size_t i = 0; i < _sft.size(); i++)
		if (_sft[i].f)
			fclose(_sft[i].f);
}

void Machine::setFonts(const std::vector<uint8_t> &f8, const std::vector<uint8_t> &f14) {
	_font8 = f8;
	_font14 = f14;
	if (!f14.empty()) memcpy(mem + 0xC0000, &f14[0], std::min<size_t>(f14.size(), 0x1000));
	if (!f8.empty()) memcpy(mem + 0xC1000, &f8[0], std::min<size_t>(f8.size(), 0x1000));
}

uint32_t Machine::ticks() const {
	return (uint32_t)(cpu.instructions * 182065ULL / 10000ULL / kInstrPerSecond);
}

void Machine::unhandled(const char *what) {
	int &n = _unhandled[what];
	if (n++ == 0) {
		log += std::string("unhandled ") + what + "\n";
	}
}

// ---------------------------------------------------------------------------------------------
// DOS memory manager (first fit, paragraph granularity)
// ---------------------------------------------------------------------------------------------

uint16_t Machine::memLargest() const {
	uint16_t best = 0;
	for (size_t i = 0; i < _blocks.size(); i++)
		if (!_blocks[i].used && _blocks[i].size > best)
			best = _blocks[i].size;
	return best;
}

uint16_t Machine::memAlloc(uint16_t paras, uint16_t owner, uint16_t *largest) {
	for (size_t i = 0; i < _blocks.size(); i++) {
		Block &b = _blocks[i];
		if (b.used || b.size < paras)
			continue;
		if (b.size > paras) {
			Block rest;
			rest.start = (uint16_t)(b.start + paras);
			rest.size = (uint16_t)(b.size - paras);
			rest.owner = 0;
			rest.used = false;
			b.size = paras;
			_blocks.insert(_blocks.begin() + i + 1, rest);
		}
		_blocks[i].used = true;
		_blocks[i].owner = owner;
		memset(mem + ((uint32_t)_blocks[i].start << 4), 0, (size_t)paras << 4);
		return _blocks[i].start;
	}
	if (largest)
		*largest = memLargest();
	return 0;
}

bool Machine::memFree(uint16_t seg) {
	for (size_t i = 0; i < _blocks.size(); i++) {
		if (_blocks[i].start == seg && _blocks[i].used) {
			_blocks[i].used = false;
			_blocks[i].owner = 0;
			// merge with neighbours
			for (size_t j = 0; j + 1 < _blocks.size();) {
				if (!_blocks[j].used && !_blocks[j + 1].used) {
					_blocks[j].size = (uint16_t)(_blocks[j].size + _blocks[j + 1].size);
					_blocks.erase(_blocks.begin() + j + 1);
				} else {
					j++;
				}
			}
			return true;
		}
	}
	return false;
}

bool Machine::memResize(uint16_t seg, uint16_t paras, uint16_t *maxAvail) {
	for (size_t i = 0; i < _blocks.size(); i++) {
		if (_blocks[i].start != seg || !_blocks[i].used)
			continue;
		Block &b = _blocks[i];
		if (paras <= b.size) {
			if (paras < b.size) {
				Block rest;
				rest.start = (uint16_t)(b.start + paras);
				rest.size = (uint16_t)(b.size - paras);
				rest.owner = 0;
				rest.used = false;
				b.size = paras;
				_blocks.insert(_blocks.begin() + i + 1, rest);
				// merge the new free tail with a following free block
				if (i + 2 < _blocks.size() && !_blocks[i + 2].used) {
					_blocks[i + 1].size = (uint16_t)(_blocks[i + 1].size + _blocks[i + 2].size);
					_blocks.erase(_blocks.begin() + i + 2);
				}
			}
			return true;
		}
		const uint16_t extra = (uint16_t)(paras - b.size);
		if (i + 1 < _blocks.size() && !_blocks[i + 1].used && _blocks[i + 1].size >= extra) {
			_blocks[i + 1].start = (uint16_t)(_blocks[i + 1].start + extra);
			_blocks[i + 1].size = (uint16_t)(_blocks[i + 1].size - extra);
			b.size = paras;
			if (_blocks[i + 1].size == 0)
				_blocks.erase(_blocks.begin() + i + 1);
			return true;
		}
		if (maxAvail)
			*maxAvail = (uint16_t)(b.size + ((i + 1 < _blocks.size() && !_blocks[i + 1].used) ? _blocks[i + 1].size : 0));
		return false;
	}
	if (maxAvail)
		*maxAvail = 0;
	return false;
}

void Machine::memFreeOwner(uint16_t owner) {
	for (size_t i = 0; i < _blocks.size(); i++)
		if (_blocks[i].used && _blocks[i].owner == owner) {
			memFree(_blocks[i].start);
			i = (size_t)-1;     // restart: the vector changed
		}
}

// ---------------------------------------------------------------------------------------------
// files
// ---------------------------------------------------------------------------------------------

std::string Machine::readDosString(uint16_t seg, uint16_t off) {
	std::string s;
	for (int i = 0; i < 260; i++) {
		const uint8_t c = mem[Cpu::lin(seg, (uint16_t)(off + i))];
		if (!c)
			break;
		s += (char)c;
	}
	return s;
}

std::string Machine::hostPath(const std::string &dos) const {
	std::string p = dos;
	if (p.size() >= 2 && p[1] == ':')
		p = p.substr(2);
	for (size_t i = 0; i < p.size(); i++)
		if (p[i] == '\\')
			p[i] = '/';
	while (!p.empty() && p[0] == '/')
		p.erase(0, 1);
	// DOS cuts every path component to an 8-character name and a 3-character extension, and ignores trailing spaces
	std::string out, comp;
	for (size_t i = 0; i <= p.size(); i++) {
		if (i == p.size() || p[i] == '/') {
			while (!comp.empty() && comp[comp.size() - 1] == ' ')
				comp.erase(comp.size() - 1);
			const size_t dot = comp.find('.');
			if (dot != std::string::npos && comp != "." && comp != "..") {
				std::string name = comp.substr(0, dot), ext = comp.substr(dot + 1);
				const size_t d2 = ext.find('.');
				if (d2 != std::string::npos)
					ext = ext.substr(0, d2);
				comp = name.substr(0, 8) + "." + ext.substr(0, 3);
			} else if (comp.size() > 8 && comp != "." && comp != "..") {
				comp = comp.substr(0, 8);
			}
			out += comp;
			if (i < p.size())
				out += "/";
			comp.clear();
		} else {
			comp += p[i];
		}
	}
	return _root + "/" + out;
}

int Machine::sftAlloc(FILE *f, const std::string &path, bool console) {
	for (size_t i = 5; i < _sft.size(); i++) {
		if (_sft[i].refs == 0) {
			_sft[i].f = f; _sft[i].path = path; _sft[i].refs = 1; _sft[i].console = console;
			return (int)i;
		}
	}
	SftEntry e;
	e.f = f; e.path = path; e.refs = 1; e.console = console;
	_sft.push_back(e);
	return (int)_sft.size() - 1;
}

void Machine::sftRelease(int idx) {
	if (idx < 0 || idx >= (int)_sft.size() || _sft[idx].refs <= 0)
		return;
	if (--_sft[idx].refs == 0 && _sft[idx].f) {
		fclose(_sft[idx].f);
		_sft[idx].f = 0;
	}
}

int Machine::jftLookup(int handle) {
	const uint16_t psp = pspSeg();
	const uint16_t max = cpu.rw(psp, 0x32);
	if (handle < 0 || handle >= max)
		return -1;
	const uint16_t off = cpu.rw(psp, 0x34), seg = cpu.rw(psp, 0x36);
	const uint8_t b = mem[Cpu::lin(seg, (uint16_t)(off + handle))];
	return b == 0xFF ? -1 : b;
}

int Machine::jftAssign(int sftIndex, bool) {
	const uint16_t psp = pspSeg();
	const uint16_t max = cpu.rw(psp, 0x32);
	const uint16_t off = cpu.rw(psp, 0x34), seg = cpu.rw(psp, 0x36);
	for (int h = 0; h < max; h++) {
		if (mem[Cpu::lin(seg, (uint16_t)(off + h))] == 0xFF) {
			mem[Cpu::lin(seg, (uint16_t)(off + h))] = (uint8_t)sftIndex;
			return h;
		}
	}
	return -1;
}

static bool wildMatch(const std::string &pat, const std::string &name) {
	// DOS 8.3 wildcard: split into name and extension parts
	auto split = [](const std::string &s, std::string &n, std::string &e) {
		const size_t d = s.find('.');
		if (d == std::string::npos) { n = s; e = ""; } else { n = s.substr(0, d); e = s.substr(d + 1); }
	};
	std::string pn, pe, nn, ne;
	split(pat, pn, pe);
	split(name, nn, ne);
	auto part = [](const std::string &p, const std::string &s) {
		size_t i = 0;
		for (; i < p.size(); i++) {
			if (p[i] == '*') return true;
			if (i >= s.size()) return false;
			if (p[i] != '?' && toupper((unsigned char)p[i]) != toupper((unsigned char)s[i])) return false;
		}
		return i == s.size();
	};
	return part(pn, nn) && part(pe, ne);
}

// ---------------------------------------------------------------------------------------------
// programs
// ---------------------------------------------------------------------------------------------

bool Machine::loadProgram(const std::string &dosPath, uint16_t parentPsp, uint16_t tailSeg, uint16_t tailOff, uint16_t envSeg, bool, uint16_t *pspOut) {
	std::string hp = hostPath(dosPath);
	FILE *f = fopen(hp.c_str(), "rb");
	if (!f)
		return false;
	std::vector<uint8_t> file;
	{
		fseek(f, 0, SEEK_END);
		const long n = ftell(f);
		fseek(f, 0, SEEK_SET);
		file.resize((size_t)n);
		if (n > 0 && fread(&file[0], 1, (size_t)n, f) != (size_t)n) { fclose(f); return false; }
		fclose(f);
	}
	const bool isMz = file.size() > 0x1C && file[0] == 'M' && file[1] == 'Z';
	uint16_t imageParas, minAlloc, maxAlloc, ss = 0, sp = 0xFFFE, ipv = 0x100, csv = 0, hdrParas = 0;
	uint32_t imageBytes;
	uint16_t relocCount = 0, relocOff = 0;
	if (isMz) {
		const uint16_t cblp = (uint16_t)(file[2] | (file[3] << 8)), cp = (uint16_t)(file[4] | (file[5] << 8));
		relocCount = (uint16_t)(file[6] | (file[7] << 8));
		hdrParas = (uint16_t)(file[8] | (file[9] << 8));
		minAlloc = (uint16_t)(file[10] | (file[11] << 8));
		maxAlloc = (uint16_t)(file[12] | (file[13] << 8));
		ss = (uint16_t)(file[14] | (file[15] << 8));
		sp = (uint16_t)(file[16] | (file[17] << 8));
		ipv = (uint16_t)(file[20] | (file[21] << 8));
		csv = (uint16_t)(file[22] | (file[23] << 8));
		relocOff = (uint16_t)(file[24] | (file[25] << 8));
		imageBytes = (uint32_t)(cblp == 0 ? cp * 512u : (cp - 1) * 512u + cblp) - (uint32_t)hdrParas * 16;
		imageParas = (uint16_t)((imageBytes + 15) / 16);
	} else {
		imageBytes = (uint32_t)file.size();
		imageParas = (uint16_t)((imageBytes + 0x100 + 15) / 16);
		minAlloc = 0;
		maxAlloc = 0xFFFF;
	}
	const uint32_t needMin = 0x10u + imageParas + minAlloc;
	uint16_t avail = memLargest();
	if (needMin > avail)
		return false;
	uint32_t want = isMz ? 0x10u + imageParas + (uint32_t)maxAlloc : 0xFFFFu;
	if (want > avail)
		want = avail;
	const uint16_t psp = memAlloc((uint16_t)want, 0, 0);
	if (!psp)
		return false;
	for (size_t i = 0; i < _blocks.size(); i++)
		if (_blocks[i].start == psp)
			_blocks[i].owner = psp;
	// PSP
	mem[Cpu::lin(psp, 0)] = 0xCD; mem[Cpu::lin(psp, 1)] = 0x20;
	cpu.ww(psp, 2, (uint16_t)(psp + want));
	cpu.ww(psp, 0x16, parentPsp);
	// environment
	uint16_t env = envSeg;
	if (env == 0) {
		env = memAlloc(0x10, psp, 0);
		const char *vars = "COMSPEC=C:\\COMMAND.COM";
		size_t o = 0;
		memcpy(mem + Cpu::lin(env, 0), vars, strlen(vars) + 1);
		o = strlen(vars) + 1;
		mem[Cpu::lin(env, (uint16_t)o++)] = 0;
		cpu.ww(env, (uint16_t)o, 1);
		o += 2;
		std::string full = "C:\\YENDOR\\" + dosPath;
		memcpy(mem + Cpu::lin(env, (uint16_t)o), full.c_str(), full.size() + 1);
	}
	cpu.ww(psp, 0x2C, env);
	// command tail (the whole 128-byte block)
	if (tailSeg || tailOff)
		memcpy(mem + Cpu::lin(psp, 0x80), mem + Cpu::lin(tailSeg, tailOff), 128);
	else
		mem[Cpu::lin(psp, 0x81)] = 0x0D;
	// handle table: the first 20 entries come from the parent
	cpu.ww(psp, 0x32, 20);
	cpu.ww(psp, 0x34, 0x18);
	cpu.ww(psp, 0x36, psp);
	for (int i = 0; i < 20; i++)
		mem[Cpu::lin(psp, (uint16_t)(0x18 + i))] = 0xFF;
	if (parentPsp == 0) {
		for (int i = 0; i < 5; i++) {
			mem[Cpu::lin(psp, (uint16_t)(0x18 + i))] = (uint8_t)i;
		}
	} else {
		const uint16_t poff = cpu.rw(parentPsp, 0x34), pseg = cpu.rw(parentPsp, 0x36), pmax = cpu.rw(parentPsp, 0x32);
		for (int i = 0; i < 20 && i < pmax; i++) {
			const uint8_t b = mem[Cpu::lin(pseg, (uint16_t)(poff + i))];
			mem[Cpu::lin(psp, (uint16_t)(0x18 + i))] = b;
			if (b != 0xFF)
				_sft[b].refs++;
		}
	}
	// image
	const uint16_t loadSeg = (uint16_t)(psp + 0x10);
	if (isMz) {
		memcpy(mem + Cpu::lin(loadSeg, 0), &file[hdrParas * 16], imageBytes);
		for (int i = 0; i < relocCount; i++) {
			const uint16_t ro = (uint16_t)(file[relocOff + i * 4] | (file[relocOff + i * 4 + 1] << 8));
			const uint16_t rs = (uint16_t)(file[relocOff + i * 4 + 2] | (file[relocOff + i * 4 + 3] << 8));
			const uint16_t seg = (uint16_t)(loadSeg + rs);
			cpu.ww(seg, ro, (uint16_t)(cpu.rw(seg, ro) + loadSeg));
		}
		cpu.s[CS] = (uint16_t)(loadSeg + csv);
		cpu.ip = ipv;
		cpu.s[SS] = (uint16_t)(loadSeg + ss);
		cpu.r[SP] = sp;
	} else {
		memcpy(mem + Cpu::lin(psp, 0x100), &file[0], file.size());
		cpu.s[CS] = psp;
		cpu.ip = 0x100;
		cpu.s[SS] = psp;
		cpu.r[SP] = 0xFFFE;
		cpu.ww(psp, 0xFFFE, 0);
	}
	cpu.s[DS] = cpu.s[ES] = psp;
	memset(cpu.r, 0, sizeof(cpu.r));
	cpu.r[SP] = isMz ? sp : 0xFFFE;
	cpu.flags = 0x0202;
	*pspOut = psp;
	return true;
}

int Machine::runChild(const std::string &dosPath, uint16_t paramSeg, uint16_t paramOff) {
	// save the parent's context (the INT 21h frame is already on its stack)
	uint16_t r[8], s[4], ip, flags;
	memcpy(r, cpu.r, sizeof(r));
	memcpy(s, cpu.s, sizeof(s));
	ip = cpu.ip;
	flags = cpu.flags;
	const int parent = currentPsp;
	const uint16_t envSeg = cpu.rw(paramSeg, paramOff);
	const uint16_t tailOff = cpu.rw(paramSeg, (uint16_t)(paramOff + 2)), tailSeg = cpu.rw(paramSeg, (uint16_t)(paramOff + 4));
	uint16_t psp;
	if (!loadProgram(dosPath, (uint16_t)parent, tailSeg, tailOff, envSeg, false, &psp)) {
		memcpy(cpu.r, r, sizeof(r));
		memcpy(cpu.s, s, sizeof(s));
		return -1;
	}
	currentPsp = psp;
	Process pr;
	pr.psp = psp;
	_procs.push_back(pr);
	programTrace.push_back(dosPath);
	if (onProgramStart)
		onProgramStart(*this, psp);
	const bool savedDone = _childDone;
	_childDone = false;
	runLoop([this] { return _childDone; });
	_childDone = savedDone;
	// the child has terminated: restore the parent
	currentPsp = parent;
	memcpy(cpu.r, r, sizeof(r));
	memcpy(cpu.s, s, sizeof(s));
	cpu.ip = ip;
	cpu.flags = flags;
	_lastExit = _childExit;
	_lastExitType = _childExitType;
	return 0;
}

void Machine::terminate(uint8_t code, uint8_t type, bool tsr, uint16_t keepParas) {
	const uint16_t psp = pspSeg();
	{ char b[64]; snprintf(b, sizeof(b), "exit psp %04X code %02X type %d\n", psp, code, type); log += b; }
	_childExit = code;
	_childExitType = type;
	// close the handles this process owns: the inherited first 20 and anything it opened
	const uint16_t off = cpu.rw(psp, 0x34), seg = cpu.rw(psp, 0x36);
	for (int h = 0; h < 20; h++) {
		uint8_t &b = mem[Cpu::lin(psp, (uint16_t)(0x18 + h))];
		if (b != 0xFF) {
			sftRelease(b);
			b = 0xFF;
		}
	}
	(void)off; (void)seg;
	if (!_procs.empty() && _procs.back().psp == psp) {
		for (size_t i = 0; i < _procs.back().extraOpen.size(); i++)
			sftRelease(_procs.back().extraOpen[i]);
		_procs.pop_back();
	}
	if (tsr) {
		memResize(psp, keepParas, 0);
	} else {
		memFreeOwner(psp);
	}
	_childDone = true;
	_skipIret = true;
}

void Machine::runLoop(const std::function<bool()> &done) {
	uint64_t lastPump = cpu.instructions;
	while (!done() && !quit) {
		recent.push_back(((uint32_t)cpu.s[CS] << 16) | cpu.ip);
		if (watchOn && cpu.s[CS] == watchCs && cpu.ip == watchIp) {
			watchOn = false;
			log += "watch hit; recent: " + dumpRecent(200) + "\n";
		}
		if (recent.size() > 256) recent.pop_front();
		if (!cpu.step()) {
			log += "cpu stopped: " + cpu.fault + "\n";
			quit = true;
			break;
		}
		if (cpu.instructions - lastPump >= 256) {
			lastPump = cpu.instructions;
			pump();
		}
		if (maxInstr && cpu.instructions > maxInstr) {
			log += "instruction limit reached\n";
			quit = true;
		}
	}
}

std::string Machine::dumpRecent(size_t n) const {
	std::string o;
	char b[32];
	for (size_t i = recent.size() > n ? recent.size() - n : 0; i < recent.size(); i++) {
		snprintf(b, sizeof(b), "%04X:%04X ", recent[i] >> 16, recent[i] & 0xFFFF);
		o += b;
	}
	return o;
}

int Machine::run(const std::string &program, uint64_t maxInstructions) {
	maxInstr = maxInstructions;
	uint16_t psp;
	if (!loadProgram(program, 0, 0, 0, 0, false, &psp))
		return -1;
	currentPsp = psp;
	Process pr;
	pr.psp = psp;
	_procs.push_back(pr);
	programTrace.push_back(program);
	_nextTick = (uint64_t)kInstrPerSecond * 10 / 182;
	if (onProgramStart)
		onProgramStart(*this, psp);
	_childDone = false;
	runLoop([this] { return _childDone; });
	return _childExit;
}

// ---------------------------------------------------------------------------------------------
// time, scripted input, interrupts from outside
// ---------------------------------------------------------------------------------------------

void Machine::syncInterrupt(uint8_t n) {
	uint16_t r[8], s[4], ip, flags;
	memcpy(r, cpu.r, sizeof(r));
	memcpy(s, cpu.s, sizeof(s));
	ip = cpu.ip;
	flags = cpu.flags;
	cpu.s[CS] = kHleSeg;
	cpu.ip = kReturnMarker;
	cpu.interrupt(n);
	runLoop([this] { return cpu.s[CS] == kHleSeg && cpu.ip == kReturnMarker; });
	syncResultAx = cpu.r[AX];
	const uint16_t keepAx = cpu.r[AX];
	const uint16_t keepFlags = cpu.flags;
	memcpy(cpu.r, r, sizeof(r));
	memcpy(cpu.s, s, sizeof(s));
	cpu.ip = ip;
	cpu.flags = flags;
	(void)keepAx; (void)keepFlags;
}

void Machine::callFar(uint16_t seg, uint16_t off, uint16_t ax, uint16_t bx, uint16_t cx, uint16_t dx) {
	uint16_t r[8], s[4], ip, flags;
	memcpy(r, cpu.r, sizeof(r));
	memcpy(s, cpu.s, sizeof(s));
	ip = cpu.ip;
	flags = cpu.flags;
	cpu.r[AX] = ax; cpu.r[BX] = bx; cpu.r[CX] = cx; cpu.r[DX] = dx;
	cpu.push(kHleSeg);
	cpu.push(kReturnMarker);
	cpu.s[CS] = seg;
	cpu.ip = off;
	runLoop([this] { return cpu.s[CS] == kHleSeg && cpu.ip == kReturnMarker; });
	memcpy(cpu.r, r, sizeof(r));
	memcpy(cpu.s, s, sizeof(s));
	cpu.ip = ip;
	cpu.flags = flags;
}

void Machine::deliverTick() {
	_nextTick += (uint64_t)kInstrPerSecond * 100 / 1821;
	if (!(cpu.flags & IF) || cpu.s[CS] == kHleSeg || cpu.irqInhibit)
		return;
	const uint16_t off = cpu.rw(0, 0x1C * 4), seg = cpu.rw(0, 0x1C * 4 + 2);
	if (seg == kHleSeg)
		return;
	cpu.interrupt(0x1C);
	(void)off;
}

void Machine::deliverStep(const ScriptStep &st) {
	switch (st.kind) {
	case ScriptStep::Key:
		_keys.push_back((uint8_t)st.a);
		if (st.b >= 0 && st.a == 0)
			_keys.push_back((uint8_t)st.b);
		break;
	case ScriptStep::MouseMove: {
		_mouseX += st.a; _mouseY += st.b;
		_mickX += st.a; _mickY += st.b;
		if ((_mouseMask & 1) && _mouseSeg)
			callFar(_mouseSeg, _mouseOff, 1, _buttons, (uint16_t)_mouseX, (uint16_t)_mouseY);
		break;
	}
	case ScriptStep::MouseDown: {
		const int b = st.a;
		_buttons |= (uint8_t)(1 << b);
		_press[b]++;
		const uint16_t bit = b == 0 ? 2 : 8;
		if ((_mouseMask & bit) && _mouseSeg)
			callFar(_mouseSeg, _mouseOff, bit, _buttons, (uint16_t)_mouseX, (uint16_t)_mouseY);
		break;
	}
	case ScriptStep::MouseUp: {
		const int b = st.a;
		_buttons &= (uint8_t)~(1 << b);
		const uint16_t bit = b == 0 ? 4 : 16;
		if ((_mouseMask & bit) && _mouseSeg)
			callFar(_mouseSeg, _mouseOff, bit, _buttons, (uint16_t)_mouseX, (uint16_t)_mouseY);
		break;
	}
	case ScriptStep::Shot:
		if (onShot)
			onShot(st.name);
		break;
	case ScriptStep::Quit:
		quit = true;
		break;
	}
}

void Machine::pump() {
	if (cpu.instructions >= _nextTick)
		deliverTick();
	while (_scriptPos < _script.size() && _script[_scriptPos].at <= cpu.instructions) {
		const ScriptStep st = _script[_scriptPos++];
		deliverStep(st);
	}
}

// ---------------------------------------------------------------------------------------------
// Bus
// ---------------------------------------------------------------------------------------------

uint8_t Machine::in8(uint16_t port) {
	if (port >= 0x3C0 && port <= 0x3DF)
		return ega.inPort(port);
	if (port == 0x61)
		return 0;
	return 0xFF;
}

void Machine::out8(uint16_t port, uint8_t v) {
	if (port >= 0x3C0 && port <= 0x3DF)
		ega.outPort(port, v);
	// PIT (40h-43h), speaker (61h), DAC and attribute registers are ignored
}

bool Machine::softInt(Cpu &c, uint8_t n) {
	if (n == 0x62) {
		if (int62Hook && int62Hook(*this))
			return true;
	}
	(void)c;
	return false;
}

void Machine::iretFromStub() {
	const uint16_t nip = cpu.pop(), ncs = cpu.pop(), nf = cpu.pop();
	cpu.ip = nip;
	cpu.s[CS] = ncs;
	const uint16_t arith = CF | PF | AF | ZF | SF | OF;
	cpu.flags = (uint16_t)((nf & ~arith) | (cpu.flags & arith) | 0x0002);
}

void Machine::hleStub(Cpu &, uint16_t offset) {
	_skipIret = false;
	switch (offset) {
	case 0x10: int10(); break;
	case 0x16: int16h(); break;
	case 0x1A: int1a(); break;
	case 0x1C: break;
	case 0x20: terminate(0, 0, false, 0); break;
	case 0x21: int21(); break;
	case 0x33: int33(); break;
	case kReturnMarker: return;      // handled by the loops
	default: {
		char b[48];
		snprintf(b, sizeof(b), "HLE stub %02X", offset);
		unhandled(b);
		break;
	}
	}
	if (!_skipIret)
		iretFromStub();
}

// ---------------------------------------------------------------------------------------------
// BIOS: video, keyboard, clock, mouse
// ---------------------------------------------------------------------------------------------

void Machine::int10() {
	const uint8_t ah = (uint8_t)(cpu.r[AX] >> 8), al = (uint8_t)cpu.r[AX];
	switch (ah) {
	case 0x00:
		ega.videoMode = al & 0x7F;
		if (al == 0x10 || al == 0x0E || al == 0x0D) {
			memset(ega.plane, 0, sizeof(ega.plane));
			ega.displayedPage = 0;
		}
		break;
	case 0x05: ega.displayedPage = al & 1; break;
	case 0x0F: cpu.r[AX] = (uint16_t)(0x5000 | ega.videoMode); cpu.r[BX] &= 0x00FF; break;
	case 0x10:
		if (al == 0x02) { for (int i = 0; i < 17; i++) ega.egaPalette[i] = mem[Cpu::lin(cpu.s[ES], (uint16_t)(cpu.r[DX] + i))]; }
		else if (al == 0x00) { ega.egaPalette[(cpu.r[BX] & 0xFF) & 15] = (uint8_t)(cpu.r[BX] >> 8); }
		break;
	case 0x11:
		if (al == 0x30) {
			const uint8_t bh = (uint8_t)(cpu.r[BX] >> 8);
			cpu.s[ES] = 0xC000;
			if (bh == 2) { cpu.r[BP] = 0x0000; cpu.r[CX] = 14; }
			else if (bh == 3) { cpu.r[BP] = 0x1000; cpu.r[CX] = 8; }
			else { cpu.r[BP] = 0; cpu.r[CX] = 16; }
			cpu.r[DX] = (cpu.r[DX] & 0xFF00) | 24;
		}
		break;
	case 0x12:
		if ((cpu.r[BX] & 0xFF) == 0x10) { cpu.r[BX] = 0x0003; cpu.r[CX] = 0x0009; }
		break;
	case 0x1A: cpu.r[AX] = 0x1A1A; cpu.r[BX] = 0x0008; break;     // pretend a VGA is present
	case 0x01: case 0x02: case 0x03: case 0x06: case 0x07: case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0E:
		break;
	default: {
		char b[48];
		snprintf(b, sizeof(b), "INT 10h AH=%02X", ah);
		unhandled(b);
	}
	}
}

void Machine::int16h() {
	const uint8_t ah = (uint8_t)(cpu.r[AX] >> 8);
	if (ah == 0x00 || ah == 0x10) {
		while (_keys.empty() && !quit) { cpu.instructions += 20000; pump(); }
		if (_keys.empty()) { cpu.r[AX] = 0; return; }
		const uint8_t k = _keys.front(); _keys.pop_front();
		cpu.r[AX] = (uint16_t)k;
	} else if (ah == 0x01 || ah == 0x11) {
		if (_keys.empty()) { cpu.flags |= ZF; } else { cpu.flags &= (uint16_t)~ZF; cpu.r[AX] = _keys.front(); }
	} else if (ah == 0x02) {
		cpu.r[AX] &= 0xFF00;
	} else {
		unhandled("INT 16h");
	}
}

void Machine::int1a() {
	const uint8_t ah = (uint8_t)(cpu.r[AX] >> 8);
	if (ah == 0x00) {
		cpu.instructions += 3000;        // a polling loop makes progress in virtual time
		const uint32_t t = ticks();
		cpu.r[CX] = (uint16_t)(t >> 16);
		cpu.r[DX] = (uint16_t)t;
		cpu.r[AX] &= 0xFF00;
	} else {
		unhandled("INT 1Ah");
	}
}

void Machine::int33() {
	switch (cpu.r[AX]) {
	case 0x0000: cpu.r[AX] = 0xFFFF; cpu.r[BX] = 2; _mouseMask = 0; _mouseSeg = 0; break;
	case 0x0003: cpu.r[BX] = _buttons; cpu.r[CX] = (uint16_t)_mouseX; cpu.r[DX] = (uint16_t)_mouseY; break;
	case 0x0005: {
		const int b = cpu.r[BX] & 1;
		cpu.r[AX] = _buttons;
		cpu.r[BX] = (uint16_t)_press[b];
		_press[b] = 0;
		cpu.r[CX] = (uint16_t)_mouseX; cpu.r[DX] = (uint16_t)_mouseY;
		break;
	}
	case 0x000B:
		cpu.r[CX] = (uint16_t)_mickX; cpu.r[DX] = (uint16_t)_mickY;
		_mickX = _mickY = 0;
		break;
	case 0x000C: _mouseMask = cpu.r[CX]; _mouseSeg = cpu.s[ES]; _mouseOff = cpu.r[DX]; break;
	case 0x0001: case 0x0002: case 0x0004: case 0x0007: case 0x0008: case 0x000F: case 0x0010:
		break;
	default: {
		char b[48];
		snprintf(b, sizeof(b), "INT 33h AX=%04X", cpu.r[AX]);
		unhandled(b);
	}
	}
}

// ---------------------------------------------------------------------------------------------
// DOS
// ---------------------------------------------------------------------------------------------

void Machine::int21() {
	const uint8_t ah = (uint8_t)(cpu.r[AX] >> 8), al = (uint8_t)cpu.r[AX];
	switch (ah) {
	case 0x01: case 0x07: case 0x08: {
		while (_keys.empty() && !quit) { cpu.instructions += 20000; pump(); }
		if (_keys.empty()) { cpu.r[AX] = (uint16_t)((ah << 8) | 0); break; }
		const uint8_t k = _keys.front(); _keys.pop_front();
		cpu.r[AX] = (uint16_t)((ah << 8) | k);
		break;
	}
	case 0x02: console += (char)(cpu.r[DX] & 0xFF); break;
	case 0x06:
		if ((cpu.r[DX] & 0xFF) == 0xFF) {
			if (_keys.empty()) { cpu.flags |= ZF; cpu.r[AX] &= 0xFF00; }
			else { cpu.flags &= (uint16_t)~ZF; cpu.r[AX] = (uint16_t)((cpu.r[AX] & 0xFF00) | _keys.front()); _keys.pop_front(); }
		} else {
			console += (char)(cpu.r[DX] & 0xFF);
		}
		break;
	case 0x09: {
		for (int i = 0; i < 4096; i++) {
			const uint8_t c = mem[Cpu::lin(cpu.s[DS], (uint16_t)(cpu.r[DX] + i))];
			if (c == '$') break;
			console += (char)c;
		}
		console += "\n";
		break;
	}
	case 0x0A: {
		const uint16_t seg = cpu.s[DS], off = cpu.r[DX];
		const int max = mem[Cpu::lin(seg, off)];
		int n = 0;
		for (;;) {
			while (_keys.empty() && !quit) { cpu.instructions += 20000; pump(); }
			if (_keys.empty()) break;
			const uint8_t k = _keys.front(); _keys.pop_front();
			if (k == 0x0D) break;
			if (n < max - 1) mem[Cpu::lin(seg, (uint16_t)(off + 2 + n++))] = k;
		}
		mem[Cpu::lin(seg, (uint16_t)(off + 1))] = (uint8_t)n;
		mem[Cpu::lin(seg, (uint16_t)(off + 2 + n))] = 0x0D;
		break;
	}
	case 0x0B: cpu.r[AX] = (uint16_t)((cpu.r[AX] & 0xFF00) | (_keys.empty() ? 0 : 0xFF)); break;
	case 0x0C: {
		_keys.clear();
		if (al == 0x01 || al == 0x06 || al == 0x07 || al == 0x08 || al == 0x0A) {
			cpu.r[AX] = (uint16_t)((al << 8) | al);
			int21();
		}
		break;
	}
	case 0x0E: cpu.r[AX] = (uint16_t)((cpu.r[AX] & 0xFF00) | 26); break;
	case 0x19: cpu.r[AX] = (uint16_t)((cpu.r[AX] & 0xFF00) | 2); break;
	case 0x1A: _dtaSeg = cpu.s[DS]; _dtaOff = cpu.r[DX]; break;
	case 0x2F: cpu.s[ES] = _dtaSeg ? _dtaSeg : pspSeg(); cpu.r[BX] = _dtaSeg ? _dtaOff : 0x80; break;
	case 0x25: cpu.ww(0, (uint16_t)(al * 4), cpu.r[DX]); cpu.ww(0, (uint16_t)(al * 4 + 2), cpu.s[DS]); break;
	case 0x35: cpu.r[BX] = cpu.rw(0, (uint16_t)(al * 4)); cpu.s[ES] = cpu.rw(0, (uint16_t)(al * 4 + 2)); break;
	case 0x2A: cpu.r[CX] = 1994; cpu.r[DX] = 0x0A0F; cpu.r[AX] = (uint16_t)((cpu.r[AX] & 0xFF00) | 3); break;
	case 0x2C: {
		const uint32_t sec = (uint32_t)(cpu.instructions / kInstrPerSecond) + 12 * 3600;
		cpu.r[CX] = (uint16_t)(((sec / 3600 % 24) << 8) | (sec / 60 % 60));
		cpu.r[DX] = (uint16_t)(((sec % 60) << 8) | (uint16_t)(cpu.instructions * 100 / kInstrPerSecond % 100));
		cpu.instructions += 1500;
		break;
	}
	case 0x30: cpu.r[AX] = 0x0005; cpu.r[BX] = 0; cpu.r[CX] = 0; break;
	case 0x31: terminate(al, 3, true, cpu.r[DX]); break;
	case 0x36: cpu.r[AX] = 4; cpu.r[BX] = 10000; cpu.r[CX] = 512; cpu.r[DX] = 20000; break;
	case 0x39: case 0x3B: setCarry(false); break;
	case 0x3C: case 0x3D: {
		const std::string dos = readDosString(cpu.s[DS], cpu.r[DX]);
		std::string hp = hostPath(dos);
		FILE *f = 0;
		if (ah == 0x3C) {
			f = fopen(hp.c_str(), "w+b");
		} else {
			const int mode = al & 3;
			f = fopen(hp.c_str(), mode == 0 ? "rb" : "r+b");
		}
		if (!f) { log += "open failed: " + dos + "\n"; dosError(2); break; }
		const int sft = sftAlloc(f, hp, false);
		const int h = jftAssign(sft, true);
		if (h < 0) { sftRelease(sft); dosError(4); break; }
		if (h >= 20 && !_procs.empty())
			_procs.back().extraOpen.push_back(sft);
		cpu.r[AX] = (uint16_t)h;
		setCarry(false);
		break;
	}
	case 0x3E: {
		const int sft = jftLookup(cpu.r[BX]);
		if (sft < 0) { dosError(6); break; }
		const uint16_t psp = pspSeg();
		mem[Cpu::lin(cpu.rw(psp, 0x36), (uint16_t)(cpu.rw(psp, 0x34) + cpu.r[BX]))] = 0xFF;
		sftRelease(sft);
		setCarry(false);
		break;
	}
	case 0x3F: {
		const int sft = jftLookup(cpu.r[BX]);
		if (sft < 0) { dosError(6); break; }
		if (_sft[sft].console || !_sft[sft].f) { cpu.r[AX] = 0; setCarry(false); break; }
		std::vector<uint8_t> buf(cpu.r[CX]);
		const size_t n = cpu.r[CX] ? fread(&buf[0], 1, cpu.r[CX], _sft[sft].f) : 0;
		for (size_t i = 0; i < n; i++)
			mem[Cpu::lin(cpu.s[DS], (uint16_t)(cpu.r[DX] + i))] = buf[i];
		cpu.r[AX] = (uint16_t)n;
		setCarry(false);
		break;
	}
	case 0x40: {
		const int sft = jftLookup(cpu.r[BX]);
		if (sft < 0) { dosError(6); break; }
		if (_sft[sft].console || !_sft[sft].f) {
			for (int i = 0; i < cpu.r[CX]; i++) console += (char)mem[Cpu::lin(cpu.s[DS], (uint16_t)(cpu.r[DX] + i))];
			cpu.r[AX] = cpu.r[CX];
			setCarry(false);
			break;
		}
		if (cpu.r[CX] == 0) {
			// a zero-length write truncates the file at the current position
			fflush(_sft[sft].f);
			const long pos = ftell(_sft[sft].f);
#ifdef _WIN32
			_chsize(_fileno(_sft[sft].f), pos);
#else
			if (ftruncate(fileno(_sft[sft].f), pos) != 0) {}
#endif
			cpu.r[AX] = 0;
			setCarry(false);
			break;
		}
		std::vector<uint8_t> buf(cpu.r[CX]);
		for (int i = 0; i < cpu.r[CX]; i++) buf[i] = mem[Cpu::lin(cpu.s[DS], (uint16_t)(cpu.r[DX] + i))];
		const size_t n = fwrite(&buf[0], 1, buf.size(), _sft[sft].f);
		fflush(_sft[sft].f);
		cpu.r[AX] = (uint16_t)n;
		setCarry(false);
		break;
	}
	case 0x41: {
		const std::string hp = hostPath(readDosString(cpu.s[DS], cpu.r[DX]));
		if (remove(hp.c_str()) == 0) setCarry(false); else dosError(2);
		break;
	}
	case 0x42: {
		const int sft = jftLookup(cpu.r[BX]);
		if (sft < 0 || !_sft[sft].f) { dosError(6); break; }
		const long off = (long)(((uint32_t)cpu.r[CX] << 16) | cpu.r[DX]);
		fseek(_sft[sft].f, off, al == 0 ? SEEK_SET : (al == 1 ? SEEK_CUR : SEEK_END));
		const long pos = ftell(_sft[sft].f);
		cpu.r[DX] = (uint16_t)((uint32_t)pos >> 16);
		cpu.r[AX] = (uint16_t)pos;
		setCarry(false);
		break;
	}
	case 0x43: {
		const std::string hp = hostPath(readDosString(cpu.s[DS], cpu.r[DX]));
		FILE *f = fopen(hp.c_str(), "rb");
		if (f) { fclose(f); cpu.r[CX] = 0x20; setCarry(false); }
		else {
#ifdef _WIN32
			DWORD a = GetFileAttributesA(hp.c_str());
			if (a != INVALID_FILE_ATTRIBUTES) { cpu.r[CX] = (a & FILE_ATTRIBUTE_DIRECTORY) ? 0x10 : 0x20; setCarry(false); }
			else dosError(2);
#else
			dosError(2);
#endif
		}
		break;
	}
	case 0x44:
		if (al == 0x00) { cpu.r[DX] = (cpu.r[BX] < 5) ? 0x80D3 : 0x0002; setCarry(false); }
		else { setCarry(false); }
		break;
	case 0x45: {
		const int sft = jftLookup(cpu.r[BX]);
		if (sft < 0) { dosError(6); break; }
		_sft[sft].refs++;
		const int h = jftAssign(sft, true);
		if (h < 0) { dosError(4); break; }
		cpu.r[AX] = (uint16_t)h;
		setCarry(false);
		break;
	}
	case 0x47: mem[Cpu::lin(cpu.s[DS], cpu.r[SI])] = 0; setCarry(false); break;
	case 0x48: {
		uint16_t largest = 0;
		const uint16_t seg = memAlloc(cpu.r[BX], pspSeg(), &largest);
		if (seg) { cpu.r[AX] = seg; setCarry(false); }
		else { cpu.r[BX] = largest; dosError(8); }
		break;
	}
	case 0x49:
		if (memFree(cpu.s[ES])) setCarry(false); else dosError(9);
		break;
	case 0x4A: {
		uint16_t maxAvail = 0;
		if (memResize(cpu.s[ES], cpu.r[BX], &maxAvail)) setCarry(false);
		else { cpu.r[BX] = maxAvail; dosError(8); }
		break;
	}
	case 0x4B: {
		if (al != 0x00) { dosError(1); break; }
		const std::string path = readDosString(cpu.s[DS], cpu.r[DX]);
		const uint16_t pseg = cpu.s[ES], poff = cpu.r[BX];
		const int rc = runChild(path, pseg, poff);
		_skipIret = false;           // the child's exit set it; the parent's own INT 21h returns normally
		if (rc != 0) { dosError(2); break; }
		setCarry(false);
		break;
	}
	case 0x4C: terminate(al, 0, false, 0); break;
	case 0x4D: cpu.r[AX] = (uint16_t)((_lastExitType << 8) | _lastExit); break;
	case 0x4E: case 0x4F: {
		if (ah == 0x4E) {
			std::string pat = readDosString(cpu.s[DS], cpu.r[DX]);
			_findAttr = (uint8_t)cpu.r[CX];
			std::string dir = pat, mask = pat;
			const size_t sl = pat.find_last_of("\\/");
			if (sl != std::string::npos) { dir = pat.substr(0, sl); mask = pat.substr(sl + 1); } else { dir = ""; }
			_findList.clear();
			_findPos = 0;
#ifdef _WIN32
			WIN32_FIND_DATAA fd;
			const std::string spec = hostPath(dir) + "/*";
			HANDLE h = FindFirstFileA(spec.c_str(), &fd);
			if (h != INVALID_HANDLE_VALUE) {
				do {
					std::string n = fd.cFileName;
					if (n == "." || n == "..") continue;
					const bool isDir = fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
					if (isDir && !(_findAttr & 0x10)) continue;
					if (!wildMatch(mask, n)) continue;
					std::string up;
					for (size_t i = 0; i < n.size(); i++) up += (char)toupper((unsigned char)n[i]);
					_findList.push_back(std::string(isDir ? "D" : "F") + up + "|" + std::to_string(fd.nFileSizeLow));
				} while (FindNextFileA(h, &fd));
				FindClose(h);
			}
#else
			DIR *d = opendir(hostPath(dir).c_str());
			if (d) {
				struct dirent *de;
				while ((de = readdir(d)) != 0) {
					std::string n = de->d_name;
					if (n == "." || n == "..") continue;
					if (!wildMatch(mask, n)) continue;
					std::string up;
					for (size_t i = 0; i < n.size(); i++) up += (char)toupper((unsigned char)n[i]);
					_findList.push_back(std::string("F") + up + "|0");
				}
				closedir(d);
			}
#endif
			std::sort(_findList.begin(), _findList.end());
		}
		if (_findPos >= _findList.size()) { dosError(ah == 0x4E ? 2 : 18); break; }
		const std::string e = _findList[_findPos++];
		const bool isDir = e[0] == 'D';
		const size_t bar = e.find('|');
		const std::string name = e.substr(1, bar - 1);
		const uint32_t size = (uint32_t)atol(e.c_str() + bar + 1);
		const uint16_t seg = _dtaSeg ? _dtaSeg : pspSeg(), off = _dtaSeg ? _dtaOff : 0x80;
		mem[Cpu::lin(seg, (uint16_t)(off + 0x15))] = isDir ? 0x10 : 0x20;
		cpu.ww(seg, (uint16_t)(off + 0x16), 0x6000);
		cpu.ww(seg, (uint16_t)(off + 0x18), 0x1E21);
		cpu.ww(seg, (uint16_t)(off + 0x1A), (uint16_t)size);
		cpu.ww(seg, (uint16_t)(off + 0x1C), (uint16_t)(size >> 16));
		for (size_t i = 0; i < 13; i++)
			mem[Cpu::lin(seg, (uint16_t)(off + 0x1E + i))] = i < name.size() ? (uint8_t)name[i] : 0;
		if (name.size() < 13) mem[Cpu::lin(seg, (uint16_t)(off + 0x1E + name.size()))] = 0;
		setCarry(false);
		break;
	}
	case 0x51: case 0x62: cpu.r[BX] = pspSeg(); break;
	case 0x56: {
		const std::string a = hostPath(readDosString(cpu.s[DS], cpu.r[DX])), b = hostPath(readDosString(cpu.s[ES], cpu.r[DI]));
		if (rename(a.c_str(), b.c_str()) == 0) setCarry(false); else dosError(5);
		break;
	}
	case 0x67: {
		const uint16_t count = cpu.r[BX];
		const uint16_t psp = pspSeg();
		const uint16_t cur = cpu.rw(psp, 0x32);
		if (count <= cur) { cpu.ww(psp, 0x32, count); setCarry(false); break; }
		const uint16_t seg = memAlloc((uint16_t)((count + 15) / 16), psp, 0);
		if (!seg) { dosError(8); break; }
		const uint16_t off = cpu.rw(psp, 0x34), oseg = cpu.rw(psp, 0x36);
		for (int i = 0; i < count; i++)
			mem[Cpu::lin(seg, (uint16_t)i)] = i < cur ? mem[Cpu::lin(oseg, (uint16_t)(off + i))] : 0xFF;
		cpu.ww(psp, 0x32, count);
		cpu.ww(psp, 0x34, 0);
		cpu.ww(psp, 0x36, seg);
		setCarry(false);
		break;
	}
	default: {
		char b[48];
		snprintf(b, sizeof(b), "INT 21h AH=%02X", ah);
		unhandled(b);
		dosError(1);
	}
	}
}

} // namespace Yemu
