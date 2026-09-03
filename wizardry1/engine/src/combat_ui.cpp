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
    std::vector<std::string> *transcript = nullptr;

    TextScreen &t() { return ui.ts(); }

    void say(const CombatLog &lines) {
        for (const auto &l : lines) log.push_back(l);
        if (transcript) for (const auto &l : lines) transcript->push_back(l);
        while (log.size() > 6) log.pop_front();
    }
    void say(const std::string &s) {
        log.push_back(s);
        if (transcript) transcript->push_back(s);
        while (log.size() > 6) log.pop_front();
    }

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

// Pick a group letter A..nGroups-1 with living monsters; -1 on quit.
int pickGroup(CombatCtx &c, bool &quit) {
    quit = false;
    if (c.bt.nGroups <= 1) return c.bt.grp[0].alive ? 0 : -1;
    auto &t = c.t();
    t.gotoXY(0, 23);
    t.write(std::string("WHICH GROUP (A-") + char('A' + c.bt.nGroups - 1) + ")?");
    for (;;) {
        int k = c.ui.getKey();
        if (c.ui.quit()) { quit = true; return -1; }
        if (k == KEY_RETURN) return -1;
        int g = k - 'A';
        if (g >= 0 && g < c.bt.nGroups && c.bt.grp[g].alive) return g;
    }
}

// C)AST -- choose one of the caster's available spells, target it, apply it.
// Returns true if a spell was actually cast.
bool doCast(CombatCtx &c, Character &ch, bool &quit) {
    quit = false;
    auto &t = c.t();
    struct Avail { int no; const char *name; };
    Avail av[16]; int n = 0;
    for (int sp = 1; sp <= 50 && n < 16; ++sp) {
        if (!ch.spellKnown[sp]) continue;
        const SpellDef *d = spellDef(sp);
        if (!d) continue;
        int pool = d->priest ? ch.priestSpells[d->level] : ch.mageSpells[d->level];
        if (pool <= 0) continue;
        av[n++] = {sp, d->name};
    }
    if (n == 0) { c.say(ch.name + " HAS NO SPELLS READY"); return false; }

    t.clearRect(0, 22, 40, 2);
    t.gotoXY(0, 22);
    std::string line = "CAST: ";
    for (int i = 0; i < n; ++i) { line += char('1' + i); line += ')'; line += av[i].name; line += ' '; }
    t.write(line.substr(0, 39));
    int k = c.ui.getKey();
    if (c.ui.quit()) { quit = true; return false; }
    int pick = k - '1';
    if (pick < 0 || pick >= n) return false;

    int sp = av[pick].no;
    const SpellDef *d = spellDef(sp);
    int tg = -1, tgAlly = -1;
    if (d->targ == SP_ENEMY_GROUP || d->targ == SP_ONE_ENEMY) {
        tg = pickGroup(c, quit);
        if (quit || tg < 0) return false;
    } else if (d->targ == SP_ONE_ALLY) {
        t.gotoXY(0, 23); t.write("ON WHOM (1-");
        t.write(std::to_string(c.party.count())); t.write(")?");
        int w = c.ui.getKey();
        if (c.ui.quit()) { quit = true; return false; }
        tgAlly = w - '1';
        if (tgAlly < 0 || tgAlly >= c.party.count()) return false;
    } else {                                // self / party
        tgAlly = -1;
        for (int i = 0; i < c.party.count(); ++i) if (&c.party.member(i) == &ch) tgAlly = i;
    }

    (d->priest ? ch.priestSpells[d->level] : ch.mageSpells[d->level])--;
    CombatLog l;
    l.push_back(ch.name + " CASTS " + d->name + "!");
    castSpell(c.bt, c.sc, c.party, false, ch.charLevel, sp, tg, -1, tgAlly, c.rng, l);
    c.say(l);
    return true;
}

// A monster's turn: cast (if a spellcaster) or melee.
void monsterTurn(CombatCtx &c, int g, int m) {
    MonsterRec r = c.bt.grp[g].rec(c.sc);
    int ml = std::max(r.magSpels(), r.priSpels());
    if (ml > 0 && c.rng.mod(3) == 0) {
        // pick a rough offensive spell for the monster's caster level
        int sp = r.priSpels() > 0
                     ? (ml >= 4 ? 41 /*LITOKAN*/ : ml >= 2 ? 24 /*BADIOS*/ : 24)
                     : (ml >= 3 ? 7 /*MAHALITO*/ : ml >= 1 ? 3 /*KATINO*/ : 1);
        CombatLog l;
        l.push_back("A MONSTER CASTS A SPELL!");
        castSpell(c.bt, c.sc, c.party, true, r.hpDice(), sp, 0, -1, -1, c.rng, l);
        c.say(l);
        return;
    }
    CombatLog l;
    monsterAttack(c.bt, c.sc, c.party, g, m, c.rng, l);
    c.say(l);
}

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
            t.write(ch.name + ": F)IGHT  C)AST  P)ARRY  R)UN");
            int k = c.ui.getKey();
            if (c.ui.quit()) { out = CombatResult::WindowClosed; return true; }
            if (k == 'P') break;
            if (k == 'C') {
                bool q = false;
                if (doCast(c, ch, q)) break;
                if (q) { out = CombatResult::WindowClosed; return true; }
                continue;
            }
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
            Status ms = c.bt.grp[g].status[m];
            if (int(ms) >= int(Status::Dead) || ms == Status::Asleep || ms == Status::Paralyzed)
                continue;
            monsterTurn(c, g, m);
            if (!partyCanFight(c.party)) break;
        }
    c.pause();

    if (!partyCanFight(c.party)) { out = CombatResult::PartyWiped; return true; }
    return false;
}

} // namespace

CombatResult runCombat(Ui &ui, Party &party, const Scenario &sc,
                       const StringPool *sp, Rng &rng, int enemyInx, int mazeLevel,
                       std::vector<std::string> *transcript) {
    struct ClearOverlay { Ui &u; ~ClearOverlay() { u.setOverlay(nullptr); } } _co{ui};
    ui.setOverlay(nullptr);

    for (int i = 0; i < party.count(); ++i) {
        Character &ch = party.member(i);
        if (ch.swingCnt < 1 || ch.hpDamRc[1] < 1) deriveStats(ch);   // ensure combat stats
    }

    CombatCtx c{ui, party, sc, sp, rng};
    c.transcript = transcript;
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
