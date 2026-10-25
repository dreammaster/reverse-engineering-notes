// The DOS runtime the screens are written against: the WIZARDRY (segment 1)
// text procs on top of wiz::Console, the keyboard (GETKEY / KEYREADY /
// WAITCR), GETSTR, and putting the composed screen on the Platform.
//
// Every method is named after the p-code proc it ports (see docs/drawing.md;
// tools/pcode_decomp.py WIZARDRY <n> prints the original).
#pragma once
#include "wiz/console.h"
#include "wiz/platform.h"
#include "wiz/string_pool.h"

#include <string>

namespace wiz {

class Dos {
public:
    Dos(Platform &p, Console &c, const StringPool &sp) : plat(p), con(c), sp_(sp) {}

    Platform &plat;
    Console &con;
    int inchar = 0;              // INCHAR (global 3): the last key GETKEY returned
    int partyCount = 0;          // PARTYCNT (global 4): '#' menu items accept 1..partyCount
    bool quit = false;           // the window was closed / the key script ended
    Win *window1 = nullptr;      // WINDOW1 (global 17): the (1,1,38,22) backdrop frame

    // CONUNIT init + the backdrop window (WIZARDRY mainline / KANJIREA:10)
    void initScreen();
    // regression dumps: write NNN_KK.grid (composed screen + VRAM CRC) at every key wait
    void setGridDump(const std::string &dir) { dumpDir_ = dir; }

    // ---- strings -------------------------------------------------------------
    std::string str(int n) const;                       // proc 38  GETSTR
    static int visLen(const std::string &s);            // proc 42  length without attribute escapes
    static std::string itoa(long n);                    // proc 51
    static std::string long2str(long long v);           // proc 52  TWIZLONG -> text
    static std::string substitute(const std::string &tmpl, const std::string &arg);   // proc 57 (replaces '^')

    // ---- keyboard --------------------------------------------------------------
    int  getKey();               // proc 11: upper-cases, ignores 91..127; presents first
    bool keyReady();             // proc 33
    void waitCR();               // proc 12
    // proc 32: a digit 0..max (RETURN = 0); anything else beeps.  -1 if the window closed.
    int  getNum(int max);
    void pause(int units);       // CONUNIT selector 19 (units of ~2 ticks of 18.2 Hz); key aborts
    void present();              // push the composed screen to the platform

    // ---- window text procs --------------------------------------------------------
    void putC(Win *w, int ch) { con.putChar(w, u16(ch & 0xFF)); }      // 35
    void putS(Win *w, const std::string &s) { con.putString(w, s); }   // 36
    void gotoXY(Win *w, int x, int y) { con.gotoXY(w, x, y); }         // 22
    void newline(Win *w) { con.newline(w); }                           // 24
    void putNum(Win *w, long n, int width);                            // 23
    void putCentered(Win *w, const std::string &s);                    // 13
    void putStr(Win *w, int n) { putS(w, str(n)); }                    // 39
    void putStrC(Win *w, int n) { putCentered(w, str(n)); }            // 40
    void putPadded(Win *w, long long v);                               // 10: right-aligned in 12

    // ---- boxes / menus -------------------------------------------------------------------
    int  menuWidth(const std::string &menu) const;                     // proc 53
    Win *centerBox(int width, int y, int h, int prio, bool refresh);   // proc 55
    Win *boxMsg(int n, int y, int extraH, int prio);                   // proc 43
    // proc 30: prints the '/'-separated item list (`X)` hotkeys, lower-case = the
    // RETURN item, '#' = party digits) and reads a key.  Returns the item index
    // (a '#' item consumes 6 indices), or -1 if the window was closed.
    int  menu(Win *w, const std::string &spec);
    int  menuStr(Win *w, int n) { return menu(w, str(n)); }            // proc 41

    // ---- line input (proc 29) ---------------------------------------------------------------
    std::string getLine(Win *w, int maxLen);

private:
    const StringPool &sp_;
    int pending_ = KEY_NONE;
    std::string dumpDir_;
    int dumpN_ = 0;
    void dumpGrid(int key);
};

} // namespace wiz
