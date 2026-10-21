// wlrun <gamedir> <command> ...: inspection tool for the Wasteland 1 data files.
#include "wl/gamefile.h"
#include "wl/graphics.h"
#include "fod/gfx.h"
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
		} else if (cmd == "tiles" && argc >= 5) {   // tiles <ALLHTDS1> <set> <out.png>
			const auto sets = loadTilesets(fod::readFile(dir + "/" + argv[3]));
			std::printf("%zu tilesets\n", sets.size());
			const auto &set = sets.at(size_t(std::atoi(argv[4])));
			const int cols = 16, rows = int((set.size() + 15) / 16);
			fod::Image img(cols * 16, rows * 16);
			for (size_t i = 0; i < set.size(); ++i)
				for (int y = 0; y < 16; ++y)
					for (int x = 0; x < 16; ++x) {
						const uint32_t c = egaPalette()[set[i].at(x, y)];
						img.set(int(i % 16) * 16 + x, int(i / 16) * 16 + y, {uint8_t(c >> 16), uint8_t(c >> 8), uint8_t(c)});
					}
			img.writePng(argv[5]);
			std::printf("%zu tiles\n", set.size());
		} else if (cmd == "pics" && argc >= 5) {   // pics <ALLPICS1> <n> <out.png>
			const auto pics = loadPics(fod::readFile(dir + "/" + argv[3]));
			std::printf("%zu pics\n", pics.size());
			const Pic &p = pics.at(size_t(std::atoi(argv[4]))).base;
			fod::Image img(p.width, p.height);
			for (int y = 0; y < p.height; ++y)
				for (int x = 0; x < p.width; ++x) {
					const uint32_t c = egaPalette()[p.at(x, y)];
					img.set(x, y, {uint8_t(c >> 16), uint8_t(c >> 8), uint8_t(c)});
				}
			img.writePng(argv[5]);
		} else if (cmd == "rendermap" && argc >= 7) {   // rendermap <GAMEn> <map> <ALLHTDSn> <firstTileset> <out.png>
			const GameFile g = loadGameFile(dir + "/" + argv[3]);
			const GameMap &m = g.maps.at(size_t(std::atoi(argv[4])));
			const auto sets = loadTilesets(fod::readFile(dir + "/" + argv[5]));
			const size_t ts = size_t(m.info.tileset) - size_t(std::atoi(argv[6]));
			const auto &set = sets.at(ts);
			fod::Image img(m.size * 16, m.size * 16);
			for (int ty = 0; ty < m.size; ++ty)
				for (int tx = 0; tx < m.size; ++tx) {
					const int id = m.tiles[size_t(ty) * size_t(m.size) + size_t(tx)];
					const Pic *tile = nullptr;
					if (id >= 10 && size_t(id - 10) < set.size()) tile = &set[size_t(id - 10)];
					for (int y = 0; y < 16; ++y)
						for (int x = 0; x < 16; ++x) {
							const uint32_t c = tile ? egaPalette()[tile->at(x, y)] : 0xff000000;
							img.set(tx * 16 + x, ty * 16 + y, {uint8_t(c >> 16), uint8_t(c >> 8), uint8_t(c)});
						}
				}
			img.writePng(argv[7]);
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
