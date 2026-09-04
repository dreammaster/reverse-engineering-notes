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

#include <vector>

namespace wiz {

enum : u8 { ATTR_NORMAL = 0, ATTR_INVERSE = 1 };

// DOS window-border glyphs -- CHARSET codes 1..8 (WIZARDRY proc 19 /
// CONUNIT sub_159A: rounded single-line box).  The emphasis style is 9..0x0E.
enum : u8 {
    BRD_TL = 1, BRD_TOP = 2, BRD_TR = 3, BRD_LEFT = 4,
    BRD_RIGHT = 5, BRD_BL = 6, BRD_BOT = 7, BRD_BR = 8,
};

class TextScreen {
public:
    static constexpr int kCols = 40, kRows = 24;

    TextScreen() { setupRoot(); }

    // --- framed windows (DOS WIZARDRY proc 19 / CONUNIT sub_159A) ---------
    // A bordered rectangle in absolute screen cells; the interior becomes
    // the active window and writes are relative to it.  DOS screens compose
    // 2-4 of these to fill the screen -- their borders line up at the edges.
    // `closeWindow` restores whatever the window covered.
    void openWindow(int x, int y, int w, int h, bool emphasis = false);
    void closeWindow();
    int  windowDepth() const { return int(frames_.size()) - 1; }

    // Paint just a border (glyphs 1..8, or 9..0x0E emphasised) into the cell
    // buffer -- no stack, no save/restore.  For screens that redraw wholesale
    // each frame and manage their own layout; `openWindow` is for transient
    // dialogs stacked over a persistent screen.
    void frame(int x, int y, int w, int h, bool emphasis = false);

    // --- window (viewport) -------------------------------------------------
    void setWindow(int x, int y, int w, int h);
    void resetWindow();      // active window := interior of the innermost frame

    // --- cursor / output (coords are window-relative) --------------------
    void gotoXY(int x, int y);
    void putChar(char c);                 // handles the control codes
    void write(const std::string &s);
    void writeln(const std::string &s = "");
    void writeField(const std::string &s, int width);   // right-justified, Pascal ': w'
    void writeCentered(const std::string &s, int row);

    void setAttr(u8 a) { attr_ = a; }
    u8 attr() const { return attr_; }

    // Absolute-position write (ignores the active window) -- for titles set
    // into a border row, DOS-style.
    void writeAt(int x, int y, const std::string &s);

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
    struct Frame {
        int x, y, w, h;
        bool border = true;
        bool emphasis = false;
        std::vector<char> savedC;        // cells covered when opened
        std::vector<u8>   savedA;
    };
    std::vector<Frame> frames_;          // frames_[0] = the (unbordered) screen

    struct { int x = 0, y = 0, w = kCols, h = kRows; } win_;
    bool in(int x, int y) const { return x >= 0 && y >= 0 && x < kCols && y < kRows; }
    int  absX() const { return win_.x + cx_; }
    int  absY() const { return win_.y + cy_; }
    void newline();
    void setupRoot();                    // clear + open the root frame
    void paintBorder(const Frame &f);
    void putGlyph(int x, int y, u8 g) { if (in(x, y)) { cell_[y][x] = char(g); attrCell_[y][x] = ATTR_NORMAL; } }

    char cell_[kRows][kCols];
    u8   attrCell_[kRows][kCols];
    int  cx_ = 0, cy_ = 0;            // window-relative
    u8   attr_ = ATTR_NORMAL;
};

} // namespace wiz
