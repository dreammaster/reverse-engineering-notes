// wlemu: runs the unpacked WL.EXE image in a small 8086 interpreter with DOS/BIOS/EGA stubs. Reference oracle for the
// Wasteland 1 port: scripted keys, screenshots, breakpoints and memory peeks.
//
//   wlemu <image.bin> <gamedir> <command>...
//   commands: run <millionSteps> | steps <n> | key <hex scancode+ascii> | type <text> | shot <png> | regs
//             bp <seg:off> | trace <n> | peek <seg:off> <len> | poke <seg:off> <byte>... | call <seg:off> | ticks <n>
#include "x86.h"
#include "fod/bytes.h"
#include "fod/gfx.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <filesystem>
#include <map>
#include <set>

namespace {

struct Ega {
	uint8_t planes[4][65536] = {};
	uint8_t mapMask = 0xF, seqIdx = 0, gcIdx = 0, gc[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0xFF}, latch[4] = {}, crtcIdx = 0, crtc[32] = {};
	int status = 0;

	uint8_t read(uint32_t addr) {
		const size_t off = (addr - 0xA0000) & 0xFFFF;
		for (int p = 0; p < 4; ++p) latch[p] = planes[p][off];
		return planes[gc[4] & 3][off];
	}
	void write(uint32_t addr, uint8_t v) {
		const size_t off = (addr - 0xA0000) & 0xFFFF;
		const int mode = gc[5] & 3, rot = gc[3] & 7, func = (gc[3] >> 3) & 3;
		const uint8_t bitmask = gc[8], sr = gc[0], esr = gc[1];
		const uint8_t rotated = uint8_t((v >> rot) | (v << (8 - rot)));
		for (int p = 0; p < 4; ++p) {
			if (!(mapMask & (1 << p))) continue;
			uint8_t val = 0, bm = bitmask;
			switch (mode) {
			case 0: val = rotated; if (esr & (1 << p)) val = (sr & (1 << p)) ? 0xFF : 0; break;
			case 1: planes[p][off] = latch[p]; continue;
			case 2: val = (v & (1 << p)) ? 0xFF : 0; break;
			default: val = (sr & (1 << p)) ? 0xFF : 0; bm = uint8_t(bitmask & rotated); break;
			}
			switch (func) {
			case 1: val &= latch[p]; break;
			case 2: val |= latch[p]; break;
			case 3: val ^= latch[p]; break;
			default: break;
			}
			planes[p][off] = uint8_t((val & bm) | (latch[p] & ~bm));
		}
	}
};

struct Machine {
	x86::Cpu cpu;
	Ega ega;
	std::string gameDir;
	std::map<int, FILE *> files;
	int nextHandle = 5;
	std::deque<uint16_t> keys;
	bool needKey = false, exited = false;
	uint64_t nextTick = 0;
	std::set<uint32_t> breaks;
	int videoMode = 3;
	std::map<std::string, std::string> names;   // lower case -> real file name

	explicit Machine(const std::string &dir) : gameDir(dir) {
		for (const auto &e : std::filesystem::directory_iterator(dir)) {
			std::string n = e.path().filename().string(), l = n;
			for (auto &c : l) c = char(std::tolower(static_cast<unsigned char>(c)));
			names[l] = n;
		}
		cpu.onWrite = [this](uint32_t a, uint8_t v) { ega.write(a, v); };
		cpu.onRead = [this](uint32_t a) { return ega.read(a); };
		cpu.onIn = [this](x86::Cpu &, uint16_t port, bool wide) -> uint16_t { return portIn(port, wide); };
		cpu.onOut = [this](x86::Cpu &, uint16_t port, uint16_t v, bool wide) { portOut(port, v, wide); };
		cpu.onInt = [this](x86::Cpu &c, int n) { return handleInt(c, n); };
		// IVT -> a single IRET at F000:0000
		cpu.mem[0xF0000] = 0xCF;
		for (int i = 0; i < 256; ++i) { cpu.ww(0, uint16_t(i * 4), 0); cpu.ww(0, uint16_t(i * 4 + 2), 0xF000); }
	}

