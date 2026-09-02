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
#include "wiz/platform.h"

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
        "  roll    <race> <align> [s3hex]     roll a character\n");
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

static void showSurface(const Surface &s, const char *ppm) {
    if (ppm) {                                   // explicit file -> headless
        s.savePPM(ppm, kDefaultPalette, 16);
        std::printf("wrote %s  (%dx%d)\n", ppm, s.width(), s.height());
        return;
    }
    auto p = makeSdlPlatform("wiz1", 2);          // else open a window
    if (!p) { std::puts("no SDL2 backend; pass an out.ppm path"); return; }
    while (p->running()) {
        p->present(s, kDefaultPalette, 16);
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
        // 200.TITLE is 5120 B; try 512x80 1bpp (512*80/8 = 5120).
        Surface s(512, 80);
        s.fill(0);
        s.blit1bpp(bytes.data(), 512, 80, 0, 0, 15);
        showSurface(s, ppm);
        return 0;
    }
    return usage();
}

int main(int argc, char **argv) {
    if (argc < 2) return usage();
    std::string cmd = argv[1];
    if (cmd == "rng") return cmdRng(argc, argv);
    if (cmd == "roll") return cmdRoll(argc, argv);
    if (cmd == "roster") return cmdRoster(argv[2]);
    if (cmd == "show") return cmdShow(argc, argv);
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
