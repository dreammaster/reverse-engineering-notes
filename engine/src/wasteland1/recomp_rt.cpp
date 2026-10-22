#include "wasteland1/recomp_rt.h"
#include "wasteland1/exe.h"
#include "wasteland1/x86.h"
#include "shared/bytes.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <map>

namespace wl {
namespace rc {

void Sys::indexGameDir() {
	names.clear();
	for (const auto &e : std::filesystem::directory_iterator(gameDir)) {
		const std::string n = e.path().filename().string();
		std::string l = n;
		for (auto &c : l) c = char(std::tolower(static_cast<unsigned char>(c)));
		names[l] = n;
	}
}

Sys::~Sys() {
	for (auto &kv : files) std::fclose(kv.second);
}

namespace {

Sys &sys(Machine &m) {
	if (!m.sys) throw Fault("recompiled runtime not booted");
	return *m.sys;
}

bool isInterpEntry(uint16_t a) { return a == 0x06C9 || a == 0x06CE || a == 0x4330 || a == 0x4388 || a == 0x43A8 || a == 0x43D0; }

void toCpu(const Regs &r, x86::Cpu &c) {
	c.ax = r.a.x; c.bx = r.b.x; c.cx = r.c.x; c.dx = r.d.x;
	c.si = r.si; c.di = r.di; c.bp = r.bp; c.sp = r.sp;
	c.es = r.es; c.cs = r.cs; c.ss = r.ss; c.ds = r.ds;
	c.flags = r.flags();
}

void fromCpu(const x86::Cpu &c, Regs &r) {
	r.a.x = c.ax; r.b.x = c.bx; r.c.x = c.cx; r.d.x = c.dx;
	r.si = c.si; r.di = c.di; r.bp = c.bp; r.sp = c.sp;
	r.es = c.es; r.cs = c.cs; r.ss = c.ss; r.ds = c.ds;
	r.setFlags(c.flags);
}

std::string readName(Machine &m, uint16_t seg, uint16_t off) {
	std::string s;
	for (int i = 0; i < 80; ++i) {
		const char c = char(rb(m, seg, uint16_t(off + i)));
		if (!c) break;
		s += c;
	}
	return s;
}

} // namespace

void callGenerated(Machine &m, Regs &r, uint16_t address) {
	push(m, r, 0);
	dispatch(m, r, address);
}

void fatal(const char *message) { throw Fault(message); }
void unknownTarget(Machine &m, Regs &r, uint16_t target) {
	sys(m).unknownTargets.insert(target);
	const uint16_t sp0 = r.sp;
	interpret(m, r, target);
	stackCheck(target, sp0, r.sp);
}

void stackCheck(uint16_t function, uint16_t spBefore, uint16_t spAfter) {
	if (uint16_t(spAfter - spBefore) != 2) std::fprintf(stderr, "stack: f_%04X sp %04X -> %04X (delta %d)", function, spBefore, spAfter, int(uint16_t(spAfter - spBefore))), std::fputc(10, stderr);
}

void fatalAt(const char *message, unsigned value) {
	char buf[96];
	std::snprintf(buf, sizeof buf, "%s %04X", message, value);
	throw Fault(buf);
}

void speakerUpdate(Sys &s);

uint16_t portIn(Machine &m, Regs &, uint16_t port, bool) {
	Ega &e = m.ega;
	if (port == 0x3DA) { e.status ^= 9; return uint16_t(e.status); }
	if (port == 0x40) {   // PIT channel 0: a free running down counter (lo, hi, lo, hi ...), used to measure the CPU speed
		Sys &s = sys(m);
		const uint16_t v = uint16_t(s.pit);
		const bool hi = s.pitHigh;
		s.pitHigh = !hi;
		if (hi) s.pit -= 400;
		return hi ? uint16_t(v >> 8) : uint16_t(v & 0xFF);
	}
	if (port == 0x3C5) return e.seqIdx == 2 ? e.mapMask : 0;
	if (port == 0x3CF) return e.gc[e.gcIdx & 15];
	return 0;
}

void speakerUpdate(Sys &s) {
	const int hz = s.speakerOn && s.pitDivisor > 1 ? int(1193182u / s.pitDivisor) : 0;
	if (hz == s.speakerHz) return;
	s.speakerHz = hz;
	if (s.onSpeaker) s.onSpeaker(hz);
}

void portOut(Machine &m, Regs &, uint16_t port, uint16_t v, bool wide) {
	Ega &e = m.ega;
	switch (port) {
	case 0x42: {   // PIT channel 2 divisor (low byte, then high byte): the speaker tone
		Sys &s = sys(m);
		if (s.pitLow) s.pitDivisor = uint16_t((s.pitDivisor & 0xFF00) | (v & 0xFF));
		else s.pitDivisor = uint16_t((s.pitDivisor & 0x00FF) | ((v & 0xFF) << 8));
		s.pitLow = !s.pitLow;
		if (s.pitLow) speakerUpdate(s);
		break;
	}
	case 0x43: sys(m).pitLow = true; break;
	case 0x61: {
		Sys &s = sys(m);
		s.speakerOn = (v & 3) == 3;
		speakerUpdate(s);
		break;
	}
	case 0x3C4: e.seqIdx = uint8_t(v & 31); if (wide && e.seqIdx == 2) e.mapMask = uint8_t(v >> 8); break;
	case 0x3C5: if (e.seqIdx == 2) e.mapMask = uint8_t(v); break;
	case 0x3CE: e.gcIdx = uint8_t(v & 15); if (wide) e.gc[e.gcIdx] = uint8_t(v >> 8); break;
	case 0x3CF: e.gc[e.gcIdx & 15] = uint8_t(v); break;
	case 0x3D4: e.crtcIdx = uint8_t(v & 31); if (wide) e.crtc[e.crtcIdx] = uint8_t(v >> 8); break;
	case 0x3D5: e.crtc[e.crtcIdx & 31] = uint8_t(v); break;
	default: break;
	}
}

void intr(Machine &m, Regs &r, int n) {
	Sys &s = sys(m);
	const uint8_t ah = r.a.b.h, al = r.a.b.l;
	static const bool log = std::getenv("WL_DOSLOG") != nullptr;
	if (log && n == 0x21) std::fprintf(stderr, "int21 ax=%04X bx=%04X cx=%04X dx=%04X ds=%04X\n", r.a.x, r.b.x, r.c.x, r.d.x, r.ds);
	switch (n) {
	case 0x21:
		switch (ah) {
		case 0x19: r.a.x = 0x0002; return;
		case 0x1C: r.a.x = 0x0001; r.c.x = 512; r.d.x = 1000; return;
		case 0x25: ww(m, 0, uint16_t(al * 4), r.d.x); ww(m, 0, uint16_t(al * 4 + 2), r.ds); return;
		case 0x35: r.b.x = rw(m, 0, uint16_t(al * 4)); r.es = rw(m, 0, uint16_t(al * 4 + 2)); return;
		case 0x3D: {
			std::string name = readName(m, r.ds, r.d.x), l = name;
			for (auto &ch : l) ch = char(std::tolower(static_cast<unsigned char>(ch)));
			const size_t slash = l.find_last_of("\\/:");
			if (slash != std::string::npos) l = l.substr(slash + 1);
			if (log) std::fprintf(stderr, "  name=\"%s\" first bytes %02X %02X\n", name.c_str(), rb(m, r.ds, r.d.x), rb(m, r.ds, uint16_t(r.d.x + 1)));
			const auto it = s.names.find(l);
			FILE *f = it == s.names.end() ? nullptr : std::fopen((s.gameDir + "/" + it->second).c_str(), (al & 3) ? "r+b" : "rb");
			if (!f) { if (log) std::fprintf(stderr, "open %s failed\n", name.c_str()); r.a.x = 2; r.cf = true; return; }
			s.files[s.nextHandle] = f;
			s.openNames[s.nextHandle] = it->second;
			r.a.x = uint16_t(s.nextHandle++);
			r.cf = false;
			return;
		}
		case 0x3E: {
			auto it = s.files.find(r.b.x);
			if (it != s.files.end()) { std::fclose(it->second); s.files.erase(it); s.openNames.erase(r.b.x); }
			r.cf = false;
			return;
		}
		case 0x3F: {
			auto it = s.files.find(r.b.x);
			if (it == s.files.end()) { r.a.x = 6; r.cf = true; return; }
			std::vector<uint8_t> buf(r.c.x);
			const size_t got = std::fread(buf.data(), 1, buf.size(), it->second);
			for (size_t i = 0; i < got; ++i) wb(m, r.ds, uint16_t(r.d.x + i), buf[i]);
			r.a.x = uint16_t(got);
			r.cf = false;
			return;
		}
		case 0x3C: {   // create / truncate (the file name keeps its case if it is new)
			std::string name = readName(m, r.ds, r.d.x), l = name;
			for (auto &ch : l) ch = char(std::tolower(static_cast<unsigned char>(ch)));
			const size_t slash = l.find_last_of("\\/:");
			if (slash != std::string::npos) { l = l.substr(slash + 1); name = name.substr(slash + 1); }
			const auto it = s.names.find(l);
			const std::string real = it == s.names.end() ? name : it->second;
			FILE *f = std::fopen((s.gameDir + "/" + real).c_str(), "w+b");
			if (!f) { r.a.x = 5; r.cf = true; return; }
			s.names[l] = real;
			s.files[s.nextHandle] = f;
			s.openNames[s.nextHandle] = real;
			r.a.x = uint16_t(s.nextHandle++);
			r.cf = false;
			return;
		}
		case 0x40: {
			auto it = s.files.find(r.b.x);
			if (it == s.files.end()) { r.a.x = r.c.x; r.cf = false; return; }   // stdout and friends: ignored
			std::vector<uint8_t> buf(r.c.x);
			for (size_t i = 0; i < buf.size(); ++i) buf[i] = rb(m, r.ds, uint16_t(r.d.x + i));
			const size_t put = std::fwrite(buf.data(), 1, buf.size(), it->second);
			std::fflush(it->second);
			r.a.x = uint16_t(put);
			r.cf = false;
			return;
		}
		case 0x42: {
			auto it = s.files.find(r.b.x);
			if (it == s.files.end()) { r.a.x = 6; r.cf = true; return; }
			const long off = long((uint32_t(r.c.x) << 16) | r.d.x);
			std::fseek(it->second, off, al == 0 ? SEEK_SET : al == 1 ? SEEK_CUR : SEEK_END);
			const long pos = std::ftell(it->second);
			r.a.x = uint16_t(pos); r.d.x = uint16_t(uint32_t(pos) >> 16);
			r.cf = false;
			return;
		}
		case 0x4C: s.exited = true; throw ExitProgram{al};
		default: std::fprintf(stderr, "int 21h AH=%02X unhandled\n", ah); return;
		}
	case 0x10:
		if (ah == 0) s.videoMode = al;
		return;
	case 0x16:
		if (ah == 0) {
			while (m.keys.empty()) {
				if (!m.onIdle) throw WaitForInput{};
				m.onIdle();
			}
			r.a.x = m.keys.front(); m.keys.pop_front();
			return;
		}
		if (ah == 1) {
			if (m.keys.empty()) {
				r.zf = true;
				++s.emptyPolls;
				if (m.onIdle) m.onIdle();
				else if (s.stopOnIdle && s.emptyPolls > s.stopOnIdle) throw WaitForInput{};
			} else {
				s.emptyPolls = 0;
				r.zf = false;
				r.a.x = m.keys.front();
			}
			return;
		}
		r.a.x = 0;
		return;
	case 0x17: r.a.x = 0x9000; return;
	case 0x33: if (r.a.x == 0) r.a.x = 0; return;   // no mouse
	case 0x25: case 0x26: r.a.x = 0x0101; r.cf = true; return;
	case 3: throw Fault("int3");
	default: throw Fault("unsupported interrupt");
	}
}

void deliverTimer(Machine &m, Regs &r) {
	Sys &s = sys(m);
	// BIOS tick counter
	uint32_t ticks = uint32_t(rw(m, 0x40, 0x6C)) | (uint32_t(rw(m, 0x40, 0x6E)) << 16);
	++ticks;
	ww(m, 0x40, 0x6C, uint16_t(ticks)); ww(m, 0x40, 0x6E, uint16_t(ticks >> 16));
	const uint16_t off = rw(m, 0, 0x20), seg = rw(m, 0, 0x22);
	++s.ticks;
	if (!r.ifl && std::getenv("WL_DOSLOG") && s.ticks < 100000) { std::fprintf(stderr, "ifl=0 at tick %llu sites:", (unsigned long long)s.ticks); for (int i = 1; i <= 8; ++i) std::fprintf(stderr, " %04X", s.lastSites[(s.edges + 16 - unsigned(i)) & 15]); std::fputc(10, stderr); }
	static int shown = 0;
	if (std::getenv("WL_DOSLOG") && shown++ < 5) std::fprintf(stderr, "timer: ifl=%d vec=%04X:%04X inTimer=%d", r.ifl, seg, off, s.inTimer), std::fputc(10, stderr);
	if (!r.ifl || (off == 0 && seg == 0xF000) || s.inTimer) return;
	const uint32_t target = ((uint32_t(seg) << 4) + off) - 0x10000;
	if (target >= 0x10000 || !isEntry(uint16_t(target))) return;
	s.inTimer = true;
	push(m, r, r.flags()); push(m, r, r.cs); push(m, r, 0);
	Regs h = r;
	h.ifl = false;
	try {
		dispatch(m, h, uint16_t(target));
	} catch (...) {
		s.inTimer = false;
		throw;
	}
	s.inTimer = false;
	if (std::getenv("WL_DOSLOG") && s.ticks < 14) std::fprintf(stderr, "tick %llu: sp %04X -> %04X ifl %d -> %d stack %04X %04X %04X", (unsigned long long)s.ticks, r.sp, h.sp, r.ifl, h.ifl, rw(m, r.ss, r.sp), rw(m, r.ss, uint16_t(r.sp + 2)), rw(m, r.ss, uint16_t(r.sp + 4))), std::fputc(10, stderr);
	r.sp = h.sp;
	r.setFlags(h.flags());
}

void backedge(Machine &m, Regs &r, uint16_t site) {
	Sys &s = sys(m);
	s.lastSites[s.edges & 15] = site;
	if (const char *e = std::getenv("WL_SPLOG")) {
		static uint16_t lastSp = 0;
		static int shown = 0;
		if (r.sp > 0x200 && lastSp <= 0x200 && shown++ < std::atoi(e)) std::fprintf(stderr, "edge %llu site %04X sp %04X (was %04X)", (unsigned long long)s.edges, site, r.sp, lastSp), std::fputc(10, stderr);
		lastSp = r.sp;
	}
	if (s.maxEdges && s.edges > s.maxEdges) {
		char buf[512];
		int n = std::snprintf(buf, sizeof buf, "watchdog; last back edges:");
		for (int i = 1; i <= 16; ++i) n += std::snprintf(buf + n, sizeof buf - size_t(n), " %04X", s.lastSites[(s.edges + 16 - unsigned(i)) & 15]);
		n += std::snprintf(buf + n, sizeof buf - size_t(n), "; ticks=%llu D150=%04X A6E6=%04X ifl=%d; stack:", (unsigned long long)s.ticks, m.d16(0xD150), m.d16(0xA6E6), r.ifl);
		for (int i = 0; i < 12; ++i) n += std::snprintf(buf + n, sizeof buf - size_t(n), " %04X", rw(m, r.ss, uint16_t(r.sp + 2 * i)));
		throw Fault(buf);
	}
	if (s.realtime) {
		if ((++s.edges & 255) != 0) return;
		const int64_t now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		if (now < s.nextTickUs) return;
		s.nextTickUs = s.nextTickUs == 0 || now - s.nextTickUs > 500000 ? now + 54925 : s.nextTickUs + 54925;
		deliverTimer(m, r);
		if (s.onTick) s.onTick();
		return;
	}
	if (++s.edges < s.nextTick) return;
	s.nextTick = s.edges + s.edgesPerTick;
	deliverTimer(m, r);
}

void farJump(Machine &m, Regs &r, uint16_t seg, uint16_t off) {
	if (seg == 0xF000) {   // the BIOS handler: just an IRET
		pop(m, r); pop(m, r); r.setFlags(pop(m, r));
		return;
	}
	const uint32_t t = ((uint32_t(seg) << 4) + off) - 0x10000;
	if (t < 0x10000 && isEntry(uint16_t(t))) { dispatch(m, r, uint16_t(t)); return; }
	throw Fault("far jump to unknown target");
}

void interpret(Machine &m, Regs &r, uint16_t entry) {
	Sys &s = sys(m);
	if (!s.cpu) {
		s.cpu = std::make_unique<x86::Cpu>(m.mem.data());
		x86::Cpu &c = *s.cpu;
		c.onWrite = [&m](uint32_t a, uint8_t v) { m.ega.write(a, v); };
		c.onRead = [&m](uint32_t a) { return m.ega.read(a); };
		c.onIn = [&m](x86::Cpu &, uint16_t port, bool wide) { Regs d; return portIn(m, d, port, wide); };
		c.onOut = [&m](x86::Cpu &, uint16_t port, uint16_t v, bool wide) { Regs d; portOut(m, d, port, v, wide); };
		c.onInt = [&m](x86::Cpu &cc, int n) {
			Regs d;
			fromCpu(cc, d);
			intr(m, d, n);
			toCpu(d, cc);
			return true;
		};
	}
	x86::Cpu &c = *s.cpu;
	toCpu(r, c);
	c.ip = entry;
	c.halted = false;
	c.fault.clear();
	const uint16_t sp0 = c.sp;   // points at the return address pushed by the caller
	bool first = true;
	while (true) {
		const uint16_t ip = c.ip;
		if (!first && c.cs == 0x1000 && isEntry(ip) && !isInterpEntry(ip)) {
			const uint16_t ret = c.rw(c.ss, c.sp);
			Regs d;
			fromCpu(c, d);
			dispatch(m, d, ip);
			toCpu(d, c);
			c.ip = ret;
			if (c.sp > sp0) break;
			continue;
		}
		first = false;
		if (!c.step()) throw Fault(c.fault.empty() ? "interpreter halted" : ("interpreter: " + c.fault).c_str());
		if (c.sp > sp0) break;
	}
	fromCpu(c, r);
}

void boot(Machine &m, const Bytes &image, const std::string &gameDir, Regs &r) {
	m.sys = std::make_shared<Sys>();
	Sys &s = *m.sys;
	s.gameDir = gameDir;
	s.indexGameDir();
	if (image.size() > 2 && image[0] == 'M' && image[1] == 'Z') {   // WL.EXE itself: unpack it
		const ExeImage exe = loadExePack(image, 0x1000);
		std::memcpy(&m.mem[0x10000], exe.image.data(), exe.image.size());
	} else {
		std::memcpy(&m.mem[0x10000], image.data(), image.size());
	}
	m.mem[0xFF00] = 0xCD; m.mem[0xFF01] = 0x20;
	// IVT -> a single IRET at F000:0000
	m.mem[0xF0000] = 0xCF;
	for (int i = 0; i < 256; ++i) { ww(m, 0, uint16_t(i * 4), 0); ww(m, 0, uint16_t(i * 4 + 2), 0xF000); }
	m.onPortraitTick = [&m]() {   // WL.EXE 190F8: the animated location portrait, called from the text engine's wait loops
		Sys &sy = *m.sys;
		Regs t = sy.current ? *sy.current : Regs();
		callGenerated(m, t, 0x90F8);
	};
	r = Regs();
	r.cs = 0x1000; r.ds = r.es = 0x0FF0; r.ss = 0x3900; r.sp = 0x0200;
	r.ifl = true;
}

void run(Machine &m, Regs &r) {
	push(m, r, 0xFFFF);
	f_10B6(m, r);
}

} // namespace rc
} // namespace wl
