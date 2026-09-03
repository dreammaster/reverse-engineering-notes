#include "wiz/maze_ui.h"
#include "wiz/maze.h"
#include "wiz/maze3d.h"
#include "wiz/scenario.h"
#include "wiz/string_pool.h"
#include "wiz/specials.h"
#include "wiz/equip.h"
#include "wiz/camp_ui.h"
#include "wiz/combat_ui.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace wiz {
namespace {

// --- HUD geometry (640x192 text surface, 16x8 font) --------------------
constexpr int kPicX = 6, kPicY = 2, kPicScale = 2, kPicClipH = 134;

const char *alignName(const Scenario &sc, Align a) {
    return int(a) < int(sc.aligns().size()) ? sc.aligns()[int(a)].c_str() : "?";
}
const char *className(const Scenario &sc, Class c) {
    return int(c) < int(sc.classes().size()) ? sc.classes()[int(c)].c_str() : "?";
}
const char *statusName(const Scenario &sc, Status s) {
    return int(s) < int(sc.statuses().size()) ? sc.statuses()[int(s)].c_str() : "?";
}

struct MazeCtx {
    Ui &ui;
    Party &party;
    const Scenario &sc;
    const StringPool *sp;
    Rng &rng;
    MazeState &st;
    MazeLevel m;
    FightMap fm;
    Surface pic{kPicW, kPicH};
    Surface pane{kPicW * kPicScale, std::min(kPicH * kPicScale, kPicClipH)};
    bool needDraw = true;

    TextScreen &t() { return ui.ts(); }