	uint16_t portIn(uint16_t port, bool) {
		if (port == 0x3DA) { ega.status ^= 9; return uint16_t(ega.status); }
		if (port == 0x3C5) return ega.seqIdx == 2 ? ega.mapMask : 0;
		if (port == 0x3CF) return ega.gc[ega.gcIdx & 15];
		return 0;
	}
	void portOut(uint16_t port, uint16_t v, bool wide) {
		switch (port) {
		case 0x3C4: ega.seqIdx = uint8_t(v & 31); if (wide) { if (ega.seqIdx == 2) ega.mapMask = uint8_t(v >> 8); } break;
		case 0x3C5: if (ega.seqIdx == 2) ega.mapMask = uint8_t(v); break;
		case 0x3CE: ega.gcIdx = uint8_t(v & 15); if (wide) ega.gc[ega.gcIdx] = uint8_t(v >> 8); break;
		case 0x3CF: ega.gc[ega.gcIdx & 15] = uint8_t(v); break;
		case 0x3D4: ega.crtcIdx = uint8_t(v & 31); if (wide) ega.crtc[ega.crtcIdx] = uint8_t(v >> 8); break;
		case 0x3D5: ega.crtc[ega.crtcIdx & 31] = uint8_t(v); break;
		default: break;
		}
	}

	std::string readName(uint16_t seg, uint16_t off) {
		std::string s;
		for (int i = 0; i < 80; ++i) { const char c = char(cpu.rb(seg, uint16_t(off + i))); if (!c) break; s += c; }
		return s;
	}

	void setCarry(bool c) { cpu.setFlag(x86::CF, c); }

	bool handleInt(x86::Cpu &c, int n) {
		const uint8_t ah = uint8_t(c.ax >> 8), al = uint8_t(c.ax);
		switch (n) {
		case 0x21:
			switch (ah) {
			case 0x19: c.ax = 0x0002; return true;
			case 0x1C: c.ax = 0x0001; c.cx = 512; c.dx = 1000; return true;
			case 0x25: c.ww(0, uint16_t(al * 4), c.dx); c.ww(0, uint16_t(al * 4 + 2), c.ds); return true;
			case 0x35: c.bx = c.rw(0, uint16_t(al * 4)); c.es = c.rw(0, uint16_t(al * 4 + 2)); return true;
			case 0x3D: {
				std::string name = readName(c.ds, c.dx), l = name;
				for (auto &ch : l) ch = char(std::tolower(static_cast<unsigned char>(ch)));
				const size_t slash = l.find_last_of("\\/:");
				if (slash != std::string::npos) l = l.substr(slash + 1);
				const auto it = names.find(l);
				if (it == names.end()) { std::fprintf(stderr, "open %s: not found\n", name.c_str()); c.ax = 2; setCarry(true); return true; }
				FILE *f = std::fopen((gameDir + "/" + it->second).c_str(), "rb");
				if (!f) { c.ax = 2; setCarry(true); return true; }
				std::fprintf(stderr, "open %s -> handle %d\n", name.c_str(), nextHandle);
				files[nextHandle] = f;
				c.ax = uint16_t(nextHandle++);
				setCarry(false);
				return true;
			}
			case 0x3E: { auto it = files.find(c.bx); if (it != files.end()) { std::fclose(it->second); files.erase(it); } setCarry(false); return true; }
			case 0x3F: {
				auto it = files.find(c.bx);
				if (it == files.end()) { c.ax = 6; setCarry(true); return true; }
				std::vector<uint8_t> buf(c.cx);
				const size_t n2 = std::fread(buf.data(), 1, buf.size(), it->second);
				for (size_t i = 0; i < n2; ++i) c.wb(c.ds, uint16_t(c.dx + i), buf[i]);
				c.ax = uint16_t(n2);
				setCarry(false);
				return true;
			}
			case 0x40: c.ax = c.cx; setCarry(false); return true;
			case 0x42: {
				auto it = files.find(c.bx);
				if (it == files.end()) { c.ax = 6; setCarry(true); return true; }
				const long off = long((uint32_t(c.cx) << 16) | c.dx);
				std::fseek(it->second, off, al == 0 ? SEEK_SET : al == 1 ? SEEK_CUR : SEEK_END);
				const long pos = std::ftell(it->second);
				c.ax = uint16_t(pos); c.dx = uint16_t(uint32_t(pos) >> 16);
				setCarry(false);
				return true;
			}
			case 0x4C: exited = true; c.halted = true; std::fprintf(stderr, "exit %d\n", al); return true;
			default: std::fprintf(stderr, "int 21h AH=%02X unhandled at %04X:%04X\n", ah, c.cs, c.ip); return true;
			}
		case 0x10:
			if (ah == 0) videoMode = al;
			return true;
		case 0x16:
			if (ah == 0) {
				if (keys.empty()) { c.ip = uint16_t(c.ip - 2); needKey = true; return true; }
				c.ax = keys.front(); keys.pop_front();
				return true;
			}
			if (ah == 1) {
				if (keys.empty()) c.setFlag(x86::ZF, true);
				else { c.setFlag(x86::ZF, false); c.ax = keys.front(); }
				return true;
			}
			c.ax = 0;
			return true;
		case 0x17: c.ax = 0x9000; return true;
		case 0x33: if (c.ax == 0) c.ax = 0; return true;   // no mouse
		case 0x25: case 0x26: c.ax = 0x0101; setCarry(true); return true;
		case 3: std::fprintf(stderr, "int3 at %04X:%04X\n", c.cs, c.ip); c.fault = "int3"; return true;
		default: return false;   // through the IVT (timer etc.)
		}
	}

