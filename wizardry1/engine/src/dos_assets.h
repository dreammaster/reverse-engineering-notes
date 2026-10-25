// Shared by the DOS-screen commands (dos_cmds.cpp, dos_game.cpp): loading the
// extracted game directory and decoding key scripts.
#pragma once
#include "wiz/scenario.h"
#include "wiz/string_pool.h"

#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace wiz {

inline std::vector<u8> slurp(const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    return std::vector<u8>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

// "\r" "\e" "\n" "\b" "\xNN" escapes in a key script
inline std::string unescapeKeys(const char *s) {
    std::string o;
    for (; *s; ++s) {
        if (*s != '\\' || !s[1]) { o += *s; continue; }
        ++s;
        switch (*s) {
            case 'r': o += '\r'; break;
            case 'e': o += '\x1b'; break;
            case 'n': o += '\n'; break;
            case 'b': o += '\b'; break;
            case 'x': o += char(std::strtol(std::string(s + 1, 2).c_str(), nullptr, 16)); s += 2; break;
            default: o += *s; break;
        }
    }
    return o;
}

// Everything a DOS-style screen needs, loaded from the extracted game directory.
struct DosAssets {
    Scenario sc;
    StringPool sp;
    std::vector<u8> charset, monsters, title;
    bool load(const std::string &dir) {
        if (!sc.load(slurp(dir + "/SCENARIO.DATA"))) return false;
        if (!sp.load(slurp(dir + "/ASCII.KRN"))) return false;
        charset = slurp(dir + "/200.CHARSET");
        monsters = slurp(dir + "/200.MONSTERS");
        title = slurp(dir + "/200.TITLE");
        return !charset.empty();
    }
};

} // namespace wiz
