// CEMETARY / BADSTUFF (SHOPS procs P010215 / P010218) -- run when the whole
// party is wiped out in the maze (XGOTO := XCEMETRY).  See docs/town.md.
#pragma once
#include "wiz/roller_ui.h"          // Ui
#include "wiz/party.h"
#include "wiz/roster.h"

#include <string>
#include <vector>

namespace wiz {

// BADSTUFF: every member (unless already LOST) dies, drops half their gold,
// and rolls each non-cursed item for destruction (RANDOM mod 21 > LUCK).
// A body is left recoverable at (mazeX,mazeY,mazeLevel) unless
// `RANDOM mod 50 < level`, in which case it is lost for good (-1,-1,-1).
// The dead records are written back to the roster and the party is emptied;
// the tombstone screen waits for RETURN.
inline void runCemetery(Ui &ui, Party &party, Roster &roster,
                        int mazeX, int mazeY, int mazeLevel, Rng &rng) {
    int n = party.count();
    std::vector<std::pair<std::string, int>> graves;   // name, age-in-years
    for (int i = 0; i < n; ++i) {
        Character &c = party.member(i);
        graves.emplace_back(c.name, c.age / 52);
        if (c.status == Status::Lost) continue;
        if (int(c.status) < int(Status::Dead)) c.status = Status::Dead;
        c.inMaze = false;
        c.hpLeft = 0;
        c.gold.v /= 2;

        int w = 0;                                   // BREAKPOS
        for (int k = 0; k < c.possCount; ++k) {
            Possession &p = c.poss[k];
            bool destroyed = !p.cursed && rng.mod(21) > c.attrib[LUCK];
            if (!destroyed) c.poss[w++] = p;
        }
        c.possCount = w;

        if (rng.mod(50) < mazeLevel) {               // body lost for good
            c.lostX = c.lostY = c.lostLevel = -1;
        } else {                                     // recoverable via `I`nspect
            c.lostX = mazeX; c.lostY = mazeY; c.lostLevel = mazeLevel;
        }
    }
    party.disband(roster);                            // persist + clear

    TextScreen &t = ui.ts();
    t.resetWindow();
    t.putChar(12);
    for (int i = 0; i < int(graves.size()); ++i) {           // TOMBSTON
        int tx = 20 * (i % 2), ty = 6 * (i / 2);
        t.gotoXY(tx + 3, ty + 0); t.write(graves[i].first.substr(0, 15));
        t.gotoXY(tx + 2, ty + 1); t.write(".-------.");
        t.gotoXY(tx + 2, ty + 2); t.write("| R.I.P |");
        t.gotoXY(tx + 2, ty + 3); t.write("| age " + std::to_string(graves[i].second));
        t.gotoXY(tx + 2, ty + 4); t.write("|_______|");
    }
    t.gotoXY(1, 20); t.write("YOUR ENTIRE PARTY HAS BEEN SLAUGHTERED");
    t.gotoXY(1, 22); t.write("  PRESS RETURN TO LEAVE THE CEMETERY  ");
    ui.refresh();
    ui.pressAnyKey("");
}

} // namespace wiz
