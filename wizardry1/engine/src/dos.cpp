#include "wiz/dos.h"
#include "wiz/rng.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace wiz {

// ---- strings ----------------------------------------------------------------------
std::string Dos::str(int n) const {                       // WIZARDRY:38 (GETSTR)
    bool ok = false;
    std::string s = sp_.get(n, &ok);
    return ok ? s : "**ERR**";
}

int Dos::visLen(const std::string &s) {                   // WIZARDRY:42
    int n = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = s[i];
        if (c >= 129 && c <= 159) ++i;                    // attribute escape + its char = one cell
        ++n;
    }
    return n;
}

std::string Dos::itoa(long n) {                           // WIZARDRY:51
    bool neg = n < 0;
    unsigned long a = neg ? 0ul - (unsigned long)n : (unsigned long)n;
    std::string s = a ? "" : "0";
    for (; a; a /= 10) s.insert(s.begin(), char('0' + a % 10));
    return neg ? "-" + s : s;
}

std::string Dos::long2str(long long v) {                  // WIZARDRY:52
    if (v < 0) v = 0;
    return std::to_string(v);
}

std::string Dos::substitute(const std::string &tmpl, const std::string &arg) {   // WIZARDRY:56/57
    size_t p = tmpl.find('^');
    if (p == std::string::npos) return tmpl;
    return tmpl.substr(0, p) + arg + tmpl.substr(p + 1);
}

std::string Dos::substituteChar(const std::string &s, int ch, const std::string &arg) {   // WIZARDRY:56
    size_t p = s.find(char(ch));
    if (p == std::string::npos) return s;
    return s.substr(0, p) + arg + s.substr(p + 1);
}

std::string Dos::nameWith(const std::string &name, int base) const {                    // WIZARDRY:31
    if (name.empty()) return name;
    if (name[0] == '*') return name;
    std::string n = name;
    if (n[0] == '+') { base += 2; n.erase(0, 1); }
    else if (n[0] == '-') { base += 4; n.erase(0, 1); }
    char c = n.empty() ? ' ' : n[0];
    bool vowel = c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U';
    return substitute(str(vowel ? base : base + 1), n);
}

// ---- keyboard ------------------------------------------------------------------------
void Dos::present() {
    static Surface s;
    con.render(s);
    plat.present(s, Console::palette(), 4);
}

void Dos::initScreen() {
    con.init();
    window1 = con.open(1, 1, 38, 22, 0, true);
}

void Dos::dumpGrid(int key) {
    if (dumpDir_.empty()) return;
    char name[64];
    std::snprintf(name, sizeof name, "/%03d_%02x.grid", ++dumpN_, key & 0xFF);
    std::string out;
    auto vis = con.visible();
    char b[16];
    for (int r = 0; r < Console::kRows; ++r) {
        for (int c = 0; c < Console::kCols; ++c) {
            std::snprintf(b, sizeof b, c ? " %04x" : "%04x", vis[size_t(r) * Console::kCols + c]);
            out += b;
        }
        out += '\n';
    }
    std::snprintf(b, sizeof b, "%08x", con.vramCrc());
    out += std::string("vram ") + b + "\n";
    if (rngProbe_) {
        u16 st[4];
        rngProbe_->state(st);
        std::snprintf(b, sizeof b, "%04x", st[0]);
        out += std::string("rng ") + b;
        for (int i = 1; i < 4; ++i) { std::snprintf(b, sizeof b, ",%04x", st[i]); out += b; }
        out += "\n";
    }
    if (FILE *f = std::fopen((dumpDir_ + name).c_str(), "wb")) {
        std::fwrite(out.data(), 1, out.size(), f);
        std::fclose(f);
    }
    if (const char *m = std::getenv("WIZ1_MAXDUMPS"))                // regression runs: stop after N snapshots
        if (dumpN_ >= std::atoi(m)) quit = true;
}

bool Dos::keyReady() {                                    // WIZARDRY:33
    if (quit) return true;
    if (pending_ == KEY_NONE) pending_ = plat.pollKey();
    if (pending_ == KEY_QUIT) quit = true;
    return pending_ != KEY_NONE;
}

