// The WIZARDRY mainline's XGOTO dispatch over the segments ported from the DOS p-code
// (WIZARDRY:1: `case XGOTO of 1: ROLLER; 2,3: CASTLE; 4: CAMP; 5,6,10,16,25: SHOPS; 7: RUNNER;
// 8: COMBAT; 9,13,14,15,20: UTILITIE; 12,17,18,27: CAMP; 19: ROLLER; 23,26: SPECIALS ...`).
//
// Shared by `castle-test` (the regression harness: stops at the first unported screen) and
// `dosgame` (the playable build, whose hooks bridge to the legacy screens).
#pragma once
#include "dos_assets.h"
#include "wiz/camp_dos.h"
#include "wiz/castle.h"
#include "wiz/combat_dos.h"
#include "wiz/maze_dos.h"
#include "wiz/rewards_dos.h"
#include "wiz/roller_dos.h"
#include "wiz/shops.h"
#include "wiz/specials_dos.h"

#include <functional>

namespace wiz {

// Screens that are not ported yet.  A hook returns the next XGOTO, or -1 to stop.
struct FlowHooks {
    // XGOTO 24 (REWARDS / SPECIALS / UTILITIE's camp jobs, `g36` says which), 14 (E)QUIP) and 15
    // (R)EORDER) for character `member`.  Returns the next XGOTO (18 = back to CAMP).
    std::function<int(int xgoto, int g36, int member)> other;
    std::function<int()> combat;       // XGOTO 8 (the encounter is in MazeGlobals::g37 / g29)
};

class DosFlow {
    Dos &d_;
    Party &party_;
public:
    MazeGlobals mg;                    // the WIZARDRY globals the maze shares with CAMP / the castle
private:
    CastleWorld castleWorld_;
    ShopsWorld shopsWorld_;
    RollerWorld rollerWorld_;
public:
    Castle castle;
    Shops shops;
    RollerScreen roller;
private:
    MazeWorld mazeWorld_;
    CampWorld campWorld_;
    SpecialsWorld specialsWorld_;
    CombatWorld combatWorld_;
    RewardsWorld rewardsWorld_;
public:
    MazeScreen maze;
    Camp camp;
    Specials specials;
    Combat combat;
    Rewards rewards;
    FlowHooks hooks;

    DosFlow(Dos &d, Party &party, Roster &roster, const Scenario &sc, Rng &rng, Shop &shop,
            const std::string &rosterPath, const std::string &partyPath, const std::string &shopPath)
        : d_(d), party_(party),
          castleWorld_{party, roster, sc, rng, rosterPath, partyPath},
          shopsWorld_{party, roster, shop, sc, rng, rosterPath, partyPath, shopPath},
          rollerWorld_{party, roster, sc, rng, rosterPath, partyPath},
          castle(d, castleWorld_), shops(d, shopsWorld_, castle), roller(d, rollerWorld_),
          mazeWorld_{party, roster, sc, rng, rosterPath, partyPath, mg, castle.partyWindow()},
          campWorld_{party, castle.partyWindow(), mg, sc, rng},
          specialsWorld_{party, roster, sc, rng, rosterPath, partyPath, mg},
          combatWorld_{party, roster, sc, rng, rosterPath, partyPath, mg, castle.partyWindow()},
          rewardsWorld_{party, roster, sc, rng, rosterPath, partyPath, mg},
          maze(d, mazeWorld_), camp(d, campWorld_), specials(d, specialsWorld_), combat(d, combatWorld_), rewards(d, rewardsWorld_) {}

