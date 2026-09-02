#include "wiz/combat_ui.h"
#include "wiz/combat.h"
#include "wiz/scenario.h"
#include "wiz/string_pool.h"
#include "wiz/roller.h"          // deriveStats

#include <cstdio>
#include <deque>
#include <string>

namespace wiz {
namespace {

const char *className(const Scenario &sc, Class c) {
    return int(c) < int(sc.classes().size()) ? sc.classes()[int(c)].c_str() : "?";
}
const char *statusName(const Scenario &sc, Status s) {
    return int(s) < int(sc.statuses().size()) ? sc.statuses()[int(s)].c_str() : "?";
}

struct CombatCtx {
    Ui &ui;
    Party &party;
    const Scenario &sc;
    const StringPool *sp;
    Rng &rng;
    Battle bt;
    std::deque<std::string> log;

    TextScreen &t() { return ui.ts(); }

    void say(const CombatLog &lines) {
        for (const auto &l : lines) log.push_back(l);
        while (log.size() > 6) log.pop_front();
    }
    void say(const std::string &s) { log.push_back(s); while (log.size() > 6) log.pop_front(); }

    void draw() {
        auto &t = this->t();
        t.resetWindow();
        t.putChar(12);
        t.gotoXY(0, 0); t.write("*** ENCOUNTER ***");
        for (int g = 0; g < bt.nGroups; ++g) {
            t.gotoXY(0, 1 + g);
            char b[48];
            std::snprintf(b, sizeof b, "%c) %s", 'A' + g,
                          groupName(sc, sp, bt.grp[g]).c_str());
            t.write(b);
        }
        t.gotoXY(0, 6); t.write(" # NAME            CLASS  HP    STATUS");
        for (int i = 0; i < party.count(); ++i) {
            const Character &c = party.member(i);
            t.gotoXY(0, 7 + i);
            char b[64];
            std::snprintf(b, sizeof b, "%d %-15s %-6.3s %3d/%-3d %s",
                          i + 1, c.name.c_str(), className(sc, c.cls),
                          c.hpLeft, c.hpMax, statusName(sc, c.status));
            t.write(b);
        }
        t.clearRect(0, 15, 40, 7);
        int row = 0;
        for (const auto &l : log) { t.gotoXY(0, 15 + row++); t.write(l.substr(0, 39)); }
        ui.refresh();
    }

    void pause() { draw(); ui.pressAnyKey("[RETURN]"); }
};

// One round: each conscious member acts, then every living monster attacks.
// Returns a CombatResult once the fight is decided, else CombatResult::Won as
// a sentinel meaning "continue" (checked by the caller loop).
bool round(CombatCtx &c, CombatResult &out) {
    auto &t = c.t();

    for (int i = 0; i < c.party.count(); ++i) {
        Character &ch = c.party.member(i);
        if (ch.status != Status::OK && ch.status != Status::Afraid) continue;
        if (allMonstersDead(c.bt)) break;

        for (;;) {
            c.draw();
            t.resetWindow();
            t.clearRect(0, 22, 40, 2);
            t.gotoXY(0, 22);
            t.write(ch.name + ": F)IGHT  P)ARRY  R)UN");
            int k = c.ui.getKey();
            if (c.ui.quit()) { out = CombatResult::WindowClosed; return true; }
            if (k == 'P') break;
            if (k == 'R') {
                if (c.rng.mod(4) != 0) { out = CombatResult::Fled; return true; }
                c.say("-- CAN'T RUN! --");
                break;
            }
            if (k == 'F') {
                int tg = 0;
                if (c.bt.nGroups > 1) {
                    t.gotoXY(0, 23); t.write("FIGHT WHICH GROUP (A-");
                    t.write(std::string(1, char('A' + c.bt.nGroups - 1)));
                    t.write(")?");
                    int g = c.ui.getKey();
                    if (c.ui.quit()) { out = CombatResult::WindowClosed; return true; }
                    tg = g - 'A';
                    if (tg < 0 || tg >= c.bt.nGroups) continue;
                }
                if (c.bt.grp[tg].alive == 0) { c.say("-- THAT GROUP IS GONE --"); break; }
                CombatLog l;
                partyAttack(c.bt, c.sc, ch, tg, c.rng, l);
                c.say(l);
                break;
            }
        }
        if (allMonstersDead(c.bt)) break;
    }

    if (allMonstersDead(c.bt)) { out = CombatResult::Won; return true; }

    // monsters retaliate
    for (int g = 0; g < c.bt.nGroups; ++g)
        for (int m = 0; m < c.bt.grp[g].count; ++m) {
            if (int(c.bt.grp[g].status[m]) >= int(Status::Dead)) continue;
            CombatLog l;
            monsterAttack(c.bt, c.sc, c.party, g, m, c.rng, l);
            c.say(l);
        }
    c.pause();

    if (!partyCanFight(c.party)) { out = CombatResult::PartyWiped; return true; }
    return false;
}

} // namespace

CombatResult runCombat(Ui &ui, Party &party, const Scenario &sc,
                       const StringPool *sp, Rng &rng, int enemyInx, int mazeLevel) {
    struct ClearOverlay { Ui &u; ~ClearOverlay() { u.setOverlay(nullptr); } } _co{ui};
    ui.setOverlay(nullptr);

    for (int i = 0; i < party.count(); ++i) {
        Character &ch = party.member(i);
        if (ch.swingCnt < 1 || ch.hpDamRc[1] < 1) deriveStats(ch);   // ensure combat stats
    }

    CombatCtx c{ui, party, sc, sp, rng};
    buildEncounter(c.bt, sc, enemyInx, mazeLevel, rng);
    c.say("A GROUP OF MONSTERS BLOCKS YOUR WAY!");
    c.pause();

    for (int guard = 0; guard < 200; ++guard) {
        CombatResult out;
        if (round(c, out)) {
            if (out == CombatResult::Won) {
                CombatLog l;
                distributeRewards(c.bt, sc, party, rng, l);
                c.say("*** VICTORY ***");
                c.say(l);
                c.pause();
            } else if (out == CombatResult::Fled) {
                c.say("YOU HAVE FLED FROM COMBAT.");
                c.pause();
            }
            return out;
        }
    }
    return CombatResult::Fled;
}

} // namespace wiz
