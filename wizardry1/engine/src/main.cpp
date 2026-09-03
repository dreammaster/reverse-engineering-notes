// Standalone CLI harness for the Wizardry data layer -- mirrors the Python
// tools so its output can be diffed against them.  Not the game yet.
#include "wiz/ucsd_volume.h"
#include "wiz/scenario.h"
#include "wiz/string_pool.h"
#include "wiz/rng.h"
#include "wiz/roller.h"
#include "wiz/character.h"
#include "wiz/surface.h"
#include "wiz/font.h"
#include "wiz/textscreen.h"
#include "wiz/bitmap.h"
#include "wiz/platform.h"
#include "wiz/roster.h"
#include "wiz/roller_ui.h"
#include "wiz/party.h"
#include "wiz/shop.h"
#include "wiz/town_ui.h"
#include "wiz/inn.h"
#include "wiz/temple.h"
#include "wiz/maze.h"
#include "wiz/runner.h"
#include "wiz/maze3d.h"
#include "wiz/maze_ui.h"
#include "wiz/specials.h"
#include "wiz/camp_ui.h"
#include "wiz/combat_ui.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace wiz;

static std::vector<u8> readFile(const char *path) {
    FILE *f = std::fopen(path, "rb");
    if (!f) return {};
    std::fseek(f, 0, SEEK_END);
    long n = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    std::vector<u8> v(size_t(n < 0 ? 0 : n));
    if (!v.empty() && std::fread(v.data(), 1, v.size(), f) != v.size()) v.clear();
    std::fclose(f);
    return v;
}

static int usage() {
    std::puts(
        "wiz1 <command> <args>\n"
        "  files   <WIZ1.DSK>                 list the p-System volume\n"
        "  extract <WIZ1.DSK> <NAME> <out>    write one file out\n"
        "  toc     <SCENARIO.DATA>            scenario table of contents\n"
        "  monsters <SCENARIO.DATA> <ASCII.KRN>\n"
        "  items    <SCENARIO.DATA> <ASCII.KRN>\n"
        "  exp      <SCENARIO.DATA>           xp-per-level table\n"
        "  str     <ASCII.KRN> <key>...       decode string pool keys\n"
        "  strings <ASCII.KRN>               dump every string\n"
        "  roster  <SCENARIO.DATA>            list the roster + round-trip check\n"
        "  rng     [s3hex] [n]               RANDOM sequence\n"
        "  roll    <race> <align> [s3hex]     roll a character\n"
        "  roller  <CHARSET> <SCENARIO.DATA> [TITLE] [roster.dat]\n"
        "  town    <CHARSET> <SCENARIO.DATA> [TITLE] [roster.dat] [party.dat]\n");
    return 2;
}

static int cmdFiles(const char *dsk) {
    UcsdVolume v;
    if (!v.load(dsk)) { std::fprintf(stderr, "cannot load %s\n", dsk); return 1; }
    std::printf("volume %s  (%zu blocks)\n\n", v.volumeName().c_str(), v.totalBlocks());
    for (const auto &e : v.entries())
        std::printf("  %2d  %-16s  kind=%d  blocks %u..%u  %zu bytes\n",
                    e.index, e.name.c_str(), int(e.kind), e.firstBlock,
                    e.lastBlock - 1, e.size());
    return 0;
}

static int cmdExtract(const char *dsk, const char *name, const char *out) {
    UcsdVolume v;
    if (!v.load(dsk)) return 1;
    const auto *e = v.find(name);
    if (!e) { std::fprintf(stderr, "not found: %s\n", name); return 1; }
    auto bytes = v.fileBytes(*e);
    FILE *f = std::fopen(out, "wb");
    if (!f) return 1;
    std::fwrite(bytes.data(), 1, bytes.size(), f);
    std::fclose(f);
    std::printf("wrote %s  (%zu bytes)\n", out, bytes.size());
    return 0;
}

static int cmdToc(const char *path) {
    Scenario sc;
    if (!sc.load(readFile(path))) { std::fprintf(stderr, "bad scenario\n"); return 1; }
    std::printf("game: %s\n\n", sc.gameName().c_str());
    std::printf("  %-8s %6s %8s\n", "type", "count", "recsize");
    for (int t = Scenario::Maze; t < Scenario::TypeCount; ++t)
        std::printf("  %-8s %6d %8d\n", Scenario::typeName(Scenario::Type(t)),
                    sc.count(Scenario::Type(t)), sc.recSize(Scenario::Type(t)));
    std::printf("\n  races  :");
    for (auto &s : sc.races()) std::printf(" %s", s.c_str());
    std::printf("\n  classes:");
    for (auto &s : sc.classes()) std::printf(" %s", s.c_str());
    std::printf("\n");
    return 0;
}

static int cmdMonsters(const char *scn, const char *krn, bool items) {
    Scenario sc;
    StringPool sp;
    if (!sc.load(readFile(scn)) || !sp.load(readFile(krn))) return 1;
    auto type = items ? Scenario::Object : Scenario::Monster;
    for (int i = 0; i < sc.count(type); ++i) {
        int key = items ? StringPool::objectNameKey(i) : StringPool::monsterNameKey(i);
        std::printf("  [%3d] %s\n", i, sp.get(key).c_str());
    }
    return 0;
}

static int cmdExp(const char *scn) {
    Scenario sc;
    if (!sc.load(readFile(scn))) return 1;
    Bytes b = sc.record(Scenario::Exp, 0);
    if (b.empty()) return 1;
    ExpTable xp{b};
    static const char *cn[] = {"FIG", "MAG", "PRI", "THI", "BIS", "SAM", "LOR", "NIN"};
    std::printf("        L1     L2     L3     L4     L5\n");
    for (int c = 0; c < 8; ++c) {
        std::printf("  %s", cn[c]);
        for (int l = 1; l <= 5; ++l)
            std::printf(" %6lld", (long long)xp.threshold(c, l).value());
        std::printf("\n");
    }
    return 0;
}

static int cmdStr(int argc, char **argv) {
    StringPool sp;
    if (!sp.load(readFile(argv[2]))) return 1;
    for (int i = 3; i < argc; ++i) {
        int k = std::atoi(argv[i]);
        bool ok;
        std::string s = sp.get(k, &ok);
        std::printf("[%d] %s%s\n", k, s.c_str(), ok ? "" : "  <no key>");
    }
    return 0;
}

