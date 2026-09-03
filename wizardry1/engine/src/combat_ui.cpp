#include "wiz/combat_ui.h"
#include "wiz/combat.h"
#include "wiz/rewards.h"
#include "wiz/scenario.h"
#include "wiz/string_pool.h"
#include "wiz/roller.h"          // deriveStats

#include <cctype>
#include <cstdio>
#include <deque>
#include <string>
#include <vector>

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
    int attk012 = 2;
    int mazeLevel = 1;
    int parleyThresh = -1;

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

// ---- ACHEST (REWARDS P010D08): the chest / trap mini-game ------------

enum class ChestExit { Rewards, Left, Alarm, Wiped };

// Ask "<prompt>" and read a party slot digit; -1 if out of range / not OK.
int askChar(CombatCtx &c, const char *prompt, bool needOk = true) {
    auto &t = c.t();
    t.clearRect(0, 22, 40, 2);
    t.gotoXY(0, 22); t.write(prompt);
    int k = c.ui.getKey();
    if (c.ui.quit()) return -2;
    int idx = k - '1';
    if (idx < 0 || idx >= c.party.count()) return -1;
    if (needOk && c.party.member(idx).status != Status::OK) return -1;
    return idx;
}

void trapHit(CombatCtx &c, const ChestTrap &tr, int chestChar, ChestExit &ce) {
    CombatLog l;
    l.push_back("OOPS! A " + trapName(tr) + "!");
    TrapOutcome o = springTrap(tr, c.party, chestChar, c.mazeLevel, c.rng, l);
    c.say(l);
    if (o.wiped)         ce = ChestExit::Wiped;
    else if (o.alarm)    ce = ChestExit::Alarm;
    else if (o.teleport) c.say("YOU ARE WHISKED AWAY!");   // maze relocates on return
}

ChestExit runChest(CombatCtx &c, const RewardRec &rw) {
    ChestTrap tr = pickChestTrap(rw, c.mazeLevel, c.rng);
    std::vector<char> looked(c.party.count(), 0);
    c.say("A CHEST!  YOU MAY:");
    c.draw();

    for (int guard = 0; guard < 60; ++guard) {
        auto &t = c.t();
        t.clearRect(0, 22, 40, 2);
        t.gotoXY(0, 22); t.write("O)PEN  I)NSPECT  D)ISARM  C)ALFO  L)EAVE");
        int k = c.ui.getKey();
        if (c.ui.quit()) return ChestExit::Left;
        ChestExit ce = ChestExit::Rewards;

        if (k == 'L') return ChestExit::Left;

        if (k == 'O') {
            int who = askChar(c, "WHO (#) WILL OPEN?");
            if (who < 0) continue;
            if (tr.type == 0) return ChestExit::Rewards;
            if (c.rng.mod(1000) < c.party.member(who).charLevel) return ChestExit::Rewards;
            trapHit(c, tr, who, ce);
            return ce;
        }

        if (k == 'I') {
            int who = askChar(c, "WHO (#) WILL INSPECT?");
            if (who < 0) continue;
            if (looked[who]) { c.say("YOU ALREADY LOOKED!"); c.draw(); continue; }
            looked[who] = 1;
            Character &ch = c.party.member(who);
            int chance = ch.attrib[AGI];
            if (ch.cls == Class::Thief)      chance *= 6;
            else if (ch.cls == Class::Ninja) chance *= 4;
            if (chance > 95) chance = 95;
            if (c.rng.mod(100) < chance) {
                c.say(std::string("IT IS A ") + trapName(tr));      // PRTRAP
                c.draw();
            } else if (c.rng.mod(20) > ch.attrib[AGI]) {
                trapHit(c, tr, who, ce);                            // set it off
                return ce;
            } else {
                ChestTrap rnd{ c.rng.mod(8), c.rng.mod(5) };        // PRRNDTRP
                c.say(std::string("IT LOOKS LIKE A ") + trapName(rnd));
                c.draw();
            }
            continue;
        }

        if (k == 'C') {
            int who = askChar(c, "WHO (#) WILL CAST CALFO?");
            if (who < 0) continue;
            Character &ch = c.party.member(who);
            if (!ch.spellKnown[28] || ch.priestSpells[2] <= 0) { c.say("CAN'T CAST CALFO!"); c.draw(); continue; }
            ch.priestSpells[2]--;
            if (c.rng.mod(100) < 95) c.say(std::string("IT IS A ") + trapName(tr));
            else { ChestTrap rnd{ c.rng.mod(8), c.rng.mod(5) };
                   c.say(std::string("IT LOOKS LIKE A ") + trapName(rnd)); }
            c.draw();
            continue;
        }

        if (k == 'D') {
            int who = askChar(c, "WHO (#) WILL DISARM?");
            if (who < 0) continue;
            auto &t2 = c.t();
            t2.clearRect(0, 22, 40, 2);
            t2.gotoXY(0, 22); t2.write("WHAT TRAP > ");
            std::string guess = c.ui.getLine(24);
            if (c.ui.quit()) return ChestExit::Left;
            for (auto &ch : guess) ch = char(std::toupper((unsigned char)ch));
            Character &ch = c.party.member(who);
            bool right = (guess == trapName(tr));
            if (!right) { trapHit(c, tr, who, ce); return ce; }
            bool rogue = ch.cls == Class::Thief || ch.cls == Class::Ninja;
            if (c.rng.mod(70) < ch.charLevel - c.mazeLevel + (rogue ? 50 : 0)) {
                c.say("YOU DISARMED IT!");
                return ChestExit::Rewards;
            }
            if (c.rng.mod(20) < ch.attrib[AGI]) { c.say("DISARM FAILED!!"); c.draw(); continue; }
            c.say("YOU SET IT OFF!");
            trapHit(c, tr, who, ce);
            return ce;
        }
    }
    return ChestExit::Rewards;
}

