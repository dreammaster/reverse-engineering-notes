#include "wiz/roller_ui.h"
#include "wiz/roller.h"
#include "wiz/scenario.h"
#include "wiz/bitmap.h"

#include <array>
#include <cctype>
#include <cstdio>
#include <string>

namespace wiz {

// ---- Ui ---------------------------------------------------------------------

void Ui::refresh() {
    ts_.render(surf_, font_);
    p_.present(surf_, kDefaultPalette, 16);
}

int Ui::getKey() {
    refresh();
    int k = p_.waitKey();
    if (k == KEY_QUIT) { quit_ = true; return KEY_QUIT; }
    if (k >= 'a' && k <= 'z') k -= 32;
    return k;
}

std::string Ui::getLine(int maxLen) {
    std::string s;
    int col = ts_.cursorX(), row = ts_.cursorY();
    for (;;) {
        int k = getKey();
        if (quit_) return s;
        if (k == KEY_RETURN) break;
        if (k == KEY_BACKSPACE) {
            if (!s.empty()) { s.pop_back(); ts_.gotoXY(col + int(s.size()), row); ts_.write(" "); }
        } else if (k >= 32 && k < 127 && int(s.size()) < maxLen) {
            s.push_back(char(k));
        }
        ts_.gotoXY(col, row);
        ts_.write(s);
        ts_.write(" ");
    }
    return s;
}

std::string Ui::getPass(int maxLen, Rng &rng) {
    std::string s;
    for (;;) {
        int k = getKey();
        if (quit_) return s;
        if (k == KEY_RETURN) break;
        if (int(s.size()) < maxLen && k >= 32 && k < 127) {
            s.push_back(char(k));
            for (int i = 0; i <= rng.mod(2); ++i) ts_.write("X");
        }
    }
    return s;
}

int Ui::menu(const char *valid) {
    for (;;) {
        int k = getKey();
        if (quit_) return KEY_QUIT;
        for (const char *v = valid; *v; ++v)
            if (std::toupper((unsigned char)*v) == k) return k;
    }
}

void Ui::pressAnyKey(const char *msg) {
    ts_.write(msg);
    getKey();
}

// ---- ROLLER ---------------------------------------------------------------

namespace {

const char *raceName(const Scenario &sc, Race r) {
    return int(r) < int(sc.races().size()) ? sc.races()[int(r)].c_str() : "?";
}
const char *className(const Scenario &sc, Class c) {
    return int(c) < int(sc.classes().size()) ? sc.classes()[int(c)].c_str() : "?";
}
const char *alignName(const Scenario &sc, Align a) {
    return int(a) < int(sc.aligns().size()) ? sc.aligns()[int(a)].c_str() : "?";
}
const char *statusName(const Scenario &sc, Status s) {
    return int(s) < int(sc.statuses().size()) ? sc.statuses()[int(s)].c_str() : "?";
}

const char *kAttrLabel[6] = {"STRENGTH", "I.Q.", "PIETY", "VITALITY", "AGILITY", "LUCK"};

struct RollerCtx {
    Ui &ui;
    Roster &roster;
    const Scenario &sc;
    Rng &rng;
    const std::string &path;