int Dos::getKey() {                                       // WIZARDRY:11
    if (quit) { inchar = 0; return 0; }
    present();
    for (;;) {
        int k = pending_;
        pending_ = KEY_NONE;
        if (k == KEY_NONE) k = plat.waitKey();
        dumpGrid(k == KEY_QUIT ? 0 : k);                  // the screen the player is looking at
        if (k == KEY_QUIT) { quit = true; inchar = 0; return 0; }
        if (k > 95 && k < 127) k -= 32;
        if (k < 91 || k > 127) { inchar = k; return k; }
        // 91..127 after upper-casing: the DOS keyboard loop ignores these
    }
}

void Dos::waitCR() {                                      // WIZARDRY:12
    do getKey(); while (inchar != 13 && !quit);
}

int Dos::getNum(int max) {                                // WIZARDRY:32
    for (;;) {
        getKey();
        if (quit) return -1;
        int k = inchar;
        if (k == 13) k = 48;
        else if (k == 48) k = 0;
        int n = k - 48;
        if (n >= 0 && n <= max) return n;
    }
}

void Dos::pause(int units) {                              // CONUNIT selector 19
    present();
    dumpGrid(0xFF);
    long ticks = long(units) * 2;                         // INTERP: shl dx,1 ; 18.2 Hz ticks
    for (long t = 0; t < ticks && !keyReady(); ++t) plat.delayMs(55);
}

// ---- window text ----------------------------------------------------------------------
void Dos::putNum(Win *w, long n, int width) {             // WIZARDRY:23
    std::string s = itoa(n);
    if (int(s.size()) > width) {
        if (n < 0) { putC(w, '-'); --width; }
        while (width > 0) { putC(w, '*'); --width; }
        return;
    }
    while (width > int(s.size())) { putC(w, ' '); --width; }
    putS(w, s);
}

void Dos::putCentered(Win *w, const std::string &s) {     // WIZARDRY:13
    int y = w->cy;
    int x = ((w->w - 2) / 2) - ((1 + visLen(s)) / 2);
    gotoXY(w, x, y);
    putS(w, s);
    gotoXY(w, 0, y + 1);
}

void Dos::putPadded(Win *w, long long v) {                // WIZARDRY:10
    std::string s = long2str(v);
    for (int i = int(s.size()); i <= 11; ++i) putC(w, ' ');
    putS(w, s);
}

// ---- boxes / menus ------------------------------------------------------------------------
int Dos::menuWidth(const std::string &menu) const {       // WIZARDRY:53
    int best = 0;
    size_t start = 0;
    for (;;) {
        size_t p = menu.find('/', start);
        std::string item = menu.substr(start, p == std::string::npos ? std::string::npos : p - start);
        best = std::max(best, visLen(item));
        if (p == std::string::npos) break;
        start = p + 1;
    }
    return best;
}

Win *Dos::centerBox(int width, int y, int h, int prio, bool refresh) {   // WIZARDRY:55
    int a = (width + 2) / 2;
    return con.open(19 - a, y, a + a + 2, h, prio, refresh);
}

Win *Dos::boxMsg(int n, int y, int extraH, int prio) {                    // WIZARDRY:43
    std::string s = str(n);
    int half = (1 + visLen(s)) / 2;
    Win *w = con.open(19 - half, y, half + half + 2, extraH + 2, prio, true);
    putS(w, s);
    return w;
}

