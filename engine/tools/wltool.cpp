// wlrun <gamedir> <command> ...: inspection tool for the Wasteland 1 data files.
#include "wl/gamefile.h"
#include <cstdio>
#include <cstdlib>

using namespace wl;

int main(int argc, char **argv) {
	if (argc < 3) { std::fprintf(stderr, "usage: wlrun <gamedir> maps|map <file> [n]\n"); return 1; }
	const std::string dir = argv[1], cmd = argv[2];
	try {
		if (cmd == "maps") {
			for (const char *name : {"GAME1", "GAME2"}) {
				const GameFile g = loadGameFile(dir + "/" + name);
				std::printf("%s: %zu blocks, %zu maps, %zu save, %zu shop lists\n", name, g.blocks.size(), g.maps.size(), g.saveBlocks.size(), g.shopBlocks.size());
				for (size_t i = 0; i < g.maps.size(); ++i) {
					const GameMap &m = g.maps[i];
					std::printf("  map %2zu: %dx%d tileset=%d strings=%zu npcs=%zu monsters=%zu tilemap@%zu size=%zu\n", i, m.size, m.size, m.info.tileset, m.strings.size(), m.npcs.size(), m.monsters.size(), m.tilemapOffset, m.msqSize);
				}
			}
		} else if (cmd == "map" && argc >= 5) {
			const GameFile g = loadGameFile(dir + "/" + argv[3]);
			const GameMap &m = g.maps.at(size_t(std::atoi(argv[4])));
			std::printf("size %d tileset %d encFreq %d maxEnc %d bg %d time %d\n", m.size, m.info.tileset, m.info.encounterFrequency, m.info.maxEncounters, m.info.backgroundTile, m.info.timeFactor);
			for (size_t i = 0; i < m.strings.size(); ++i) std::printf("%3zu: %s\n", i, m.strings[i].c_str());
			for (auto &n : m.npcs) std::printf("npc %s\n", n.name().c_str());
			for (auto &mm : m.monsters) std::printf("monster %s xp=%d skill=%d\n", mm.name.c_str(), mm.experience, mm.skill);
		}
	} catch (const std::exception &e) { std::fprintf(stderr, "error: %s\n", e.what()); return 1; }
	return 0;
}
