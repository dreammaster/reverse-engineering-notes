// Commands that drive the DOS-faithful screens (wiz::Dos / wiz::Console).
#include "dos_assets.h"
#include "wiz/camp_dos.h"
#include "wiz/castle.h"
#include "wiz/maze_dos.h"
#include "wiz/roller_dos.h"
#include "wiz/shops.h"
#include "wiz/platform.h"

#include <cstdio>
#include <cstdlib>

namespace wiz {

// wiz1 castle-test <dataDir> <keys> [gridDir]   (WIZ1_TOWN_PARTY=n seeds a party)
int cmdCastleTest(int argc, char **argv) {
    if (argc < 4) { std::puts("castle-test <dataDir> <keys> [gridDir]"); return 2; }
    DosAssets a;
    if (!a.load(argv[2])) { std::fprintf(stderr, "bad data dir\n"); return 1; }
    auto plat = makeNullPlatform(unescapeKeys(argv[3]), std::getenv("WIZ1_FRAMES") ? std::getenv("WIZ1_FRAMES") : "");
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
    dos.setRngProbe(&rng);
    CastleWorld world{party, roster, a.sc, rng, "", ""};
    Shop shop;
    shop.seedFrom(a.sc);
    ShopsWorld sworld{party, roster, shop, a.sc, rng, "", "", ""};
    Castle castle(dos, world);
    Shops shops(dos, sworld, castle);
    RollerWorld rworld{party, roster, a.sc, rng, "", ""};
    RollerScreen roller(dos, rworld);
    MazeGlobals mg;
    MazeWorld mworld{party, roster, a.sc, rng, "", "", mg, castle.partyWindow()};
    MazeScreen maze(dos, mworld);
    CampWorld campWorld{party, castle.partyWindow(), mg.protect, mg.g41};
    Camp camp(dos, campWorld);

    // the WIZARDRY mainline's XGOTO dispatch, restricted to the segments ported so far
    int xgoto = XCASTLE;
    int member = -1;
    while (!dos.quit) {
        if (xgoto == XCASTLE || xgoto == XCASTLE2) {
            mg.protect = mg.light = mg.identify = 0;      // CASTLE:1 clears the maze's spell timers
            mg.g31 = mg.g29 = mg.g30 = 0; mg.randomStart = false; mg.g41 = 3;
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
        } else if (xgoto == XROLLER || xgoto == 19) {
            RollerResult r = roller.run(xgoto);
            if (r.windowClosed) break;
            xgoto = r.xgoto;
        } else if (xgoto == XMAZE_ENTRY) {                // SHOPS:31 -> UTILITIE:9/10
            mg.x = mg.y = 0; mg.dir = 0; mg.level = -1; mg.g38 = mg.g39 = mg.g40 = 0;
            maze.enter();
            xgoto = 13;
        } else if (xgoto == 13 || xgoto == 20) {          // UTILITIE:29 (derived stats: not ported)
            xgoto = xgoto == 13 ? 12 : 7;
        } else if (xgoto == 12) {                         // CAMP:1
            CampResult r = camp.run(12);
            if (r.windowClosed) break;
            xgoto = r.xgoto;
            if (r.inspect) break;                         // the character sheet: not ported yet
        } else if (xgoto == 7) {                          // RUNNER:1
            MazeResult r = maze.run();
            if (r.windowClosed) break;
            xgoto = r.xgoto;
        } else break;                                   // not ported yet: ...
    }
    std::printf("dos -> xgoto %d member %d  party %d\n", xgoto, member, party.count());
    return 0;
}

} // namespace wiz
