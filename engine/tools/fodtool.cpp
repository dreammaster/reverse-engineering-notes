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
#include "fod/vm.h"
#include "fod/world.h"
#include "fod/screen.h"
#include "fod/game.h"
#include "fod/combat.h"
#include "fod/progress.h"
#include "fod/creator.h"
#include "fod/lzss.h"
#include "fod/savegame.h"
#include "texthost.h"
#include <cstdio>
#include <set>
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
		} else if (cmd == "run" && argc >= 6) {   // run <mapNo> <x> <y> [event]: run the script of a cell
			GameData gd(dir);
			Disk1 d = Disk1::newGame();
			d.partyCount = 1;
			d.partyIds[0] = 0;
			d.roster[0].setHp(20); d.roster[0].setHpMax(20);
			World w(gd, d);
			w.loadMap(std::atoi(argv[3]));
			const int x = std::atoi(argv[4]), y = std::atoi(argv[5]);
			const int ev = argc >= 7 ? std::atoi(argv[6]) : 0;
			TextHost host;
			ScriptVM vm(w, host);
			ScriptCall c;
			c.cellScript = w.map->cell(x, y).script;
			c.event = ev;
			std::printf("cell (%d,%d) tile=%d script=%d\n", x, y, w.map->cell(x, y).tileIndex(), c.cellScript);
			const ScriptResult r = vm.run(c);
			std::printf("-> ran=%d changedMap=%d request=%d\n", r.ran, r.changedMap, r.request);
		} else if (cmd == "screen" && argc >= 7) {   // screen <mapNo> <x> <y> <out.png>
			GameData gd(dir);
			Disk1 d = Disk1::newGame();
			d.partyCount = 2;
			d.partyIds[0] = 0; d.partyIds[1] = 1;
			const char *names[2] = {"Lena", "Ross"};
			for (int i = 0; i < 2; ++i) { std::memcpy(d.roster[i].b.data(), names[i], std::strlen(names[i])); d.roster[i].setHp(20); d.roster[i].setHpMax(20); }
			World w(gd, d);
			w.loadMap(std::atoi(argv[3]));
			w.setPartyPosition(std::atoi(argv[4]), std::atoi(argv[5]));
			Assets assets(dir);
			Screen scr;
			renderMainScreen(scr, assets, w, {"Welcome to the beautiful", "island of Florida!"});
			scr.toImage().writePng(argv[6]);
			std::printf("wrote %s\n", argv[6]);
		} else if (cmd == "fontsheet" && argc >= 4) {
			Assets assets(dir);
			Screen scr;
			for (int i = 0; i < 209; ++i) drawGlyph(scr, assets, (i % 26) * 12, (i / 26) * 12, i);
			scr.toImage().writePng(argv[3]);
		} else if (cmd == "sim" && argc >= 5) {   // sim <out.png> <keys>: U D L R = arrows, other chars = key
			GameData gd(dir);
			Assets assets(dir);
			struct SimPlatform : Platform {
				std::string keys; size_t pos = 0; Screen last, snap; bool snapped = false;
				int waitKey() override {
					if (pos >= keys.size()) { if (!snapped) { snap = last; snapped = true; } return -1; }
					const char c = keys[pos++];
					switch (c) { case 'U': return 0xFD; case 'D': return 0xFC; case 'L': return 0xFB; case 'R': return 0xFA; case '_': return 0xFE; case '!': return 0xFF; case 'a': case 'b': case 'c': case 'd': case 'e': return 0x3B + (c - 'a'); default: return c; }
				}
				void present(const Screen &s) override { last = s; }
			} plat;
			plat.keys = argv[4];
			Disk1 d = Game::defaultParty(gd);
			Game game(gd, assets, plat, d);
			game.run();
			(plat.snapped ? plat.snap : plat.last).toImage().writePng(argv[3]);
			std::printf("pos=(%d,%d) map=%d wrote %s\n", game.world().disk.mapX, game.world().disk.mapY, game.world().mapNo, argv[3]);
		} else if (cmd == "duel" && argc >= 5) {   // duel <mapNo> <groupIdx> [rounds]: party member 0 attacks a group slot
			GameData gd(dir);
			World w(gd, Game::defaultParty(gd));
			w.loadMap(std::atoi(argv[3]));
			TextHost host;
			w.party(0).passiveSkill(4) = 30;
			Combat cb(w, host);
			Combatant c;
			c.type = 0; c.index = 0; c.tType = 2; c.tIndex = uint8_t(std::atoi(argv[4])); c.tSub = 0;
			const int rounds = argc >= 6 ? std::atoi(argv[5]) : 5;
			{
				cb.calcOnly = true;
				int d = 0, wd = 0;
				Combatant e = c;
				e.type = 2; e.index = c.tIndex; e.sub = 0; e.tType = 0; e.tIndex = 0;
				std::printf("hit%% party->group %d, group->party %d\n", cb.rollAttack(c, d, wd), cb.rollAttack(e, d, wd));
				cb.calcOnly = false;
			}
			for (int i = 0; i < rounds; ++i) {
				cb.selectTarget(c);
				cb.resolveAttack(c);
				Combatant e = c;   // enemy strikes back
				e.type = 2; e.index = c.tIndex; e.sub = 0; e.tType = 0; e.tIndex = 0;
				cb.selectTarget(e);
				cb.resolveAttack(e);
			}
		} else if (cmd == "fight" && argc >= 4) {   // fight <mapNo> [seed]: force a random encounter, auto-fight it
			GameData gd(dir);
			World w(gd, Game::defaultParty(gd), argc >= 5 ? uint32_t(std::atoi(argv[4])) : 1u);
			w.loadMap(std::atoi(argv[3]));
			for (int i = 0; i < w.partyCount(); ++i) { w.party(i).setHp(300); w.party(i).setHpMax(300); w.party(i).passiveSkill(4) = 30; }
			int tries = 0;
			while (!w.tryRandomEncounter() && ++tries < 1000) {}
			std::printf("encounter after %d tries\n", tries);
			TextHost host;
			Combat cb(w, host);
			const int n = cb.buildCombatants(true);
			std::printf("%d enemy combatants\n", n);
			const Combat::Outcome o = cb.run(true);
			std::printf("fled=%d wiped=%d groundItems=%zu\n", o.fled, o.wiped, w.map->groundItems().size());
			for (int i = 0; i < w.partyCount(); ++i) std::printf("%s hp=%d exp=%u\n", w.party(i).name().c_str(), w.party(i).hp(), w.party(i).exp());
		} else if (cmd == "shop" && argc >= 6) {   // shop <map> <shopIdx> <keys>: browse a shop with scripted key presses
			GameData gd(dir);
			World w(gd, Game::defaultParty(gd));
			w.loadMap(std::atoi(argv[3]));
			w.disk.cash = 500;
			TextHost host;
			for (const char *k = argv[5]; *k; ++k) host.keys.push_back(*k);
			runShop(w, host, std::atoi(argv[4]));
			std::printf("cash=%u\n", w.disk.cash);
		} else if (cmd == "chunk" && argc >= 6) {   // chunk <file> <entries> <idx> <out.bin>: decompressed archive chunk
			Archive ar(path(dir, argv[3]), size_t(std::atoi(argv[4])));
			const Bytes b = ar.chunkLzss(size_t(std::atoi(argv[5])));
			if (argc >= 7) { FILE *f = std::fopen(argv[6], "wb"); std::fwrite(b.data(), 1, b.size(), f); std::fclose(f); }
			std::printf("%zu bytes\n", b.size());
		} else if (cmd == "pset" && argc >= 7) {   // pset <file> <entries> <idx> <out.png>: draw all frames of a picture set
			Archive ar(path(dir, argv[3]), size_t(std::atoi(argv[4])));
			PictureSet ps(ar.chunkLzss(size_t(std::atoi(argv[5]))));
			Screen scr;
			std::printf("%zu frames\n", ps.count());
			for (size_t i = 0; i < ps.count(); ++i) {
				std::printf("  %zu: %dx%d\n", i, ps.frameWidth(int(i)), ps.frameHeight(int(i)));
				ps.blit(scr, int(i), 0, 0);
			}
			scr.toImage().writePng(argv[6]);
		} else if (cmd == "portrait" && argc >= 7) {   // portrait <out.png> <mapNo> <pictureId> <ticks>: a KANI picture beside a dialog
			GameData gd(dir);
			Assets assets(dir);
			struct TickPlatform : Platform {
				int ticks = 0; Screen last, snap;
				int waitKey() override { return -1; }
				int waitKeyTimeout(int) override { if (ticks-- > 0) return -2; snap = last; return 0xFE; }
				void present(const Screen &s) override { last = s; }
			} plat;
			plat.ticks = std::atoi(argv[6]);
			Game game(gd, assets, plat, Game::defaultParty(gd));
			game.world().loadMap(std::atoi(argv[4]));
			game.showPortrait(std::atoi(argv[5]));
			game.message("Hello, stranger. Care to look at my wares?");
			plat.snap.toImage().writePng(argv[3]);
		} else if (cmd == "gamefight" && argc >= 6) {   // gamefight <out.png> <mapNo> <nth key>: screenshot of an encounter on the game screen
			GameData gd(dir);
			Assets assets(dir);
			struct FightPlatform : Platform {
				int n = 0, target = 0; Screen last, snap;
				int waitKey() override { if (++n >= target) { snap = last; return -1; } return 0xFE; }
				void present(const Screen &s) override { last = s; }
			} plat;
			plat.target = std::atoi(argv[5]);
			Game game(gd, assets, plat, Game::defaultParty(gd));
			game.world().loadMap(std::atoi(argv[4]));
			game.world().setPartyPosition(game.world().disk.mapX, game.world().disk.mapY);
			Combat cb(game.world(), game);
			for (int i = 0; i < 200 && cb.buildCombatants(false) <= 0; ++i) game.world().tryRandomEncounter();
			std::printf("combatants=%zu\n", cb.list.size());
			game.runFight(cb, false);
			plat.snap.toImage().writePng(argv[3]);
		} else if (cmd == "borders" && argc >= 5) {   // borders <variant> <out.png>
			Assets assets(dir);
			Screen scr;
			drawBorders(scr, assets, std::atoi(argv[3]));
			scr.toImage().writePng(argv[4]);
		} else if (cmd == "savetest" && argc >= 5) {   // savetest <map> <dir>: serialise/parse round trip of a map and a save
			GameData gd(dir);
			World w(gd, Game::defaultParty(gd));
			w.loadMap(std::atoi(argv[3]));
			const Bytes once = w.map->serialize();
			Map again(once);
			std::printf("map %d: chunk %zu bytes, round trip %s\n", w.mapNo, once.size(), again.serialize() == once ? "identical" : "DIFFERENT");
			std::printf("save %s\n", saveGame(w, argv[4], 1) ? "ok" : "failed");
			Disk1 d;
			std::vector<std::pair<int, Bytes>> maps;
			std::printf("load %s, %zu maps, newest slot %d\n", loadGame(argv[4], 1, d, maps) ? "ok" : "failed", maps.size(), newestSave(argv[4]));
		} else if (cmd == "pict" && argc >= 5) {   // pict <file> <out.png>: a full-screen 4bpp picture (LZSS, 320x200)
			const Bytes px = lzssDecode(readFile(path(dir, argv[3])));
			Screen scr;
			for (int y = 0; y < 200; ++y)
				for (int x = 0; x < 160 && size_t(y) * 160 + size_t(x) < px.size(); ++x) {
					scr.put(x * 2, y, px[size_t(y) * 160 + size_t(x)] >> 4);
					scr.put(x * 2 + 1, y, px[size_t(y) * 160 + size_t(x)] & 15);
				}
			scr.toImage().writePng(argv[4]);
			std::printf("%zu bytes\n", px.size());
		} else if (cmd == "creator" && argc >= 5) {   // creator <out.png> <keys>: drive the FOD party creation screens
			GameData gd(dir);
			Assets assets(dir);
			struct SimPlatform : Platform {
				std::string keys; size_t pos = 0; Screen last, snap; bool snapped = false;
				int waitKey() override {
					if (pos >= keys.size()) { if (!snapped) { snap = last; snapped = true; } return -1; }
					const char c = keys[pos++];
					switch (c) { case 'U': return 0xFD; case 'D': return 0xFC; case 'L': return 0xFB; case 'R': return 0xFA; case '_': return 0xFE; case '!': return 0xFF; case '<': return 8;
					             case 'a': case 'b': case 'c': return 0x3B + (c - 'a'); default: return c; }
				}
				void present(const Screen &s) override { last = s; }
			} plat;
			plat.keys = argv[4];
			Disk1 d = emptyRoster();
			const bool played = runPartyCreator(gd, assets, plat, d);
			(plat.snapped ? plat.snap : plat.last).toImage().writePng(argv[3]);
			std::printf("played=%d party=%d\n", played, d.partyCount);
			for (int i = 0; i < d.partyCount; ++i) std::printf("  %s prof=%d hp=%d ST=%d\n", d.roster[size_t(i)].name().c_str(), d.roster[size_t(i)].profession(), d.roster[size_t(i)].hp(), d.roster[size_t(i)].attr(0));
		} else if (cmd == "scriptall") {   // run every cell/monster script of every map once, looking for crashes
			GameData gd(dir);
			int runs = 0, failures = 0;
			for (int n = 1; n <= 28; ++n) {
				for (int ev = 0; ev < 8; ++ev) {
					World w(gd, Game::defaultParty(gd), unsigned(n * 31 + ev));
					for (int i = 0; i < 4; ++i) { w.party(i).setHp(100); w.party(i).setHpMax(100); }
					try {
						w.loadMap(n);
						struct QuietHost : TextHost {
							void print(const std::string &) override {}
							void message(const std::string &) override {}
							void showMenu(const std::string &, bool) override {}
							void log(const std::string &) override {}
							int readKey() override { return 0xFF; }
						} host;
						ScriptVM vm(w, host);
						std::set<int> seen;
						for (int y = 0; y < w.map->height(); ++y)
							for (int x = 0; x < w.map->width(); ++x) {
								const int sc = w.map->cell(x, y).script;
								if (sc == 0xFFFF || !seen.insert(sc).second) continue;
								ScriptCall c; c.cellScript = sc; c.event = ev; c.member = 0; c.arg8 = 0;
								vm.run(c);
								++runs;
							}
					} catch (const std::exception &e) {
						std::printf("map %d event %d: %s\n", n, ev, e.what());
						++failures;
					}
				}
			}
			std::printf("%d script runs, %d failures\n", runs, failures);
		} else if (cmd == "items") {
			Globals g(readFile(path(dir, "GLOBALS")));
			std::printf("range bands %d, unit '%s'\n", g.rangeBands(), g.unit().c_str());
			for (size_t i = 0; i < g.items().size(); ++i)
				std::printf("%3zu %-24s flags=%02X weaponDef=%u uniq=%u\n", i, g.items()[i].name.c_str(), g.items()[i].flags,
				            g.items()[i].weaponDef, g.items()[i].uniqueBit);
		} else if (cmd == "weapons") {
			Weapons w(readFile(path(dir, "WEAPONS")));
			for (size_t i = 0; i < w.weapons().size(); ++i) {
				std::printf("weapon %zu flags=%02X jam=%u modes=%zu\n", i, w.weapons()[i].flags(), w.weapons()[i].reliability(),
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
					std::printf("%-30s dmg %u-%u armor=%u hit=%u xp=%u\n", e.name.c_str(), e.dmgMin, e.dmgMax, e.armor, e.hitPct, e.xpPerLevel);
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