    // Runs from `xgoto` until the window closes, a hook stops, or an unported screen is reached.
    // Returns the XGOTO it stopped at (-1 when a hook stopped it).
    int run(int xgoto, int *memberOut = nullptr) {
        int member = -1, campMember = -1;
        while (!d_.quit) {
            d_.partyCount = party_.count();
            if (xgoto == XCASTLE || xgoto == XCASTLE2) {
                mg.protect = mg.light = mg.identify = 0;         // CASTLE:1 clears the maze's spell timers
                mg.g31 = mg.g29 = mg.g30 = 0; mg.randomStart = false; mg.g41 = 3;
                CastleResult r = castle.run(xgoto);
                if (r.windowClosed) break;
                xgoto = r.xgoto;
                member = r.member;
            } else if (xgoto == XSHOPS) { shops.boltac(); xgoto = XCASTLE; }
            else if (xgoto == XTEMPLE) { shops.temple(); xgoto = XCASTLE; }
            else if (xgoto == XEDGE) {
                ShopsResult r = shops.edgeOfTown();
                if (r.windowClosed) break;
                xgoto = r.xgoto;
                if (xgoto == XMAZE_ENTRY) {                      // SHOPS:31
                    mg.x = mg.y = 0; mg.dir = 0; mg.level = -1; mg.g38 = mg.g39 = mg.g40 = 0;
                }
            } else if (xgoto == XROLLER || xgoto == 19) {
                RollerResult r = roller.run(xgoto);
                if (r.windowClosed) break;
                xgoto = r.xgoto;
                if (xgoto == 17) {                               // INSPECT: CAMP's sheet
                    CampResult c = camp.run(17);
                    if (c.windowClosed) break;
                    xgoto = c.xgoto;
                }
            } else if (xgoto == XCAMP) {                         // the Tavern's INSPECT
                CampResult c = camp.run(XCAMP, member);
                if (c.windowClosed) break;
                xgoto = c.xgoto;
            } else if (xgoto == 9) {                             // UTILITIE:9 -- into / between / out of the maze
                if (mg.level == 0) { xgoto = 10; continue; }
                bool fresh = mg.level < 0;
                maze.enter();
                xgoto = fresh ? 13 : 7;
            } else if (xgoto == 13 || xgoto == 20) {             // UTILITIE:29 (the derived stats: not ported)
                xgoto = xgoto == 13 ? 12 : 7;
            } else if (xgoto == 12 || xgoto == 18 || xgoto == 27) {   // CAMP:1
                CampResult r = camp.run(xgoto, campMember);
                if (r.windowClosed) break;
                xgoto = r.xgoto;
                if (xgoto == 24 || xgoto == 14 || xgoto == 15) {      // another segment's job
                    campMember = r.member;
                    mg.g36 = r.g36;
                    mg.g9 = r.member;
                    if (!(xgoto == 24 && r.g36 <= 3)) {               // 0..2 REWARDS, 3 DUMAPIC (SPECIALS) below
                        if (!hooks.other) break;
                        xgoto = hooks.other(xgoto, r.g36, r.member);
                        if (xgoto < 0) break;
                    }
                }
            } else if (xgoto == 7) {                             // RUNNER:1
                MazeResult r = maze.run();
                if (r.windowClosed) break;
                xgoto = r.xgoto;
            } else if (xgoto == 8) {                             // COMBAT
                if (hooks.combat) {
                    xgoto = hooks.combat();
                    if (xgoto < 0) break;
                } else {
                    CombatOutcome r = combat.run();
                    if (r.windowClosed) break;
                    xgoto = r.xgoto;
                }
            } else if (xgoto == 11 || xgoto == 22 || (xgoto == 24 && mg.g36 <= 2)) {     // REWARDS
                RewardsResult r = rewards.run(xgoto);
                if (r.windowClosed) break;
                xgoto = r.xgoto;
            } else if (xgoto == 23 || xgoto == 26 || (xgoto == 24 && mg.g36 == 3)) {   // SPECIALS
                SpecialsResult r = specials.run(xgoto);
                if (r.windowClosed) break;
                xgoto = r.xgoto;
            } else if (xgoto == 10) {                            // SHOPS:41 -- out of the maze alive
                shops.leaveMaze(castle.partyWindow());
                xgoto = XCASTLE;
            } else if (xgoto == 16) {                            // SHOPS:27 -- the party is dead
                shops.partyWiped(castle.partyWindow(), mg.level, mg.x, mg.y);
                xgoto = XCASTLE;
            } else break;                                        // not ported yet
        }
        if (memberOut) *memberOut = member;
        return d_.quit ? -1 : xgoto;
    }

};

} // namespace wiz
