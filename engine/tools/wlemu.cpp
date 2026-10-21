// wlemu: runs the unpacked WL.EXE image in a small 8086 interpreter with DOS/BIOS/EGA stubs. Reference oracle for the
// Wasteland 1 port: scripted keys, screenshots, breakpoints and memory peeks.
//
//   wlemu <image.bin> <gamedir> <command>...
//   commands: run <millionSteps> | steps <n> | key <hex scancode+ascii> | type <text> | shot <png> | regs
//             bp <seg:off> | trace <n> | peek <seg:off> <len> | poke <seg:off> <byte>... | call <seg:off> | ticks <n>
#include "emu.h"
#include "fod/gfx.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using emu::Machine;

namespace {

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
		} else if (cmd == "dump" && i + 1 < argc) {   // dump <file>: 64000 bytes, one palette index per pixel
			FILE *f = std::fopen(argv[++i], "wb");
			if (f) {
				const size_t start = (size_t(m.ega.crtc[0xC]) << 8) | m.ega.crtc[0xD];
				for (int y = 0; y < 200; ++y)
					for (int x = 0; x < 320; ++x) {
						const size_t off = (start + size_t(y) * 40 + size_t(x >> 3)) & 0xFFFF;
						int col = 0;
						for (int p = 0; p < 4; ++p) col |= ((m.ega.planes[p][off] >> (7 - (x & 7))) & 1) << p;
						std::fputc(col, f);
					}
				std::fclose(f);
			}
		} else if (cmd == "stats") {
			for (int pl = 0; pl < 4; ++pl) { int nz = 0; for (int j = 0; j < 65536; ++j) nz += m.ega.planes[pl][j] != 0; std::printf("plane %d nonzero %d\n", pl, nz); }
			std::printf("videoMode %d mapMask %02X crtc start %02X%02X\n", m.videoMode, m.ega.mapMask, m.ega.crtc[12], m.ega.crtc[13]);
		} else if (cmd == "syms" && i + 1 < argc) {   // syms <file>: lines "<linear hex> <name>"
			FILE *f = std::fopen(argv[++i], "r");
			char name[128];
			unsigned addr;
			while (f && std::fscanf(f, "%x %127s", &addr, name) == 2) m.syms[addr] = name;
			if (f) std::fclose(f);
		} else if (cmd == "calls" && i + 1 < argc) {   // calls on|off: log every function entry
			const bool on = std::string(argv[++i]) == "on";
			if (on) {
				m.cpu.onStep = [&m](x86::Cpu &cpu) {
					const uint32_t a = x86::Cpu::lin(cpu.cs, cpu.ip);
					const auto it = m.syms.find(a);
					if (it == m.syms.end()) return;
					std::printf("[%8llu] %s ax=%04X bx=%04X cx=%04X dx=%04X si=%04X di=%04X ret=%04X\n", (unsigned long long)cpu.steps, it->second.c_str(),
					            cpu.ax, cpu.bx, cpu.cx, cpu.dx, cpu.si, cpu.di, cpu.rw(cpu.ss, cpu.sp));
				};
			} else m.cpu.onStep = nullptr;
		} else if (cmd == "callfn" && i + 1 < argc) {   // callfn <off> [reg=hex ...]: run a near function of the code segment until it returns
			const uint16_t off = uint16_t(std::strtoul(argv[++i], nullptr, 16));
			while (i + 1 < argc && std::strchr(argv[i + 1], '=')) {
				const std::string a = argv[++i];
				const uint16_t v = uint16_t(std::strtoul(a.c_str() + a.find('=') + 1, nullptr, 16));
				const std::string r = a.substr(0, a.find('='));
				if (r == "ax") c.ax = v; else if (r == "bx") c.bx = v; else if (r == "cx") c.cx = v; else if (r == "dx") c.dx = v;
				else if (r == "si") c.si = v; else if (r == "di") c.di = v; else if (r == "bp") c.bp = v; else if (r == "es") c.es = v; else if (r == "ds") c.ds = v;
				else if (r == "al") c.ax = uint16_t((c.ax & 0xFF00) | (v & 0xFF)); else if (r == "ah") c.ax = uint16_t((c.ax & 0xFF) | (v << 8));
				else if (r == "bl") c.bx = uint16_t((c.bx & 0xFF00) | (v & 0xFF)); else if (r == "bh") c.bx = uint16_t((c.bx & 0xFF) | (v << 8));
				else if (r == "cl") c.cx = uint16_t((c.cx & 0xFF00) | (v & 0xFF)); else if (r == "ch") c.cx = uint16_t((c.cx & 0xFF) | (v << 8));
				else if (r == "dl") c.dx = uint16_t((c.dx & 0xFF00) | (v & 0xFF)); else if (r == "dh") c.dx = uint16_t((c.dx & 0xFF) | (v << 8));
			}
			const uint16_t savedCs = c.cs, savedIp = c.ip, savedSp = c.sp, savedDs = c.ds, savedEs = c.es, savedSs = c.ss;
			c.cs = 0x1000; c.ds = 0x1CE2; c.ss = 0x3900;
			c.push(0x0000);                     // return to 1000:0000, a breakpoint
			c.ip = off;
			m.breaks.insert(0x10000);
			const uint64_t before = c.steps;
			std::string why = m.run(20000000);
			m.breaks.erase(0x10000);
			std::printf("callfn %04X: %s after %llu steps  AX=%04X BX=%04X CX=%04X DX=%04X SI=%04X DI=%04X CF=%d ZF=%d SF=%d\n", off, why.c_str(), (unsigned long long)(c.steps - before),
			            c.ax, c.bx, c.cx, c.dx, c.si, c.di, c.flag(x86::CF), c.flag(x86::ZF), c.flag(x86::SF));
			(void)savedCs; (void)savedIp; (void)savedSp; (void)savedDs; (void)savedEs; (void)savedSs;
		} else if (cmd == "callsum" && i + 1 < argc) {   // callsum on|print|clear: per-function call counts in order of first use
			const std::string sub = argv[++i];
			if (sub == "on") {
				m.cpu.onStep = [&m](x86::Cpu &cpu) {
					const auto it = m.syms.find(x86::Cpu::lin(cpu.cs, cpu.ip));
					if (it == m.syms.end()) return;
					auto &e = m.callCounts[it->second];
					if (e.first == 0) e.first = ++m.callOrder;
					++e.second;
				};
			} else if (sub == "print") {
				std::vector<std::pair<uint64_t, std::string>> order;
				for (auto &kv : m.callCounts) order.push_back({kv.second.first, kv.first + " x" + std::to_string(kv.second.second)});
				std::sort(order.begin(), order.end());
				for (auto &o : order) std::printf("%s\n", o.second.c_str());
			} else if (sub == "clear") { m.callCounts.clear(); m.callOrder = 0; }
		} else if (cmd == "save" && i + 1 < argc) {
			FILE *f = std::fopen(argv[++i], "wb");
			if (f) {
				const uint16_t regs[14] = {c.ax, c.cx, c.dx, c.bx, c.sp, c.bp, c.si, c.di, c.es, c.cs, c.ss, c.ds, c.ip, c.flags};
				std::fwrite(regs, sizeof regs, 1, f);
				std::fwrite(c.mem.data(), 1, c.mem.size(), f);
				std::fwrite(&m.ega, sizeof m.ega, 1, f);
				const int cnt = int(m.files.size());
				std::fwrite(&cnt, sizeof cnt, 1, f);
				for (auto &kv : m.files) {
					const int h = kv.first, len = int(m.openNames[h].size());
					const long pos = std::ftell(kv.second);
					std::fwrite(&h, sizeof h, 1, f); std::fwrite(&len, sizeof len, 1, f); std::fwrite(m.openNames[h].data(), 1, size_t(len), f); std::fwrite(&pos, sizeof pos, 1, f);
				}
				std::fwrite(&m.nextHandle, sizeof m.nextHandle, 1, f);
				std::fclose(f);
			}
		} else if (cmd == "load" && i + 1 < argc) {
			FILE *f = std::fopen(argv[++i], "rb");
			if (f) {
				uint16_t regs[14];
				if (std::fread(regs, sizeof regs, 1, f) == 1) {
					c.ax = regs[0]; c.cx = regs[1]; c.dx = regs[2]; c.bx = regs[3]; c.sp = regs[4]; c.bp = regs[5]; c.si = regs[6]; c.di = regs[7];
					c.es = regs[8]; c.cs = regs[9]; c.ss = regs[10]; c.ds = regs[11]; c.ip = regs[12]; c.flags = regs[13];
					if (std::fread(c.mem.data(), 1, c.mem.size(), f) != c.mem.size()) std::fprintf(stderr, "short state file\n");
					if (std::fread(&m.ega, sizeof m.ega, 1, f) != 1) std::fprintf(stderr, "short state file\n");
					for (auto &kv : m.files) std::fclose(kv.second);
					m.files.clear(); m.openNames.clear();
					int cnt = 0;
					if (std::fread(&cnt, sizeof cnt, 1, f) == 1)
						for (int k = 0; k < cnt; ++k) {
							int h = 0, len = 0; long pos = 0;
							if (std::fread(&h, sizeof h, 1, f) != 1 || std::fread(&len, sizeof len, 1, f) != 1) break;
							std::string nm(size_t(len), 0);
							if (std::fread(nm.data(), 1, size_t(len), f) != size_t(len) || std::fread(&pos, sizeof pos, 1, f) != 1) break;
							FILE *g = std::fopen((m.gameDir + "/" + nm).c_str(), "rb");
							if (g) { std::fseek(g, pos, SEEK_SET); m.files[h] = g; m.openNames[h] = nm; }
						}
					if (std::fread(&m.nextHandle, sizeof m.nextHandle, 1, f) != 1) std::fprintf(stderr, "short state file\n");
				}
				std::fclose(f);
			}
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
