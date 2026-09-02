// A 40x24 character grid -- the surface the game's menus are written against.
//
// The DOS game does WRITE / WRITELN / GOTOXY(x,y) into "windows" (rectangular
// viewports; see WIZARDRY procs 19/22/25).  This models that: a flat grid, a
// current window that clamps/offsets the cursor, the UCSD control codes
// (CHR 12 clear+home, 11 clear-to-end-of-screen, 29 clear-to-end-of-line,
// 8 backspace, 13/10 newline), and rendering through the game Font.
#pragma once
#include "wiz/surface.h"
#include "wiz/font.h"

namespace wiz {

enum : u8 { ATTR_NORMAL = 0, ATTR_INVERSE = 1 };

class TextScreen {
public:
    static constexpr int kCols = 40, kRows = 24;

    TextScreen() { setWindow(0, 0, kCols, kRows); clear(); }

    // --- window (viewport) -------------------------------------------------
    void setWindow(int x, int y, int w, int h);
    void resetWindow() { setWindow(0, 0, kCols, kRows); }

    // --- cursor / output (coords are window-relative) --------------------
    void gotoXY(int x, int y);
    void putChar(char c);                 // handles the control codes
    void write(const std::string &s);
    void writeln(const std::string &s = "");
    void writeField(const std::string &s, int width);   // right-justified, Pascal ': w'
    void writeCentered(const std::string &s, int row);

    void setAttr(u8 a) { attr_ = a; }
    u8 attr() const { return attr_; }

    // --- bulk ops -------------------------------------------------------
    void clear();                         // whole screen
    void clearWindow();                   // CHR 12 within a window
    void clearRect(int x, int y, int w, int h);   // CLRRECT
    void clearToEndOfLine();              // CHR 29
    void clearToEndOfScreen();            // CHR 11

    // --- render ------------------------------------------------------------
    void render(Surface &s, const Font &font, u8 fg = 10, u8 bg = 0,
                u8 inverseFg = 0, u8 inverseBg = 10) const;

    int cursorX() const { return cx_; }
    int cursorY() const { return cy_; }
    char at(int x, int y) const { return in(x, y) ? cell_[y][x] : ' '; }

private:
    struct { int x = 0, y = 0, w = kCols, h = kRows; } win_;
    bool in(int x, int y) const { return x >= 0 && y >= 0 && x < kCols && y < kRows; }
    int  absX() const { return win_.x + cx_; }
    int  absY() const { return win_.y + cy_; }
    void newline();

    char cell_[kRows][kCols];
    u8   attrCell_[kRows][kCols];
    int  cx_ = 0, cy_ = 0;            // window-relative
    u8   attr_ = ATTR_NORMAL;
};

} // namespace wiz
