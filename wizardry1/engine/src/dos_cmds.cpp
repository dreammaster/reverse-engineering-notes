// Commands that drive the DOS-faithful screens (wiz::Dos / wiz::Console).
#include "wiz/castle.h"
#include "wiz/shops.h"
#include "wiz/platform.h"
#include "wiz/scenario.h"
#include "wiz/string_pool.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>

namespace wiz {
namespace {

std::vector<u8> slurp(const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    return std::vector<u8>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

std::string unescapeKeys(const char *s) {                 // "\r" "\e" "\xNN"
    std::string o;
    for (; *s; ++s) {
        if (*s != '\\' || !s[1]) { o += *s; continue; }
        ++s;
        switch (*s) {
            case 'r': o += '\r'; break;
            case 'e': o += '\x1b'; break;
            case 'n': o += '\n'; break;
            case 'b': o += '\b'; break;
            case 'x': o += char(std::strtol(std::string(s + 1, 2).c_str(), nullptr, 16)); s += 2; break;
            default: o += *s; break;
        }
    }
    return o;
}

// Everything a DOS-style screen needs, loaded from the extracted game directory.
struct DosAssets {
    Scenario sc;
    StringPool sp;
    std::vector<u8> charset, monsters, title;
    bool load(const std::string &dir) {
        if (!sc.load(slurp(dir + "/SCENARIO.DATA"))) return false;
        if (!sp.load(slurp(dir + "/ASCII.KRN"))) return false;
        charset = slurp(dir + "/200.CHARSET");
        monsters = slurp(dir + "/200.MONSTERS");
        title = slurp(dir + "/200.TITLE");
        return !charset.empty();
    }
};

} // namespace

// wiz1 castle-test <dataDir> <keys> [gridDir]   (WIZ1_TOWN_PARTY=n seeds a party)
int cmdCastleTest(int argc, char **argv) {
    if (argc < 4) { std::puts("castle-test <dataDir> <keys> [gridDir]"); return 2; }
    DosAssets a;
    if (!a.load(argv[2])) { std::fprintf(stderr, "bad data dir\n"); return 1; }
    auto plat = makeNullPlatform(unescapeKeys(argv[3]), "");
    Console con;
    con.loadCharset(a.charset.data(), a.charset.size());
    con.loadMonsters(a.monsters);
    Dos dos(*plat, con, a.sp);
    if (argc > 4) dos.setGridDump(argv[4]);
    dos.initScreen();

    Roster roster;
    roster.seedFrom(a.sc);
    startupResetInMaze(roster);
    Party party;
    if (const char *n = std::getenv("WIZ1_TOWN_PARTY")) {
        int want = std::atoi(n);
        for (int i = 0; i < roster.count() && party.count() < want; ++i)
            if (roster.slot(i).status != Status::Lost) party.add(roster, i);
    }
    Rng rng;
    // test hooks shared with tools/wizvm.py: WIZ1_RNG="s0,s1,s2,s3" (hex) and
    // WIZ1_PATCH="slot:word:value,..." (poke roster words) reproduce an oracle run
    if (const char *r = std::getenv("WIZ1_RNG")) {
        unsigned v[4] = {0, 0, 0, 0};
        std::sscanf(r, "%x,%x,%x,%x", &v[0], &v[1], &v[2], &v[3]);
        rng.reseed(u16(v[0]), u16(v[1]), u16(v[2]), u16(v[3]));
    }
    if (const char *pt = std::getenv("WIZ1_PATCH")) {
        std::string spec = pt;
        size_t pos = 0;
        while (pos < spec.size()) {
            size_t e = spec.find(',', pos);
            std::string item = spec.substr(pos, e == std::string::npos ? std::string::npos : e - pos);
            int sl = 0, wd = 0, val = 0;
            if (std::sscanf(item.c_str(), "%i:%i:%i", &sl, &wd, &val) == 3 && sl >= 0 && sl < roster.count()) {
                auto raw = roster.slot(sl).write();
                raw[size_t(wd) * 2] = u8(val);
                raw[size_t(wd) * 2 + 1] = u8(val >> 8);
                roster.slot(sl).read({raw.data(), raw.size()});
            }
            if (e == std::string::npos) break;
            pos = e + 1;
        }
    }
    CastleWorld world{party, roster, a.sc, rng, "", ""};
    Shop shop;
    shop.seedFrom(a.sc);
    ShopsWorld sworld{party, roster, shop, a.sc, rng, "", "", ""};
    Castle castle(dos, world);
    Shops shops(dos, sworld, castle);

    // the WIZARDRY mainline's XGOTO dispatch, restricted to the segments ported so far
    int xgoto = XCASTLE;
    int member = -1;
    while (!dos.quit) {
        if (xgoto == XCASTLE || xgoto == XCASTLE2) {
            CastleResult r = castle.run(xgoto);
            xgoto = r.xgoto;
            member = r.member;
            if (r.windowClosed) break;
        } else if (xgoto == XSHOPS) { shops.boltac(); xgoto = XCASTLE; }
        else if (xgoto == XTEMPLE) { shops.temple(); xgoto = XCASTLE; }
        else if (xgoto == XEDGE) {
            ShopsResult r = shops.edgeOfTown();
            if (r.windowClosed) break;
            xgoto = r.xgoto;
        } else break;                                   // not ported yet: CAMP, ROLLER, maze, ...
    }
    std::printf("dos -> xgoto %d member %d  party %d\n", xgoto, member, party.count());
    return 0;
}

} // namespace wiz
