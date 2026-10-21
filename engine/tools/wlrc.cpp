// wlrc <image.bin> <gamedir> <command> ...: runs the statically recompiled WL.EXE on a native Machine.
// Commands are executed in order whenever the program polls the keyboard with an empty buffer:
//   key <hex>    push a BIOS key (scancode << 8 | ascii)      keys <text>   push characters
//   wait <n>     skip n empty polls                            shot <png>    screenshot of the visible page
//   save <file>  state file in the wlemu layout                tick <n>      edges per timer interrupt
#include "wl/recomp_rt.h"
#include "fod/gfx.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace wl;

static fod::Image screenshot(const Ega &ega) {
	static const uint32_t pal[16] = {0x000000, 0x0000aa, 0x00aa00, 0x00aaaa, 0xaa0000, 0xaa00aa, 0xaa5500, 0xaaaaaa,
	                                 0x555555, 0x5555ff, 0x55ff55, 0x55ffff, 0xff5555, 0xff55ff, 0xffff55, 0xffffff};
	fod::Image img(320, 200);
	for (int y = 0; y < 200; ++y)
		for (int x = 0; x < 320; ++x) {
			const uint32_t c = pal[ega.pixel(x, y)];
			img.set(x, y, {uint8_t(c >> 16), uint8_t(c >> 8), uint8_t(c)});
		}
	return img;
}

int main(int argc, char **argv) {
	if (argc < 3) { std::fprintf(stderr, "usage: wlrc <image.bin> <gamedir> [key <hex> | keys <text> | wait <n> | shot <png> | save <file> | tick <n>]...\n"); return 1; }
	Machine m;
	struct Report { Machine &m; ~Report() { if (m.sys) for (uint16_t t : m.sys->unknownTargets) std::fprintf(stderr, "indirect target %04X is not a function start\n", t); } } report{m};
	rc::Regs r;
	int waitPolls = 0, next = 3;
	try {
		rc::boot(m, fod::readFile(argv[1]), argv[2], r);
		if (const char *e = std::getenv("WL_MAXEDGES")) m.sys->maxEdges = uint64_t(std::atoll(e));
		m.onIdle = [&]() {
			if (waitPolls > 0) { --waitPolls; return; }
			while (next < argc) {
				const std::string cmd = argv[next++];
				if (cmd == "key" && next < argc) m.keys.push_back(uint16_t(std::strtoul(argv[next++], nullptr, 16)));
				else if (cmd == "keys" && next < argc) { for (const char *p = argv[next++]; *p; ++p) m.keys.push_back(uint16_t(uint8_t(*p))); }
				else if (cmd == "wait" && next < argc) { waitPolls = std::atoi(argv[next++]); return; }
				else if (cmd == "shot" && next < argc) screenshot(m.ega).writePng(argv[next++]);
				else if (cmd == "save" && next < argc) m.saveState(argv[next++]);
				else if (cmd == "maxedges" && next < argc) m.sys->maxEdges = uint64_t(std::atoll(argv[next++]));
				else if (cmd == "diff" && next < argc) { Machine o; if (!o.loadState(argv[next++])) std::fputs("cannot load state", stderr); else std::printf("%zu differences", m.diff(o, 30)); std::puts(""); }
				else if (cmd == "tick" && next < argc) m.sys->edgesPerTick = uint64_t(std::atoll(argv[next++]));
				else { std::fprintf(stderr, "bad command %s\n", cmd.c_str()); throw rc::ExitProgram{1}; }
				if (!m.keys.empty()) return;
			}
			throw rc::ExitProgram{0};
		};
		rc::run(m, r);
	} catch (const rc::ExitProgram &e) {
		return e.code;
	} catch (const std::exception &e) {
		std::fprintf(stderr, "error: %s\n", e.what());
		return 2;
	}
	return 0;
}
