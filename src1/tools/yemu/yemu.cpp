/* yemu <gameDir> <program> [options]
 *
 * Development tool: runs an original Chapter 1 executable (normally SW.EXE) in an 8086 emulator with the EGA, DOS, BIOS and mouse
 * emulated, on a scratch copy of the game directory. The real VID16.DRV runs in the emulator, so the screen captures are what
 * the original produces; they are the reference for the reimplementation.
 *
 *   --fonts <dir>     directory with FONT8X8.BIN / FONT8X14.BIN (the BIOS ROM fonts the driver reads)
 *   --work <dir>      scratch copy of the game dir (default: $TMP/yemu_work); recreated on every run
 *   --out <dir>       where "shot" steps write their PNGs (default: .)
 *   --script <file>   input script (below)
 *   --max <n>         instruction limit (default 400 million)
 *   --trace62         print a histogram of INT 62h calls at the end
 *   --log62 <file>    write one line per INT 62h call (function, context fields, image-data hash); see Trace format below
 *   --dump <dir>      at every program start and end write <n>_<PROG>_{start,end}.ydmp (parameter block, context, cursor store, EGA planes)
 *   --clock <n>       make the DOS clock's seconds/hundredths (INT 21h/2Ch DX) always n: reproducible random seeds (the ported
 *                     programs use the same value)
 *   --compare         run Yendor1::Vid in lockstep with the real driver and report any difference
 *
 * Script lines (times are virtual seconds, 1 s = 3 million instructions):
 *   wait <s>          advance the script clock        key <hex byte>      press a key (DOS INT 21h/06h byte)
 *   ext <scan>        an extended key (0 + scan)      text <string>       type characters
 *   move <dx> <dy>    relative mouse motion           moveto <x> <y>      absolute mouse position
 *   click <l|r>       press and release a button      shot <name>         save the displayed page as <name>.png
 *   quit              stop the run
 */

#include "Compare.h"
#include "Machine.h"
#include "data/Palette.h"
#include "tools/Png.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

namespace fs = std::filesystem;