static std::string escape(const std::string &s) {   // match tools/strpool.py
    std::string o;
    for (unsigned char c : s) {
        if (c >= 32 && c < 127) o.push_back(char(c));
        else { char b[5]; std::snprintf(b, sizeof b, "\\x%02x", c); o += b; }
    }
    return o;
}

static int cmdStrings(const char *krn) {
    StringPool sp;
    if (!sp.load(readFile(krn))) return 1;
    for (int k = sp.keyLo(); k <= sp.keyHi(); ++k) {
        bool ok;
        std::string s = sp.get(k, &ok);
        if (ok) std::printf("%5d  %s\n", k, escape(s).c_str());
    }
    return 0;
}

static int cmdRng(int argc, char **argv) {
    Rng rng(argc > 2 ? u16(std::strtoul(argv[2], nullptr, 0)) : u16(0x7351));
    int n = argc > 3 ? std::atoi(argv[3]) : 16;
    for (int i = 0; i < n; ++i) std::printf("%u\n", rng.next());
    return 0;
}

static const char *kRaces[] = {"norace", "human", "elf", "dwarf", "gnome", "hobbit"};
static const char *kAligns[] = {"unalign", "good", "neutral", "evil"};
static const char *kClasses[] = {"FIGHTER", "MAGE", "PRIEST", "THIEF",
                                 "BISHOP", "SAMURAI", "LORD", "NINJA"};

static int cmdRoll(int argc, char **argv) {
    if (argc < 4) { std::puts("roll <race> <align> [s3hex]"); return 2; }
    Race race = Race::Human;
    for (int i = 0; i < 6; ++i) if (argv[2] == std::string(kRaces[i])) race = Race(i);
    Align al = Align::Good;
    for (int i = 0; i < 4; ++i) if (argv[3] == std::string(kAligns[i])) al = Align(i);
    u16 s3 = argc > 4 ? u16(std::strtoul(argv[4], nullptr, 0)) : u16(0x7351);

    Rng rng(s3);
    Character c;
    c.name = "ROLLED";
    c.race = race;
    c.align = al;
    raceBaseAttrs(race, c.attrib);
    int base[ATTR_COUNT];
    raceBaseAttrs(race, base);
    int bonus = rollBonusPoints(rng);

    // demo point spend: round-robin over the six attributes, cap 18
    int spent = bonus;
    for (int k = 0; spent > 0; k = (k + 1) % ATTR_COUNT)
        if (c.attrib[k] < 18) { c.attrib[k]++; spent--; }

    bool elig[8];
    classEligibility(c.attrib, al, elig);
    for (int cls = 7; cls >= 0; --cls)          // prefer the fanciest class
        if (elig[cls]) { c.cls = Class(cls); break; }

    c.age = rollAge(rng);
    c.gold.v = rollGold(rng);
    c.hpMax = c.hpLeft = rollHp(c.cls, c.attrib[VIT], rng);
    startingSpells(c);

    std::printf("race %s  align %s  s3=0x%04x   bonus points %d\n",
                kRaces[int(race)], kAligns[int(al)], s3, bonus);
    std::printf("  base    STR %2d  IQ %2d  PIE %2d  VIT %2d  AGI %2d  LCK %2d\n",
                base[STR], base[IQ], base[PIETY], base[VIT], base[AGI], base[LUCK]);
    std::printf("  +demo   STR %2d  IQ %2d  PIE %2d  VIT %2d  AGI %2d  LCK %2d\n",
                c.attrib[STR], c.attrib[IQ], c.attrib[PIETY],
                c.attrib[VIT], c.attrib[AGI], c.attrib[LUCK]);
    std::printf("  eligible:");
    for (int i = 0; i < 8; ++i) if (elig[i]) std::printf(" %s", kClasses[i]);
    std::printf("\n  -> %s  age %d wk (%.1f yr)  gold %lld  HP %d\n",
                kClasses[int(c.cls)], c.age, c.age / 52.0,
                (long long)c.gold.v, c.hpMax);
    if (c.mageSpells[1])   std::printf("  knows HALITO, KATINO (2 mage casts)\n");
    if (c.priestSpells[1]) std::printf("  knows DIOS, BADIOS (2 priest casts)\n");

    // encode -> a real 208-byte roster record, and read it back
    c.name = "ROLLED";
    c.charLevel = c.maxLevelAcquired = 1;
    auto rec = c.write();
    Character rt;
    rt.read({rec.data(), rec.size()});
    std::printf("  208-byte record: %02x %02x %02x %02x ...  round-trip %s\n",
                rec[0], rec[1], rec[2], rec[3],
                (rt.name == c.name && rt.cls == c.cls && rt.attrib[VIT] == c.attrib[VIT]
                 && rt.gold.v == c.gold.v) ? "ok" : "FAIL");
    return 0;
}

static const char *kRaceShort[] = {"---", "HUM", "ELF", "DWA", "GNO", "HOB"};
static const char *kAlignShort[] = {"-", "G", "N", "E"};
static const char *kStatus[] = {"OK", "AFRAID", "ASLEEP", "PLYZE",
                                "STONED", "DEAD", "ASHES", "LOST"};

static int cmdRoster(const char *scn) {
    Scenario sc;
    if (!sc.load(readFile(scn))) return 1;
    int mism = 0;
    for (int i = 0; i < sc.count(Scenario::Char); ++i) {
        Bytes rec = sc.record(Scenario::Char, i);
        if (rec.size() < Character::kRecordBytes) continue;
        Character c;
        c.read(rec);
        auto back = c.write();                          // round-trip check
        bool same = std::memcmp(back.data(), rec.p, Character::kRecordBytes) == 0;
        if (!same) ++mism;
        if (c.name.empty() && c.status == Status::Lost) continue;   // empty slot
        std::printf("[%2d] %-16s %s %-7s %s L%-2d %-6s  "
                    "S%2d I%2d P%2d V%2d A%2d L%2d  AC%d HP %d/%d  %lldgp%s\n",
                    i, c.name.c_str(), kRaceShort[int(c.race) & 7],
                    kClasses[int(c.cls) & 7], kAlignShort[int(c.align) & 3],
                    c.charLevel, kStatus[int(c.status) & 7],
                    c.attrib[STR], c.attrib[IQ], c.attrib[PIETY],
                    c.attrib[VIT], c.attrib[AGI], c.attrib[LUCK],
                    c.armorClass, c.hpLeft, c.hpMax, (long long)c.gold.v,
                    same ? "" : "  [!round-trip]");
    }
    std::printf("round-trip: %d/%d records re-encode identically\n",
                sc.count(Scenario::Char) - mism, sc.count(Scenario::Char));
    return mism ? 1 : 0;
}

