// fodtool: inspect Fountain of Dreams data files with the fod library.
//   fodtool <gamedir> maps                      list maps with sizes
//   fodtool <gamedir> render <mapNo> <out.png>  render a map (1-based map number)
//   fodtool <gamedir> script <mapNo> <index>    disassemble one script of a map's bank
//   fodtool <gamedir> items | weapons | professions | enemies
#include "fod/archive.h"
#include "fod/data.h"
#include "fod/gfx.h"
#include "fod/map.h"
#include "fod/script.h"
#include <cstdio>
#include <iostream>

using namespace fod;

static std::string path(const std::string &dir, const std::string &name) { return dir + "/" + name; }

static Archive openSet(const std::string &dir, int mapNo, const char *kind) {
	const std::string name = std::string(mapFilePrefix(mapNo)) + kind;
	return Archive(path(dir, name), std::string(kind) == "ani" ? 53 : 28);
}

int main(int argc, char **argv) {
	if (argc < 3) {
		std::fprintf(stderr, "usage: fodtool <gamedir> <maps|render|script|items|weapons|professions|enemies> ...\n");
		return 2;
	}
	const std::string dir = argv[1], cmd = argv[2];
	try {
		if (cmd == "maps") {
			for (int n = 1; n <= 28; ++n) {
				Archive a = openSet(dir, n, "map");
				if (!a.has(n - 1)) continue;
				Map m(a.chunk(n - 1));
				std::printf("map %2d (%s) %3dx%-3d monsters=%zu groups=%zu items=%zu encounter=%d%% step=%dmin\n", n,
				            mapFilePrefix(n), m.width(), m.height(), m.monsters().size(), m.groups().size(),
				            m.groundItems().size(), m.encounterPct(), m.minutesPerStep());
			}
		} else if (cmd == "render" && argc >= 5) {
			const int n = std::atoi(argv[3]);
			Archive a = openSet(dir, n, "map");
			Map m(a.chunk(n - 1));
			Tiles tiles(readFile(path(dir, "TILES")));
			renderMap(m, tiles).writePng(argv[4]);
			std::printf("wrote %s\n", argv[4]);
		} else if (cmd == "script" && argc >= 5) {
			const int n = std::atoi(argv[3]);
			Archive a = openSet(dir, n, "scr");
			ScriptBank bank(a.chunkLzss(n - 1));
			std::cout << bank.disassemble(unsigned(std::atoi(argv[4])));
		} else if (cmd == "items") {
			Globals g(readFile(path(dir, "GLOBALS")));
			std::printf("range bands %d, unit '%s'\n", g.rangeBands(), g.unit().c_str());
			for (size_t i = 0; i < g.items().size(); ++i)
				std::printf("%3zu %-24s flags=%02X weaponDef=%u uniq=%u\n", i, g.items()[i].name.c_str(), g.items()[i].flags,
				            g.items()[i].weaponDef, g.items()[i].uniqueBit);
		} else if (cmd == "weapons") {
			Weapons w(readFile(path(dir, "WEAPONS")));
			for (size_t i = 0; i < w.weapons().size(); ++i) {
				std::printf("weapon %zu flags=%02X jam=%u modes=%zu\n", i, w.weapons()[i].flags(), w.weapons()[i].jamPct(),
				            w.weapons()[i].modes.size());
				for (const auto &m : w.weapons()[i].modes)
					std::printf("   dmg %u-%u ammoType=%u clip=%u burst=%u hit%%=%u..%u\n", m.dmgMin, m.dmgMax, m.ammoType,
					            m.clip, m.burst, m.hitPct[0], m.hitPct[9]);
			}
		} else if (cmd == "professions" || cmd == "enemies") {
			Archetypes a(readFile(path(dir, "ARCHTYPE")));
			if (cmd == "professions")
				for (const auto &p : a.professions())
					std::printf("%-16s attrPts=%u hp %u-%u cap %u AC=%u\n", p.name.c_str(), p.attrPoints, p.hpMin, p.hpMax, p.hpCap,
					            p.baseAC);
			else
				for (const auto &e : a.enemies())
					std::printf("%-30s dmg %u-%u armor=%u hit=%u xp=%u\n", e.name.c_str(), e.dmgMin, e.dmgMax, e.armor, e.hitPct, e.xp);
		} else {
			std::fprintf(stderr, "bad command\n");
			return 2;
		}
	} catch (const std::exception &e) {
		std::fprintf(stderr, "error: %s\n", e.what());
		return 1;
	}
	return 0;
}
