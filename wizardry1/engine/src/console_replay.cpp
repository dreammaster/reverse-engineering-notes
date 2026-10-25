// `wiz1 console-replay <dataDir> <opsDir>` -- regression harness for Console.
//
// tools/wizvm.py --ops runs the real SYSTEM.PASCAL and writes, per keyboard
// wait, NNN_KK.ops (the window-manager / driver calls since the last wait)
// and NNN_KK.grid (the composed screen: 25 rows of 40 cell words + the VRAM
// CRC).  This replays the leaf operations through wiz::Console and checks the
// composed screen after every wait.
#include "wiz/console.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#endif

namespace wiz {
namespace {

std::vector<u8> slurp(const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    return std::vector<u8>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

std::vector<std::string> listFiles(const std::string &dir, const std::string &ext) {
    std::vector<std::string> out;
#if defined(_WIN32)
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA((dir + "/*" + ext).c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do out.push_back(fd.cFileName); while (FindNextFileA(h, &fd));
        FindClose(h);
    }
#else
    if (DIR *d = opendir(dir.c_str())) {
        while (dirent *e = readdir(d)) {
            std::string n = e->d_name;
            if (n.size() > ext.size() && n.compare(n.size() - ext.size(), ext.size(), ext) == 0) out.push_back(n);
        }
        closedir(d);
    }
#endif
    std::sort(out.begin(), out.end());
    return out;
}

// Parse a Python repr() string literal starting at s[i] (quote char).
std::string parsePyStr(const std::string &s, size_t &i) {
    char q = s[i++];
    std::string out;
    while (i < s.size() && s[i] != q) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            ++i;
            switch (s[i]) {
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'x': out += char(std::strtol(s.substr(i + 1, 2).c_str(), nullptr, 16)); i += 2; break;
                default: out += s[i]; break;
            }
            ++i;
        } else {
            out += s[i++];
        }
    }
    ++i;
    return out;
}

struct Replay {
    Console con;
    std::map<std::string, Win *> win;
    std::vector<u8> title;

    Win *W(const std::string &name) {
        auto it = win.find(name);
        return it == win.end() ? nullptr : it->second;
    }

    static std::vector<std::string> args(const std::string &line, size_t open) {
        // split "a, b, 'c, d', e)" at top level
        std::vector<std::string> out;
        std::string cur;
        for (size_t i = open + 1; i < line.size(); ++i) {
            char c = line[i];
            if (c == '\'' || c == '"') {
                size_t j = i;
                std::string lit = parsePyStr(line, j);
                cur += '\x01' + lit + '\x02';       // marker: literal
                i = j - 1;
            } else if (c == ',' ) {
                out.push_back(cur); cur.clear();
            } else if (c == ')') {
                out.push_back(cur); break;
            } else if (c != ' ') {
                cur += c;
            }
        }
        if (out.size() == 1 && out[0].empty()) out.clear();
        return out;
    }
    static std::string lit(const std::string &a) {
        size_t b = a.find('\x01'), e = a.rfind('\x02');
        return (b == std::string::npos) ? a : a.substr(b + 1, e - b - 1);
    }
    static int num(const std::string &a) { return std::atoi(a.c_str()); }

    bool line(const std::string &l, std::string &err) {
        if (l.empty()) return true;
        std::istringstream ss(l);
        if (l.rfind("  L0d", 0) == 0) return l0(l, err);
        if (l.rfind("  L0e", 0) == 0) return true;               // echo of a logged proc
        size_t p = l.find('(');
        if (p == std::string::npos) return true;
        std::string name = l.substr(0, p);
        auto a = args(l, p);
        if (name == "Open") {
            Win *w = con.open(num(a[0]), num(a[1]), num(a[2]), num(a[3]), num(a[4]), num(a[5]) != 0);
            size_t arrow = l.find("-> ");
            if (arrow != std::string::npos) win[l.substr(arrow + 3)] = w;
        } else if (name == "Close") {
            Win *w = W(a[0]);
            if (w) { con.close(w, num(a[1]) != 0); win.erase(a[0]); }
        } else if (name == "SetPrio") {
            if (Win *w = W(a[0])) con.setPrio(w, num(a[1]), num(a[2]) != 0);
        } else if (name == "GotoXY") {
            if (Win *w = W(a[0])) con.gotoXY(w, num(a[1]), num(a[2]));
        } else if (name == "PutC") {
            if (Win *w = W(a[0])) con.putChar(w, u16(num(a[1]) & 0xFF));
        } else if (name == "PutS") {
            if (Win *w = W(a[0])) con.putString(w, lit(a[1]));
        } else if (name == "Newline") {
            if (Win *w = W(a[0])) con.newline(w);
        } else if (name == "Clear") {
            if (Win *w = W(a[0])) con.clear(w, num(a[1]) != 0);
        } else if (name == "ClearRect") {
            if (Win *w = W(a[0])) con.clearRect(w, num(a[1]), num(a[2]), num(a[3]), num(a[4]));
        } else if (name == "Hide") {
            if (Win *w = W(a[0])) con.hide(w, num(a[1]) != 0);
        } else if (name == "Show") {
            if (Win *w = W(a[0])) con.show(w, num(a[1]) != 0);
        } else if (name == "Refresh") {
            con.refresh();
        }
        // composites (PutNum, PutStr, Menu, BoxMsg, ...) are covered by their leaves
        (void)err;
        return true;
    }