    void present() {
        // wireframe into the pic buffer (a redraw must not spend LIGHT)
        int light = st.light;
        drawMazeView(pic, m, st.pos, st.level, light, st.quickPlot, rng);
        for (int y = 0; y < pane.height(); ++y)
            for (int x = 0; x < pane.width(); ++x)
                pane.set(x, y, pic.get(x / kPicScale, y / kPicScale));
        ui.setOverlay(&pane, kPicX, kPicY);
        ui.refresh();
    }
};

// ---- PRSTATS (P010E0B): the party panel, rows 18..23 -------------------

void prStats(MazeCtx &c) {
    auto &t = c.t();
    t.resetWindow();
    t.clearRect(0, 18, 40, 6);
    // sort a view of the party by status (alive first) -- indices only
    int order[Party::kMax];
    for (int i = 0; i < c.party.count(); ++i) order[i] = i;
    for (int i = 0; i < c.party.count() - 1; ++i)
        for (int j = i + 1; j < c.party.count(); ++j)
            if (int(c.party.member(order[i]).status) > int(c.party.member(order[j]).status))
                std::swap(order[i], order[j]);

    bool anyAlive = false;
    for (int row = 0; row < c.party.count(); ++row) {
        Character &ch = c.party.member(order[row]);
        int y = 18 + row;
        char b[48];
        t.gotoXY(0, y);
        std::snprintf(b, sizeof b, "%d %s", row + 1, ch.name.c_str());
        t.write(b);

        t.gotoXY(15, y);
        std::snprintf(b, sizeof b, "%c-%.3s", alignName(c.sc, ch.align)[0],
                      className(c.sc, ch.cls));
        t.write(b);

        int ac = ch.armorClass - c.st.protect;
        t.gotoXY(21, y);
        if (ac >= 0)            std::snprintf(b, sizeof b, "%3d", ac);
        else if (ac > -10)      std::snprintf(b, sizeof b, " -%d", -ac);
        else                    std::snprintf(b, sizeof b, " LO");
        t.write(b);

        if (int(ch.status) >= int(Status::Dead)) ch.hpLeft = 0;
        t.gotoXY(25, y);
        std::snprintf(b, sizeof b, "%4d ", ch.hpLeft);
        t.write(b);

        t.gotoXY(31, y);
        if (ch.status == Status::OK) {
            anyAlive = true;
            std::snprintf(b, sizeof b, "%4d", ch.hpMax);
            t.write(b);
        } else {
            t.write(statusName(c.sc, ch.status));
        }
    }
    (void)anyAlive;
}

void msgClear(MazeCtx &c) {
    c.t().resetWindow();
    c.t().clearRect(0, 15, 40, 3);
}
void msg(MazeCtx &c, const std::string &s) {
    msgClear(c);
    c.t().gotoXY(1, 17);
    c.t().write(s.substr(0, 38));
}

// Hand off to combat.  Returns true (with `out` set) if the maze session
// ends -- the window closed or the party was wiped.
bool runFight(MazeCtx &c, int enemyInx, MazeExit &out, int attk012 = 2) {
    CombatResult r = runCombat(c.ui, c.party, c.sc, c.sp, c.rng, enemyInx,
                               c.st.level, attk012);
    c.needDraw = true;
    if (r == CombatResult::WindowClosed) { out = MazeExit::WindowClosed; return true; }
    if (r == CombatResult::PartyWiped)   { out = MazeExit::PartyWiped;   return true; }
    return false;
}

// ---- RUNINIT (P010E25): the fixed HUD frame ---------------------------

void runInit(MazeCtx &c) {
    auto &t = c.t();
    t.resetWindow();
    t.putChar(12);
    t.gotoXY(13, 1); t.write("F)ORWARD  C)AMP    S)TATUS");
    t.gotoXY(13, 2); t.write("L)EFT     Q)UICK   A<-W->D");
    t.gotoXY(13, 3); t.write("R)IGHT    T)IME");
    t.gotoXY(13, 4); t.write("K)ICK     I)NSPECT");
    t.gotoXY(13, 7); t.write("SPELLS :");
    prStats(c);
}

// ---- UPDATEHP (P010E1C): per-move poison / regen ---------------------

// Once per actual move, each conscious member has a 1-in-4 chance to take
// POISNAMT damage and HEALPTS regen (net); poison can kill.
void updateHp(MazeCtx &c) {
    for (int i = 0; i < c.party.count(); ++i) {
        Character &ch = c.party.member(i);
        if (int(ch.status) >= int(Status::Dead)) continue;
        if (c.rng.mod(4) != 2) continue;
        ch.hpLeft += ch.healPts - ch.poison;
        if (ch.hpLeft <= 0) {
            ch.poison = 0;
            ch.hpLeft = 0;
            if (int(ch.status) < int(Status::Dead)) {
                ch.status = Status::Dead;
                msg(c, ch.name + " DIED");
                prStats(c);
                c.present();
                c.ui.delayMs(400);
            }
        } else if (ch.hpLeft > ch.hpMax) {
            ch.hpLeft = ch.hpMax;
        }
    }
}

// PRSTATS' ANYALIVE test: the maze ends at the cemetery when nobody is OK.
bool anyConscious(const Party &p) {
    for (int i = 0; i < p.count(); ++i)
        if (p.member(i).status == Status::OK) return true;
    return false;
}

// ---- ROCKWATR (P010E16): pit / ouch damage ---------------------------

void rockDamage(MazeCtx &c, int sq) {
    auto &m = c.m;
    int base = m.aux0(sq), dice = m.aux2(sq), sides = std::max(1, m.aux1(sq));
    for (int i = 0; i < c.party.count(); ++i) {
        Character &ch = c.party.member(i);
        if (int(ch.status) >= int(Status::Dead)) continue;
        if (c.rng.mod(25) + c.st.level <= ch.attrib[AGI]) continue;      // dodged
        int dmg = base;
        for (int d = 0; d < dice; ++d) dmg += c.rng.mod(sides) + 1;
        ch.hpLeft -= dmg;
        if (ch.hpLeft < 0) {
            ch.hpLeft = 0;
            ch.status = Status::Dead;
            msg(c, ch.name + " DIED");
        }
    }
    prStats(c);
}

// ---- QUIETXFR / stairs / teleport ------------------------------------

// Returns true if the transfer left the maze (stairs to level 0).
bool quietXfr(MazeCtx &c, int sq, MazeExit &out) {
    int tgt = c.m.aux0(sq);
    c.st.pos.x = wrap20(c.m.aux2(sq));
    c.st.pos.y = wrap20(c.m.aux1(sq));
    if (tgt != c.st.level) {
        if (tgt <= 0) { out = MazeExit::ToTown; return true; }   // NEWMAZE: level 0 -> castle
        c.st.level = tgt;
        c.m.load(c.sc.record(Scenario::Maze, c.st.level - 1));
        c.fm.build(c.m, c.rng);
        c.fm.clearRoom(c.m, c.st.pos.x, c.st.pos.y);
    }
    c.needDraw = true;
    return false;
}

// ---- SCNMSG (SPECIALS SPCMISC / DOMSG) ------------------------------

// DOMSG: page a scripted message over the text area.  Rows 3..14, 12 lines
// per page; '@'/'^' lines are centred in the 40-col grid.  The maze view is
// hidden for the duration; every page ends on a keypress.
void showScrollText(MazeCtx &c, const std::vector<ScnLine> &lines, bool /*pressRet*/) {
    auto &t = c.t();
    c.ui.setOverlay(nullptr);
    constexpr int kTop = 3, kRows = 12;
    int shown = 0;
    do {
        t.resetWindow();
        t.putChar(12);
        int n = std::min(kRows, int(lines.size()) - shown);
        for (int i = 0; i < n; ++i) {
            const ScnLine &ln = lines[shown + i];
            int col = ln.center ? std::max(0, (40 - int(ln.text.size())) / 2) : 1;
            t.gotoXY(col, kTop + i);
            t.write(ln.text.substr(0, 39));
            std::printf("SCNMSG| %s\n", ln.text.c_str());
        }
        shown += n;
        bool more = shown < int(lines.size());
        t.gotoXY(12, kTop + kRows + 2);
        t.write(more ? "[RET] FOR MORE" : "PRESS [RET]");
        c.ui.refresh();
        c.ui.pressAnyKey("");
        if (c.ui.quit()) return;
    } while (shown < int(lines.size()));
    runInit(c);                 // repaint the HUD frame the message covered
    c.needDraw = true;
}

// TRYGET: hand item `itemIdx` to the first member who can carry it.
void scnGiveItem(MazeCtx &c, int itemIdx) {
    int nObj = c.sc.count(Scenario::Object);
    if (itemIdx < 0 || itemIdx >= nObj) return;
    for (int i = 0; i < c.party.count(); ++i) {
        Character &ch = c.party.member(i);
        if (ch.possCount >= 8) continue;
        bool dup = false;
        for (int k = 0; k < ch.possCount; ++k) dup = dup || ch.poss[k].itemIndex == itemIdx;
        if (dup) continue;
        ch.poss[ch.possCount++] = Possession{false, false, false, itemIdx};
        std::string nm;
        bool ok = false;
        if (c.sp) nm = c.sp->get(StringPool::objectNameKey(itemIdx, 0), &ok);
        msg(c, ch.name + " GOT " + (ok ? nm : "AN ITEM"));
        return;
    }
}

// SPCMISC for a ScnMsg descriptor.  Returns true (out set) only if the maze
// session ends (it never does here, but keep the specSquare contract).
bool runScnMsg(MazeCtx &c, int sq, MazeExit &out) {
    (void)out;
    int kind  = c.m.aux2(sq);
    int msgNo = c.m.aux1(sq);
    int aux0  = c.m.aux0(sq);
    if (kind == SCN_NONE) return false;

    // SPCMISC's AUX0 gate (one-shot / countdown / persistent), session-tracked
    // since the engine holds SCENARIO.DATA read-only.
    int fkey = c.st.level * 100 + sq;
    int fired = c.st.scnMsgFired.count(fkey) ? c.st.scnMsgFired[fkey] : 0;
    if (!scnMsgMayFire(kind, aux0, fired)) return false;
    if (scnMsgCounts(kind)) c.st.scnMsgFired[fkey] = fired + 1;

    if (c.sp) {
        std::vector<ScnLine> lines = scnMsgLines(*c.sp, msgNo);
        if (!lines.empty()) showScrollText(c, lines, true);
    }

    switch (kind) {
        case SCN_PLAIN:
            break;
        case SCN_GIVE:
            scnGiveItem(c, aux0);        // AUX0 = the item index (TRYGET)
            break;
        default:
            // WHOWADE / GETYN / bounce-to-shop / riddle / fee side effects
            // are not ported yet -- the message itself is shown.
            msg(c, "(SCNMSG action " + std::to_string(kind) + " not handled)");
            break;
    }
    return false;
}

// ---- SPECSQAR (P010E10) ---------------------------------------------

// Returns true (with `out` set) if the square ends the maze session.
bool specSquare(MazeCtx &c, bool initTurn, MazeExit &out) {
    int sq = c.m.squareExtra(c.st.pos.x, c.st.pos.y);
    Square ty = c.m.squareType(sq);
    c.t().resetWindow();
    c.t().clearRect(0, 15, 40, 3);

    switch (ty) {
        case Square::Stairs: {
            if (!initTurn) break;
            bool up = c.st.level > c.m.aux0(sq);
            msg(c, std::string("STAIRS GOING ") + (up ? "UP." : "DOWN.") + "  TAKE THEM (Y/N)?");
            c.present();
            if (c.ui.menu("YN") == 'Y') return quietXfr(c, sq, out);
            break;
        }
        case Square::Chute:
            msg(c, "A CHUTE!");
            c.present(); c.ui.delayMs(400);
            return quietXfr(c, sq, out);
        case Square::Teleport:
            return quietXfr(c, sq, out);
        case Square::TurnRandom:
            if (initTurn) { c.st.pos.dir = c.rng.mod(4); c.needDraw = true; }
            break;
        case Square::Darkness:
            msg(c, "IT'S VERY DARK HERE");
            c.st.light = 0;
            break;
        case Square::Damage:
            msg(c, "OUCH!");
            rockDamage(c, sq);
            break;
        case Square::Pit:
            if (!initTurn) break;
            msg(c, "A PIT!");
            rockDamage(c, sq);
            break;
        case Square::RockWater:
            msg(c, "YOU ARE SWEPT AWAY!");
            c.present(); c.ui.delayMs(500);
            c.st.level = 1;
            c.st.pos = MazePos{0, 0, NORTH};
            c.m.load(c.sc.record(Scenario::Maze, 0));
            c.fm.build(c.m, c.rng);
            c.fm.clearRoom(c.m, 0, 0);
            c.needDraw = true;
            break;
        case Square::Buttons: {
            int lo = c.m.aux2(sq), hi = c.m.aux1(sq);
            char prompt[48];
            std::snprintf(prompt, sizeof prompt, "BUTTONS A THROUGH %c. PRESS ONE (RET=NONE)",
                          char('A' + hi - lo));
            msg(c, prompt);
            c.present();
            int k = c.ui.getKey();
            if (k >= 'A' && k <= 'A' + hi - lo) {
                c.st.level = lo + (k - 'A');
                if (c.st.level <= 0) { out = MazeExit::ToTown; return true; }
                c.m.load(c.sc.record(Scenario::Maze, c.st.level - 1));
                c.fm.build(c.m, c.rng);
                c.fm.clearRoom(c.m, c.st.pos.x, c.st.pos.y);
                c.needDraw = true;
            }
            break;
        }
        case Square::ScnMsg:
            if (!initTurn) break;
            if (runScnMsg(c, sq, out)) return true;
            break;
        case Square::Encounter: {
            // CHENCOUN: a fixed fight, ENEMYINX = AUX2 + rand%AUX1
            int a1 = std::max(1, c.m.aux1(sq));
            int inx = c.m.aux2(sq) + c.rng.mod(a1);
            if (runFight(c, inx, out)) return true;
            c.fm.clearRoom(c.m, c.st.pos.x, c.st.pos.y);
            break;
        }
        default:
            break;
    }
    return false;
}

// RUNMAIN's random-encounter test (P010E0E).
bool encounterRoll(MazeCtx &c, bool initTurn, int lastKey) {
    if (c.rng.mod(99) == 35) return true;
    if (c.fm.at(c.st.pos.x, c.st.pos.y)) return true;
    if (initTurn && (lastKey == 'K') && c.m.fights(c.st.pos.x, c.st.pos.y) &&
        c.rng.mod(8) == 3)
        return true;
    return false;
}

// ENCOUNTR: roll a random monster index from the level's ENMYCALC table.
int rollEnemyInx(MazeCtx &c) {
    int encType = 1;
    while (c.rng.mod(4) == 2 && encType < 3) ++encType;
    EnemyCalc e = c.m.enemyCalc(encType);
    int encCalc = 0;
    while (c.rng.mod(100) < e.percWorse && encCalc < e.worse01) ++encCalc;
    return e.minEnemy + c.rng.mod(std::max(1, e.range0n)) + e.multWorse * encCalc;
}

} // namespace

