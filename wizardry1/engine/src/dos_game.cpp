// The DOS-faithful game: title screen + the WIZARDRY mainline's XGOTO dispatch
// over the segments ported from the p-code so far (CASTLE, SHOPS), bridging to
// the legacy screens for the rest (Training Grounds, maze, camp).
#include "dos_assets.h"
#include "dos_flow.h"
#include "wiz/castle.h"
#include "wiz/combat_ui.h"
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

    TownWorld tw{party, roster, shop, a.sc, &a.sp, rng, rosterPath, partyPath, shopPath, mazePath};
    Ui ui(*plat, font);
    DosFlow flow(dos, party, roster, a.sc, rng, shop, rosterPath, partyPath, shopPath);

    // the screens that are not ported from the p-code yet run through the legacy Ui
    flow.hooks.inspect = [&](int member, int from) -> int {          // CAMP:5 (the character sheet)
        if (from == XCAMP) flow.castle.closeWindows();
        legacySeeMember(ui, tw, from == 17 ? 0 : member);
        if (ui.quit()) return -1;
        con.refresh();
        return from == XCAMP ? XCASTLE : from == 17 ? 19 : from;
    };
    flow.hooks.combat = [&]() -> int {                               // COMBAT
        CombatResult r = runCombat(ui, party, a.sc, &a.sp, rng, flow.mg.g37, flow.mg.level, flow.mg.g29);
        con.refresh();
        if (r == CombatResult::WindowClosed || ui.quit()) return -1;
        return r == CombatResult::PartyWiped ? 16 : 7;
    };
    flow.hooks.special = [&]() -> int { return 7; };                 // SPECIALS: not ported yet

    flow.run(XCASTLE);
    roster.save(rosterPath);
    party.save(partyPath);
    shop.save(shopPath);
    return 0;
}

} // namespace wiz
