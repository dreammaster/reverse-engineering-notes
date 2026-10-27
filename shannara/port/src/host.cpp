#include "host.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <thread>

namespace shannara {

void registerAllGenerated(Machine &m);
void installCrtShims(Machine &m);

static std::string lowerStr(std::string s) {
	for (char &c : s)
		c = (char)tolower((unsigned char)c);
	return s;
}

Host::Host(const std::string &exePath, const std::string &gameDir, const std::string &picDir, const std::string &saveDir)
    : _gameDir(gameDir), _picDir(picDir), _saveDir(saveDir) {
	_exe = new LeExe(readFile(exePath));
	_m = new Machine(*_exe, kMemSize, kBase);
	registerAllGenerated(*_m);
	_m->w32(0x63, 0x3D4);   // BIOS data area 0040:0063 (flat model: segment ignored): CRT controller base port, read by video_sync to find the status register
	std::filesystem::create_directories(saveDir);
	std::vector<std::string> dirs = {gameDir, picDir};
	if (const char *extra = getenv("HOST_EXTRA_DIRS")) { // more directories searched by base name (extracted FLICS archives ...), separated by ';'
		std::string e = extra;
		size_t pos = 0;
		while (pos < e.size()) {
			size_t q = e.find(';', pos);
			dirs.push_back(e.substr(pos, q == std::string::npos ? std::string::npos : q - pos));
			if (q == std::string::npos) break;
			pos = q + 1;
		}
	}
	for (const std::string &d : dirs) {
		std::error_code ec;
		for (auto &e : std::filesystem::directory_iterator(d, ec))
			if (e.is_regular_file())
				_dirIndex.emplace(lowerStr(e.path().filename().string()), e.path().string());
	}
	installShims();
	_m->onInterrupt = [this](Machine &, int n) { interrupt(n); };
	_m->onPortIn = [this](Machine &, uint16_t p, int b) { return portIn(p, b); };
	_m->onPortOut = [this](Machine &, uint16_t p, uint32_t v, int b) { portOut(p, v, b); };
}

uint32_t Host::callGuest(uint32_t addr, std::initializer_list<uint32_t> args) {
	Machine &m = *_m;
	uint32_t seax = m.eax, secx = m.ecx, sedx = m.edx, sebx = m.ebx, sesi = m.esi, sedi = m.edi, sebp = m.ebp, sesp = m.esp;
	bool zf = m.zf, sf = m.sf, cf = m.cf, of = m.of, df = m.df;
	for (auto it = args.end(); it != args.begin();)
		m.push(*--it);
	m.push(0);
	m.call(addr);
	uint32_t r = m.eax;
	m.esp = sesp;
	m.eax = seax; m.ecx = secx; m.edx = sedx; m.ebx = sebx; m.esi = sesi; m.edi = sedi; m.ebp = sebp;
	m.zf = zf; m.sf = sf; m.cf = cf; m.of = of; m.df = df;
	return r;
}

static uint64_t nowUs() {
	return (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

// called by Machine::tick every few thousand instructions: run due AIL timer callbacks and finished-music callbacks as if an interrupt had occurred here
void Host::serviceInterrupts(Machine &) {
	if (_inIrq)
		return;
	_inIrq = true;
	uint64_t now = nowUs();
	for (size_t i = 0; i < _timers.size(); i++) {
		Timer &t = _timers[i];
		if (!t.used || !t.running || t.cb == 0)
			continue;
		int fires = 0;
		while (now >= t.nextUs && fires < 4) {
			t.nextUs += (uint64_t)t.periodUs;
			fires++;
		}
		if (now >= t.nextUs)
			t.nextUs = now + (uint64_t)t.periodUs; // fell far behind: drop the missed ticks
		for (int k = 0; k < fires; k++)
			callGuest(t.cb, {t.user});
	}
	std::vector<std::pair<uint32_t, uint32_t>> cbs;
	_audio.takeCallbacks(cbs);
	for (auto &cb : cbs)
		callGuest(cb.first, {cb.second});
	_inIrq = false;
}

std::string Host::mp3Path(int id) {
	char name[32];
	snprintf(name, sizeof name, "%04d.mp3", id);
	auto it = _dirIndex.find(name);
	return it == _dirIndex.end() ? std::string() : it->second;
}

void Host::fillDta(const Found &f) { // DOS disk transfer area: attribute at +15h, time/date, size at +1Ah, 8.3 name at +1Eh
	Machine &m = *_m;
	m.w8(_dta + 0x15, 0x20);
	m.w16(_dta + 0x16, 0);
	m.w16(_dta + 0x18, 0x21);
	m.w32(_dta + 0x1A, f.size);
	std::string n = f.name;
	for (char &c : n)
		c = (char)toupper((unsigned char)c);
	n.resize(std::min<size_t>(n.size(), 12));
	for (size_t i = 0; i <= n.size(); i++)
		m.w8(_dta + 0x1E + (uint32_t)i, i < n.size() ? (uint8_t)n[i] : 0);
}

uint32_t Host::callGuestEax(uint32_t addr, uint32_t eax) {
	Machine &m = *_m;
	uint32_t secx = m.ecx, sedx = m.edx, sebx = m.ebx, sesi = m.esi, sedi = m.edi, sebp = m.ebp, sesp = m.esp, seax = m.eax;
	bool zf = m.zf, sf = m.sf, cf = m.cf, of = m.of, df = m.df;
	m.eax = eax;
	m.push(0);
	m.call(addr);
	uint32_t r = m.eax;
	m.esp = sesp;
	m.eax = seax; m.ecx = secx; m.edx = sedx; m.ebx = sebx; m.esi = sesi; m.edi = sedi; m.ebp = sebp;
	m.zf = zf; m.sf = sf; m.cf = cf; m.of = of; m.df = df;
	return r;
}

void Host::pump() {
	std::vector<std::pair<uint32_t, uint32_t>> cbs;
	_audio.takeCallbacks(cbs);
	for (auto &cb : cbs)
		callGuest(cb.first, {cb.second}); // the sequence-end callbacks of the music
	if (!onPump)
		return;
	auto now = (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	if (now - _lastPump < 8)
		return;
	_lastPump = now;
	onPump();
}

void Host::log(const std::string &s) {
	if (verbose)
		fprintf(stderr, "%s\n", s.c_str());
}

std::string Host::describe(uint32_t addr) {
	auto it = _names.upper_bound(addr);
	if (it == _names.begin())
		return "?";
	--it;
	char b[64];
	snprintf(b, sizeof b, "+0x%X", addr - it->first);
	return it->second + (addr == it->first ? "" : b);
}

void Host::loadNames(const std::string &tsv) {
	FILE *f = fopen(tsv.c_str(), "r");
	if (!f)
		return;
	char line[512];
	bool first = true;
	while (fgets(line, sizeof line, f)) {
		if (first) {
			first = false;
			continue;
		}
		unsigned a = 0;
		char name[256] = {};
		if (sscanf(line, "0x%x	%255[^	]", &a, name) == 2)
			_names[a + kBase] = name, _byName[name] = a + kBase;
	}
	fclose(f);
}

void Host::watch(uint32_t lo, uint32_t hi, int maxReports, const std::string &chainFilter) {
	_m->watchLo = lo;
	_m->watchHi = hi;
	_m->wlogOn = true;
	_m->wlogStore = false;
	auto n = std::make_shared<int>(0);
	_m->onWatch = [this, n, maxReports, chainFilter](uint32_t a, uint8_t v) {
		if (*n >= maxReports)
			return;
		std::string chain;
		for (uint32_t f : _m->trace)
			chain += describe(f) + " > ";
		if (!chainFilter.empty() && chain.find(chainFilter) == std::string::npos)
			return;
		if (const char *wv = getenv("HOST_WATCH_VALUE"))
			if (v != (uint8_t)strtoul(wv, nullptr, 16))
				return;
		(*n)++;
		fprintf(stderr, "WATCH write %02X to %X by %s\n", v, a, chain.c_str());
		if (getenv("HOST_WATCH_STOP"))
			throw FormatError("watch stop");
	};
}

void Host::traceAddress(uint32_t target, int count) {
	auto n = std::make_shared<int>(0);
	auto prev = _m->callHook;
	_m->callHook = [this, n, target, count, prev](Machine &m, uint32_t addr) {
		if (prev)
			prev(m, addr);
		if (addr != target || *n >= count)
			return;
		(*n)++;
		fprintf(stderr, "CALL %X from %s: eax=%X edx=%X ebx=%X ecx=%X esi=%X edi=%X stack: %X %X %X %X %X\n", addr, describe(m.r32(m.esp)).c_str(), m.eax, m.edx, m.ebx, m.ecx, m.esi, m.edi,
		        m.r32(m.esp + 4), m.r32(m.esp + 8), m.r32(m.esp + 12), m.r32(m.esp + 16), m.r32(m.esp + 20));
		if (const char *ps = getenv("HOST_PEEKSTR")) { // HOST_PEEKSTR=eax|edx|ebx|ecx: show the C string that register points to
			uint32_t v = !strcmp(ps, "eax") ? m.eax : !strcmp(ps, "edx") ? m.edx : !strcmp(ps, "ebx") ? m.ebx : m.ecx;
			std::string t;
			for (uint32_t i = 0; i < 80 && v + i < kMemSize && m.r8(v + i); i++)
				t.push_back(m.r8(v + i) >= 32 && m.r8(v + i) < 127 ? (char)m.r8(v + i) : '?');
			fprintf(stderr, "   string at %s: \"%s\"\n", ps, t.c_str());
		}
		if (const char *pm = getenv("HOST_PEEKMEM")) { // HOST_PEEKMEM=eax|edx|ebx|ecx: dump 12 dwords at the address in that register
			uint32_t v = !strcmp(pm, "eax") ? m.eax : !strcmp(pm, "edx") ? m.edx : !strcmp(pm, "ebx") ? m.ebx : m.ecx;
			fprintf(stderr, "   dwords at %s (%X):", pm, v);
			for (int i = 0; i < 12 && v + 4 * i + 4 <= kMemSize; i++)
				fprintf(stderr, " %X", m.r32(v + 4 * i));
			fprintf(stderr, "\n");
		}
		if (const char *pk = getenv("HOST_PEEK"))
			fprintf(stderr, "   [%s] = %X\n", pk, m.r32((uint32_t)strtoul(pk, nullptr, 16)));
	};
}

void Host::traceCalls(uint64_t first, uint64_t count) {
	auto n = std::make_shared<uint64_t>(0);
	_m->callHook = [this, n, first, count](Machine &m, uint32_t addr) {
		uint64_t k = (*n)++;
		if (k < first || k >= first + count)
			return;
		fprintf(stderr, "[%llu] %s  eax=%X edx=%X ebx=%X ecx=%X\n", (unsigned long long)k, describe(addr).c_str(), m.eax, m.edx, m.ebx, m.ecx);
	};
}

// ---- file system ---------------------------------------------------------------------------------------------------------------------------------------------

std::string Host::dosPath(uint32_t addr) {
	return _m->cstr(addr, 260);
}

// DOS path -> host path: the game uses "C:\MOS\X.EXT", ".\VOCS\Y.MP3" ...; everything resolves by base name in the game directory or the extracted picture directory.
// Files that are written (savegames) go to the save directory.
std::string Host::resolve(const std::string &dosName, bool forWrite) {
	size_t cut = dosName.find_last_of("\\/:");
	std::string base = cut == std::string::npos ? dosName : dosName.substr(cut + 1);
	std::string key = lowerStr(base);
	if (forWrite)
		return _saveDir + "/" + base;
	std::error_code ec;
	for (const auto &e : std::filesystem::directory_iterator(_saveDir, ec))
		if (lowerStr(e.path().filename().string()) == key)
			return e.path().string();
	auto it = _dirIndex.find(key);
	return it != _dirIndex.end() ? it->second : std::string();
}

// ---- memory arenas -------------------------------------------------------------------------------------------------------------------------------------------

uint32_t Host::dosAlloc(uint32_t bytes) {
	bytes = (bytes + 15) & ~15u;
	if (_dosNextB + bytes <= kDosArenaBEnd) {
		uint32_t p = _dosNextB;
		_dosNextB += bytes;
		return p;
	}
	if (_dosNextA + bytes <= kDosArenaAEnd) {
		uint32_t p = _dosNextA;
		_dosNextA += bytes;
		return p;
	}
	return 0;
}

uint32_t Host::dosLargest() const {
	return std::max(kDosArenaBEnd - _dosNextB, kDosArenaAEnd - _dosNextA);
}

uint32_t Host::heapAlloc(uint32_t bytes) {
	bytes = (bytes + 4095) & ~4095u;
	if (_heapNext + bytes > kHeapBase + 0x1000000)
		return 0;
	uint32_t p = _heapNext;
	_heapNext += bytes;
	_blocks[p] = bytes;
	return p;
}

// ---- VGA ports -----------------------------------------------------------------------------------------------------------------------------------------------

uint32_t Host::portIn(uint16_t port, int) {
	switch (port) {
	case 0x3DA: { // input status 1: bit 3 = vertical retrace, bit 0 = display enable; toggles so that waits terminate
		uint32_t v = (((_retrace >> 3) & 1) ? 8u : 0u) | (_retrace & 1);
		_retrace++;
		if ((_retrace & 0xFF) == 0)
			pump();
		return v;
	}
	case 0x3C9: {
		uint32_t v = _dac[(_dacRead * 3 + _dacReadComp) % 768];
		if (++_dacReadComp == 3) {
			_dacReadComp = 0;
			_dacRead = (_dacRead + 1) & 255;
		}
		return v;
	}
	default:
		return 0;
	}
}

void Host::portOut(uint16_t port, uint32_t v, int) {
	switch (port) {
	case 0x3C8:
		_dacIndex = v & 255;
		_dacComp = 0;
		break;
	case 0x3C7:
		_dacRead = v & 255;
		_dacReadComp = 0;
		break;
	case 0x3C9:
		_dac[_dacIndex * 3 + _dacComp] = (uint8_t)(v & 63);
		if (++_dacComp == 3) {
			_dacComp = 0;
			_dacIndex = (_dacIndex + 1) & 255;
		}
		break;
	default:
		break;
	}
}

void Host::flushVram() {
	// the window shows 64 KB of the frame store starting at bank * 64 KB; the gx code writes there directly
	memcpy(_m->ptr(kFrameStore) + (size_t)_bank * 65536, _m->ptr(kVramWindow), 65536);
}

// ---- VESA BIOS (reached through DPMI "simulate real-mode interrupt") -----------------------------------------------------------------------------------------

void Host::vbe(uint32_t rmcs) {
	Machine &m = *_m;
	uint32_t ax = m.r32(rmcs + 0x1C) & 0xFFFF;
	uint32_t bx = m.r32(rmcs + 0x10), dx = m.r32(rmcs + 0x14), di = m.r32(rmcs + 0);
	uint32_t es = m.r16(rmcs + 0x22);
	uint32_t buf = es * 16 + (di & 0xFFFF);
	auto done = [&](uint32_t result) {
		m.w32(rmcs + 0x1C, (m.r32(rmcs + 0x1C) & 0xFFFF0000u) | result);
		m.w16(rmcs + 0x20, 0);
	};
	log("VBE AX=" + std::to_string(ax));
	switch (ax) {
	case 0x4F00: { // controller information
		memset(m.ptr(buf), 0, 256);
		memcpy(m.ptr(buf), "VESA", 4);
		m.w16(buf + 4, 0x0200);
		done(0x004F);
		break;
	}
	case 0x4F01: { // mode information: only the 640x480x256 window geometry matters (granularity/size in KB, window segment, pitch, resolution)
		memset(m.ptr(buf), 0, 256);
		m.w16(buf + 0, 0x0007);  // attributes: supported, extended info, colour
		m.w8(buf + 2, 7);        // window A: exists, readable, writeable
		m.w16(buf + 4, 64);      // granularity KB
		m.w16(buf + 6, 64);      // window size KB
		m.w16(buf + 8, 0xA000);  // window A segment
		m.w16(buf + 16, 640);    // bytes per scan line
		m.w16(buf + 18, 640);
		m.w16(buf + 20, 480);
		m.w8(buf + 22, 8);
		m.w8(buf + 23, 16);
		m.w8(buf + 24, 1);
		m.w8(buf + 25, 8);       // bits per pixel
		m.w8(buf + 26, 1);       // banks
		m.w8(buf + 27, 4);       // memory model: packed pixel
		m.w8(buf + 28, 64);      // bank size KB
		done(0x004F);
		break;
	}
	case 0x4F02: { // set mode
		_mode = (int)(bx & 0x1FF);
		_width = 640;
		_height = 480;
		memset(m.ptr(kFrameStore), 0, 0x100000);
		memset(m.ptr(kVramWindow), 0, 65536);
		_bank = 0;
		done(0x004F);
		break;
	}
	case 0x4F05: { // window control
		if ((bx >> 8) == 0) {
			flushVram();
			_bank = (int)(dx & 0xFFFF);
			if (_bank > 15)
				_bank = 15;
			memcpy(m.ptr(kVramWindow), m.ptr(kFrameStore) + (size_t)_bank * 65536, 65536);
		} else {
			m.w32(rmcs + 0x14, (m.r32(rmcs + 0x14) & 0xFFFF0000u) | (uint32_t)_bank);
		}
		done(0x004F);
		break;
	}
	default:
		log("unhandled VBE call " + std::to_string(ax));
		done(0x014F);
		break;
	}
}

// ---- interrupts ----------------------------------------------------------------------------------------------------------------------------------------------

void Host::interrupt(int n) {
	Machine &m = *_m;
	switch (n) {
	case 0x21: dos21(); break;
	case 0x31: dpmi31(); break;
	case 0x10: int10(); break;
	case 0x33: mouse33(); break;
	case 0x16:
		m.cf = false;
		m.eax = _keys.empty() ? 0 : (uint32_t)_keys.front();
		break;
	default:
		log("unhandled INT " + std::to_string(n));
		m.cf = true;
		break;
	}
}

void Host::int10() {
	Machine &m = *_m;
	uint32_t ah = (m.eax >> 8) & 0xFF;
	if (ah == 0x00) {
		_mode = (int)(m.eax & 0xFF);
		if (_mode == 0x13) {
			_width = 320;
			_height = 200;
		}
	} else if (ah == 0x0F) {
		m.eax = (m.eax & 0xFFFF0000u) | 0x5000u | (uint32_t)_mode;
	}
}

void Host::setMouse(int x, int y, int buttons) {
	for (int b = 0; b < 3; b++) {
		bool was = (_mb >> b) & 1, now = (buttons >> b) & 1;
		if (!was && now) {
			_pressCount[b]++;
			_pressX[b] = x;
			_pressY[b] = y;
		} else if (was && !now) {
			_releaseCount[b]++;
			_releaseX[b] = x;
			_releaseY[b] = y;
		}
	}
	_mx = x;
	_my = y;
	_mb = buttons;
}

void Host::mouse33() {
	Machine &m = *_m;
	pump();
	uint32_t ax = m.eax & 0xFFFF;
	switch (ax) {
	case 0: // reset: installed, 2 buttons
		m.eax = 0xFFFF;
		m.ebx = 2;
		break;
	case 3: // position and buttons
		{
			static int lastLogged = -1;
			if (verbose && _mb != lastLogged) {
				lastLogged = _mb;
				log("mouse poll: buttons " + std::to_string(_mb) + " at " + std::to_string(_mx) + "," + std::to_string(_my));
			}
		}
		m.ebx = (uint32_t)_mb;
		m.ecx = (uint32_t)_mx;
		m.edx = (uint32_t)_my;
		break;
	case 5: case 6: { // button press / release information for button BX: status in AX, count in BX, position at the last press/release in CX/DX; counters reset
		int b = (int)(m.ebx & 0xFFFF);
		if (b < 0 || b > 2)
			b = 0;
		m.eax = (uint32_t)_mb;
		if (ax == 5) {
			m.ebx = (uint32_t)_pressCount[b];
			m.ecx = (uint32_t)_pressX[b];
			m.edx = (uint32_t)_pressY[b];
			_pressCount[b] = 0;
		} else {
			m.ebx = (uint32_t)_releaseCount[b];
			m.ecx = (uint32_t)_releaseX[b];
			m.edx = (uint32_t)_releaseY[b];
			_releaseCount[b] = 0;
		}
		break;
	}
	case 0x0B: { // motion counters (mickeys) since the last call
		m.ecx = (uint32_t)(_mx - _lastMotionX) & 0xFFFF;
		m.edx = (uint32_t)(_my - _lastMotionY) & 0xFFFF;
		_lastMotionX = _mx;
		_lastMotionY = _my;
		break;
	}
	case 4:
		_mx = (int)(m.ecx & 0xFFFF);
		_my = (int)(m.edx & 0xFFFF);
		break;
	default:
		break;
	}
}

void Host::dpmi31() {
	Machine &m = *_m;
	uint32_t ax = m.eax & 0xFFFF;
	log("DPMI AX=" + std::to_string(ax));
	m.cf = false;
	switch (ax) {
	case 0x0000: // allocate LDT descriptors
		m.eax = (m.eax & 0xFFFF0000u) | 0x0100;
		break;
	case 0x0006: // segment base
		m.ecx = 0;
		m.edx = 0;
		break;
	case 0x0007: case 0x0008: case 0x0009: case 0x000A: case 0x000B:
		break;
	case 0x0100: { // allocate DOS memory: BX paragraphs -> AX segment, DX selector
		uint32_t p = dosAlloc((m.ebx & 0xFFFF) * 16);
		log("  DOS block request " + std::to_string((m.ebx & 0xFFFF) * 16) + " bytes -> " + std::to_string(p));
		if (!p) {
			m.cf = true;
			m.eax = (m.eax & 0xFFFF0000u) | 8;
			m.ebx = (m.ebx & 0xFFFF0000u) | ((dosLargest() >> 4) & 0xFFFF);
		} else {
			m.eax = (m.eax & 0xFFFF0000u) | (p >> 4);
			m.edx = (m.edx & 0xFFFF0000u) | 0x0108;
		}
		break;
	}
	case 0x0101:
		break;
	case 0x0200:
		m.ecx = m.ecx & 0xFFFF0000u;
		m.edx = m.edx & 0xFFFF0000u;
		break;
	case 0x0201: case 0x0205:
		break;
	case 0x0204:
		m.ecx = 0x0F;
		m.edx = 0;
		break;
	case 0x0300: // simulate a real-mode interrupt: BL = interrupt number, ES:EDI = register structure
		if ((m.ebx & 0xFF) == 0x10)
			vbe(m.edi);
		else
			log("unhandled real-mode INT " + std::to_string(m.ebx & 0xFF));
		break;
	case 0x0301: case 0x0302:
		break;
	case 0x0303: // real-mode callback address
		m.ecx = (m.ecx & 0xFFFF0000u) | 0xF000;
		m.edx = (m.edx & 0xFFFF0000u) | 0x1000;
		break;
	case 0x0304:
		break;
	case 0x0400:
		m.eax = 0x0090;
		m.ebx = 3;
		m.ecx = 4;
		m.edx = 0x0102;
		break;
	case 0x0500: { // free memory information, ES:EDI buffer
		uint32_t b = m.edi;
		uint32_t avail = kHeapBase + 0x1000000 - _heapNext;
		for (int k = 0; k < 12; k++)
			m.w32(b + 4 * k, 0xFFFFFFFFu);
		m.w32(b, avail);
		m.w32(b + 4, avail / 4096);
		m.w32(b + 8, avail / 4096);
		break;
	}
	case 0x0501: { // allocate memory block: BX:CX size -> BX:CX linear address, SI:DI handle
		uint32_t size = ((m.ebx & 0xFFFF) << 16) | (m.ecx & 0xFFFF);
		uint32_t p = heapAlloc(size);
		if (!p) {
			m.cf = true;
			m.eax = (m.eax & 0xFFFF0000u) | 0x8012;
		} else {
			m.ebx = (m.ebx & 0xFFFF0000u) | (p >> 16);
			m.ecx = (m.ecx & 0xFFFF0000u) | (p & 0xFFFF);
			m.esi = (m.esi & 0xFFFF0000u) | (p >> 16);
			m.edi = (m.edi & 0xFFFF0000u) | (p & 0xFFFF);
		}
		break;
	}
	case 0x0502:
		break;
	case 0x0503: // resize: not supported
		m.cf = true;
		m.eax = (m.eax & 0xFFFF0000u) | 0x8012;
		break;
	case 0x0600: case 0x0601: case 0x0602: case 0x0603: case 0x0604:
		break;
	case 0x0800: { // map physical address: BX:CX -> linear (the VGA window maps to our VRAM window)
		uint32_t phys = ((m.ebx & 0xFFFF) << 16) | (m.ecx & 0xFFFF);
		uint32_t lin = (phys >= 0xA0000 && phys < 0xB0000) ? kVramWindow + (phys - 0xA0000) : phys;
		m.ebx = (m.ebx & 0xFFFF0000u) | (lin >> 16);
		m.ecx = (m.ecx & 0xFFFF0000u) | (lin & 0xFFFF);
		break;
	}
	default:
		log("unhandled DPMI function " + std::to_string(ax));
		m.cf = true;
		m.eax = (m.eax & 0xFFFF0000u) | 0x8001;
		break;
	}
}

void Host::dos21() {
	Machine &m = *_m;
	uint32_t ah = (m.eax >> 8) & 0xFF;
	uint32_t al = m.eax & 0xFF;
	m.cf = false;
	auto fail = [&](uint32_t code) {
		m.cf = true;
		m.eax = (m.eax & 0xFFFF0000u) | code;
	};
	auto fileOf = [&](uint32_t handle) -> FILE * {
		if (handle < 5 || handle - 5 >= _files.size())
			return nullptr;
		return _files[handle - 5].f;
	};
	switch (ah) {
	case 0x0B:
		pump();
		m.eax = (m.eax & 0xFFFFFF00u) | ((_keys.empty() && _scanPending < 0) ? 0u : 0xFFu);
		break;
	case 0x01: case 0x07: case 0x08: {
		pump();
		int k = 0;
		if (_scanPending >= 0) { // second half of an extended key
			k = _scanPending;
			_scanPending = -1;
		} else if (!_keys.empty()) {
			int e = _keys.front();
			_keys.erase(_keys.begin());
			k = e & 0xFF;
			if (k == 0)
				_scanPending = (e >> 8) & 0xFF;
		}
		m.eax = (m.eax & 0xFFFFFF00u) | (uint32_t)(k & 0xFF);
		break;
	}
	case 0x1A:
		_dta = m.edx;
		break;
	case 0x25: // set interrupt vector
		break;
	case 0x35:
		m.ebx = 0;
		break;
	case 0x2A: {
		std::time_t t = std::time(nullptr);
		std::tm *tmv = std::localtime(&t);
		m.ecx = (uint32_t)(tmv->tm_year + 1900);
		m.edx = (uint32_t)(((tmv->tm_mon + 1) << 8) | tmv->tm_mday);
		m.eax = (m.eax & 0xFFFFFF00u) | (uint32_t)tmv->tm_wday;
		break;
	}
	case 0x2C: {
		pump();
		auto now = std::chrono::steady_clock::now().time_since_epoch();
		uint64_t cs = (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(now).count() / 10;
		m.ecx = (uint32_t)((((cs / 360000) % 24) << 8) | ((cs / 6000) % 60));
		m.edx = (uint32_t)((((cs / 100) % 60) << 8) | (cs % 100));
		break;
	}
	case 0x2F:
		m.ebx = _dta;
		break;
	case 0x30:
		m.eax = 0x0005;
		m.ebx = 0xFF00;
		break;
	case 0x3C: case 0x3D: {
		std::string dn = dosPath(m.edx);
		if (verbose) {
			std::string chain;
			for (uint32_t a : m.trace)
				chain += describe(a) + " > ";
			log("  open (name at " + std::to_string(m.edx) + ") from: " + chain);
		}
		bool create = ah == 0x3C;
		bool write = create || (al & 3) != 0;
		std::string hp = resolve(dn, create);
		if (hp.empty()) {
			log("open failed: " + dn);
			fail(2);
			break;
		}
		FILE *f = fopen(hp.c_str(), create ? "w+b" : (write ? "r+b" : "rb"));
		if (!f && write && !create)
			f = fopen(hp.c_str(), "rb");
		if (!f) {
			log("open failed: " + dn + " -> " + hp);
			fail(2);
			break;
		}
		log("open " + dn + " -> " + hp);
		_files.push_back({f, dn});
		m.eax = (uint32_t)(_files.size() - 1 + 5);
		break;
	}
	case 0x3E: {
		uint32_t h = m.ebx;
		if (h >= 5 && h - 5 < _files.size() && _files[h - 5].f) {
			fclose(_files[h - 5].f);
			_files[h - 5].f = nullptr;
		}
		break;
	}
	case 0x3F: {
		FILE *f = fileOf(m.ebx);
		if (!f) {
			if (m.ebx == 0)
				m.eax = 0;
			else
				fail(6);
			break;
		}
		std::vector<uint8_t> buf(m.ecx);
		size_t n = fread(buf.data(), 1, m.ecx, f);
		if (n)
			m.writeBytes(m.edx, buf.data(), n);
		m.eax = (uint32_t)n;
		break;
	}
	case 0x40: {
		uint32_t h = m.ebx;
		if (h == 1 || h == 2) {
			std::string s;
			for (uint32_t i = 0; i < m.ecx; i++)
				s.push_back((char)m.r8(m.edx + i));
			fwrite(s.data(), 1, s.size(), stderr);
			m.eax = m.ecx;
			break;
		}
		FILE *f = fileOf(h);
		if (!f) {
			fail(6);
			break;
		}
		std::vector<uint8_t> buf(m.ecx);
		for (uint32_t i = 0; i < m.ecx; i++)
			buf[i] = m.r8(m.edx + i);
		size_t n = m.ecx ? fwrite(buf.data(), 1, m.ecx, f) : 0;
		m.eax = (uint32_t)n;
		break;
	}
	case 0x41: {
		std::string hp = resolve(dosPath(m.edx), true);
		if (std::remove(hp.c_str()) != 0)
			fail(2);
		break;
	}
	case 0x42: { // lseek: AL whence, CX:DX offset -> DX:AX position
		FILE *f = fileOf(m.ebx);
		if (!f) {
			fail(6);
			break;
		}
		int32_t off = (int32_t)(((m.ecx & 0xFFFF) << 16) | (m.edx & 0xFFFF));
		fseek(f, off, al == 0 ? SEEK_SET : al == 1 ? SEEK_CUR : SEEK_END);
		long pos = ftell(f);
		m.eax = (uint32_t)pos & 0xFFFF;
		m.edx = ((uint32_t)pos >> 16) & 0xFFFF;
		break;
	}
	case 0x43: { // file attributes
		std::string hp = resolve(dosPath(m.edx), false);
		if (hp.empty()) {
			fail(2);
			break;
		}
		m.ecx = 0x20;
		break;
	}
	case 0x44: // IOCTL: device information
		if (al == 0)
			m.edx = (m.ebx <= 4) ? 0x80u : 0u;
		break;
	case 0x48: { // allocate DOS memory: BX paragraphs
		uint32_t p = dosAlloc((m.ebx & 0xFFFFF) * 16);
		if (!p) {
			fail(8);
			m.ebx = dosLargest() >> 4;
		} else {
			m.eax = p >> 4;
		}
		break;
	}
	case 0x49: case 0x4A:
		break;
	case 0x4C:
		throw ProgramExit{(int)al};
	case 0x4E: case 0x4F: { // find first / next (the savegame list is built this way): wildcard match on the base name over the save directory and the game directories
		if (ah == 0x4E) {
			std::string pat = dosPath(m.edx);
			size_t cut = pat.find_last_of("\\/:");
			if (cut != std::string::npos)
				pat = pat.substr(cut + 1);
			pat = lowerStr(pat);
			if (pat == "*.*")
				pat = "*";
			auto match = [&](const std::string &name) { // * and ? wildcards
				std::function<bool(size_t, size_t)> rec = [&](size_t i, size_t j) -> bool {
					if (i == pat.size())
						return j == name.size();
					if (pat[i] == '*')
						return rec(i + 1, j) || (j < name.size() && rec(i, j + 1));
					return j < name.size() && (pat[i] == '?' || pat[i] == name[j]) && rec(i + 1, j + 1);
				};
				return rec(0, 0);
			};
			_found.clear();
			_foundNext = 0;
			std::map<std::string, Found> uniq;
			std::error_code ec;
			for (const auto &e : std::filesystem::directory_iterator(_saveDir, ec))
				if (e.is_regular_file() && match(lowerStr(e.path().filename().string())))
					uniq[lowerStr(e.path().filename().string())] = {e.path().filename().string(), e.path().string(), (uint32_t)e.file_size()};
			for (auto &kv : _dirIndex)
				if (!uniq.count(kv.first) && match(kv.first)) {
					std::error_code ec2;
					uniq[kv.first] = {std::filesystem::path(kv.second).filename().string(), kv.second, (uint32_t)std::filesystem::file_size(kv.second, ec2)};
				}
			for (auto &kv : uniq)
				_found.push_back(kv.second);
		}
		if (_foundNext >= _found.size()) {
			fail(ah == 0x4E ? 2 : 18);
			break;
		}
		fillDta(_found[_foundNext++]);
		m.eax = 0;
		break;
	}
	default:
		log("unhandled DOS function " + std::to_string(ah));
		fail(1);
		break;
	}
}

// ---- native replacements for library functions ---------------------------------------------------------------------------------------------------------------

void Host::installShims() {
	Machine &m = *_m;
	installCrtShims(m);
	_audio.install(m);
	m.irqEvery = 4000;
	m.irqCountdown = 4000;
	m.irqHook = [this](Machine &mm) { serviceInterrupts(mm); };
	// AIL timers (cdecl): handles are indices; callbacks run from serviceInterrupts
	auto timerShim = [&](const char *name, std::function<uint32_t(Machine &)> fn) {
		m.setShim(name, [name, fn](Machine &mm) {
			mm.eax = fn(mm);
			mm.finishShim(name);
		});
	};
	timerShim("_AIL_register_timer", [this](Machine &mm) {
		for (size_t i = 0; i < _timers.size(); i++)
			if (!_timers[i].used) {
				_timers[i] = Timer();
				_timers[i].used = true;
				_timers[i].cb = mm.stackArg(0);
				return (uint32_t)i;
			}
		return 0xFFFFFFFFu;
	});
	auto timerOf = [this](uint32_t h) -> Timer * { return h < _timers.size() && _timers[h].used ? &_timers[h] : nullptr; };
	timerShim("_AIL_set_timer_frequency", [this, timerOf](Machine &mm) {
		if (Timer *t = timerOf(mm.stackArg(0)))
			if (mm.stackArg(1))
				t->periodUs = 1e6 / (double)mm.stackArg(1);
		return 0u;
	});
	timerShim("_AIL_set_timer_period", [this, timerOf](Machine &mm) {
		if (Timer *t = timerOf(mm.stackArg(0)))
			t->periodUs = (double)mm.stackArg(1) > 0 ? (double)mm.stackArg(1) : 10000;
		return 0u;
	});
	timerShim("_AIL_set_timer_user", [this, timerOf](Machine &mm) {
		if (Timer *t = timerOf(mm.stackArg(0)))
			t->user = mm.stackArg(1);
		return 0u;
	});
	timerShim("_AIL_start_timer", [this, timerOf](Machine &mm) {
		if (Timer *t = timerOf(mm.stackArg(0))) {
			t->running = true;
			t->nextUs = nowUs() + (uint64_t)t->periodUs;
		}
		return 0u;
	});
	timerShim("_AIL_stop_timer", [this, timerOf](Machine &mm) {
		if (Timer *t = timerOf(mm.stackArg(0)))
			t->running = false;
		return 0u;
	});
	timerShim("_AIL_release_timer_handle", [this, timerOf](Machine &mm) {
		if (Timer *t = timerOf(mm.stackArg(0)))
			*t = Timer();
		return 0u;
	});
	// speech and sound effects: the original streams PCM from the voice archive through AIL sample handles; this release keeps them as numbered MP3 files (NNNN.mp3), played by the front end.
	// Semantics follow the originals: make_voice?(id, per-iteration callback) blocks until the line ends and returns 1 if a click/key skipped it; make_sound(id, loops, ...) starts an effect.
	m.setShim("make_voice?", [this](Machine &mm) {
		uint32_t id = mm.eax & 0xFFFF, cb = mm.edx;
		uint32_t skipFn = addrOf("sub_13A20");
		uint32_t ret = 0;
		std::string path = mp3Path((int)id);
		log("make_voice " + std::to_string(id) + " -> " + (path.empty() ? "(no file)" : path));
		if (!_audio.mp3.play || path.empty() || !skipFn) {
			mm.eax = 0;
			mm.finishShim("make_voice?");
			return;
		}
		if (callGuest(skipFn, {})) { // a click or key before the line starts skips it
			mm.eax = 1;
			mm.finishShim("make_voice?");
			return;
		}
		int h = _audio.mp3.play(path, 1);
		while (h && _audio.mp3.playing(h)) {
			if (cb)
				callGuest(cb, {0x5D8000u});
			if (callGuest(skipFn, {})) {
				ret = 1;
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
		}
		if (h)
			_audio.mp3.stop(h);
		mm.eax = ret;
		mm.finishShim("make_voice?");
	});
	m.setShim("make_sound", [this](Machine &mm) {
		int id = (int)(int16_t)mm.eax;
		int loops = (int)mm.edx;
		std::string path = mp3Path(id);
		log("make_sound " + std::to_string(id) + " loops " + std::to_string(loops) + " -> " + (path.empty() ? "(no file)" : path));
		if (_audio.mp3.play && !path.empty()) {
			auto it = _sounds.find(id);
			if (it != _sounds.end()) {
				_audio.mp3.stop(it->second);
				_sounds.erase(it);
			}
			int h = _audio.mp3.play(path, loops);
			if (h)
				_sounds[id] = h;
		}
		mm.eax = 0;
		mm.finishShim("make_sound");
	});
	m.setShim("stop_sound", [this](Machine &mm) {
		int id = (int)(int16_t)mm.eax;
		if (_audio.mp3.stop)
			for (auto it = _sounds.begin(); it != _sounds.end();) {
				if (id == -1 || id == -2 || it->first == id) {
					_audio.mp3.stop(it->second);
					it = _sounds.erase(it);
				} else {
					++it;
				}
			}
		mm.eax = 0;
		mm.finishShim("stop_sound");
	});
	m.setShim("sub_77AE4", [this](Machine &mm) { // is the sound effect with this id still playing?
		int id = (int)(int16_t)mm.eax;
		auto it = _sounds.find(id);
		bool on = it != _sounds.end() && _audio.mp3.playing && _audio.mp3.playing(it->second);
		if (it != _sounds.end() && !on)
			_sounds.erase(it);
		mm.eax = on ? 1 : 0;
		mm.finishShim("sub_77AE4");
	});
	m.setShim("update_music_volume", [this](Machine &mm) { // (despite the name) the volume of a sound effect: 0..127
		int id = (int)(int16_t)mm.eax;
		int vol = (int)mm.edx;
		auto it = _sounds.find(id);
		if (it != _sounds.end() && _audio.mp3.volume)
			_audio.mp3.volume(it->second, vol);
		mm.eax = 0;
		mm.finishShim("update_music_volume");
	});
	timerShim("_AIL_lock", [](Machine &) { return 0u; });
	timerShim("_AIL_unlock", [](Machine &) { return 0u; });
	timerShim("_AIL_disable_interrupts", [](Machine &) { return 0u; });
	timerShim("_AIL_restore_interrupts", [](Machine &) { return 0u; });
	// software-interrupt entry points of the C library: the original builds `int N` stubs at run time, here the interrupt is dispatched directly
	auto doInt = [this](Machine &mm, uint32_t intno, uint32_t in, uint32_t out) -> uint32_t {
		uint32_t sEsi = mm.esi, sEdi = mm.edi, sEbp = mm.ebp, sEbx = mm.ebx; // the library's versions preserve ebx, esi, edi and ebp (callers rely on it)
		mm.eax = mm.r32(in);
		mm.ebx = mm.r32(in + 4);
		mm.ecx = mm.r32(in + 8);
		mm.edx = mm.r32(in + 12);
		mm.esi = mm.r32(in + 16);
		mm.edi = mm.r32(in + 20);
		mm.cf = false;
		interrupt((int)(intno & 0xFF));
		mm.w32(out, mm.eax);
		mm.w32(out + 4, mm.ebx);
		mm.w32(out + 8, mm.ecx);
		mm.w32(out + 12, mm.edx);
		mm.w32(out + 16, mm.esi);
		mm.w32(out + 20, mm.edi);
		mm.w32(out + 24, mm.cf ? 1 : 0);
		uint32_t r = mm.eax;
		mm.esi = sEsi;
		mm.edi = sEdi;
		mm.ebp = sEbp;
		mm.ebx = sEbx;
		return r;
	};
	m.setShim("int386_", [doInt](Machine &mm) {
		uint32_t n = mm.eax, in = mm.edx, out = mm.ebx, sEcx = mm.ecx; // int386_ (unlike int386x_) preserves ecx as well
		mm.eax = doInt(mm, n, in, out);
		mm.ecx = sEcx;
		mm.finishShim("int386_");
	});
	m.setShim("int386x_", [doInt](Machine &mm) {
		uint32_t n = mm.eax, in = mm.edx, out = mm.ebx;
		mm.eax = doInt(mm, n, in, out);
		mm.finishShim("int386x_");
	});
	m.setShim("intdos_", [doInt](Machine &mm) {
		uint32_t in = mm.eax, out = mm.edx;
		mm.eax = doInt(mm, 0x21, in, out);
		mm.finishShim("intdos_");
	});
	m.setShim("segread_", [](Machine &mm) {
		uint32_t p = mm.eax;
		uint16_t sel[6] = {0x17, 0x0F, 0x17, 0x17, 0x17, 0x17}; // es cs ss ds fs gs
		for (int k = 0; k < 6; k++)
			mm.w16(p + 2 * k, sel[k]);
		mm.finishShim("segread_");
	});
	// the near-heap growth: a bump allocator in the host's arena
	m.setShim("sbrk_", [this](Machine &mm) {
		int32_t inc = (int32_t)mm.eax;
		uint32_t old = _brk;
		if ((int64_t)_brk + inc > (int64_t)kHeapEnd || (int64_t)_brk + inc < (int64_t)(kHeapBase + 0x1000000)) {
			mm.eax = 0xFFFFFFFFu;
		} else {
			_brk += inc;
			mm.eax = old;
		}
		mm.finishShim("sbrk_");
	});
	// growing the near heap: under DOS/4G the original calls __CreateNewNHeap_(amount), which allocates a DPMI block (the other branches use `lsl` on the data selector)
	m.setShim("__ExpandDGROUP_", [this](Machine &mm) {
		uint32_t amount = mm.eax, sEdx = mm.edx, sEbx = mm.ebx, sEcx = mm.ecx, sEsi = mm.esi, sEdi = mm.edi, sEbp = mm.ebp;
		uint32_t fn = _byName["__CreateNewNHeap_"];
		if (!fn)
			throw FormatError("function map not loaded (Host::loadNames)");
		mm.push(0);
		mm.eax = amount;
		mm.call(fn);
		uint32_t r = mm.eax;
		mm.edx = sEdx; mm.ebx = sEbx; mm.ecx = sEcx; mm.esi = sEsi; mm.edi = sEdi; mm.ebp = sEbp;
		mm.eax = r;
		mm.finishShim("__ExpandDGROUP_");
	});
	m.setShim("__LastFree_", [](Machine &mm) { mm.eax = 0; mm.finishShim("__LastFree_"); });
	m.setShim("getenv_", [](Machine &mm) { mm.eax = 0; mm.finishShim("getenv_"); });
	// setjmp / longjmp: the buffer remembers the registers and the resume address; longjmp unwinds the C++ stack with an exception that run() catches
	m.setShim("_setjmp_", [this](Machine &mm) {
		JmpBuf b;
		b.resume = mm.r32(mm.esp);
		b.esp = mm.esp + 4;
		b.ebp = mm.ebp;
		b.ebx = mm.ebx;
		b.ecx = mm.ecx;
		b.esi = mm.esi;
		b.edi = mm.edi;
		_jmp[mm.eax] = b;
		mm.eax = 0;
		mm.finishShim("_setjmp_");
	});
	m.setShim("longjmp_", [this](Machine &mm) {
		auto it = _jmp.find(mm.eax);
		if (it == _jmp.end())
			throw FormatError("longjmp without setjmp");
		JmpBuf b = it->second;
		uint32_t val = mm.edx ? mm.edx : 1;
		mm.esp = b.esp;
		mm.ebp = b.ebp;
		mm.ebx = b.ebx;
		mm.ecx = b.ecx;
		mm.esi = b.esi;
		mm.edi = b.edi;
		mm.eax = val;
		throw LongJump{b.resume};
	});
}

int Host::run(int64_t budget) {
	Machine &m = *_m;
	// what the Watcom/DOS4G `start` code does before it calls the C library initialisers: detect the extender, record the stack and break, set up the environment and the command line
	uint32_t block = heapAlloc(8192);
	const char *name = "SHANNARA.EXE";
	uint32_t pgm = block + 256, cmd = block + 512, env = block + 1024;
	memcpy(m.ptr(pgm), name, strlen(name) + 1);
	m.w8(cmd, 0);
	m.w8(env, 0);
	m.w8(env + 1, 0);
	m.esp = Machine::kStackTop & ~3u;
	m.w32(kBase + 0xCA9B0, m.esp);          // __STACKTOP
	m.w32(kBase + 0xCA99C, m.esp);          // __curbrk
	m.w16(kBase + 0xCA9A8, 0x24);           // __psp
	m.w8(kBase + 0xCA9D3, 5);               // DOS version 5.0
	m.w8(kBase + 0xCA9D4, 0);
	m.w8(kBase + 0xCA9CA, 1);               // __Extender: DOS/4G
	m.w8(kBase + 0xCA9CB, 0);               // __ExtenderSubtype
	m.w32(kBase + 0xCA9CD, env);            // __EnvPtr
	m.w16(kBase + 0xCA9D1, 0x17);           // __EnvSeg
	m.w16(kBase + 0xCA9C8, 0);              // __no87
	m.w32(kBase + 0xCA9AC, 0x8E0100);       // __STACKLOW
	m.w32(kBase + 0xCA998, m.esp);          // __dynend
	m.w32(kBase + 0xCA9A0, cmd);            // __LpCmdLine
	m.w32(kBase + 0xCA9A4, pgm);            // __LpPgmName
	m.budget = budget;
	m.ebx = m.ecx = m.esi = m.edi = m.ebp = m.edx = 0;
	uint32_t initRtns = _byName["__InitRtns"], cmain = _byName["__CMain"];
	if (!initRtns || !cmain)
		throw FormatError("function map not loaded (Host::loadNames)");
	uint32_t entry = 0;
	int stage = 0;
	for (;;) {
		try {
			if (stage == 0) {
				m.push(0);
				m.eax = 0xFF;
				m.call(initRtns);
				stage = 1;
			}
			if (stage == 1) {
				m.ebp = 0;
				m.edx = 0;
				m.push(0);
				m.call(cmain);
				return 0;
			}
			m.call(entry);
			return 0;
		} catch (const ProgramExit &e) {
			return e.code;
		} catch (const LongJump &j) {
			entry = j.resume;
			stage = 2;
			m.push(0);
			continue;
		} catch (const FormatError &e) {
			if (std::string(e.what()).find("budget") != std::string::npos) {
				budgetExhausted = true;
				return -1;
			}
			throw;
		}
	}
}

} // namespace shannara
