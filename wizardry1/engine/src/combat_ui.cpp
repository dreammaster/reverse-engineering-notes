#include "wiz/combat_ui.h"
#include "wiz/combat.h"
#include "wiz/rewards.h"
#include "wiz/scenario.h"
#include "wiz/string_pool.h"
#include "wiz/roller.h"          // deriveStats
#include "wiz/monster_art.h"     // blitPortrait -- 200.MONSTERS

#include <cctype>
#include <cstdio>
#include <deque>
#include <string>
#include <vector>

namespace wiz {
namespace {

// --- combat screen windows (COMBAT proc 1 + CUTIL) --------------------
// Three stacked framed panels, DOS-style (see docs/ui.md): the monster /
// encounter window, the party-status window, the scrolling message window.
constexpr int kEncY = 0,  kEncH = 7;            // rows 0..6   (interior 1..5)
constexpr int kPtyY = 6,  kPtyH = 9;            // rows 6..14  (interior 7..13)
constexpr int kMsgY = 14, kMsgH = 8;            // rows 14..21 (interior 15..20)
// rows 21..23 stay a free prompt strip (the DOS action-menu window).

// The portrait band sits in the encounter window's interior (rows 1..5).
constexpr int kPortRow = 1;                     // text row  -> y = row*Font::kH
const int kPortStartCell[4][4] = {
    {15,  0,  0,  0},                           // 1 group
    {11, 19,  0,  0},                           // 2 groups
    { 7, 15, 23,  0},                           // 3 groups
    { 3, 11, 19, 27},                           // 4 groups
};

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

    Surface portraits;                 // the picture band, an overlay
    bool havePortraits = false;

    TextScreen &t() { return ui.ts(); }

    // Compose the monster-portrait band from 200.MONSTERS (once -- the groups
    // are fixed for the fight).  No-op when the art file was not supplied.
    void buildPortraits() {
        havePortraits = false;
        if (!sc.haveMonsterArt()) return;
        const int scrW = TextScreen::kCols * Font::kW;
        portraits.resize(scrW, kPortraitH);
        portraits.fill(0);
        int n = bt.nGroups < 1 ? 1 : bt.nGroups > 4 ? 4 : bt.nGroups;
        for (int g = 0; g < n; ++g) {
            int pic = bt.grp[g].rec(sc).pic();
            Bytes rec = sc.monsterArtRecord(pic);
            if (rec.n < 512) continue;
            blitPortrait(portraits, rec, kPortStartCell[n - 1][g] * Font::kW, 0, 10);
            havePortraits = true;
            std::printf("PORTRAIT| %c) pic %d\n", char('A' + g), pic);
        }
    }

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
        t.frame(0, kEncY, 40, kEncH);
        t.frame(0, kPtyY, 40, kPtyH);
        t.frame(0, kMsgY, 40, kMsgH);
        t.writeAt(15, kEncY, " ENCOUNTER ");

        // Group roster (encounter window, rows 1..).  With the picture band
        // up the names are clipped to the space left of the first portrait
        // (as the DOS screen does -- the full name still scrolls the log).
        int nameCap = 37;
        if (havePortraits) {
            int n = bt.nGroups < 1 ? 1 : bt.nGroups > 4 ? 4 : bt.nGroups;
            nameCap = kPortStartCell[n - 1][0] - 1;
        }
        for (int g = 0; g < bt.nGroups && g < 4; ++g) {
            char b[48];
            std::snprintf(b, sizeof b, "%c) %s", 'A' + g,
                          groupName(sc, sp, bt.grp[g]).c_str());
            t.writeAt(2, kEncY + 1 + g, std::string(b).substr(0, nameCap));
        }

        t.writeAt(2, kPtyY + 1, "# NAME           CLASS HP     STATUS");
        for (int i = 0; i < party.count(); ++i) {
            const Character &c = party.member(i);
            char b[64];
            std::snprintf(b, sizeof b, "%d %-14s %-5.3s %3d/%-3d %s",
                          i + 1, c.name.c_str(), className(sc, c.cls),
                          c.hpLeft, c.hpMax, statusName(sc, c.status));
            t.writeAt(2, kPtyY + 2 + i, std::string(b).substr(0, 36));
        }

