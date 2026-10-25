// The DOS screen: a port of CONUNIT (the native console driver in
// SYSTEM.INTERP) plus the WIZARDRY window manager that sits on it.
//
// Not a text grid.  The original keeps a list of *window records* and
// composes them by priority into a 40x25 cell screen (8x8 px cells, CGA
// 320x200x4); everything the game shows is a window, a border glyph, or a
// cell-glyph wireframe.  Every routine here is a transcription of the
// handler it names -- see docs/drawing.md for the map and tools/wizvm.py for
// the executable reference this was checked against.
#pragma once
#include "wiz/types.h"
#include "wiz/surface.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace wiz {

// A window record (WIZARDRY:19 builds it on the Pascal heap):
//   +2 x  +3 y  +4 w  +5 h  (outer rectangle, border included)
//   +6 cursor x  +7 cursor y (interior coordinates)  +8 priority  +A cells.
// `prio` is the *stored* byte: logical priority + 1, bit 7 = hidden.
struct Win {
    u8 x = 0, y = 0, w = 0, h = 0;
    u8 cx = 0, cy = 0;
    u8 prio = 0;
    std::vector<u16> cell;           // (w-2)*(h-2) words, lo = glyph, hi = attribute

    int iw() const { return w - 2; }
    int ih() const { return h - 2; }
    u16 &at(int col, int row) { return cell[size_t(row) * iw() + col]; }
    bool hidden() const { return (prio & 0x80) != 0; }
};

// The eight-word argument record DRAWLINE (CONUNIT selector 5) takes.
struct DrawLineArgs {
    u16 clipLo = 0, clipHi = 0;      // only columns clipLo..clipHi are written
    u16 col = 0, row = 0;
    u16 dCol = 0, dRow = 0;          // low bytes are signed steps
    u16 count = 0;                   // 0 = run until the window edge
    u16 glyph = 0;
};

class Console {
public:
    static constexpr int kCols = 40, kRows = 25, kCells = kCols * kRows;
    static constexpr int kVram = 0x4000;       // CGA B800:0000, two 8 KB banks

    // glyph codes of the line-art bank (CHARSET 1..0x0E)
    static constexpr u16 kStdBorder[8]  = {1, 2, 3, 4, 5, 6, 7, 8};
    static constexpr u16 kEmphBorder[8] = {0x0D, 0x0A, 0x0E, 0x0B, 0x0C, 0x0E, 0x09, 0x0D};

    Console();

    // ---- assets -----------------------------------------------------------
    // 200.CHARSET: 320 glyphs x 16 bytes (8 rows x 2 bytes, 2 bpp)
    void loadCharset(const u8 *data, size_t n);
    // 200.MONSTERS: 512-byte tile records, record (pic-1)
    void loadMonsters(std::vector<u8> file);

    // ---- CONUNIT --------------------------------------------------------------
    void init();                                  // selector 1: clear everything
    void layout();                                // 2: compose all windows + flush
    void present(Win *w);                         // 3: redraw one window now
    void putChar(Win *w, u16 word);               // 4: store+draw at the cursor
    void putString(Win *w, const std::string &s); // 30
    void drawLine(Win *w, DrawLineArgs &a);       // 5
    void clearAttr(Win *w);                       // 17
    void blit13(Win *w, const u8 pics[4]);        // 13: portrait tiles
    void titleBitmap(const u8 *file, size_t n);   // 23 (null = clear)

    // ---- WIZARDRY window manager (procs 16-21, 25-28, 37, 67, 68) -------------
    Win *open(int x, int y, int w, int h, int prio, bool refresh);       // 19
    void close(Win *&w, bool refresh);                                   // 20
    void setPrio(Win *w, int prio, bool refresh);                        // 21
    void clear(Win *w, bool refresh);                                    // 25
    void clearRect(Win *w, int x, int y, int ww, int hh);                // 26
    void hide(Win *w, bool refresh);                                     // 27
    void show(Win *w, bool refresh);                                     // 28
    void refresh() { layout(); }                                         // 37
    void gotoXY(Win *w, int x, int y);                                   // 22
    void newline(Win *w);                                                // 24
    const std::vector<Win *> &windows() const { return list_; }

    // ---- output ---------------------------------------------------------------------
    // 320x200, indices 0..3 (black, cyan, red, white -- CGA mode 5)
    void render(Surface &s) const;
    static const Color *palette();               // 4 entries
    // visible cell word per screen cell, 0 = blank (for regression dumps)
    std::array<u16, kCells> visible() const;
    u32 vramCrc() const;
    u8 priorityAt(int cell) const { return prio_[cell]; }

private:
    void putBorder(const Win &w, int si, u16 g);
    void interior(const Win &w);
    void flush();
    void draw(int si, u16 ax);
    void blank(int si);
    void write1853(int si, u16 ax);
    void write1844(const Win &w, int si, u16 ax);
    void put8(int si, const u8 *rows16);
    int vaddr(int si) const { return (si / kCols) * 320 + (si % kCols) * 2; }
    void sortWins();

    std::vector<Win *> list_;                     // head first (descending priority)
    std::vector<std::unique_ptr<Win>> owned_;
    std::array<u8, kCells> prio_{};
    std::array<u16, kCells> cell_{};
    std::array<u8, kVram> vram_{};
    std::vector<u8> charset_;
    std::vector<u8> monsters_;
    std::array<std::array<u8, 512>, 4> rec_{};
};

} // namespace wiz
