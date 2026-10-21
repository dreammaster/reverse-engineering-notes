// wlrun <gamedir> <command> ...: inspection tool for the Wasteland 1 data files.
#include "wl/gamefile.h"
#include "wl/graphics.h"
#include "wl/exe.h"
#include "wl/assets.h"
#include "wl/mapview.h"
#include "wl/ui.h"
#include "wl/video.h"
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
		} else if (cmd == "save" && argc >= 4) {   // save <GAMEn>: decrypted savegame block as hex
			const GameFile g = loadGameFile(dir + "/" + argv[3]);
			const MsqBlock &b = g.blocks.at(g.saveBlocks.at(0));
			const Bytes d = xorDecrypt(g.data.data() + b.offset + 4, b.size - 4, b.size - 6);
			for (size_t i = 0; i < d.size(); i += 16) {
				std::printf("%04zx:", i);
				for (size_t j = 0; j < 16 && i + j < d.size(); ++j) std::printf(" %02x", d[i + j]);
				std::printf("\n");
			}
		} else if (cmd == "viewcmp" && argc >= 10) {   // viewcmp <GAMEn> <map> <ALLHTDSn> <firstTileset> <screen.raw> <vx> <vy>
			const GameFile g = loadGameFile(dir + "/" + argv[3]);
			const GameMap &m = g.maps.at(size_t(std::atoi(argv[4])));
			const auto sets = loadTilesets(fod::readFile(dir + "/" + argv[5]));
			const auto &set = sets.at(size_t(m.info.tileset) - size_t(std::atoi(argv[6])));
			const fod::Bytes raw = fod::readFile(argv[7]);
			const auto sprites = loadSprites(fod::readFile(dir + "/IC0_9.WLF"));
			const auto masks = loadMasks(fod::readFile(dir + "/MASKS.WLF"));
			const int vx0 = std::atoi(argv[8]), vy0 = std::atoi(argv[9]);
			// The map is drawn at tile origin (0,0), 19x9 tiles; the frame hides everything outside x 8..295, y 9..136.
			auto score = [&](int vx, int vy, bool print) {
				int total = 0;
				for (int ty = 0; ty < 9; ++ty) {
					for (int tx = 0; tx < 19; ++tx) {
						const int mx = ((vx + tx) % m.size + m.size) % m.size, my = ((vy + ty) % m.size + m.size) % m.size;
						const int id = (vx + tx < 0 || vx + tx >= m.size || vy + ty < 0 || vy + ty >= m.size) ? m.info.backgroundTile : m.tiles[size_t(my) * size_t(m.size) + size_t(mx)];
						const int idx = id - 10;
						int same = 0, seen = 0;
						for (int y = 0; y < 16; ++y)
							for (int x = 0; x < 16; ++x) {
								const int px = tx * 16 + x, py = ty * 16 + y;
								if (px < 8 || px >= 296 || py < 9 || py >= 137) continue;
								++seen;
								int pix = -1;
								if (id < 10) {
									const int bgIdx = m.info.backgroundTile - 10;
									const int bg = (bgIdx >= 0 && size_t(bgIdx) < set.size()) ? set[size_t(bgIdx)].at(x, y) : 0;
									pix = (masks[size_t(id)][size_t(y) * 16 + size_t(x)] ? bg : 0) | sprites[size_t(id)].at(x, y);
								} else if (size_t(idx) < set.size()) pix = set[size_t(idx)].at(x, y);
								if (pix == raw[size_t(py) * 320 + size_t(px)]) ++same;
							}
						total += same;
						if (print) std::printf("%3d%%", seen ? same * 100 / seen : 0);
					}
					if (print) std::printf("\n");
				}
				return total;
			};
			int best = -1, bx = 0, by = 0;
			for (int dy = -m.size; dy < m.size; ++dy)
				for (int dx = -m.size; dx < m.size; ++dx) {
					const int s = score(vx0 + dx, vy0 + dy, false);
					if (s > best) { best = s; bx = vx0 + dx; by = vy0 + dy; }
				}
			std::printf("best view origin (%d,%d) score %d\n", bx, by, best);
			score(bx, by, true);
		} else if (cmd == "spritecmp" && argc >= 10) {   // spritecmp <GAMEn> <map> <ALLHTDSn> <firstTileset> <screen.raw> <mx> <my> <screenTileX> <screenTileY>
			const GameFile g = loadGameFile(dir + "/" + argv[3]);
			const GameMap &m = g.maps.at(size_t(std::atoi(argv[4])));
			const auto sets = loadTilesets(fod::readFile(dir + "/" + argv[5]));
			const auto &set = sets.at(size_t(m.info.tileset) - size_t(std::atoi(argv[6])));
			const fod::Bytes raw = fod::readFile(argv[7]);
			const auto sprites = loadSprites(fod::readFile(dir + "/IC0_9.WLF"));
			const auto masks = loadMasks(fod::readFile(dir + "/MASKS.WLF"));
			const int mx = std::atoi(argv[8]), my = std::atoi(argv[9]), sx = std::atoi(argv[10]), sy = std::atoi(argv[11]);
			const int id = m.tiles[size_t(my) * size_t(m.size) + size_t(mx)];
			std::printf("cell (%d,%d) tile id %d action class %d action %d\n", mx, my, id, m.actionClass[size_t(my) * size_t(m.size) + size_t(mx)], m.action[size_t(my) * size_t(m.size) + size_t(mx)]);
			for (int y = 0; y < 16; ++y) {
				for (int x = 0; x < 16; ++x) std::printf("%x", raw[size_t(sy * 16 + y) * 320 + size_t(sx * 16 + x)]);
				std::printf("  ");
				for (int x = 0; x < 16; ++x) std::printf("%x", id >= 10 ? set[size_t(id - 10)].at(x, y) : 0);
				std::printf("\n");
			}
			for (size_t s = 0; s < sprites.size(); ++s) {
				int same = 0;
				for (int y = 0; y < 16; ++y)
					for (int x = 0; x < 16; ++x) {
						const int bg = id >= 10 ? set[size_t(id - 10)].at(x, y) : 0;
						const int pix = (masks[s][size_t(y) * 16 + size_t(x)] ? bg : 0) | sprites[s].at(x, y);
						if (pix == raw[size_t(sy * 16 + y) * 320 + size_t(sx * 16 + x)]) ++same;
					}
				std::printf("sprite %zu: %d/256\n", s, same);
			}
		} else if (cmd == "fontsheet" && argc >= 4) {   // fontsheet <out.png>
			const auto font = loadFont(fod::readFile(dir + "/COLORF.FNT"));
			const int Z = 3;
			fod::Image img(16 * 10 * Z, ((int(font.size()) + 15) / 16) * 10 * Z);
			for (size_t i = 0; i < font.size(); ++i)
				for (int y = 0; y < 8; ++y)
					for (int x = 0; x < 8; ++x) {
						const uint32_t c = egaPalette()[font[i].at(x, y)];
						for (int zy = 0; zy < Z; ++zy) for (int zx = 0; zx < Z; ++zx) img.set((int(i % 16) * 10 + x + 1) * Z + zx, (int(i / 16) * 10 + y + 1) * Z + zy, {uint8_t(c >> 16), uint8_t(c >> 8), uint8_t(c)});
					}
			img.writePng(argv[3]);
			std::printf("%zu glyphs\n", font.size());
		} else if (cmd == "exeverify" && argc >= 5) {   // exeverify <WL.EXE> <image.bin>: unpack and compare with the IDB image
			const ExeImage e = loadExePack(fod::readFile(argv[3]), 0x1000);
			const fod::Bytes ref = fod::readFile(argv[4]);
			size_t diff = 0, first = 0, lastBlock = size_t(-1);
			for (size_t i = 0; i < ref.size() && i < e.image.size(); ++i)
				if (ref[i] != e.image[i]) { if (!diff) first = i; ++diff; if (i / 4096 != lastBlock) { std::printf("  diffs start in block %zx (offset %zx)\n", i / 4096, i); lastBlock = i / 4096; } }
			std::printf("unpacked %zu bytes, reference %zu, %zu differing bytes (first at %zx), entry %04x:%04x\n", e.image.size(), ref.size(), diff, first, e.entryCs, e.entryIp);
		} else if (cmd == "screen" && argc >= 5) {   // screen <title|map> <out.raw> [WL.EXE]: native composition for pixel comparison with wlemu dumps
			const char *exe = argc > 5 ? argv[5] : "D:/GOG Games/Wasteland/rom/WL.EXE";
			const Assets assets(dir, exe);
			Video v;
			Ui ui(assets, v);
			const std::string which = argv[3];
			if (which == "map") {
				const GameMap &map = assets.game[0].maps.at(0);
				drawMapView(v, assets, map, 46, 58, true);
				ui.cx = 0; ui.cy = 0;
				ui.mainFrame();
				ui.titleLetters();
				ui.clockAndGauge(0, 12, 0xFF, false, true);
				ui.buttonsRow("USE ENC ORDER DISBAND VIEW SAVE RADIO", 0);
			} else if (which == "title") {
				ui.mainFrame();
				ui.titleLetters();
				ui.buttonsRow("START", 0);
				ui.clockAndGauge(0, 0, 0xFF, false, false);
				for (int y = 0; y < 128; ++y)
					for (int x = 0; x < 288; ++x) v.put(8 + x, 8 + y, assets.titlePic[size_t(y) * 288 + size_t(x)]);
			}
			FILE *f = std::fopen(argv[4], "wb");
			if (f) { std::fwrite(v.pixels().data(), 1, v.pixels().size(), f); std::fclose(f); }
		} else if (cmd == "diffscreen" && argc >= 5) {   // diffscreen <a.raw> <b.raw>: differing pixels per 8x8 cell ('.' = identical, digit = 1..9, '#' = more)
			const fod::Bytes a = fod::readFile(argv[3]), b = fod::readFile(argv[4]);
			size_t total = 0;
			for (int cy = 0; cy < 25; ++cy) {
				for (int cx = 0; cx < 40; ++cx) {
					int n = 0;
					for (int y = 0; y < 8; ++y)
						for (int x = 0; x < 8; ++x) {
							const size_t i = size_t(cy * 8 + y) * 320 + size_t(cx * 8 + x);
							if (a[i] != b[i]) ++n;
						}
					total += size_t(n);
					std::putchar(n == 0 ? '.' : n < 10 ? char('0' + n) : '#');
				}
				std::putchar('\n');
			}
			std::printf("%zu differing pixels\n", total);
		} else if (cmd == "titlepic" && argc >= 4) {   // titlepic <out.png>: TITLE.PIC as 288x128, vertical XOR
			const fod::Bytes raw = fod::readFile(dir + "/TITLE.PIC");
			const int w = 288, h = 128;
			fod::Image img(w, h);
			std::vector<int> last(size_t(w / 2), 0);
			for (int y = 0; y < h; ++y)
				for (int bx = 0; bx < w / 2; ++bx) {
					int b = raw[size_t(y) * size_t(w / 2) + size_t(bx)];
					if (y > 0) b ^= last[size_t(bx)];
					last[size_t(bx)] = b;
					for (int k = 0; k < 2; ++k) {
						const uint32_t c = egaPalette()[k == 0 ? b >> 4 : b & 15];
						img.set(bx * 2 + k, y, {uint8_t(c >> 16), uint8_t(c >> 8), uint8_t(c)});
					}
				}
			img.writePng(argv[3]);
		} else if (cmd == "cellmap" && argc >= 5) {   // cellmap <screen.raw> <image.bin>: identify every 8x8 cell as a mono or colour glyph
			const fod::Bytes raw = fod::readFile(argv[3]);
			const fod::Bytes image = fod::readFile(argv[4]);
			const auto color = loadFont(fod::readFile(dir + "/COLORF.FNT"));
			const size_t monoBase = 0x2AE20 + 0xCA60 - 0x10000;
			for (int cy = 0; cy < 25; ++cy) {
				for (int cx = 0; cx < 40; ++cx) {
					std::string tok = "?";
					bool blank = true;
					for (int y = 0; y < 8 && blank; ++y)
						for (int x = 0; x < 8; ++x)
							if (raw[size_t(cy * 8 + y) * 320 + size_t(cx * 8 + x)]) { blank = false; break; }
					if (blank) tok = ".";
					else {
						for (size_t gi = 0; gi < color.size() && tok == "?"; ++gi) {
							bool same = true;
							for (int y = 0; y < 8 && same; ++y)
								for (int x = 0; x < 8; ++x)
									if (color[gi].at(x, y) != raw[size_t(cy * 8 + y) * 320 + size_t(cx * 8 + x)]) { same = false; break; }
							if (same) { char b[16]; std::snprintf(b, sizeof b, "c%02zX", gi); tok = b; }
						}
						for (int ch = 0; ch < 128 && tok == "?"; ++ch)
							for (int inv = 0; inv < 2 && tok == "?"; ++inv) {
								bool same = true;
								for (int y = 0; y < 8 && same; ++y) {
									const uint8_t bits = image[monoBase + size_t(ch) * 8 + size_t(y)];
									for (int x = 0; x < 8; ++x) {
										int on = (bits >> (7 - x)) & 1;
										if (inv) on = !on;
										if ((on ? 15 : 0) != raw[size_t(cy * 8 + y) * 320 + size_t(cx * 8 + x)]) { same = false; break; }
									}
								}
								if (same) { char b[16]; std::snprintf(b, sizeof b, "%c%02X", inv ? 'I' : 'm', ch); tok = b; }
							}
					}
					std::printf("%-4s", tok.c_str());
				}
				std::printf("\n");
			}
		} else if (cmd == "tilefind" && argc >= 7) {   // tilefind <screen.raw> <x> <y>: best matching tiles of both ALLHTDS files for the 16x16 block
			const fod::Bytes raw = fod::readFile(argv[3]);
			const int bx = std::atoi(argv[4]), by = std::atoi(argv[5]);
			for (const char *f : {"ALLHTDS1", "ALLHTDS2"}) {
				const auto sets = loadTilesets(fod::readFile(dir + "/" + f));
				for (size_t s = 0; s < sets.size(); ++s)
					for (size_t t = 0; t < sets[s].size(); ++t) {
						int same = 0;
						for (int y = 0; y < 16; ++y)
							for (int x = 0; x < 16; ++x)
								if (sets[s][t].at(x, y) == raw[size_t(by + y) * 320 + size_t(bx + x)]) ++same;
						if (same >= 150) std::printf("%s set %zu tile %zu: %d/256\n", f, s, t, same);
					}
			}
		} else if (cmd == "tilelocate" && argc >= 6) {   // tilelocate <screen.raw> <ALLHTDSn> <set>: where do tiles of a set appear on the screen
			const fod::Bytes raw = fod::readFile(argv[3]);
			const auto sets = loadTilesets(fod::readFile(dir + "/" + argv[4]));
			const auto &set = sets.at(size_t(std::atoi(argv[5])));
			for (size_t t = 0; t < set.size(); ++t) {
				int best = 0, bx = 0, by = 0;
				for (int y = 0; y + 16 <= 200; ++y)
					for (int x = 0; x + 16 <= 320; ++x) {
						int same = 0;
						for (int j = 0; j < 16 && same + (16 - j) * 16 > best; ++j)
							for (int i = 0; i < 16; ++i)
								if (set[t].at(i, j) == raw[size_t(y + j) * 320 + size_t(x + i)]) ++same;
						if (same > best) { best = same; bx = x; by = y; }
					}
				std::printf("tile %zu best %d/256 at (%d,%d)\n", t, best, bx, by);
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
