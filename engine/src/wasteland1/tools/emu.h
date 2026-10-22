// Shared emulator machine for wlemu / wltest: 8086 CPU + DOS, BIOS and EGA stubs.
#pragma once
#include "wasteland1/x86.h"
#include "wasteland1/ega.h"
#include "shared/bytes.h"
#include "shared/image.h"
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <deque>
#include <filesystem>
#include <map>
#include <set>

namespace emu {

struct Machine {
	x86::Cpu cpu;
	wl::Ega ega;
	std::string gameDir;
	std::map<int, FILE *> files;
	int nextHandle = 5;
	std::deque<uint16_t> keys;
	bool needKey = false, exited = false;
	bool detectIdle = false;                      // stop run() when the game only polls an empty keyboard buffer
	int idlePolls = 0;                            // consecutive empty keyboard polls: the game is waiting for input
	long watchVideo = -1;                         // video memory offset to report writes to (debugging)
	uint64_t nextTick = 0;
	std::set<uint32_t> breaks;
	int videoMode = 3;
	std::map<std::string, std::string> names;   // lower case -> real file name
	std::map<uint32_t, std::string> syms;
	std::map<std::string, std::pair<uint64_t, uint64_t>> callCounts;   // name -> (first-use order, count)
	uint64_t callOrder = 0;
	std::map<int, std::string> openNames;       // handle -> real file name (for state files)

	explicit Machine(const std::string &dir) : gameDir(dir) {
		for (const auto &e : std::filesystem::directory_iterator(dir)) {
			std::string n = e.path().filename().string(), l = n;
			for (auto &c : l) c = char(std::tolower(static_cast<unsigned char>(c)));
			names[l] = n;
		}
		cpu.onWrite = [this](uint32_t a, uint8_t v) {
			if (watchVideo >= 0 && uint32_t(a - 0xA0000) == uint32_t(watchVideo)) {
				std::printf("video write %05X=%02X mask=%X at %04X:%04X stack:", a - 0xA0000, v, ega.mapMask, cpu.cs, cpu.ip);
				for (int k = 0; k < 8; ++k) std::printf(" %04X", cpu.rw(cpu.ss, uint16_t(cpu.sp + 2 * k)));
				std::printf("\n");
			}
			ega.write(a, v);
		};
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
				files[nextHandle] = f; openNames[nextHandle] = it->second;
				c.ax = uint16_t(nextHandle++);
				setCarry(false);
				return true;
			}
			case 0x3E: { auto it = files.find(c.bx); if (it != files.end()) { std::fclose(it->second); files.erase(it); openNames.erase(c.bx); } setCarry(false); return true; }
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
				if (keys.empty()) { c.setFlag(x86::ZF, true); if (detectIdle && ++idlePolls > 30) needKey = true; }
				else { idlePolls = 0; c.setFlag(x86::ZF, false); c.ax = keys.front(); }
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
		idlePolls = 0;
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

	// Boots the unpacked image (see wlemu): image at 1000:0000, PSP at 0FF0, stack 3900:0200.
	void boot(const shared::Bytes &img) {
		std::memcpy(&cpu.mem[0x10000], img.data(), img.size());
		cpu.mem[0xFF00] = 0xCD; cpu.mem[0xFF01] = 0x20;
		cpu.cs = 0x1000; cpu.ip = 0x10B6;
		cpu.ds = cpu.es = 0x0FF0;
		cpu.ss = 0x3900; cpu.sp = 0x0200;
		cpu.setFlag(x86::IF, true);
	}

	// State files (registers, 1 MB of memory, EGA, open files); the first part is what wl::Machine::loadState reads.
	bool saveState(const std::string &path) {
		FILE *f = std::fopen(path.c_str(), "wb");
		if (!f) return false;
		const uint16_t regs[14] = {cpu.ax, cpu.cx, cpu.dx, cpu.bx, cpu.sp, cpu.bp, cpu.si, cpu.di, cpu.es, cpu.cs, cpu.ss, cpu.ds, cpu.ip, cpu.flags};
		std::fwrite(regs, sizeof regs, 1, f);
		std::fwrite(cpu.mem, 1, (size_t(1) << 20), f);
		std::fwrite(&ega, sizeof ega, 1, f);
		const int cnt = int(files.size());
		std::fwrite(&cnt, sizeof cnt, 1, f);
		for (auto &kv : files) {
			const int h = kv.first, len = int(openNames[h].size());
			const long pos = std::ftell(kv.second);
			std::fwrite(&h, sizeof h, 1, f); std::fwrite(&len, sizeof len, 1, f); std::fwrite(openNames[h].data(), 1, size_t(len), f); std::fwrite(&pos, sizeof pos, 1, f);
		}
		std::fwrite(&nextHandle, sizeof nextHandle, 1, f);
		std::fclose(f);
		return true;
	}

	bool loadState(const std::string &path) {
		FILE *f = std::fopen(path.c_str(), "rb");
		if (!f) return false;
		uint16_t regs[14];
		bool ok = std::fread(regs, sizeof regs, 1, f) == 1;
		if (ok) {
			cpu.ax = regs[0]; cpu.cx = regs[1]; cpu.dx = regs[2]; cpu.bx = regs[3]; cpu.sp = regs[4]; cpu.bp = regs[5]; cpu.si = regs[6]; cpu.di = regs[7];
			cpu.es = regs[8]; cpu.cs = regs[9]; cpu.ss = regs[10]; cpu.ds = regs[11]; cpu.ip = regs[12]; cpu.flags = regs[13];
			ok = std::fread(cpu.mem, 1, (size_t(1) << 20), f) == (size_t(1) << 20) && std::fread(&ega, sizeof ega, 1, f) == 1;
		}
		for (auto &kv : files) std::fclose(kv.second);
		files.clear(); openNames.clear();
		int cnt = 0;
		if (ok && std::fread(&cnt, sizeof cnt, 1, f) == 1)
			for (int k = 0; k < cnt; ++k) {
				int h = 0, len = 0; long pos = 0;
				if (std::fread(&h, sizeof h, 1, f) != 1 || std::fread(&len, sizeof len, 1, f) != 1) break;
				std::string nm(size_t(len), 0);
				if (std::fread(nm.data(), 1, size_t(len), f) != size_t(len) || std::fread(&pos, sizeof pos, 1, f) != 1) break;
				FILE *g = std::fopen((gameDir + "/" + nm).c_str(), "rb");
				if (g) { std::fseek(g, pos, SEEK_SET); files[h] = g; openNames[h] = nm; }
			}
		if (ok && std::fread(&nextHandle, sizeof nextHandle, 1, f) != 1) ok = false;
		std::fclose(f);
		return ok;
	}

	// Runs the near function at `off` of the code segment (CS = 1000h) until it returns; registers are taken from/left in cpu.
	std::string callNear(uint16_t off, uint64_t maxSteps = 20000000) {
		cpu.cs = 0x1000; cpu.ds = 0x1CE2; cpu.ss = 0x3900;
		cpu.push(0x0000);                   // return to 1000:0000, treated as a breakpoint
		cpu.ip = off;
		breaks.insert(0x10000);
		const uint64_t savedTick = nextTick;
		nextTick = ~0ull;                   // no timer interrupts: keeps the result deterministic
		const std::string why = run(maxSteps);
		nextTick = savedTick;
		breaks.erase(0x10000);
		return why;
	}

	shared::Image screenshot() {
		shared::Image img(320, 200);
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


} // namespace emu