	// runs until `maxSteps` instructions, a breakpoint, a key wait, exit or a fault; returns the reason
	std::string run(uint64_t maxSteps) {
		needKey = false;
		const uint64_t end = cpu.steps + maxSteps;
		while (cpu.steps < end) {
			if (!cpu.step()) return cpu.fault.empty() ? (exited ? "exit" : "halt") : cpu.fault;
			if (needKey) return "key";
			if (cpu.steps >= nextTick) {   // IRQ0 about 18.2 Hz of emulated time at ~1M instr/s
				nextTick = cpu.steps + 55000;
				uint32_t ticks = uint32_t(cpu.rw(0x40, 0x6C)) | (uint32_t(cpu.rw(0x40, 0x6E)) << 16);
				++ticks;
				cpu.ww(0x40, 0x6C, uint16_t(ticks)); cpu.ww(0x40, 0x6E, uint16_t(ticks >> 16));
				if (cpu.flag(x86::IF) && cpu.rw(0, 0x20) != 0) cpu.interrupt(8, false);
			}
			if (!breaks.empty() && breaks.count(x86::Cpu::lin(cpu.cs, cpu.ip))) return "bp";
		}
		return "steps";
	}

	fod::Image screenshot() {
		fod::Image img(320, 200);
		const size_t start = (size_t(ega.crtc[0xC]) << 8) | ega.crtc[0xD];
		static const uint32_t pal[16] = {0x000000, 0x0000aa, 0x00aa00, 0x00aaaa, 0xaa0000, 0xaa00aa, 0xaa5500, 0xaaaaaa,
		                                 0x555555, 0x5555ff, 0x55ff55, 0x55ffff, 0xff5555, 0xff55ff, 0xffff55, 0xffffff};
		for (int y = 0; y < 200; ++y)
			for (int x = 0; x < 320; ++x) {
				const size_t off = (start + size_t(y) * 40 + size_t(x >> 3)) & 0xFFFF;
				const int bit = 7 - (x & 7);
				int col = 0;
				for (int p = 0; p < 4; ++p) col |= ((ega.planes[p][off] >> bit) & 1) << p;
				const uint32_t c = pal[col];
				img.set(x, y, {uint8_t(c >> 16), uint8_t(c >> 8), uint8_t(c)});
			}
		return img;
	}
};

bool parseAddr(const char *s, uint16_t &seg, uint16_t &off) {
	unsigned a = 0, b = 0;
	if (std::sscanf(s, "%x:%x", &a, &b) != 2) return false;
	seg = uint16_t(a); off = uint16_t(b);
	return true;
}

} // namespace