    bool l0(const std::string &l, std::string &err) {
        std::istringstream ss(l);
        std::string tag, op;
        ss >> tag >> op;
        if (op == "present") { std::string w; ss >> w; if (Win *x = W(w)) con.present(x); }
        else if (op == "layout") con.layout();
        else if (op == "putc") { std::string w; unsigned v; ss >> w >> std::hex >> v; if (Win *x = W(w)) con.putChar(x, u16(v)); }
        else if (op == "puts") {
            std::string w; ss >> w;
            size_t q = l.find_first_of("'\"", l.find(w) + w.size());
            if (Win *x = W(w)) con.putString(x, parsePyStr(l, q));
        } else if (op == "drawline") {
            std::string w; DrawLineArgs a; unsigned v[8];
            ss >> w >> v[0] >> v[1] >> v[2] >> v[3] >> v[4] >> v[5] >> v[6] >> v[7];
            a.clipLo = u16(v[0]); a.clipHi = u16(v[1]); a.col = u16(v[2]); a.row = u16(v[3]);
            a.dCol = u16(v[4]); a.dRow = u16(v[5]); a.count = u16(v[6]); a.glyph = u16(v[7]);
            if (Win *x = W(w)) con.drawLine(x, a);
        } else if (op == "blit13") {
            std::string w; unsigned p[4]; ss >> w >> p[0] >> p[1] >> p[2] >> p[3];
            u8 pics[4] = {u8(p[0]), u8(p[1]), u8(p[2]), u8(p[3])};
            if (Win *x = W(w)) con.blit13(x, pics);
        } else if (op == "clearattr") { std::string w; ss >> w; if (Win *x = W(w)) con.clearAttr(x); }
        else if (op == "title") {
            unsigned blk; ss >> blk;
            if (blk == 0) con.titleBitmap(nullptr, 0); else con.titleBitmap(title.data(), title.size());
        } else if (op == "sel1") {
            con.init();
        }
        // sel14/18 (assets, loaded up front), 22/24/25/26/29/31 (disk, sound, string helpers): no pixels
        (void)err;
        return true;
    }
};

} // namespace

int cmdConsoleReplay(int argc, char **argv) {
    if (argc < 4) { std::fprintf(stderr, "usage: wiz1 console-replay <dataDir> <opsDir>\n"); return 2; }
    std::string data = argv[2], dir = argv[3];
    Replay r;
    auto cs = slurp(data + "/200.CHARSET");
    r.con.loadCharset(cs.data(), cs.size());
    r.con.loadMonsters(slurp(data + "/200.MONSTERS"));
    r.title = slurp(data + "/200.TITLE");

    int bad = 0, total = 0;
    for (const std::string &f : listFiles(dir, ".ops")) {
        std::string base = f.substr(0, f.size() - 4);
        std::ifstream in(dir + "/" + f);
        std::string l, err;
        while (std::getline(in, l)) {
            if (!l.empty() && l.back() == '\r') l.pop_back();
            if (!r.line(l, err)) { std::printf("%s: %s\n", base.c_str(), err.c_str()); ++bad; err.clear(); }
        }
        // compare with the oracle's composed screen
        std::ifstream g(dir + "/" + base + ".grid");
        if (!g) continue;
        ++total;
        auto vis = r.con.visible();
        int diffs = 0;
        for (int row = 0; row < Console::kRows; ++row) {
            std::getline(g, l);
            std::istringstream ls(l);
            for (int col = 0; col < Console::kCols; ++col) {
                unsigned want = 0;
                ls >> std::hex >> want;
                unsigned got = vis[size_t(row) * Console::kCols + col];
                if (want != got) {
                    if (diffs < 6)
                        std::printf("  %s cell (%d,%d): oracle %04x  port %04x\n", base.c_str(), col, row, want, got);
                    ++diffs;
                }
            }
        }
        std::getline(g, l);
        char crc[32];
        std::snprintf(crc, sizeof crc, "vram %08x", r.con.vramCrc());
        bool vramOk = (l == crc);
        if (diffs || !vramOk) {
            std::printf("%s: MISMATCH (%d cells%s)\n", base.c_str(), diffs, vramOk ? "" : ", vram differs");
            ++bad;
        } else {
            std::printf("%s: ok\n", base.c_str());
        }
    }
    std::printf("%d screens, %d bad\n", total, bad);
    return bad ? 1 : 0;
}

} // namespace wiz