static void showSurface(const Surface &s, const char *ppm,
                        const Color *pal = kDefaultPalette, int palLen = 16) {
    if (ppm) {                                   // explicit file -> headless
        s.savePPM(ppm, pal, palLen);
        std::printf("wrote %s  (%dx%d)\n", ppm, s.width(), s.height());
        return;
    }
    auto p = makeSdlPlatform("wiz1", 2);          // else open a window
    if (!p) { std::puts("no SDL2 backend; pass an out.ppm path"); return; }
    while (p->running()) {
        p->present(s, pal, palLen);
        int k = p->waitKey();
        if (k == KEY_QUIT || k == KEY_ESC || k == KEY_RETURN) break;
    }
}

static int cmdShow(int argc, char **argv) {
    if (argc < 4) { std::puts("show <font|title|glyphs> <FILE> [out.ppm]"); return 2; }
    std::string what = argv[2];
    auto bytes = readFile(argv[3]);
    const char *ppm = argc > 4 ? argv[4] : nullptr;
    if (bytes.empty()) { std::fprintf(stderr, "cannot read %s\n", argv[3]); return 1; }

    if (what == "font" || what == "glyphs") {
        Font f;
        if (!f.load(bytes)) return 1;
        int cols = 32, rows = (f.glyphCount() + cols - 1) / cols;
        Surface s(cols * (Font::kW + 1) + 1, rows * (Font::kH + 1) + 1);
        s.fill(0);
        for (int g = 0; g < f.glyphCount(); ++g) {
            int gx = 1 + (g % cols) * (Font::kW + 1);
            int gy = 1 + (g / cols) * (Font::kH + 1);
            f.drawGlyph(s, g, gx, gy, 10 /* green */);
        }
        std::printf("%d glyphs, %dx%d cells\n", f.glyphCount(), Font::kW, Font::kH);
        showSurface(s, ppm);
        return 0;
    }
    if (what == "title") {
        Surface s = loadTitle({bytes.data(), bytes.size()});   // 320x64 2bpp CGA
        showSurface(s, ppm, kCgaPalette, 4);
        return 0;
    }
    return usage();
}

// A static mock of ROLLER's MAKEMENU screen, to exercise the text grid.
static int cmdMockup(int argc, char **argv) {
    if (argc < 3) { std::puts("mockup <CHARSET> [out.ppm]"); return 2; }
    Font f;
    if (!f.load(readFile(argv[2]))) { std::fprintf(stderr, "bad charset\n"); return 1; }

    TextScreen t;
    t.putChar(12);                                     // clear + home
    t.gotoXY(0, 0); t.writeField("NAME ", 10); t.write("GANDALF");
    t.gotoXY(0, 1); t.writeField("PASSWORD", 9);
    t.gotoXY(0, 2); t.writeField("RACE", 9); t.write(" ELF");
    t.gotoXY(0, 3); t.writeField("POINTS", 9); t.write(" 4");
    static const char *attr[] = {"STRENGTH", "I.Q.", "PIETY", "VITALITY",
                                 "AGILITY", "LUCK"};
    static const int val[] = {9, 15, 12, 8, 11, 7};
    for (int i = 0; i < 6; ++i) {
        t.gotoXY(0, 5 + i); t.writeField(attr[i], 9);
        t.gotoXY(10, 5 + i); char b[4]; std::snprintf(b, 4, "%2d", val[i]); t.write(b);
    }
    static const char *cls[] = {"A) FIGHTER", "B) MAGE", "C) PRIEST"};
    for (int i = 0; i < 3; ++i) { t.gotoXY(20, 5 + i); t.write(cls[i]); }
    t.gotoXY(13, 6); t.setAttr(ATTR_INVERSE); t.write("<--"); t.setAttr(ATTR_NORMAL);
    t.gotoXY(0, 12); t.writeField("ALIGNMENT", 9); t.write(" GOOD");
    t.gotoXY(0, 13); t.writeField("CLASS", 9);
    t.setWindow(0, 15, 40, 9);
    t.writeln("ENTER [+,-] TO ALTER A SCORE,");
    t.writeln("      [RET] TO GO TO NEXT SCORE,");
    t.writeln("      [ESC] TO GO ON WHEN POINTS USED UP");

    Surface s(TextScreen::kCols * Font::kW, TextScreen::kRows * Font::kH);
    t.render(s, f);
    showSurface(s, argc > 3 ? argv[3] : nullptr);
    return 0;
}

static int cmdRoller(int argc, char **argv) {
    if (argc < 4) {
        std::puts("roller <CHARSET> <SCENARIO.DATA> [TITLE] [roster.dat]");
        return 2;
    }
    Font font;
    if (!font.load(readFile(argv[2]))) { std::fprintf(stderr, "bad charset\n"); return 1; }
    Scenario sc;
    if (!sc.load(readFile(argv[3]))) { std::fprintf(stderr, "bad scenario\n"); return 1; }
    auto title = argc > 4 ? readFile(argv[4]) : std::vector<u8>{};
    std::string rosterPath = argc > 5 ? argv[5] : "roster.dat";

    Roster roster;
    if (!roster.load(rosterPath)) {
        roster.seedFrom(sc);
        std::printf("seeded %s from the scenario roster\n", rosterPath.c_str());
    }

    auto p = makeSdlPlatform("Wizardry - Training Grounds", 2);
    if (!p) { std::puts("roller needs the SDL2 backend"); return 1; }

    Rng rng;
    Ui ui(*p, font);
    if (!title.empty() && !showTitle(*p, font, {title.data(), title.size()})) return 0;
    runRoller(ui, roster, sc, rng, rosterPath);
    roster.save(rosterPath);
    return 0;
}