        int row = 0;
        for (const auto &l : log) t.writeAt(2, kMsgY + 1 + row++, l.substr(0, 36));

        ui.setOverlay(havePortraits ? &portraits : nullptr, 0, kPortRow * Font::kH);
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

// U)SE an item in combat (CUTIL USEITEM P010604).  A packed item is useable
// when its SPELLPWR names a spell and it is either a SPECIAL or equipped.
// The item invokes that spell for free (no pool cost), targeted per the
// spell's own targeting, then rolls CHGCHANC to transform to CHANGETO.
bool doUseItem(CombatCtx &c, Character &ch, bool &quit) {
    quit = false;
    auto &t = c.t();
    int list[8], n = 0;
    for (int i = 0; i < ch.possCount && n < 8; ++i) {
        ObjectRec o{c.sc.record(Scenario::Object, ch.poss[i].itemIndex)};
        if (o.spellPwr() > 0 &&
            (o.type() == ObjType::Special || ch.poss[i].equipped))
            list[n++] = i;
    }
    if (n == 0) { c.say(ch.name + " HAS NOTHING TO USE"); return false; }

    auto itemName = [&](int slot) -> std::string {
        const Possession &p = ch.poss[slot];
        if (c.sp) {
            bool ok = false;
            std::string s = c.sp->get(
                StringPool::objectNameKey(p.itemIndex, p.identified ? 1 : 0), &ok);
            if (ok && !s.empty()) return s;
        }
        return "ITEM #" + std::to_string(p.itemIndex);
    };

    t.clearRect(0, 22, 40, 2);
    t.gotoXY(0, 22);
    std::string line = "USE: ";
    for (int i = 0; i < n; ++i) {
        line += char('1' + i); line += ')'; line += itemName(list[i]); line += ' ';
    }
    t.write(line.substr(0, 39));
    int k = c.ui.getKey();
    if (c.ui.quit()) { quit = true; return false; }
    int pick = k - '1';
    if (pick < 0 || pick >= n) return false;

    Possession &p = ch.poss[list[pick]];
    ObjectRec o{c.sc.record(Scenario::Object, p.itemIndex)};
    int sp = o.spellPwr();
    const SpellDef *d = spellDef(sp);
    if (!d) { c.say("NOTHING HAPPENS"); return true; }

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
    } else {
        for (int i = 0; i < c.party.count(); ++i)
            if (&c.party.member(i) == &ch) tgAlly = i;
    }

