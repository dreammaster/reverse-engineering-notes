// The DOS-faithful game: title screen + the WIZARDRY mainline's XGOTO dispatch
// over the segments ported from the p-code so far (CASTLE, SHOPS), bridging to
// the legacy screens for the rest (Training Grounds, maze, camp).
#include "dos_assets.h"
#include "wiz/castle.h"
#include "wiz/cemetery.h"
#include "wiz/font.h"
#include "wiz/maze_ui.h"
#include "wiz/platform.h"
#include "wiz/roller_dos.h"
#include "wiz/roller_ui.h"
#include "wiz/shops.h"
#include "wiz/title.h"
#include "wiz/town_ui.h"

#include <cstdio>
#include <cstdlib>

namespace wiz {

// wiz1 title-test <dataDir> <keys> [gridDir]
int cmdTitleTest(int argc, char **argv) {
    if (argc < 4) { std::puts("title-test <dataDir> <keys> [gridDir]"); return 2; }
    DosAssets a;
    if (!a.load(argv[2])) { std::fprintf(stderr, "bad data dir\n"); return 1; }
    auto plat = makeNullPlatform(unescapeKeys(argv[3]), std::getenv("WIZ1_FRAMES") ? std::getenv("WIZ1_FRAMES") : "");
    Console con;
    con.loadCharset(a.charset.data(), a.charset.size());
    con.loadMonsters(a.monsters);
    Dos dos(*plat, con, a.sp);
    if (argc > 4) dos.setGridDump(argv[4]);
    dos.initScreen();
    Rng rng;
    TitleResult r = runTitle(dos, rng, a.title);
    std::printf("title -> %d\n", int(r));
    return 0;
}

// wiz1 dosgame <dataDir> [stateDir]   (SDL window)
int cmdDosGame(int argc, char **argv) {
    if (argc < 3) { std::puts("dosgame <dataDir> [stateDir]"); return 2; }
    DosAssets a;
    if (!a.load(argv[2])) { std::fprintf(stderr, "bad data dir\n"); return 1; }
    std::string dir = argc > 3 ? argv[3] : ".";
    std::string rosterPath = dir + "/roster.dat", partyPath = dir + "/party.dat",
                shopPath = dir + "/shop.dat", mazePath = dir + "/maze.dat";
    auto plat = makeSdlPlatform("Wizardry", 2);
    if (!plat) { std::puts("dosgame needs the SDL2 backend"); return 1; }
    Font font;
    if (!font.load(a.charset)) { std::fprintf(stderr, "bad charset\n"); return 1; }

    Console con;
    con.loadCharset(a.charset.data(), a.charset.size());
    con.loadMonsters(a.monsters);
    Dos dos(*plat, con, a.sp);
    dos.initScreen();
    Rng rng;

    Roster roster;
    if (!roster.load(rosterPath)) roster.seedFrom(a.sc);
    startupResetInMaze(roster);                         // S)TART: everyone is back in town
    Party party;
    Shop shop;
    if (!shop.load(shopPath, a.sc)) shop.seedFrom(a.sc);

    TitleResult tr;
    do tr = runTitle(dos, rng, a.title); while (tr == TitleResult::MakeDisk);
    if (tr == TitleResult::Quit) return 0;

    CastleWorld cw{party, roster, a.sc, rng, rosterPath, partyPath};
    ShopsWorld sw{party, roster, shop, a.sc, rng, rosterPath, partyPath, shopPath};
    TownWorld tw{party, roster, shop, a.sc, &a.sp, rng, rosterPath, partyPath, shopPath, mazePath};
    Castle castle(dos, cw);
    Shops shops(dos, sw, castle);
    RollerWorld rw{party, roster, a.sc, rng, rosterPath, partyPath};
    RollerScreen roller(dos, rw);
    Ui ui(*plat, font);
    MazeState mst;

    int xgoto = XCASTLE;
    int member = -1;
    while (!dos.quit && xgoto != XQUIT) {
        dos.partyCount = party.count();
        if (xgoto == XCASTLE || xgoto == XCASTLE2) {
            CastleResult r = castle.run(xgoto);
            if (r.windowClosed) break;
            xgoto = r.xgoto;
            member = r.member;
        } else if (xgoto == XSHOPS) {
            shops.boltac();
            xgoto = XCASTLE;
        } else if (xgoto == XTEMPLE) {
            shops.temple();
            xgoto = XCASTLE;
        } else if (xgoto == XEDGE) {
            ShopsResult r = shops.edgeOfTown();
            if (r.windowClosed) break;
            xgoto = r.xgoto;
        } else if (xgoto == XCAMP) {                    // legacy character sheet (CAMP not ported yet)
            castle.closeWindows();
            legacySeeMember(ui, tw, member);
            if (ui.quit()) break;
            con.refresh();
            xgoto = XCASTLE;
        } else if (xgoto == XROLLER || xgoto == 19) {   // Training Grounds (ROLLER)
            RollerResult r = roller.run(xgoto);
            if (r.windowClosed) break;
            xgoto = r.xgoto;
            if (xgoto == 17) {                          // INSPECT: CAMP's sheet (legacy view for now)
                legacySeeMember(ui, tw, 0);
                if (ui.quit()) break;
                con.refresh();
                xgoto = 19;
            }
        } else if (xgoto == XMAZE_ENTRY) {              // legacy maze
            castle.closeWindows();
            mst = MazeState{};
            MazeExit me = runMaze(ui, party, roster, a.sc, &a.sp, rng, mst);
            if (me == MazeExit::WindowClosed) break;
            if (me == MazeExit::PartyWiped)
                runCemetery(ui, party, roster, mst.pos.x, mst.pos.y, mst.level, rng);
            else
                endgameCheck(ui, party);
            roster.save(rosterPath);
            party.save(partyPath);
            con.refresh();
            xgoto = XCASTLE;
        } else {                                        // U)TILITIES etc.: not ported yet
            xgoto = XCASTLE;
        }
    }
    roster.save(rosterPath);
    party.save(partyPath);
    shop.save(shopPath);
    return 0;
}

} // namespace wiz