// Headless flow test: run the ROLLER with a scripted key string, print the
// resulting roster.  Escapes: \r \b \e (esc); \xHH.
static std::string unescape(const std::string &in) {
    std::string o;
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] != '\\') { o.push_back(in[i]); continue; }
        char n = ++i < in.size() ? in[i] : 0;
        if (n == 'r') o.push_back('\r');
        else if (n == 'b') o.push_back('\b');
        else if (n == 'e') o.push_back('\x1b');
        else if (n == 'x' && i + 2 < in.size()) {
            o.push_back(char(std::stoi(in.substr(i + 1, 2), nullptr, 16)));
            i += 2;
        } else o.push_back(n);
    }
    return o;
}

static int cmdRollerTest(int argc, char **argv) {
    if (argc < 5) {
        std::puts("roller-test <CHARSET> <SCENARIO.DATA> <keyscript> [dumpdir]");
        return 2;
    }
    Font font;
    Scenario sc;
    if (!font.load(readFile(argv[2])) || !sc.load(readFile(argv[3]))) return 1;
    Roster roster;
    roster.seedFrom(sc);

    auto p = makeNullPlatform(unescape(argv[4]), argc > 5 ? argv[5] : "");
    Rng rng;
    Ui ui(*p, font);
    runRoller(ui, roster, sc, rng, "");            // empty path -> no autosave

    for (int i = 0; i < roster.count(); ++i) {
        const Character &c = roster.slot(i);
        if (c.status == Status::Lost && c.name.empty()) continue;
        std::printf("[%2d] %-16s R%d C%d A%d L%d %s  S%d I%d P%d V%d A%d L%d  HP%d/%d %lldgp\n",
                    i, c.name.c_str(), int(c.race), int(c.cls), int(c.align),
                    c.charLevel, c.status == Status::Lost ? "LOST" : "OK",
                    c.attrib[0], c.attrib[1], c.attrib[2], c.attrib[3],
                    c.attrib[4], c.attrib[5], c.hpLeft, c.hpMax,
                    (long long)c.gold.v);
    }
    return 0;
}

// ---- the town ------------------------------------------------------------

// Drive the roller <-> town loop until the player leaves the game.
static void playTownLoop(Ui &ui, TownWorld &world, bool startInTown) {
    bool inTown = startInTown;
    for (;;) {
        if (!inTown) {
            runRoller(ui, world.roster, world.sc, world.rng, world.rosterPath);
            if (ui.quit()) return;
            inTown = true;
        }
        TownExit e = runTown(ui, world);
        if (e == TownExit::WindowClosed || e == TownExit::LeaveGame) return;
        if (e == TownExit::ToRoller) inTown = false;
        else if (e == TownExit::ToMaze) {
            MazeState st;                            // ENTMAZE: level 1, (0,0) N
            MazeExit me = runMaze(ui, world.party, world.sc, world.sp, world.rng, st);
            if (me == MazeExit::WindowClosed) return;
            // PartyWiped / ToTown both drop back to the town; a real XCEMETRY
            // would run the cemetery scene first.  The party's working copies
            // (HP, deaths) are written back when they leave at the Edge of Town.
        }
    }
}

static int cmdTown(int argc, char **argv) {
    if (argc < 4) {
        std::puts("town <CHARSET> <SCENARIO.DATA> [TITLE] [ASCII.KRN] "
                  "[roster.dat] [party.dat] [shop.dat]");
        return 2;
    }
    Font font;
    if (!font.load(readFile(argv[2]))) { std::fprintf(stderr, "bad charset\n"); return 1; }
    Scenario sc;
    if (!sc.load(readFile(argv[3]))) { std::fprintf(stderr, "bad scenario\n"); return 1; }
    auto title = argc > 4 ? readFile(argv[4]) : std::vector<u8>{};
    StringPool sp;
    bool haveSp = argc > 5 && sp.load(readFile(argv[5]));
    std::string rosterPath = argc > 6 ? argv[6] : "roster.dat";
    std::string partyPath  = argc > 7 ? argv[7] : "party.dat";
    std::string shopPath   = argc > 8 ? argv[8] : "shop.dat";

    Roster roster;
    if (!roster.load(rosterPath)) {
        roster.seedFrom(sc);
        std::printf("seeded %s from the scenario roster\n", rosterPath.c_str());
    }
    Party party;
    party.load(partyPath, roster);
    Shop shop;
    if (!shop.load(shopPath, sc)) shop.seedFrom(sc);

    auto p = makeSdlPlatform("Wizardry - Castle", 2);
    if (!p) { std::puts("town needs the SDL2 backend"); return 1; }

    Rng rng;
    Ui ui(*p, font);
    if (!title.empty() && !showTitle(*p, font, {title.data(), title.size()})) return 0;
    TownWorld world{party, roster, shop, sc, haveSp ? &sp : nullptr, rng,
                    rosterPath, partyPath, shopPath};
    playTownLoop(ui, world, party.count() > 0);
    roster.save(rosterPath);
    party.save(partyPath);
    shop.save(shopPath);
    return 0;
}