    TextScreen &ts() { return ui.ts(); }
    void dirty() { roster.save(path); }
};

// The character-sheet frame shared by MAKECHAR.
void makeMenuFrame(RollerCtx &c, const Character &ch) {
    auto &t = c.ts();
    t.resetWindow();
    t.putChar(12);
    t.gotoXY(0, 0);  t.writeField("NAME ", 10); t.write(ch.name);
    t.gotoXY(0, 1);  t.writeField("PASSWORD", 9);
    t.gotoXY(0, 2);  t.writeField("RACE", 9);
    t.gotoXY(0, 3);  t.writeField("POINTS", 9);
    for (int i = 0; i < 6; ++i) { t.gotoXY(0, 5 + i); t.writeField(kAttrLabel[i], 9); }
    t.gotoXY(0, 12); t.write("ALIGNMENT");
    t.gotoXY(0, 13); t.writeField("CLASS", 9);
}

void drawClassList(RollerCtx &c, const bool elig[8]) {
    auto &t = c.ts();
    for (int i = 0; i < 8; ++i) {
        t.gotoXY(20, 5 + i);
        t.write("                    ");
        t.gotoXY(20, 5 + i);
        if (elig[i]) {
            char b[8];
            std::snprintf(b, sizeof b, "%c) ", 'A' + i);
            t.write(b);
            t.write(className(c.sc, Class(i)));
        }
    }
}

// GIVEPTS -- the point-allocation loop.  Returns false on quit.
bool givePts(RollerCtx &c, Character &ch, int base[6]) {
    auto &t = c.ts();
    int a[6];
    for (int i = 0; i < 6; ++i) a[i] = base[i];
    int ptsLeft = rollBonusPoints(c.rng);

    t.gotoXY(0, 15);
    t.setWindow(0, 15, 40, 9);
    t.putChar(11);
    t.writeln("ENTER [+,-] TO ALTER A SCORE,");
    t.writeln("      [RET] TO GO TO NEXT SCORE,");
    t.writeln("      [ESC] TO GO ON WHEN POINTS USED UP");
    t.resetWindow();

    for (int i = 0; i < 6; ++i) {
        t.gotoXY(10, 5 + i);
        char b[4]; std::snprintf(b, sizeof b, "%2d", a[i]); t.write(b);
    }

    bool elig[8];
    bool canChg = classEligibility(a, ch.align, elig);
    drawClassList(c, elig);

    int cur = 0;
    int key = 0;
    do {
        t.gotoXY(13, 5 + cur); t.setAttr(ATTR_INVERSE); t.write("<--"); t.setAttr(ATTR_NORMAL);
        int inner;
        do {
            t.gotoXY(10, 3);
            char b[4]; std::snprintf(b, sizeof b, "%2d", ptsLeft); t.write(b);
            inner = c.ui.getKey();
            if (c.ui.quit()) return false;
            bool plus  = inner == '+' || inner == ';';
            bool minus = inner == '-' || inner == '=';
            if (plus && a[cur] < 18 && ptsLeft > 0) { a[cur]++; ptsLeft--; }
            else if (minus && a[cur] > base[cur])   { a[cur]--; ptsLeft++; }
            if (plus || minus) {
                t.gotoXY(10, 5 + cur);
                std::snprintf(b, sizeof b, "%2d", a[cur]); t.write(b);
                canChg = classEligibility(a, ch.align, elig);
                drawClassList(c, elig);
            }
            key = inner;
        } while (inner != KEY_ESC && inner != KEY_RETURN);

        if (key == KEY_RETURN) {
            t.gotoXY(13, 5 + cur); t.write("   ");
            cur = (cur < 5) ? cur + 1 : 0;
        }
    } while (!(key == KEY_ESC && canChg && ptsLeft == 0));

    // choose a class among the eligible ones
    int chosen;
    for (;;) {
        t.setWindow(0, 15, 40, 9);
        t.putChar(11);
        t.write("CHOOSE A CLASS >");
        t.resetWindow();
        int k = c.ui.menu("ABCDEFGH");
        if (c.ui.quit()) return false;
        chosen = k - 'A';
        if (elig[chosen]) break;
    }
    t.setWindow(0, 15, 40, 9); t.putChar(11); t.resetWindow();
    t.gotoXY(10, 13); t.write(className(c.sc, Class(chosen)));
    ch.cls = Class(chosen);
    for (int i = 0; i < 6; ++i) ch.attrib[i] = a[i];
    return true;
}

// MAKECHAR
void makeChar(RollerCtx &c, int slotIdx, const std::string &startName) {
    Character ch{};
    ch.name = startName;
    ch.status = Status::OK;
    ch.charLevel = ch.maxLevelAcquired = 1;
    ch.armorClass = 10;
    for (int &l : ch.luckSkill) l = 16;
    ch.age = rollAge(c.rng);
    ch.gold.v = rollGold(c.rng);

    auto &t = c.ts();

    // password (twice)
    std::string pw;
    for (;;) {
        makeMenuFrame(c, ch);
        t.setWindow(0, 15, 40, 9); t.putChar(11);
        t.writeln("ENTER A PASSWORD ([RET] FOR NONE)");
        t.resetWindow();
        t.gotoXY(10, 1); t.putChar(29);
        t.gotoXY(10, 1);
        std::string p1 = c.ui.getPass(15, c.rng);
        if (c.ui.quit()) return;
        t.setWindow(0, 15, 40, 9); t.putChar(11);
        t.writeln("ENTER IT AGAIN TO BE SURE");
        t.resetWindow();
        t.gotoXY(10, 1); t.putChar(29);
        t.gotoXY(10, 1);
        std::string p2 = c.ui.getPass(15, c.rng);
        if (c.ui.quit()) return;
        if (p1 == p2) { pw = p1; break; }
    }
    ch.password = pw;

    // race
    {
        t.setWindow(0, 15, 40, 9); t.putChar(11); t.resetWindow();
        t.gotoXY(0, 17);
        for (int r = 1; r <= 5; ++r) {
            char b[8]; std::snprintf(b, sizeof b, "%c) ", '@' + r); t.write(b);
            t.writeln(raceName(c.sc, Race(r)));
        }
        int k = c.ui.menu("ABCDE");
        if (c.ui.quit()) return;
        ch.race = Race(k - '@');
        t.gotoXY(10, 2); t.write(raceName(c.sc, ch.race));
        t.gotoXY(0, 17); for (int r = 0; r < 6; ++r) { t.putChar(29); t.putChar('\n'); }
    }
    int base[6];
    raceBaseAttrs(ch.race, base);

    // alignment
    {
        t.gotoXY(0, 17);
        for (int i = 1; i <= 3; ++i) {
            char b[8]; std::snprintf(b, sizeof b, "%c) ", '@' + i); t.write(b);
            t.writeln(alignName(c.sc, Align(i)));
        }
        int k = c.ui.menu("ABC");
        if (c.ui.quit()) return;
        ch.align = Align(k - '@');
        t.gotoXY(10, 12); t.write(alignName(c.sc, ch.align));
        t.gotoXY(0, 17); for (int r = 0; r < 4; ++r) { t.putChar(29); t.putChar('\n'); }
    }

    if (!givePts(c, ch, base)) return;

    // keep?
    {
        t.setWindow(0, 15, 40, 9); t.putChar(11);
        t.write("KEEP THIS CHARACTER (Y/N)? >");
        t.resetWindow();
        int k = c.ui.menu("YN");
        if (c.ui.quit() || k == 'N') return;
    }

    startingSpells(ch);
    ch.hpMax = ch.hpLeft = rollHp(ch.cls, ch.attrib[VIT], c.rng);

    c.roster.slot(slotIdx) = ch;
    c.dirty();
}

// *ROSTER
void dsp20nm(RollerCtx &c) {
    auto &t = c.ts();
    t.resetWindow();
    t.putChar(12);
    t.writeln("NAMES IN USE:");
    t.writeln("----------------------------------------");
    int line = 0;
    for (int i = 0; i < c.roster.count(); ++i) {
        const Character &ch = c.roster.slot(i);
        if (ch.status == Status::Lost) continue;
        t.gotoXY(0, 2 + line++);
        char buf[64];
        std::snprintf(buf, sizeof buf, "%s LEVEL %d %s %s (%s)",
                      ch.name.c_str(), ch.charLevel, raceName(c.sc, ch.race),
                      className(c.sc, ch.cls), statusName(c.sc, ch.status));
        t.write(buf);
        if (ch.inMaze) t.write(" OUT");
    }
    t.gotoXY(0, 22);
    t.writeln("----------------------------------------");
    t.write("YOU MAY L)EAVE WHEN READY");
    c.ui.menu("L");
}

// character-sheet view (used by TRAINING > Inspect)
void inspect(RollerCtx &c, const Character &ch) {
    auto &t = c.ts();
    t.resetWindow();
    t.putChar(12);
    t.gotoXY(0, 0); t.writeField("NAME ", 10); t.write(ch.name);
    t.gotoXY(0, 1); t.writeField("RACE", 9);  t.write(std::string(" ") + raceName(c.sc, ch.race));
    t.gotoXY(0, 2); t.writeField("CLASS", 9); t.write(std::string(" ") + className(c.sc, ch.cls));
    t.gotoXY(0, 3); t.writeField("ALIGN", 9); t.write(std::string(" ") + alignName(c.sc, ch.align));
    t.gotoXY(0, 4); t.writeField("LEVEL", 9);
    char b[32]; std::snprintf(b, sizeof b, " %d", ch.charLevel); t.write(b);
    for (int i = 0; i < 6; ++i) {
        t.gotoXY(0, 6 + i); t.writeField(kAttrLabel[i], 9);
        std::snprintf(b, sizeof b, " %2d", ch.attrib[i]); t.write(b);
    }
    t.gotoXY(0, 13); t.writeField("HITS", 9);
    std::snprintf(b, sizeof b, " %d/%d", ch.hpLeft, ch.hpMax); t.write(b);
    t.gotoXY(0, 14); t.writeField("A.C.", 9);
    std::snprintf(b, sizeof b, " %d", ch.armorClass); t.write(b);
    t.gotoXY(0, 15); t.writeField("GOLD", 9);
    std::snprintf(b, sizeof b, " %lld", (long long)ch.gold.v); t.write(b);
    t.gotoXY(0, 22);
    c.ui.pressAnyKey();
}

// TRAINING -- an existing character
void training(RollerCtx &c, int slotIdx) {
    auto &t = c.ts();
    Character &ch = c.roster.slot(slotIdx);

    // password check
    t.resetWindow();
    t.putChar(12);
    t.gotoXY(9, 10); t.write("PASSWORD >");
    std::string pw = c.ui.getPass(15, c.rng);
    if (c.ui.quit()) return;
    if (pw != ch.password) return;

    for (;;) {
        t.putChar(12);
        char hdr[64];
        std::snprintf(hdr, sizeof hdr, "%s LEVEL %d %s %s (%s)",
                      ch.name.c_str(), ch.charLevel, raceName(c.sc, ch.race),
                      className(c.sc, ch.cls), statusName(c.sc, ch.status));
        t.writeln(hdr);
        t.writeln("");
        t.writeln("YOU MAY I)NSPECT THIS CHARACTER,");
        t.writeln("         D)ELETE  THIS CHARACTER,");
        t.writeln("         R)EROLL  THIS CHARACTER,");
        t.writeln("         S)ET NEW PASSWORD, OR");
        t.writeln("  PRESS [RET] TO LEAVE");
        int k = c.ui.getKey();
        if (c.ui.quit() || k == KEY_RETURN) return;
        if (k == 'I') inspect(c, ch);
        else if (k == 'D') {
            t.putChar(12);
            t.write("ARE YOU SURE YOU WANT TO DELETE (Y/N) ?");
            if (c.ui.menu("YN") == 'Y') {
                ch.status = Status::Lost;
                ch.inMaze = false;
                c.dirty();
                return;
            }
        } else if (k == 'R') {
            t.putChar(12);
            t.write("ARE YOU SURE YOU WANT TO REROLL (Y/N) ?");
            if (c.ui.menu("YN") == 'Y') {
                std::string name = ch.name;
                ch.status = Status::Lost;
                c.dirty();
                makeChar(c, slotIdx, name);
                return;
            }
        } else if (k == 'S') {
            t.putChar(12);
            t.write("ENTER NEW PASSWORD ([RET] FOR NONE)");
            t.gotoXY(10, 2);
            std::string p1 = c.ui.getPass(15, c.rng);
            t.putChar(12);
            t.write("ENTER AGAIN TO BE SURE");
            t.gotoXY(10, 2);
            std::string p2 = c.ui.getPass(15, c.rng);
            t.putChar(12);
            if (p1 == p2) { ch.password = p1; c.dirty(); t.write("PASSWORD CHANGED - "); }
            else t.write("THEY ARE NOT THE SAME - ");
            c.ui.pressAnyKey("PRESS [RET]");
        }
    }
}

// CREATE -- name not on the roster
void create(RollerCtx &c, const std::string &name) {
    auto &t = c.ts();
    int slotIdx = c.roster.findLost();
    if (slotIdx < 0) {
        t.putChar(12);
        t.writeln("THERE IS NO ROOM LEFT - TRY DELETING");
        c.ui.pressAnyKey();
        return;
    }
    t.putChar(12);
    t.writeln("THAT CHARACTER DOES NOT EXIST. DO YOU");
    t.write("WANT TO CREATE IT (Y/N) ?> ");
    if (c.ui.menu("YN") != 'Y') return;
    makeChar(c, slotIdx, name);
}

} // namespace

bool showTitle(Platform &p, const Font &font, Bytes titleData) {
    // 320x64 CGA logo centred on a 640x400 screen, welcome text below.
    Surface s(640, 200);
    s.fill(0);
    Surface logo = loadTitle(titleData);
    for (int y = 0; y < logo.height(); ++y)
        for (int x = 0; x < logo.width(); ++x)
            if (u8 c = logo.get(x, y)) s.set(160 + x, 24 + y, c);
    // draw the text in palette index 2 (green)
    TextScreen ts;
    ts.gotoXY(4, 14);  ts.write("WELCOME TO THE WORLD OF WIZARDRY!");
    ts.gotoXY(14, 18); ts.write("PRESS ANY KEY");
    for (int r = 0; r < TextScreen::kRows; ++r)
        for (int c = 0; c < TextScreen::kCols; ++c)
            if (char ch = ts.at(c, r); ch != ' ')
                font.drawGlyph(s, (unsigned char)ch, c * Font::kW, r * Font::kH, 2);
    for (;;) {
        p.present(s, kCgaPalette, 4);
        int k = p.waitKey();
        if (k == KEY_QUIT) return false;
        if (k != KEY_NONE) return true;
    }
}

void runRoller(Ui &ui, Roster &roster, const Scenario &sc, Rng &rng,
               const std::string &rosterPath) {
    RollerCtx c{ui, roster, sc, rng, rosterPath};
    auto &t = ui.ts();

    while (!ui.quit()) {
        t.resetWindow();
        t.putChar(12);
        t.writeCentered("TRAINING GROUNDS", 0);
        t.gotoXY(0, 2); t.writeln("YOU MAY ENTER A CHARACTER NAME TO ADD,");
        t.gotoXY(8, 3);  t.writeln("INSPECT OR EDIT,");
        t.gotoXY(8, 5);  t.writeln("\"*ROSTER\" TO SEE ROSTER,");
        t.gotoXY(0, 7);  t.writeln("OR PRESS [RET] FOR CASTLE.");

        t.gotoXY(13, 9); t.write("NAME >");
        std::string name = ui.getLine(15);
        if (ui.quit()) return;
        if (name.empty()) return;                       // -> castle

        if (name == "*ROSTER") { dsp20nm(c); continue; }

        int idx = roster.findByName(name);
        if (idx < 0) create(c, name);
        else training(c, idx);
    }
}

} // namespace wiz