static std::vector<uint8_t> loadFile(const std::string &p) {
	std::ifstream f(p, std::ios::binary);
	return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

static void savePng(Yemu::Machine &m, const std::string &path) {
	static const uint8_t defaultPal[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F};
	bool any = false;
	for (int i = 0; i < 16; i++)
		any = any || m.ega.egaPalette[i];
	Yendor1::Data::Rgb pal[16];
	for (int i = 0; i < 16; i++)
		pal[i] = Yendor1::Data::egaToRgb(any ? m.ega.egaPalette[i] : defaultPal[i]);
	std::vector<uint8_t> rgb(640 * 350 * 3);
	for (int y = 0; y < 350; y++)
		for (int x = 0; x < 640; x++) {
			const Yendor1::Data::Rgb &c = pal[m.ega.pixel(m.ega.displayedPage, x, y)];
			uint8_t *d = &rgb[(y * 640 + x) * 3];
			d[0] = c.r; d[1] = c.g; d[2] = c.b;
		}
	if (!Yendor1::Tools::writePng(path.c_str(), 640, 350, &rgb[0]))
		fprintf(stderr, "cannot write %s\n", path.c_str());
	else
		printf("shot: %s (page %d)\n", path.c_str(), m.ega.displayedPage);
}

int main(int argc, char **argv) {
	if (argc < 3) {
		fprintf(stderr, "usage: %s <gameDir> <program> [--fonts dir] [--work dir] [--out dir] [--script file] [--max n] [--trace62]\n", argv[0]);
		return 2;
	}
	const std::string gameDir = argv[1], program = argv[2];
	std::string fontsDir, workDir, outDir = ".", scriptFile;
	uint64_t maxInstr = 400000000ULL;
	int fixedClock = -1;
	unsigned long logPc = 0xFFFFFFFFul;
	unsigned traceWin = 0;
	bool trace62 = false, compare = false;
	std::string log62File, dumpDir;
	for (int i = 3; i < argc; i++) {
		const std::string a = argv[i];
		if (a == "--fonts" && i + 1 < argc) fontsDir = argv[++i];
		else if (a == "--work" && i + 1 < argc) workDir = argv[++i];
		else if (a == "--out" && i + 1 < argc) outDir = argv[++i];
		else if (a == "--script" && i + 1 < argc) scriptFile = argv[++i];
		else if (a == "--max" && i + 1 < argc) maxInstr = strtoull(argv[++i], 0, 10);
		else if (a == "--clock" && i + 1 < argc) fixedClock = atoi(argv[++i]);
		else if (a == "--logpc" && i + 1 < argc) logPc = strtoul(argv[++i], 0, 16);
		else if (a == "--tracewin" && i + 1 < argc) traceWin = (unsigned)atoi(argv[++i]);
		else if (a == "--trace62") trace62 = true;
		else if (a == "--compare") compare = true;
		else if (a == "--dump" && i + 1 < argc) dumpDir = argv[++i];
		else if (a == "--log62" && i + 1 < argc) log62File = argv[++i];
	}
	if (workDir.empty())
		workDir = (fs::temp_directory_path() / "yemu_work").string();
	std::error_code ec;
	fs::remove_all(workDir, ec);
	fs::create_directories(workDir, ec);
	fs::copy(gameDir, workDir, fs::copy_options::recursive, ec);
	if (ec) {
		fprintf(stderr, "cannot copy %s to %s: %s\n", gameDir.c_str(), workDir.c_str(), ec.message().c_str());
		return 1;
	}
	fs::create_directories(outDir, ec);

	Yemu::Machine m(workDir);
	if (!fontsDir.empty())
		m.setFonts(loadFile(fontsDir + "/FONT8X8.BIN"), loadFile(fontsDir + "/FONT8X14.BIN"));
	if (const char *w = getenv("YEMU_WATCH")) {
		unsigned c, i;
		if (sscanf(w, "%x:%x", &c, &i) == 2) { m.watchOn = true; m.watchCs = (uint16_t)c; m.watchIp = (uint16_t)i; }
	}
	m.onShot = [&](const std::string &name) { savePng(m, outDir + "/" + name + ".png"); };
	std::map<int, uint64_t> hist;
	int watch62 = -1;
	if (const char *w = getenv("YEMU_WATCH62")) watch62 = (int)strtol(w, 0, 16);
	if (trace62 || watch62 >= 0) {
		m.int62Hook = [&](Yemu::Machine &mm) {
			const int ah = mm.cpu.r[Yemu::AX] >> 8;
			hist[ah]++;
			if (ah == watch62 && hist[ah] == 1)
				mm.log += "first INT 62h AH=" + std::to_string(ah) + "; recent: " + mm.dumpRecent(200) + "\n";
			return false;
		};
	}

	FILE *trace62Out = 0;
	if (!log62File.empty()) {
		trace62Out = fopen(log62File.c_str(), "w");
		if (logPc != 0xFFFFFFFFul)
			m.onPcLog = [&](const std::string &l) { if (trace62Out) fprintf(trace62Out, "#%s\n", l.c_str()); };
		m.onProgramStart = [&](Yemu::Machine &mm, int) {
			if (trace62Out)
				fprintf(trace62Out, "# start %s\n", mm.programTrace.back().c_str());
		};
		auto fnv = [](const uint8_t *p, size_t n) { uint32_t h = 2166136261u; for (size_t i = 0; i < n; i++) { h ^= p[i]; h *= 16777619u; } return h; };
		m.int62Hook = [&, fnv](Yemu::Machine &mm) {
			const int ah = mm.cpu.r[Yemu::AX] >> 8, al = mm.cpu.r[Yemu::AX] & 0xFF;
			if (!mm.ctxSegment || !trace62Out)
				return false;
			const uint16_t cs = mm.ctxSegment;
			auto w = [&](int off) { return (unsigned)mm.cpu.rw(cs, (uint16_t)off); };
			auto b = [&](int off) { return (unsigned)mm.mem[Yemu::Cpu::lin(cs, (uint16_t)off)]; };
			unsigned hash = 0, rows = w(0x10), bytes = w(0x12);
			const uint32_t base = Yemu::Cpu::lin((uint16_t)w(0x16), (uint16_t)w(0x18));
			if (ah == 0x0A && rows * bytes * 4 < 0x10000)
				hash = fnv(&mm.mem[base & 0xFFFFF], rows * bytes * 4);
			else if (ah == 0x0C) {
				const unsigned hr = mm.mem[base & 0xFFFFF] | (mm.mem[(base + 1) & 0xFFFFF] << 8), hb = mm.mem[(base + 2) & 0xFFFFF] | (mm.mem[(base + 3) & 0xFFFFF] << 8);
				if (hr * hb * 4 < 0x10000)
					hash = fnv(&mm.mem[(base + 5) & 0xFFFFF], hr * hb * 4);
			}
			fprintf(trace62Out, "%02X %02X x1=%u y1=%u x2=%u y2=%u x3=%u y3=%u col=%u pg=%u dst=%u key=%u rows=%u w=%u mask=%02X font=%u tx=%u ty=%u ch=%u fg=%u bg=%u img=%08X\n",
				ah, al, w(0x1A), w(0x1C), w(0x1E), w(0x20), w(0x22), w(0x24), b(0x30), b(0x31), b(0x32), b(0x34), rows, bytes, b(0x14), b(0x35),
				b(0x2A), b(0x2B), b(0x2C), b(0x2E), b(0x2F), hash);
			return false;
		};
	}
	int dumpCount = 0;
	std::map<int, std::string> dumpName;
	if (!dumpDir.empty()) {
		fs::create_directories(dumpDir, ec);
		auto writeDump = [&](Yemu::Machine &mm, int psp, const char *when, const std::string &name) {
			char fn[256];
			snprintf(fn, sizeof(fn), "%s/%02d_%s_%s.ydmp", dumpDir.c_str(), dumpCount, name.c_str(), when);
			FILE *f = fopen(fn, "wb");
			if (!f) return;
			auto w16 = [&](uint16_t v) { fputc(v & 0xFF, f); fputc(v >> 8, f); };
			fwrite("YDMP", 1, 4, f);
			const uint16_t cs = mm.ctxSegment;
			const uint16_t cur = cs ? mm.cpu.rw(cs, 0x96E) : 0;
			w16(cs); w16(cur);
			fwrite(&mm.mem[Yemu::Cpu::lin((uint16_t)psp, 0x80)], 1, 128, f);
			if (cs) fwrite(&mm.mem[Yemu::Cpu::lin(cs, 0)], 1, 0x980, f); else { for (int i = 0; i < 0x980; i++) fputc(0, f); }
			if (cur) fwrite(&mm.mem[Yemu::Cpu::lin(cur, 0)], 1, 13 * 0x85, f); else { for (int i = 0; i < 13 * 0x85; i++) fputc(0, f); }
			for (int p = 0; p < 4; p++) fwrite(mm.ega.plane[p], 1, 65536, f);
			fputc(mm.ega.displayedPage, f);
			fclose(f);
		};
		m.onProgramStart = [&, writeDump](Yemu::Machine &mm, int psp) {
			std::string name = mm.programTrace.back();
			const size_t dot = name.find('.');
			if (dot != std::string::npos) name = name.substr(0, dot);
			dumpName[psp] = name;
			++dumpCount;
			writeDump(mm, psp, "start", name);
			if (trace62Out) fprintf(trace62Out, "# start %s\n", mm.programTrace.back().c_str());
		};
		m.onProgramEnd = [&, writeDump](Yemu::Machine &mm, int psp) {
			writeDump(mm, psp, "end", dumpName[psp]);
			if (trace62Out) fprintf(trace62Out, "# end %s\n", dumpName[psp].c_str());
		};
	}
	Yemu::VidCompare vidCompare;
	if (compare)
		vidCompare.attach(m, fontsDir);
	// script
	if (!scriptFile.empty()) {
		std::ifstream sf(scriptFile);
		std::string line;
		double t = 0;
		int mx = 320, my = 175;
		while (std::getline(sf, line)) {
			std::istringstream ls(line);
			std::string cmd;
			if (!(ls >> cmd) || cmd[0] == '#')
				continue;
			Yemu::ScriptStep st;
			st.at = (uint64_t)(t * Yemu::Machine::kInstrPerSecond);
			st.a = st.b = st.c = 0;
			if (cmd == "wait") { double s; ls >> s; t += s; continue; }
			else if (cmd == "key") { unsigned v; ls >> std::hex >> v; st.kind = Yemu::ScriptStep::Key; st.a = (int)v; st.b = -1; m.schedule(st); }
			else if (cmd == "ext") { unsigned v; ls >> std::hex >> v; st.kind = Yemu::ScriptStep::Key; st.a = 0; st.b = (int)v; m.schedule(st); }
			else if (cmd == "text") {
				std::string rest;
				std::getline(ls, rest);
				if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
				for (size_t i = 0; i < rest.size(); i++) {
					st.kind = Yemu::ScriptStep::Key; st.a = (unsigned char)rest[i]; st.b = -1; st.at += 20000; m.schedule(st);
				}
			}
			else if (cmd == "move") { int dx, dy; ls >> dx >> dy; st.kind = Yemu::ScriptStep::MouseMove; st.a = dx; st.b = dy; mx += dx; my += dy; m.schedule(st); }
			else if (cmd == "moveto") { int x, y; ls >> x >> y; st.kind = Yemu::ScriptStep::MouseTo; st.a = x; st.b = y; mx = x; my = y; m.schedule(st); }
			else if (cmd == "click") {
				std::string b; ls >> b;
				st.kind = Yemu::ScriptStep::MouseDown; st.a = (b == "r") ? 1 : 0; m.schedule(st);
				st.kind = Yemu::ScriptStep::MouseUp; st.at += 150000; m.schedule(st);
			}
			else if (cmd == "shot") { st.kind = Yemu::ScriptStep::Shot; ls >> st.name; m.schedule(st); }
			else if (cmd == "quit") { st.kind = Yemu::ScriptStep::Quit; m.schedule(st); }
			else fprintf(stderr, "script: unknown command '%s'\n", cmd.c_str());
		}
	}

	m.fixedClockDx = fixedClock;
	m.logPcLinear = (uint32_t)logPc;
	m.traceWinHit = traceWin;
	const int code = m.run(program, maxInstr);
	printf("exit code %d after %llu instructions (%.1f virtual seconds)\n", code, (unsigned long long)m.cpu.instructions,
		(double)m.cpu.instructions / Yemu::Machine::kInstrPerSecond);
	printf("programs started:");
	for (size_t i = 0; i < m.programTrace.size(); i++)
		printf(" %s", m.programTrace[i].c_str());
	printf("\n");
	if (!m.console.empty())
		printf("console output:\n%s\n", m.console.c_str());
	if (!m.log.empty())
		printf("log:\n%s", m.log.c_str());
	if (trace62Out)
		fclose(trace62Out);
	if (compare)
		vidCompare.report(stdout);
	if (getenv("YEMU_RECENT"))
		printf("recent: %s\n", m.dumpRecent(80).c_str());
	if (trace62) {
		printf("INT 62h histogram:\n");
		for (std::map<int, uint64_t>::const_iterator it = hist.begin(); it != hist.end(); ++it)
			printf("  AH=%02X %llu\n", it->first, (unsigned long long)it->second);
	}
	return 0;
}