// Headless: seed the world, script one trip through the town, print the party.
// The optional 7th arg mangles a roster slot for testing, e.g. "wound=7:5"
// sets roster slot 7 to status 5 (DEAD), HP 0.
static int cmdTownTest(int argc, char **argv) {
    if (argc < 5) {
        std::puts("town-test <CHARSET> <SCENARIO.DATA> <keyscript> [dumpdir] [ASCII.KRN] [wound=S:ST]");
        return 2;
    }
    Font font;
    Scenario sc;
    if (!font.load(readFile(argv[2])) || !sc.load(readFile(argv[3]))) return 1;
    Roster roster;
    roster.seedFrom(sc);
    Party party;
    Shop shop;
    shop.seedFrom(sc);
    StringPool sp;
    bool haveSp = argc > 6 && sp.load(readFile(argv[6]));

    if (argc > 7 && std::strncmp(argv[7], "wound=", 6) == 0) {
        int slot = -1, st = 5;
        std::sscanf(argv[7] + 6, "%d:%d", &slot, &st);
        if (slot >= 0 && slot < roster.count()) {
            roster.slot(slot).status = Status(st);
            roster.slot(slot).hpLeft = 0;
            std::printf("wounded roster slot %d (%s) -> status %d\n",
                        slot, roster.slot(slot).name.c_str(), st);
        }
    }

    auto p = makeNullPlatform(unescape(argv[4]), argc > 5 ? argv[5] : "");
    Rng rng;
    Ui ui(*p, font);
    TownWorld world{party, roster, shop, sc, haveSp ? &sp : nullptr, rng, "", "", ""};
    TownExit e = runTown(ui, world);
    if (e == TownExit::ToMaze) {                     // continue into the maze
        MazeState mst;
        MazeExit me = runMaze(ui, party, sc, haveSp ? &sp : nullptr, rng, mst);
        static const char *mx[] = {"TO-TOWN", "PARTY-WIPED", "WINDOW-CLOSED"};
        std::printf("maze exit: %s  pos (%d,%d) %s level %d\n", mx[int(me)],
                    mst.pos.x, mst.pos.y, dirName(mst.pos.dir), mst.level);
    }

    static const char *kExit[] = {"ROLLER", "MAZE", "LEAVE", "WINDOW-CLOSED"};
    std::printf("exit: %s\n", kExit[int(e)]);
    std::printf("party (%d):\n", party.count());
    for (int i = 0; i < party.count(); ++i) {
        const Character &c = party.member(i);
        std::printf("  %d) %-16s slot=%d A%d %s HP%d/%d %lldgp poss:",
                    i + 1, c.name.c_str(), party.rosterSlot(i), int(c.align),
                    c.inMaze ? "OUT" : "in", c.hpLeft, c.hpMax, (long long)c.gold.v);
        for (int j = 0; j < c.possCount; ++j)
            std::printf(" #%d%s", c.poss[j].itemIndex, c.poss[j].identified ? "" : "?");
        std::printf("\n");
    }
    for (int i = 0; i < roster.count(); ++i) {
        const Character &r = roster.slot(i);
        if (!r.name.empty() && r.status != Status::OK && r.status != Status::Lost)
            std::printf("roster %d) %-16s status %d HP%d/%d age %dwk\n",
                        i, r.name.c_str(), int(r.status), r.hpLeft, r.hpMax, r.age);
    }
    return 0;
}

// Exercise the Adventurer's Inn rules (wiz/inn.h) deterministically:
// a wounded, XP-flush Fighter rests in the Economy room, heals + ages, then
// makes a level.
static int cmdInnTest(int argc, char **argv) {
    if (argc < 3) { std::puts("inn-test <SCENARIO.DATA>"); return 2; }
    Scenario sc;
    if (!sc.load(readFile(argv[2]))) { std::fprintf(stderr, "bad scenario\n"); return 1; }
    ExpTable exp{sc.record(Scenario::Exp, 0)};

    Character c{};
    c.name = "GROMP";
    c.cls = Class::Fighter;
    c.race = Race::Human;
    c.align = Align::Neutral;
    c.status = Status::OK;
    c.charLevel = c.maxLevelAcquired = 1;
    for (int i = 0; i < ATTR_COUNT; ++i) c.attrib[i] = 12;
    c.attrib[VIT] = 15;
    c.hpMax = 10;
    c.hpLeft = 3;
    c.age = 20 * 52;                       // 20 years
    c.gold.v = 1000;
    c.exp.v = exp.threshold(int(Class::Fighter), 1).value();   // exactly enough for L2

    std::printf("before: L%d HP %d/%d age %dwk %lldgp exp %lld  (need %lld for L2)\n",
                c.charLevel, c.hpLeft, c.hpMax, c.age, (long long)c.gold.v,
                (long long)c.exp.v,
                (long long)exp.threshold(int(Class::Fighter), 1).value());

    Rng rng;
    const RoomTier &rt = kRooms[2];        // Economy: +3 hp / 50 gp per week
    int weeks = 0;
    while (c.gold.v >= rt.goldPerWeek && c.hpLeft < c.hpMax) {
        c.hpLeft = std::min(c.hpMax, c.hpLeft + rt.hpPerWeek);
        c.gold.v -= rt.goldPerWeek;
        c.age += 1;
        ++weeks;
    }
    InnLog log;
    checkNewLevel(c, exp, rng, log);
    setSpells(c);

    std::printf("rested %d week(s)\n", weeks);
    for (const auto &m : log) std::printf("  | %s\n", m.c_str());
    std::printf("after:  L%d HP %d/%d age %dwk %lldgp  S%d I%d P%d V%d A%d L%d\n",
                c.charLevel, c.hpLeft, c.hpMax, c.age, (long long)c.gold.v,
                c.attrib[0], c.attrib[1], c.attrib[2], c.attrib[3],
                c.attrib[4], c.attrib[5]);
    return c.charLevel == 2 ? 0 : 1;
}

// Exercise the Temple of Cant rules (wiz/temple.h) deterministically: fee
// calculation + a resurrection attempt on a DEAD and an ASHES character.
static int cmdTempleTest(int argc, char **argv) {
    (void)argc; (void)argv;
    auto make = [](Status st, int vit, int lvl) {
        Character c{};
        c.name = "LAZARUS";
        c.cls = Class::Fighter;
        c.status = st;
        c.charLevel = c.maxLevelAcquired = lvl;
        for (int i = 0; i < ATTR_COUNT; ++i) c.attrib[i] = 12;
        c.attrib[VIT] = vit;
        c.hpMax = 20;
        c.hpLeft = 0;
        c.age = 15 * 52;
        return c;
    };

    struct Case { const char *tag; Status st; int vit; int lvl; };
    const Case cases[] = {
        {"paralyzed", Status::Paralyzed, 12, 3},
        {"dead-hi-vit", Status::Dead, 18, 5},
        {"ashes-lo-vit", Status::Ashes, 3, 8},
    };
    Rng rng;
    int fails = 0;
    for (const Case &k : cases) {
        Character who = make(k.st, k.vit, k.lvl);
        int64_t fee = templeFee(k.st, k.lvl);
        InnLog log;
        Status before = who.status;
        CantResult r = doCant(who, rng, log);
        std::printf("%-13s L%d fee=%lld  %d -> %d  (%s)  age +%dwk\n",
                    k.tag, k.lvl, (long long)fee, int(before), int(who.status),
                    r == CantResult::Cured ? "CURED" : "WORSENED",
                    who.age - 15 * 52);
        for (const auto &m : log) std::printf("   | %s\n", m.c_str());
        if (k.st == Status::Paralyzed && who.status != Status::OK) ++fails;
    }
    return fails ? 1 : 0;
}