    CombatLog l;
    l.push_back(ch.name + " USES " + itemName(list[pick]) + "!");
    castSpell(c.bt, c.sc, c.party, false, ch.charLevel, sp, tg, -1, tgAlly, c.rng, l);
    if (c.rng.mod(100) < o.chgChance()) {         // CHGITEM
        p.itemIndex = o.changeTo();
        p.identified = false;
    }
    c.say(l);
    return true;
}

// A monster's turn -- CUTIL's action priority: spell > breath > yell for
// help > flee > melee.
void monsterTurn(CombatCtx &c, int g, int m) {
    MonsterRec r = c.bt.grp[g].rec(c.sc);
    CombatLog l;

    int ml = std::max(r.magSpels(), r.priSpels());
    if (ml > 0 && c.rng.mod(3) == 0) {              // ENEMYSPL
        int sp = r.priSpels() > 0
                     ? (ml >= 4 ? 41 /*LITOKAN*/ : 24 /*BADIOS*/)
                     : (ml >= 3 ? 7 /*MAHALITO*/ : ml >= 1 ? 3 /*KATINO*/ : 1);
        l.push_back("A MONSTER CASTS A SPELL!");
        castSpell(c.bt, c.sc, c.party, true, r.hpDice(), sp, 0, -1, -1, c.rng, l);
        c.say(l);
        return;
    }
    if (r.breathe() > 0 && c.rng.mod(100) < 60) {   // BREATHES
        monsterBreath(c.bt, c.sc, c.party, g, m, c.rng, l);
        c.say(l);
        return;
    }
    if (((r.sppc() >> 6) & 1) && c.bt.grp[g].alive < 5 && c.rng.mod(100) < 75) {  // YELLHELP
        monsterYell(c.bt, c.sc, g, c.rng, l);
        c.say(l);
        return;
    }
    int partyStanding = 0;
    for (int i = 0; i < c.party.count(); ++i)
        if (int(c.party.member(i).status) < int(Status::Dead)) ++partyStanding;
    if (((r.sppc() >> 5) & 1) && partyStanding > c.bt.grp[g].alive &&
        c.rng.mod(100) < 65) {                      // RUNENMY
        monsterFlee(c.bt, g, m, l);
        c.say(l);
        return;
    }
    monsterAttack(c.bt, c.sc, c.party, g, m, c.rng, l);
    c.say(l);
}

// One round: each conscious member acts, then every living monster attacks.
// Returns a CombatResult once the fight is decided, else CombatResult::Won as
// a sentinel meaning "continue" (checked by the caller loop).  `partyActs` /
// `monstersAct` are false on the first round of a surprise.
bool round(CombatCtx &c, CombatResult &out, bool partyActs = true,
           bool monstersAct = true) {
    auto &t = c.t();

    for (int i = 0; partyActs && i < c.party.count(); ++i) {
        Character &ch = c.party.member(i);
        if (ch.status != Status::OK && ch.status != Status::Afraid) continue;
        if (allMonstersDead(c.bt)) break;

        for (;;) {
            c.draw();
            t.resetWindow();
            t.clearRect(0, 22, 40, 2);
            t.gotoXY(0, 22);
            bool dispel = canDispel(ch);
            t.write(ch.name + ": F)IGHT C)AST P)ARRY R)UN U)SE" +
                    std::string(dispel ? " D)ISPEL" : ""));
            int k = c.ui.getKey();
            if (c.ui.quit()) { out = CombatResult::WindowClosed; return true; }
            if (k == 'P') break;
            if (k == 'U') {
                bool q = false;
                bool used = doUseItem(c, ch, q);
                if (c.bt.recalled) { c.pause(); out = CombatResult::Recalled; return true; }
                if (used) break;
                if (q) { out = CombatResult::WindowClosed; return true; }
                continue;
            }
            if (k == 'D' && dispel) {
                bool q = false;
                int g = pickGroup(c, q);            // WHICHGRP 'DISPELL WHICH GROUP# ?'
                if (q) { out = CombatResult::WindowClosed; return true; }
                if (g < 0) continue;
                CombatLog l;
                partyDispel(c.bt, c.sc, ch, g, c.rng, l);
                c.say(l);
                break;
            }
            if (k == 'C') {
                bool q = false;
                bool cast = doCast(c, ch, q);
                if (c.bt.recalled) { c.pause(); out = CombatResult::Recalled; return true; }
                if (cast) break;
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
    for (int g = 0; monstersAct && g < c.bt.nGroups; ++g)
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
    c.buildPortraits();                 // the 200.MONSTERS picture band

    // INITATTK's surprise roll: 1 = the party surprised the monsters (a free
    // party round), 2 = the monsters surprised the party (a free monster
    // round), 0 = neither.
    c.bt.surprise = (rng.mod(100) > 80) ? 1 : (rng.mod(100) > 80 ? 2 : 0);

    if (friendlyParley(c)) return CombatResult::Friendly;
    if (c.bt.nGroups > 1) {
        CombatLog l;
        l.push_back(std::to_string(c.bt.nGroups) + " GROUPS OF MONSTERS!");
        for (int g = 0; g < c.bt.nGroups; ++g)
            l.push_back(std::string(1, char('A' + g)) + ") " + groupName(sc, sp, c.bt.grp[g]));
        c.say(l);
    } else {
        c.say("A GROUP OF MONSTERS BLOCKS YOUR WAY!");
    }
    if (c.bt.surprise == 1)      c.say("YOU SURPRISED THE MONSTERS!");
    else if (c.bt.surprise == 2) c.say("THE MONSTERS SURPRISED YOU!");
    c.pause();

    for (int guard = 0; guard < 200; ++guard) {
        CombatResult out;
        bool first = guard == 0;
        if (round(c, out, !(first && c.bt.surprise == 2),
                          !(first && c.bt.surprise == 1))) {
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