// ---- FRIENDLY (COMBAT P010509): the parley -------------------------

// Rolls the friendly-encounter check.  Returns true if the party chose to
// leave in peace (runCombat -> CombatResult::Friendly); false to proceed
// into the fight (also the common "not friendly this time" path).
bool friendlyParley(CombatCtx &c) {
    bool anyGood = false;                            // GOODLEAV
    for (int i = 0; i < c.party.count(); ++i)
        anyGood = anyGood || c.party.member(i).align == Align::Good;
    if (!anyGood) return false;

    int z = c.rng.mod(100);
    int thresh;
    switch (c.bt.grp[0].rec(c.sc).cls()) {           // BATTLERC[1].B.CLASS
        case 0: thresh = 60; break;   // fighter
        case 1: thresh = 55; break;   // mage
        case 2: thresh = 65; break;   // priest
        case 3: thresh = 53; break;   // thief
        case 4: thresh = 80; break;
        case 7: thresh = 75; break;
        default: thresh = 50; break;
    }
    if (c.parleyThresh >= 0) thresh = c.parleyThresh;    // test override
    if (z < 50 || z > thresh) return false;          // hostile after all

    for (int g = 0; g < c.bt.nGroups; ++g) c.bt.grp[g].identified = true;
    CombatLog l;
    l.push_back("A FRIENDLY GROUP OF " + groupName(c.sc, c.sp, c.bt.grp[0]) + ".");
    l.push_back("THEY HAIL YOU IN WELCOME!");
    c.say(l);
    c.draw();
    c.t().gotoXY(0, 22);
    c.t().write("F)IGHT OR L)EAVE IN PEACE?");
    c.ui.refresh();

    for (;;) {
        int k = c.ui.getKey();
        if (c.ui.quit() || k == 'L') { c.say("YOU LEAVE IN PEACE."); return true; }
        if (k == 'F') {
            for (int i = 0; i < c.party.count(); ++i) {  // attacking friends
                Character &ch = c.party.member(i);
                if (ch.align == Align::Good && c.rng.mod(2000) == 565) {
                    ch.align = Align::Evil;
                    c.say(ch.name + " TURNS EVIL!");
                }
            }
            c.say("YOU ATTACK THE FRIENDLY GROUP!");
            return false;
        }
    }
}

} // namespace

namespace {

// GIVEEXP + ACHEST + CHSTGOLD after a win.  Returns true if the party was
// wiped out by a chest trap (caller -> PartyWiped).
bool awardVictory(CombatCtx &c, int enemyInx) {
    CombatLog l;
    giveExp(c.bt, c.sc, c.party, l);
    c.say("*** VICTORY ***");
    c.say(l);
    c.pause();

    int o2 = 1;
    int ridx = chooseRewardIndex(c.bt, c.sc, c.attk012, o2);
    RewardRec rw;
    bool haveRw = loadReward(c.sc, ridx, rw);

    if (haveRw && rw.chest()) {
        switch (runChest(c, rw)) {
            case ChestExit::Left:  c.say("YOU LEAVE THE CHEST."); c.pause(); return false;
            case ChestExit::Wiped: c.pause(); return true;
            case ChestExit::Alarm:
                c.say("THE ALARM SOUNDS!");
                c.pause();
                if (runCombat(c.ui, c.party, c.sc, c.sp, c.rng, enemyInx,
                              c.mazeLevel, 2, c.transcript) == CombatResult::PartyWiped)
                    return true;
                break;                                  // then still grab the loot
            case ChestExit::Rewards: break;
        }
    }

    CombatLog l2;
    std::vector<ItemGrant> grants;
    bool hc = false;
    rollTreasure(c.bt, c.sc, c.sp, c.party, c.attk012, c.rng, l2, grants, hc);
    if (!l2.empty()) { c.say(l2); c.pause(); }
    return !partyCanFight(c.party);
}

} // namespace

CombatResult runCombat(Ui &ui, Party &party, const Scenario &sc,
                       const StringPool *sp, Rng &rng, int enemyInx, int mazeLevel,
                       int attk012, std::vector<std::string> *transcript,
                       int parleyThresh) {
    struct ClearOverlay { Ui &u; ~ClearOverlay() { u.setOverlay(nullptr); } } _co{ui};
    ui.setOverlay(nullptr);

    for (int i = 0; i < party.count(); ++i) {
        Character &ch = party.member(i);
        if (ch.swingCnt < 1 || ch.hpDamRc[1] < 1) deriveStats(ch);   // ensure combat stats
    }

    CombatCtx c{ui, party, sc, sp, rng};
    c.transcript = transcript;
    c.attk012 = attk012;
    c.mazeLevel = mazeLevel;
    c.parleyThresh = parleyThresh;
    buildEncounter(c.bt, sc, enemyInx, mazeLevel, rng);

    // INITATTK's surprise roll (kept for RNG fidelity; the free-round effect
    // is not modelled yet): party surprised (1), monsters surprised (2), or
    // neither (0).
    c.bt.surprise = (rng.mod(100) > 80) ? 1 : (rng.mod(100) > 80 ? 2 : 0);

    if (friendlyParley(c)) return CombatResult::Friendly;
    c.say("A GROUP OF MONSTERS BLOCKS YOUR WAY!");
    c.pause();

    for (int guard = 0; guard < 200; ++guard) {
        CombatResult out;
        if (round(c, out)) {
            if (out == CombatResult::Won) {
                if (awardVictory(c, enemyInx)) return CombatResult::PartyWiped;
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