// ---- the maze -----------------------------------------------------------

static char squareMark(Square s) {
    switch (s) {
        case Square::Stairs:     return 'S';
        case Square::Pit:        return 'P';
        case Square::Chute:      return 'C';
        case Square::TurnRandom: return 'R';   // spinner
        case Square::Darkness:   return 'D';
        case Square::Teleport:   return 'T';
        case Square::Damage:     return 'O';   // "ouch"
        case Square::Buttons:    return 'B';
        case Square::RockWater:  return 'K';
        case Square::Fizzle:     return 'F';
        case Square::ScnMsg:     return 'M';
        case Square::Encounter:  return 'E';
        default:                 return ' ';
    }
}

// Top-down ASCII map, north up.  Cell = "+--" wide; interior char is the
// party arrow or a special-square letter.
static void drawMazeTop(const MazeLevel &m, const MazePos &p) {
    static const char *arrow = "^>v<";
    for (int y = 19; y >= 0; --y) {
        std::string a, b;
        for (int x = 0; x < 20; ++x) {
            Wall n = m.wall(x, y, NORTH);
            a += '+';
            a += (n == Wall::Wall ? "--" : n == Wall::Open ? "  " : "··");
            Wall w = m.wall(x, y, WEST);
            b += (w == Wall::Wall ? '|' : w == Wall::Open ? ' ' : ':');
            char c = ' ';
            if (x == p.x && y == p.y) c = arrow[p.dir & 3];
            else if (Square s = m.squareAt(x, y); s != Square::Normal) c = squareMark(s);
            b += c; b += ' ';
        }
        Wall e = m.wall(19, y, EAST);
        a += '+';
        b += (e == Wall::Wall ? '|' : e == Wall::Open ? ' ' : ':');
        std::printf("%s\n%s\n", a.c_str(), b.c_str());
    }
    std::string bottom;
    for (int x = 0; x < 20; ++x) {
        Wall s = m.wall(x, 0, SOUTH);
        bottom += '+';
        bottom += (s == Wall::Wall ? "--" : s == Wall::Open ? "  " : "··");
    }
    bottom += '+';
    std::printf("%s\n", bottom.c_str());
}

// ASCII rendering of the 82x79 wireframe pic (2x2 blocks -> 41x40 chars).
static void printMaze3d(const MazeLevel &m, const MazePos &p, int level) {
    Surface pic(kPicW, kPicH);
    int light = 0;
    Rng rng;
    drawMazeView(pic, m, p, level, light, false, rng);
    for (int y = 0; y < kPicH; y += 2) {
        std::string row;
        for (int x = 0; x < kPicW; x += 2) {
            bool on = pic.get(x, y) || pic.get(x + 1, y) ||
                      pic.get(x, y + 1) || pic.get(x + 1, y + 1);
            row += on ? '#' : ' ';
        }
        std::printf("  |%s|\n", row.c_str());
    }
}

static void printCell(const MazeLevel &m, const MazePos &p) {
    std::printf("@ (%d,%d) facing %s   walls: F=%d L=%d R=%d B=%d\n",
                p.x, p.y, dirName(p.dir),
                int(m.wall(p.x, p.y, p.dir)),
                int(m.wall(p.x, p.y, (p.dir + 3) & 3)),
                int(m.wall(p.x, p.y, (p.dir + 1) & 3)),
                int(m.wall(p.x, p.y, (p.dir + 2) & 3)));
    int sx = m.squareExtra(p.x, p.y);
    Square s = m.squareType(sx);
    if (s != Square::Normal)
        std::printf("  square: %s  aux0=%d aux1=%d aux2=%d\n",
                    [](Square q) {
                        static const char *n[] = {"NORMAL","STAIRS","PIT","CHUTE","SPINNER",
                            "DARK","TELEPORT","DAMAGE","BUTTONS","ROCKWATER","FIZZLE","SCNMSG","ENCOUNTER"};
                        return n[int(q)];
                    }(s),
                    m.aux0(sx), m.aux1(sx), m.aux2(sx));
}

// wiz1 maze <SCENARIO.DATA> [level 1-10] [keyscript F/L/R/K/Q]
// wiz1 maze-scan <SCENARIO.DATA> [ASCII.KRN] -- list every special-square
// descriptor across all maze levels (SCNMSG descriptors resolve their text).
static int cmdMazeScan(int argc, char **argv) {
    if (argc < 3) { std::puts("maze-scan <SCENARIO.DATA> [ASCII.KRN]"); return 2; }
    Scenario sc;
    if (!sc.load(readFile(argv[2]))) return 1;
    StringPool sp;
    bool haveSp = argc > 3 && sp.load(readFile(argv[3]));
    for (int lv = 0; lv < sc.count(Scenario::Maze); ++lv) {
        MazeLevel m;
        if (!m.load(sc.record(Scenario::Maze, lv))) continue;
        for (int d = 0; d < 16; ++d) {
            Square ty = m.squareType(d);
            if (ty == Square::Normal) continue;
            int cells = 0, cx = -1, cy = -1;
            for (int y = 0; y < 20; ++y)
                for (int x = 0; x < 20; ++x)
                    if (m.squareExtra(x, y) == d && m.squareAt(x, y) == ty) {
                        ++cells; if (cx < 0) { cx = x; cy = y; }
                    }
            if (!cells) continue;
            std::printf("L%d d%-2d ty=%-2d aux0=%-6d aux1=%-6d aux2=%-4d  x%d,y%d (%d cells)\n",
                        lv + 1, d, int(ty), m.aux0(d), m.aux1(d), m.aux2(d), cx, cy, cells);
            if (ty == Square::ScnMsg && haveSp) {
                auto lines = scnMsgLines(sp, m.aux1(d));
                for (auto &ln : lines) std::printf("       | %s\n", ln.text.c_str());
            }
        }
    }
    return 0;
}