int Dos::menu(Win *w, const std::string &spec) {                          // WIZARDRY:30
    std::string s = spec;
    int retItem = -1;                 // L135: index of the RETURN item
    int lower = -1;                   // L136: counts down from the lower-case marker
    int count = 0;                    // L131
    int col0 = w->cx;                 // L133
    int maxCx = -1;                   // L132
    bool lineStart = true;            // L137
    int startY = w->cy;               // L129
    std::string hot;                  // L138..: hotkeys
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char ch = s[i];
        if (ch == '/') {
            if (int(w->cx) > maxCx) maxCx = w->cx;
            con.newline(w);
            lineStart = true;
            continue;
        }
        if (lineStart) {
            if (w->cy == 0) { w->cy = u8(startY); col0 = maxCx + 1; }
            con.gotoXY(w, col0, w->cy);
            if (ch >= 97) {
                if (retItem < 0) { lower = 1; retItem = count; }
                ch = u8(ch - 32);
            }
            hot.push_back(char(ch));
            lineStart = false;
            ++count;
        }
        if (ch >= 129 && ch <= 159) {
            std::string two;
            two.push_back(char(ch));
            ++i;
            two.push_back(i < s.size() ? s[i] : ' ');
            putS(w, two);
        } else if (lower == 0) {
            putC(w, 126);
        } else {
            putC(w, ch);
        }
        if (lower >= 0) --lower;
    }
    for (;;) {
        getKey();
        if (quit) return -1;
        int idx = 0;
        for (int k = 0; k < count; ++k) {
            if (inchar == 13 && retItem == k) return idx;
            if ((unsigned char)hot[k] == inchar && inchar != '#') return idx;
            if (hot[k] == '#') {
                if (inchar > '0' && inchar <= '0' + partyCount) return idx + inchar - '1';
                idx += 6;
            } else {
                idx += 1;
            }
        }
        // PRINTBEL
    }
}

// ---- line input ---------------------------------------------------------------------------
std::string Dos::getLine(Win *w, int maxLen) {                            // WIZARDRY:29
    std::string s;
    for (;;) {
        int x = w->cx, y = w->cy;
        gotoXY(w, x, y);
        putC(w, '_');
        getKey();
        gotoXY(w, x, y);
        putC(w, ' ');
        gotoXY(w, x, y);
        if (quit) return s;
        if (inchar == 8 && !s.empty()) {
            s.pop_back();
            gotoXY(w, x - 1, y);
            putC(w, ' ');
            gotoXY(w, x - 1, y);
        } else if (inchar == 13 || inchar == 27) {
            putC(w, ' ');
            if (inchar == 27)
                while (!s.empty()) {
                    s.pop_back();
                    int cx = w->cx;
                    gotoXY(w, cx - 1, w->cy);
                    putC(w, ' ');
                    gotoXY(w, cx - 1, w->cy);
                }
            return s;
        } else if (inchar > 31 && inchar <= 127 && x < w->w - 3 && int(s.size()) < maxLen) {
            s.push_back(char(inchar));
            putC(w, inchar);
        }                                                               // else PRINTBEL
    }
}

std::string Dos::getName(Win *w, int maxLen) {                         // KANJIREA:42
    std::string s;
    bool underscore = true;
    for (;;) {
        int x = w->cx, y = w->cy;
        gotoXY(w, x, y);
        putC(w, underscore ? 95 : 127);
        getKey();
        gotoXY(w, x, y);
        putC(w, ' ');
        gotoXY(w, x, y);
        if (quit) return s;
        auto erase = [&] {                                              // KANJIREA:43
            s.pop_back();
            gotoXY(w, w->cx - 1, w->cy);
            putC(w, ' ');
            gotoXY(w, w->cx - 1, w->cy);
        };
        if (inchar == 8 && !s.empty()) {
            erase();
        } else if (inchar == 1) {
            underscore = !underscore;
        } else if (inchar > 31 && inchar <= 127 && x < w->w - 3 && int(s.size()) < maxLen) {
            if (inchar == 42 || inchar < 33 || inchar > 47) {
                s.push_back(char(inchar));
                putC(w, inchar);
            }
        } else if (inchar == 13 || inchar == 27) {
            putC(w, ' ');
            if (inchar == 27)
                while (!s.empty()) erase();
            return s;
        } else if (inchar == 11 || inchar == 10 || inchar == 12 || inchar == 9 || inchar == 222 ||
                   inchar == 223 || (inchar >= 166 && inchar <= 221)) {
            // kana / candidate-window keys: no effect in the English path
        }                                                                // else PRINTBEL
    }
}

std::string Dos::getPassword(Win *w, Rng &rng) {                        // CASTLE:2 / ROLLER:2
    std::string pw;
    for (;;) {
        getKey();
        if (quit) return pw;
        if (inchar == 13) break;
        if (pw.size() < 15) {
            int reps = rng.mod(2);
            for (int k = 0; k <= reps; ++k)
                if (w->cx < w->w - 3) putC(w, 'X');
            pw.push_back(char(inchar));
        }                                                                // else PRINTBEL
    }
    newline(w);
    return pw;
}

} // namespace wiz
