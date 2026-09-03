#include "wiz/maze_ui.h"
#include "wiz/maze.h"
#include "wiz/maze3d.h"
#include "wiz/roster.h"
#include "wiz/scenario.h"
#include "wiz/string_pool.h"
#include "wiz/specials.h"
#include "wiz/equip.h"
#include "wiz/camp_ui.h"
#include "wiz/combat_ui.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace wiz {

// ---- MazeState save / restore (the interrupted-delve session) --------

namespace {
constexpr char kMazeMagic[4] = {'W', 'Z', 'M', '2'};
void put32(std::FILE *f, int32_t v) { std::fwrite(&v, 4, 1, f); }
bool get32(std::FILE *f, int32_t &v) { return std::fread(&v, 4, 1, f) == 1; }
}

bool MazeState::save(const std::string &path) const {
    std::FILE *f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    std::fwrite(kMazeMagic, 1, 4, f);
    put32(f, active ? 1 : 0);
    put32(f, level);
    put32(f, pos.x); put32(f, pos.y); put32(f, pos.dir);
    put32(f, light); put32(f, protect); put32(f, quickPlot ? 1 : 0);
    put32(f, timeDelay);
    put32(f, int32_t(scnMsgFired.size()));
    for (const auto &kv : scnMsgFired) { put32(f, kv.first); put32(f, kv.second); }
    std::fclose(f);
    return true;
}

bool MazeState::load(const std::string &path) {
    std::FILE *f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    char m[4];
    int32_t v, n = 0;
    bool ok = std::fread(m, 1, 4, f) == 4 && std::memcmp(m, kMazeMagic, 4) == 0;
    auto rd = [&](int &dst) { ok = ok && get32(f, v); if (ok) dst = v; };
    int a = 0, qp = 0;
    rd(a); rd(level); rd(pos.x); rd(pos.y); rd(pos.dir);
    rd(light); rd(protect); rd(qp); rd(timeDelay);
    if (ok && get32(f, n)) {
        scnMsgFired.clear();
        for (int i = 0; i < n && ok; ++i) {
            int32_t k = 0, c = 0;
            ok = get32(f, k) && get32(f, c);
            if (ok) scnMsgFired[k] = c;
        }
    }
    std::fclose(f);
    if (!ok) return false;
    active = a != 0;
    quickPlot = qp != 0;
    return true;
}
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
    Roster &roster;
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

    // PAUSE1: the SETTIME-tunable message delay.  DOS TIMEDLAY is a busy-loop
    // count (default 2000); scale it to ~1/5 ms per unit so the default
    // matches the engine's old hard-coded ~400 ms feel.
    void pause1() { ui.delayMs(std::max(1, st.timeDelay / 5)); }

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

// ---- SETTIME (P010E22): tune the message delay ----------------------