MazeExit runMaze(Ui &ui, Party &party, const Scenario &sc, const StringPool *sp,
                 Rng &rng, MazeState &st) {
    MazeCtx c{ui, party, sc, sp, rng, st};
    struct ClearOverlay { Ui &u; ~ClearOverlay() { u.setOverlay(nullptr); } } _co{ui};
    for (int i = 0; i < party.count(); ++i) equipRecalc(party.member(i), sc);
    if (st.level < 1) st.level = 1;
    if (!c.m.load(sc.record(Scenario::Maze, st.level - 1))) return MazeExit::ToTown;
    c.fm.build(c.m, rng);
    c.fm.clearRoom(c.m, st.pos.x, st.pos.y);

    runInit(c);
    bool initTurn = true;
    int lastKey = 0;

    for (;;) {
        auto &t = c.t();
        t.resetWindow();
        char hdr[40];
        std::snprintf(hdr, sizeof hdr, "LEVEL %d   %2d,%-2d  %-5s", st.level,
                      st.pos.x, st.pos.y, dirName(st.pos.dir));
        t.gotoXY(14, 0); t.write(hdr);
        t.gotoXY(22, 7); t.write(st.light > 0 ? "LIGHT  " : "       ");
        t.gotoXY(22, 8); t.write(st.protect > 0 ? "PROTECT" : "       ");

        Square here = c.m.squareAt(st.pos.x, st.pos.y);
        if (here != Square::Normal && initTurn) {
            MazeExit out;
            if (specSquare(c, initTurn, out)) return out;
        }

        if (initTurn && encounterRoll(c, initTurn, lastKey)) {
            MazeExit out;
            // ENCOUNTR's ATTK012: a wandering monster (0), a set-piece fight
            // room on first entry (1 -> double gold), or a re-fought room (2).
            int a012 = !c.m.fights(st.pos.x, st.pos.y) ? 0
                     : (c.fm.at(st.pos.x, st.pos.y) ? 1 : 2);
            if (runFight(c, rollEnemyInx(c), out, a012)) return out;
            c.fm.clearRoom(c.m, st.pos.x, st.pos.y);
        }

        if (initTurn) updateHp(c);
        if (!anyConscious(party)) return MazeExit::PartyWiped;

        prStats(c);
        c.present();
        initTurn = false;

        int k = c.ui.getKey();
        if (c.ui.quit()) return MazeExit::WindowClosed;
        lastKey = k;

        if (k == KEY_ESC || k == KEY_RETURN) return MazeExit::ToTown;
        if (k == 'F' || k == 'W' || k == KEY_UP) {
            if (canWalk(c.m, st.pos)) { stepForward(st.pos); initTurn = true; c.needDraw = true; msgClear(c); }
            else { msg(c, "OUCH!  A WALL."); }
        } else if (k == 'K') {
            if (canKick(c.m, st.pos)) { stepForward(st.pos); initTurn = true; c.needDraw = true; msgClear(c); }
            else { msg(c, "OUCH!  A WALL."); }
        } else if (k == 'L' || k == 'A' || k == KEY_LEFT) { turn(st.pos, 3); c.needDraw = true; msgClear(c); }
        else if (k == 'R' || k == 'D' || k == KEY_RIGHT)  { turn(st.pos, 1); c.needDraw = true; msgClear(c); }
        else if (k == 'S') { prStats(c); }
        else if (k == 'Q') {
            st.quickPlot = !st.quickPlot;
            msg(c, std::string("QUICK PLOT ") + (st.quickPlot ? "ON" : "OFF"));
            c.needDraw = true;
        } else if (k == 'C') {
            switch (runCamp(c.ui, c.party, c.sc, c.sp, c.rng)) {
                case CampExit::WindowClosed: return MazeExit::WindowClosed;
                case CampExit::Disbanded:    return MazeExit::ToTown;
                case CampExit::ToMaze:       runInit(c); c.needDraw = true; break;
            }
        } else if (k == 'I') {
            return MazeExit::ToTown;                 // SPECIALS.INSPECT not ported
        }
    }
}

} // namespace wiz