// wiz1 scnmsg-test <ASCII.KRN> <msgNo> -- print one scripted message.
static int cmdScnMsgTest(int argc, char **argv) {
    if (argc < 4) { std::puts("scnmsg-test <ASCII.KRN> <msgNo>"); return 2; }
    StringPool sp;
    if (!sp.load(readFile(argv[2]))) return 1;
    int no = std::atoi(argv[3]);
    auto lines = scnMsgLines(sp, no);
    std::printf("msg %d: %d line(s), key base %d\n", no, int(lines.size()), scnMsgKey(no, 0));
    for (auto &ln : lines)
        std::printf("%s| %s\n", ln.center ? "C" : " ", ln.text.c_str());
    return 0;
}

static int cmdMaze(int argc, char **argv) {
    if (argc < 3) { std::puts("maze <SCENARIO.DATA> [level] [keyscript]"); return 2; }
    Scenario sc;
    if (!sc.load(readFile(argv[2]))) { std::fprintf(stderr, "bad scenario\n"); return 1; }
    int level = argc > 3 ? std::atoi(argv[3]) : 1;
    if (level < 1 || level > sc.count(Scenario::Maze)) { std::fprintf(stderr, "level 1..%d\n", sc.count(Scenario::Maze)); return 1; }

    MazeLevel m;
    if (!m.load(sc.record(Scenario::Maze, level - 1))) { std::fprintf(stderr, "bad maze record\n"); return 1; }

    MazePos p{0, 0, NORTH};
    std::string keys = argc > 4 ? unescape(argv[4]) : "";
    std::printf("=== maze level %d ===\n", level);
    drawMazeTop(m, p);
    printCell(m, p);
    printMaze3d(m, p, level);

    int bumps = 0;
    for (char k : keys) {
        k = char(std::toupper((unsigned char)k));
        if (k == 'Q') break;
        if (k == 'F' || k == 'W') { if (canWalk(m, p)) stepForward(p); else { std::puts("bump!"); ++bumps; } }
        else if (k == 'K') { if (canKick(m, p)) stepForward(p); else { std::puts("bump!"); ++bumps; } }
        else if (k == 'L' || k == 'A') turn(p, 3);
        else if (k == 'R' || k == 'D') turn(p, 1);
        else continue;
        std::printf("\n> %c\n", k);
        printCell(m, p);
        printMaze3d(m, p, level);
    }
    if (!keys.empty()) {
        std::puts("");
        drawMazeTop(m, p);
        std::printf("RESULT (%d,%d) %s bumps=%d square=%d\n", p.x, p.y, dirName(p.dir),
                    bumps, int(m.squareAt(p.x, p.y)));
    }
    return 0;
}

// Build a scratch party of the first live roster characters, for the
// standalone maze commands.
static Party scratchParty(const Scenario &sc, Roster &roster, int n = 6) {
    roster.seedFrom(sc);
    Party p;
    for (int i = 0; i < roster.count() && p.count() < n; ++i)
        if (roster.slot(i).status != Status::Lost) p.add(roster, i);
    for (int i = 0; i < p.count(); ++i) { deriveStats(p.member(i)); setSpells(p.member(i)); }
    return p;
}

// wiz1 camp-test <CHARSET> <SCENARIO.DATA> <keyscript> [ASCII.KRN] [grants]
//   grants: "m:i,m:i,..."  give member m (0-based) identified object i
static int cmdCampTest(int argc, char **argv) {
    if (argc < 5) { std::puts("camp-test <CHARSET> <SCENARIO.DATA> <keyscript> [ASCII.KRN] [grants]"); return 2; }
    Font font;
    Scenario sc;
    if (!font.load(readFile(argv[2])) || !sc.load(readFile(argv[3]))) return 1;
    Roster roster;
    Party party = scratchParty(sc, roster);
    StringPool sp;
    bool haveSp = argc > 5 && sp.load(readFile(argv[5]));

    if (argc > 6) {
        std::string g = argv[6];
        size_t pos = 0;
        while (pos < g.size()) {
            size_t comma = g.find(',', pos);
            std::string tok = g.substr(pos, comma == std::string::npos ? comma : comma - pos);
            size_t colon = tok.find(':');
            if (colon != std::string::npos) {
                int m = std::atoi(tok.substr(0, colon).c_str());
                int it = std::atoi(tok.substr(colon + 1).c_str());
                if (m >= 0 && m < party.count() && party.member(m).possCount < 8) {
                    Character &ch = party.member(m);
                    ch.poss[ch.possCount++] = Possession{false, false, true, it};
                }
            }
            if (comma == std::string::npos) break;
            pos = comma + 1;
        }
    }

    auto plat = makeNullPlatform(unescape(argv[4]), "");
    Rng rng;
    Ui ui(*plat, font);
    CampExit e = runCamp(ui, party, sc, haveSp ? &sp : nullptr, rng);

    TextScreen &t = ui.ts();
    for (int y = 0; y < 24; ++y) {
        std::string row;
        for (int x = 0; x < 40; ++x) row += t.at(x, y);
        while (!row.empty() && row.back() == ' ') row.pop_back();
        std::printf("|%s\n", row.c_str());
    }
    static const char *ex[] = {"TO-MAZE", "DISBANDED", "WINDOW-CLOSED"};
    std::string order;
    for (int i = 0; i < party.count(); ++i) {
        if (i) order += ',';
        order += party.member(i).name;
    }
    std::printf("camp exit: %s | order: %s\n", ex[int(e)], order.c_str());
    for (int i = 0; i < party.count(); ++i) {
        const Character &ch = party.member(i);
        std::printf("  %d %-12s poss%d AC%d", i + 1, ch.name.c_str(), ch.possCount, ch.armorClass);
        for (int k = 0; k < ch.possCount; ++k)
            std::printf(" %s#%d", ch.poss[k].equipped ? "E" : "", ch.poss[k].itemIndex);
        std::printf("\n");
    }
    return 0;
}

// wiz1 maze-sdl <CHARSET> <SCENARIO.DATA> [level] -- play the maze with the HUD.
static int cmdMazeSdl(int argc, char **argv) {
    if (argc < 4) { std::puts("maze-sdl <CHARSET> <SCENARIO.DATA> [level]"); return 2; }
    Font font;
    Scenario sc;
    if (!font.load(readFile(argv[2])) || !sc.load(readFile(argv[3]))) return 1;
    Roster roster;
    Party party = scratchParty(sc, roster);

    auto plat = makeSdlPlatform("Wizardry - Maze", 2);
    if (!plat) { std::puts("maze-sdl needs the SDL2 backend"); return 1; }
    Rng rng;
    Ui ui(*plat, font);
    MazeState st;
    st.level = argc > 4 ? std::max(1, std::atoi(argv[4])) : 1;
    runMaze(ui, party, sc, nullptr, rng, st);
    return 0;
}