void runSetTime(MazeCtx &c) {
    auto &t = c.t();
    t.resetWindow();
    t.clearRect(0, 15, 40, 3);
    t.gotoXY(1, 16); t.write("NEW DELAY (1-5000) >");
    std::string s = c.ui.getLine(4);
    t.clearRect(0, 15, 40, 3);
    if (c.ui.quit() || s.empty()) return;
    int v = 0;
    for (char ch : s) {
        if (ch < '0' || ch > '9') return;             // non-digit -> EXITTIME
        v = 10 * v + (ch - '0');
    }
    if (v >= 1 && v <= 5000) {
        c.st.timeDelay = v;
        msg(c, "DELAY SET TO " + std::to_string(v));
    }
    c.needDraw = true;
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
                c.pause1();
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

// BOUNCEBK: shove the party back one square (opposite their facing) and show
// the descriptor's message.
void bounceBack(MazeCtx &c, int msgNo) {
    static const int dx[4] = {0, -1, 0, 1};      // N,E,S,W -> step backward
    static const int dy[4] = {-1, 0, 1, 0};
    c.st.pos.x = wrap20(c.st.pos.x + dx[c.st.pos.dir]);
    c.st.pos.y = wrap20(c.st.pos.y + dy[c.st.pos.dir]);
    c.needDraw = true;
    if (c.sp) {
        std::vector<ScnLine> lines = scnMsgLines(*c.sp, msgNo);
        if (!lines.empty()) showScrollText(c, lines, true);
    }
    runInit(c);
}

// SPCMISC for a ScnMsg descriptor.  Returns true (out set) if the maze
// session ends (a GETYN "search" that leads into a fight).
bool runScnMsg(MazeCtx &c, int sq, MazeExit &out) {
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

    // SPCMISC's fixup: a persistently-encoded AUX0 (<= -1000) carries its real
    // payload at AUX0 + 1000.
    int eff = (scnMsgCounts(kind) && aux0 <= -1000) ? aux0 + 1000 : aux0;

    // ITM2PASS: no message unless the party lacks item `eff` and is bounced.
    if (kind == SCN_NEEDITEM) {
        bool have = false;
        for (int i = 0; i < c.party.count() && !have; ++i)
            for (int k = 0; k < c.party.member(i).possCount; ++k)
                if (c.party.member(i).poss[k].itemIndex == eff) have = true;
        if (!have) bounceBack(c, msgNo);
        return false;
    }

    if (c.sp) {
        std::vector<ScnLine> lines = scnMsgLines(*c.sp, msgNo);
        if (!lines.empty()) showScrollText(c, lines, true);
    }

    switch (kind) {
        case SCN_PLAIN:
            break;
        case SCN_GIVE:
            scnGiveItem(c, eff);                 // AUX0 = the item index (TRYGET)
            break;
        case SCN_YESNO: {                        // GETYN
            auto &t = c.t();
            t.resetWindow();
            t.gotoXY(0, 22); t.write("SEARCH (Y/N) ?");
            c.ui.refresh();
            if (c.ui.menu("YN") == 'Y') {
                if (eff > 0) {                   // AUX0 -> ENEMYINX, into combat
                    if (runFight(c, eff, out, 0)) return true;
                } else {
                    scnGiveItem(c, eff < 0 ? -eff : eff);
                }
            }
            runInit(c);
            c.needDraw = true;
            break;
        }
        default:
            // bounce-to-shop / lookout / riddle / fee -- none appear in WIZ1.
            msg(c, "(SCNMSG action " + std::to_string(kind) + " not handled)");
            break;
    }
    return false;
}

// ---- SPECIALS INSPECT: EXPLROOM + LOOKLOST + PICKUP -----------------

// EXPLROOM: flood-fill from (x0,y0) through OPEN edges only (walls and doors
// stop it) -- the connected open area = "the room I'm in".
void exploreRoom(const MazeLevel &m, int x0, int y0, bool room[20][20]) {
    for (int x = 0; x < 20; ++x) for (int y = 0; y < 20; ++y) room[x][y] = false;
    room[x0][y0] = true;
    bool changed = true;
    while (changed) {
        changed = false;
        for (int x = 0; x < 20; ++x)
            for (int y = 0; y < 20; ++y) {
                if (!room[x][y]) continue;
                const int dx[4] = {0, 1, 0, -1}, dy[4] = {1, 0, -1, 0};
                for (int d = 0; d < 4; ++d) {
                    if (m.wall(x, y, d) != Wall::Open) continue;
                    int nx = wrap20(x + dx[d]), ny = wrap20(y + dy[d]);
                    if (!room[nx][ny]) { room[nx][ny] = true; changed = true; }
                }
            }
    }
}

// The maze `I` command: look around the room for characters left in the
// dungeon (a death or a camp DISBAND) and offer to carry them out.
void runInspect(MazeCtx &c) {
    auto &t = c.t();
    c.ui.setOverlay(nullptr);

    bool room[20][20];
    exploreRoom(c.m, c.st.pos.x, c.st.pos.y, room);

    int list[6], n = 0;                              // LOOKLOST
    for (int i = 0; i < c.roster.count() && n < 5; ++i) {
        const Character &r = c.roster.slot(i);
        if (r.inMaze || r.lostLevel != c.st.level) continue;
        if (r.lostX < 0 || r.lostX >= 20 || r.lostY < 0 || r.lostY >= 20) continue;
        if (!room[r.lostX][r.lostY]) continue;
        list[n++] = i;
    }

    for (;;) {
        t.resetWindow();
        t.putChar(12);
        t.gotoXY(0, 0); t.write("LOOKING...");
        t.gotoXY(0, 2); t.write("FOUND:");
        for (int j = 0; j < n; ++j) {
            char b[40];
            std::snprintf(b, sizeof b, "%d) %s", j + 1, c.roster.slot(list[j]).name.c_str());
            t.gotoXY(2, 3 + j); t.write(b);
        }
        if (n == 0) { t.gotoXY(2, 3); t.write("** NO ONE **"); }
        t.gotoXY(0, 20);
        t.write(n > 0 ? "OPTIONS: P)ICK UP  L)EAVE" : "OPTIONS: L)EAVE");
        c.ui.refresh();

        int k = c.ui.getKey();
        if (c.ui.quit() || k == 'L') break;
        if (k != 'P' || n == 0) continue;

        if (c.party.full()) { c.ui.pressAnyKey("YOU HAVE 6 - PRESS [RET]"); continue; }
        t.gotoXY(0, 20); t.putChar(11);
        t.write("GET WHO (0=EXIT) >");
        int w = c.ui.getKey();
        if (c.ui.quit()) break;
        int pick = w - '1';
        if (w == '0' || pick < 0 || pick >= n) continue;

        int slot = list[pick];
        c.party.add(c.roster, slot);                 // INMAZE := TRUE, joins the party
        Character &pm = c.party.member(c.party.count() - 1);
        pm.lostX = pm.lostY = pm.lostLevel = 0;
        Character &rs = c.roster.slot(slot);
        rs.lostX = rs.lostY = rs.lostLevel = 0;
        std::printf("PICKUP| recovered %s\n", rs.name.c_str());
        for (int j = pick; j + 1 < n; ++j) list[j] = list[j + 1];
        --n;
    }
    runInit(c);
    c.needDraw = true;
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
            c.present(); c.pause1();
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
            c.present(); c.pause1();
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

static MazeExit runMazeImpl(Ui &ui, Party &party, Roster &roster,
                            const Scenario &sc, const StringPool *sp, Rng &rng,
                            MazeState &st) {
    MazeCtx c{ui, party, roster, sc, sp, rng, st};
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
        else if (k == 'T') { runSetTime(c); }
        else if (k == 'Q') {
            st.quickPlot = !st.quickPlot;
            msg(c, std::string("QUICK PLOT ") + (st.quickPlot ? "ON" : "OFF"));
            c.needDraw = true;
        } else if (k == 'C') {
            switch (runCamp(c.ui, c.party, c.roster, c.sc, c.sp, c.rng, st)) {
                case CampExit::WindowClosed: return MazeExit::WindowClosed;
                case CampExit::Disbanded:    return MazeExit::ToTown;
                case CampExit::ToMaze:       runInit(c); c.needDraw = true; break;
            }
        } else if (k == 'I') {
            runInspect(c);                           // SPECIALS INSPECT / PICKUP
        }
    }
}

MazeExit runMaze(Ui &ui, Party &party, Roster &roster, const Scenario &sc,
                 const StringPool *sp, Rng &rng, MazeState &st) {
    st.active = true;
    MazeExit e = runMazeImpl(ui, party, roster, sc, sp, rng, st);
    // Only an interrupted delve (the window closed) is worth resuming; a
    // return to town via the stairs / camp / Esc ends the delve.
    st.active = (e == MazeExit::WindowClosed);
    return e;
}

} // namespace wiz