int main(int argc, char **argv) {
	if (argc < 4) { std::fprintf(stderr, "usage: wlemu <image.bin> <gamedir> <commands...>\n"); return 1; }
	Machine m(argv[2]);
	const fod::Bytes img = fod::readFile(argv[1]);
	std::memcpy(&m.cpu.mem[0x10000], img.data(), img.size());
	// PSP at 0FF0:0000
	m.cpu.mem[0xFF00] = 0xCD; m.cpu.mem[0xFF01] = 0x20;
	x86::Cpu &c = m.cpu;
	c.cs = 0x1000; c.ip = 0x10B6;
	c.ds = c.es = 0x0FF0;
	c.ss = 0x3900; c.sp = 0x0200;
	c.setFlag(x86::IF, true);

	for (int i = 3; i < argc; ++i) {
		const std::string cmd = argv[i];
		if (cmd == "run" && i + 1 < argc) {
			const std::string why = m.run(uint64_t(std::atof(argv[++i]) * 1e6));
			std::printf("run: %s at %04X:%04X (%llu steps)\n", why.c_str(), c.cs, c.ip, (unsigned long long)c.steps);
		} else if (cmd == "steps" && i + 1 < argc) {
			const std::string why = m.run(uint64_t(std::atoll(argv[++i])));
			std::printf("steps: %s at %04X:%04X\n", why.c_str(), c.cs, c.ip);
		} else if (cmd == "key" && i + 1 < argc) {
			m.keys.push_back(uint16_t(std::strtoul(argv[++i], nullptr, 16)));
		} else if (cmd == "type" && i + 1 < argc) {
			for (char ch : std::string(argv[++i])) m.keys.push_back(uint16_t(uint8_t(ch)));
		} else if (cmd == "shot" && i + 1 < argc) {
			m.screenshot().writePng(argv[++i]);
		} else if (cmd == "stats") {
			for (int pl = 0; pl < 4; ++pl) { int nz = 0; for (int j = 0; j < 65536; ++j) nz += m.ega.planes[pl][j] != 0; std::printf("plane %d nonzero %d\n", pl, nz); }
			std::printf("videoMode %d mapMask %02X crtc start %02X%02X\n", m.videoMode, m.ega.mapMask, m.ega.crtc[12], m.ega.crtc[13]);
		} else if (cmd == "regs") {
			std::printf("AX=%04X BX=%04X CX=%04X DX=%04X SI=%04X DI=%04X BP=%04X SP=%04X CS=%04X DS=%04X ES=%04X SS=%04X IP=%04X FL=%04X\n", c.ax, c.bx, c.cx, c.dx, c.si, c.di, c.bp, c.sp, c.cs, c.ds, c.es, c.ss, c.ip, c.flags);
		} else if (cmd == "bp" && i + 1 < argc) {
			uint16_t s, o;
			if (parseAddr(argv[++i], s, o)) m.breaks.insert(x86::Cpu::lin(s, o));
		} else if (cmd == "clearbp") {
			m.breaks.clear();
		} else if (cmd == "trace" && i + 1 < argc) {
			int n = std::atoi(argv[++i]);
			while (n-- > 0) {
				std::printf("%04X:%04X ax=%04X bx=%04X cx=%04X dx=%04X si=%04X di=%04X\n", c.cs, c.ip, c.ax, c.bx, c.cx, c.dx, c.si, c.di);
				if (!c.step()) break;
			}
		} else if (cmd == "peek" && i + 2 < argc) {
			uint16_t s, o;
			if (!parseAddr(argv[++i], s, o)) continue;
			const int len = std::atoi(argv[++i]);
			for (int j = 0; j < len; ++j) {
				if (j % 16 == 0) std::printf("%s%04X:%04X:", j ? "\n" : "", s, uint16_t(o + j));
				std::printf(" %02X", c.rb(s, uint16_t(o + j)));
			}
			std::printf("\n");
		} else if (cmd == "poke" && i + 2 < argc) {
			uint16_t s, o;
			if (!parseAddr(argv[++i], s, o)) continue;
			c.wb(s, o, uint8_t(std::strtoul(argv[++i], nullptr, 16)));
		}
	}
	return 0;
}
