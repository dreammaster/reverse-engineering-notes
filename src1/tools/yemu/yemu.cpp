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
 *
 * Script lines (times are virtual seconds, 1 s = 3 million instructions):
 *   wait <s>          advance the script clock        key <hex byte>      press a key (DOS INT 21h/06h byte)
 *   ext <scan>        an extended key (0 + scan)      text <string>       type characters
 *   move <dx> <dy>    relative mouse motion           moveto <x> <y>      absolute mouse position
 *   click <l|r>       press and release a button      shot <name>         save the displayed page as <name>.png
 *   quit              stop the run
 */

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
	bool trace62 = false;
	for (int i = 3; i < argc; i++) {
		const std::string a = argv[i];
		if (a == "--fonts" && i + 1 < argc) fontsDir = argv[++i];
		else if (a == "--work" && i + 1 < argc) workDir = argv[++i];
		else if (a == "--out" && i + 1 < argc) outDir = argv[++i];
		else if (a == "--script" && i + 1 < argc) scriptFile = argv[++i];
		else if (a == "--max" && i + 1 < argc) maxInstr = strtoull(argv[++i], 0, 10);
		else if (a == "--trace62") trace62 = true;
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
	m.onShot = [&](const std::string &name) { savePng(m, outDir + "/" + name + ".png"); };
	std::map<int, uint64_t> hist;
	if (trace62) {
		m.int62Hook = [&](Yemu::Machine &mm) {
			hist[mm.cpu.r[Yemu::AX] >> 8]++;
			return false;
		};
	}

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
			else if (cmd == "moveto") { int x, y; ls >> x >> y; st.kind = Yemu::ScriptStep::MouseMove; st.a = x - mx; st.b = y - my; mx = x; my = y; m.schedule(st); }
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
	if (trace62) {
		printf("INT 62h histogram:\n");
		for (std::map<int, uint64_t>::const_iterator it = hist.begin(); it != hist.end(); ++it)
			printf("  AH=%02X %llu\n", it->first, (unsigned long long)it->second);
	}
	return 0;
}