// Headless: script a walk through the maze and print the ending state.
static int cmdMazePlayTest(int argc, char **argv) {
    if (argc < 5) { std::puts("maze-play-test <CHARSET> <SCENARIO.DATA> <keyscript> [ASCII.KRN] [level x y dir]"); return 2; }
    Font font;
    Scenario sc;
    if (!font.load(readFile(argv[2])) || !sc.load(readFile(argv[3]))) return 1;
    Roster roster;
    Party party = scratchParty(sc, roster);

    StringPool sp;
    bool haveSp = argc > 5 && sp.load(readFile(argv[5]));

    auto plat = makeNullPlatform(unescape(argv[4]), "");
    Rng rng;
    Ui ui(*plat, font);
    MazeState st;
    if (argc > 9) {
        st.level = std::atoi(argv[6]);
        st.pos = MazePos{std::atoi(argv[7]), std::atoi(argv[8]), std::atoi(argv[9])};
    }
    MazeExit e = runMaze(ui, party, sc, haveSp ? &sp : nullptr, rng, st);
    static const char *ex[] = {"TO-TOWN", "PARTY-WIPED", "WINDOW-CLOSED"};
    std::printf("exit: %s   pos (%d,%d) %s  level %d\n", ex[int(e)],
                st.pos.x, st.pos.y, dirName(st.pos.dir), st.level);
    for (int i = 0; i < party.count(); ++i) {
        const Character &c = party.member(i);
        std::printf("  %s HP %d/%d  status %d  poss%d", c.name.c_str(),
                    c.hpLeft, c.hpMax, int(c.status), c.possCount);
        for (int k = 0; k < c.possCount; ++k) std::printf(" #%d", c.poss[k].itemIndex);
        std::printf("\n");
    }
    return 0;
}

// wiz1 combat-test <CHARSET> <SCENARIO.DATA> <monsterIdx> <keyscript> [ASCII.KRN] [attk012]
static int cmdCombatTest(int argc, char **argv) {
    if (argc < 6) { std::puts("combat-test <CHARSET> <SCENARIO.DATA> <monsterIdx> <keyscript> [ASCII.KRN] [attk012]"); return 2; }
    Font font;
    Scenario sc;
    if (!font.load(readFile(argv[2])) || !sc.load(readFile(argv[3]))) return 1;
    Roster roster;
    Party party = scratchParty(sc, roster);
    StringPool sp;
    bool haveSp = argc > 6 && sp.load(readFile(argv[6]));
    int mon = std::atoi(argv[4]);
    int attk012 = argc > 7 ? std::atoi(argv[7]) : 2;

    auto plat = makeNullPlatform(unescape(argv[5]), "");
    Rng rng;
    Ui ui(*plat, font);
    std::vector<std::string> transcript;
    CombatResult r = runCombat(ui, party, sc, haveSp ? &sp : nullptr, rng, mon, 1, attk012, &transcript);
    static const char *rn[] = {"WON", "FLED", "PARTY-WIPED", "WINDOW-CLOSED"};
    std::printf("result: %s\n", rn[int(r)]);
    int casts = 0, healed = 0;
    for (const auto &l : transcript) {
        std::printf("| %s\n", l.c_str());
        if (l.find(" CASTS ") != std::string::npos) ++casts;
        if (l.find(" IS HEALED") != std::string::npos) ++healed;
    }
    int found = 0;
    for (const auto &l : transcript) if (l.find(" FOUND - ") != std::string::npos) ++found;
    std::printf("summary: result=%s casts=%d healed=%d items=%d\n",
                rn[int(r)], casts, healed, found);
    for (int i = 0; i < party.count(); ++i) {
        const Character &c = party.member(i);
        std::printf("  %-12s HP %3d/%-3d st%d  %lld ep  %lld gp  poss%d\n", c.name.c_str(),
                    c.hpLeft, c.hpMax, int(c.status), (long long)c.exp.v,
                    (long long)c.gold.v, c.possCount);
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) return usage();
    std::string cmd = argv[1];
    if (cmd == "maze") return cmdMaze(argc, argv);
    if (cmd == "maze-scan") return cmdMazeScan(argc, argv);
    if (cmd == "scnmsg-test") return cmdScnMsgTest(argc, argv);
    if (cmd == "maze-sdl") return cmdMazeSdl(argc, argv);
    if (cmd == "maze-play-test") return cmdMazePlayTest(argc, argv);
    if (cmd == "combat-test") return cmdCombatTest(argc, argv);
    if (cmd == "camp-test") return cmdCampTest(argc, argv);
    if (cmd == "rng") return cmdRng(argc, argv);
    if (cmd == "roll") return cmdRoll(argc, argv);
    if (cmd == "roster") return cmdRoster(argv[2]);
    if (cmd == "show") return cmdShow(argc, argv);
    if (cmd == "mockup") return cmdMockup(argc, argv);
    if (cmd == "roller") return cmdRoller(argc, argv);
    if (cmd == "roller-test") return cmdRollerTest(argc, argv);
    if (cmd == "town") return cmdTown(argc, argv);
    if (cmd == "town-test") return cmdTownTest(argc, argv);
    if (cmd == "inn-test") return cmdInnTest(argc, argv);
    if (cmd == "temple-test") return cmdTempleTest(argc, argv);
    if (cmd == "files") return cmdFiles(argv[2]);
    if (cmd == "extract" && argc == 5) return cmdExtract(argv[2], argv[3], argv[4]);
    if (cmd == "toc") return cmdToc(argv[2]);
    if (cmd == "monsters" && argc == 4) return cmdMonsters(argv[2], argv[3], false);
    if (cmd == "items" && argc == 4) return cmdMonsters(argv[2], argv[3], true);
    if (cmd == "exp") return cmdExp(argv[2]);
    if (cmd == "str" && argc >= 4) return cmdStr(argc, argv);
    if (cmd == "strings") return cmdStrings(argv[2]);
    return usage();
}
