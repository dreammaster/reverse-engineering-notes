#include "wiz/console.h"

#include <algorithm>
#include <cstring>

namespace wiz {

Console::Console() { init(); }

void Console::loadCharset(const u8 *data, size_t n) {
    charset_.assign(data, data + std::min<size_t>(n, 320 * 16));
    charset_.resize(320 * 16, 0);
}

void Console::loadMonsters(std::vector<u8> file) { monsters_ = std::move(file); }

// ---- CONUNIT selector 1 -------------------------------------------------------
void Console::init() {
    prio_.fill(0);
    cell_.fill(0);
    vram_.fill(0);
}

// ---- pixels ---------------------------------------------------------------------
void Console::put8(int si, const u8 *rows16) {
    int a = vaddr(si);
    for (int r = 0; r < 8; ++r) {
        int o = a + (r & 1) * 0x2000 + (r >> 1) * 80;
        vram_[o] = rows16[2 * r];
        vram_[o + 1] = rows16[2 * r + 1];
    }
}

void Console::blank(int si) {                               // loc_1E62 (word_1411)
    cell_[si] = 0;
    static const u8 zero[16] = {};
    put8(si, zero);
}

// sub_1882: the cell word picks the drawing routine
void Console::draw(int si, u16 ax) {
    int ah = ax >> 8, al = ax & 0xFF;
    if (ah == 0) {
        if (al == 0 || al == 0x20) { blank(si); return; }
        if (!charset_.empty()) put8(si, &charset_[size_t(al) * 16]);
    } else if (ah == 0x20) {
        blank(si);
    } else if (ah >= 0x80) {                                // solid block
        static const u8 solid[16] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        put8(si, solid);
    } else {                                                // monster tile (loc_1E9A)
        int tile = (ah & 0x1F) - 1;
        int grp = ((ah >> 4) & 6) >> 1;
        if (tile >= 0 && tile < 30) put8(si, &rec_[grp][size_t(tile) * 16]);
    }
}

// sub_1853: store the word, redraw only if it visibly changed
void Console::write1853(int si, u16 ax) {
    u16 old = cell_[si];
    cell_[si] = ax;
    u16 a = (old == 0x2020 || old == 0x20) ? 0 : old;
    u16 b = (ax == 0x2020 || ax == 0x20) ? 0 : ax;
    if (a != b) draw(si, ax);
}

// sub_1844
void Console::write1844(const Win &w, int si, u16 ax) {
    u8 bl = w.prio, p = prio_[si];
    if (bl == p) {
    } else if (bl < p) {
        return;
    } else {
        prio_[si] = bl;
    }
    write1853(si, ax);
}

// ---- layout (selector 2) --------------------------------------------------------
// sub_1674 / sub_16A9: one border cell
void Console::putBorder(const Win &w, int si, u16 g) {
    u8 dl = w.prio, dh = prio_[si] & 0x3F;
    if (dl < dh) return;
    if (dl > dh) prio_[si] = dl;
    u16 old = cell_[si];
    if (g == old) return;
    cell_[si] = g;
    u8 cl = 0x80;
    int ch = old >> 8;
    if (ch != 0 && ch < 0x80) cl = 0xC0;
    prio_[si] |= cl;
}

// sub_16F2
void Console::interior(const Win &w) {
    int si0 = w.y * kCols + w.x + kCols + 1;
    int iw = w.iw(), ih = w.ih(), i = 0;
    for (int r = 0; r < ih; ++r)
        for (int c = 0; c < iw; ++c) {
            int si = si0 + r * kCols + c;
            u16 ax = w.cell[size_t(i++)];
            u8 al = w.prio;
            if (al < (prio_[si] & 0x3F)) continue;
            prio_[si] = al;
            u16 dx = cell_[si];
            if (ax == dx) continue;
            cell_[si] = ax;
            prio_[si] |= 0x80;
            int dh = dx >> 8;
            if (dh != 0 && dh < 0x80 && dh != 0x20) prio_[si] |= 0x40;
        }
}

// sub_175D
void Console::flush() {
    for (int si = 0; si < kCells; ++si) {
        u8 p = prio_[si];
        if (p == 0) blank(si);
        else if (p & 0x80) { prio_[si] = p & 0x3F; draw(si, cell_[si]); }
    }
}

static void borderCells(const Win &w, const u16 g[8], std::vector<std::pair<int, u16>> &out) {
    int start = w.y * Console::kCols + w.x;
    out.emplace_back(start, g[0]);
    for (int i = 0; i < w.w - 2; ++i) out.emplace_back(start + 1 + i, g[1]);
    out.emplace_back(start + w.w - 1, g[2]);
    for (int i = 0; i < w.h - 2; ++i) {
        out.emplace_back(start + Console::kCols * (1 + i), g[3]);
        out.emplace_back(start + Console::kCols * (1 + i) + w.w - 1, g[4]);
    }
    int bl = start + Console::kCols * (w.h - 1);
    out.emplace_back(bl, g[5]);
    for (int i = 0; i < w.w - 2; ++i) out.emplace_back(bl + 1 + i, g[6]);
    out.emplace_back(bl + w.w - 1, g[7]);
}

void Console::layout() {
    prio_.fill(0);
    std::vector<std::pair<int, u16>> cells;
    for (Win *w : list_) {
        if (w->hidden()) continue;
        const u16 *g = (w->prio == 0x3F) ? kEmphBorder : kStdBorder;
        cells.clear();
        borderCells(*w, g, cells);
        for (auto &c : cells) putBorder(*w, c.first, c.second);
        interior(*w);
    }
    flush();
}

// ---- present one window (selector 3) ------------------------------------------------
void Console::present(Win *w) {
    if (!w || w->hidden()) return;
    std::vector<std::pair<int, u16>> cells;
    borderCells(*w, kStdBorder, cells);
    for (auto &c : cells) write1844(*w, c.first, c.second);
    int si0 = w->y * kCols + w->x + kCols + 1;
    int iw = w->iw(), ih = w->ih(), i = 0;
    for (int r = 0; r < ih; ++r)
        for (int c = 0; c < iw; ++c) {
            int si = si0 + r * kCols + c;
            u16 ax = w->cell[size_t(i++)];
            if (w->prio < prio_[si]) continue;
            prio_[si] = w->prio;
            write1853(si, ax);
        }
}

// ---- putchar / putstring (4, 30): wraps, never scrolls ----------------------------------
void Console::putChar(Win *w, u16 word) {
    int iw = w->iw(), ih = w->ih();
    int cx = w->cx, cy = w->cy;
    w->cell[size_t(cy) * iw + cx] = word;
    if (!w->hidden()) {
        int si = (cy + w->y + 1) * kCols + (cx + w->x + 1);
        if (w->prio >= prio_[si]) {
            prio_[si] = w->prio;
            write1853(si, word);
        }
    }
    if (++cx == iw) {
        cx = 0;
        if (++cy == ih) cy = 0;
    }
    w->cx = u8(cx);
    w->cy = u8(cy);
}

void Console::putString(Win *w, const std::string &s) {
    for (unsigned char c : s) putChar(w, c);
}

// ---- DRAWLINE (selector 5, 0x18C1) ---------------------------------------------------------
void Console::drawLine(Win *w, DrawLineArgs &a) {
    int dl = w->iw(), dh = w->ih();
    int lo = a.clipLo & 0xFF, hi = a.clipHi & 0xFF;
    int col = a.col & 0xFF, row = a.row & 0xFF;
    int dcol = a.dCol & 0xFF, drow = a.dRow & 0xFF;
    long idx = long(row * dl) + a.col;           // cell index of (row, col)
    int step = 0;
    if (drow) {
        step = dl;
        if (drow & 0x80) step = -step;
    }
    step = (step + dcol) & 0xFF;
    if (step & 0x80) step -= 256;
    unsigned count = a.count;
    int al = col, ah = row;
    for (;;) {
        if (al >= lo && al <= hi && idx >= 0 && idx < long(w->cell.size()))
            w->cell[size_t(idx)] = a.glyph;
        idx += step;
        al = (al + dcol) & 0xFF;
        if (al & 0x80) break;
        ah = (ah + drow) & 0xFF;
        if (ah & 0x80) break;
        if (al == dl || ah == dh) break;
        count = (count - 1) & 0xFFFF;
        if (count == 0) break;
    }
    a.count = u16(count);
    a.dRow = u16(step * 2);
}

// ---- portraits (selectors 13, 17) -------------------------------------------------------------
void Console::clearAttr(Win *w) {
    for (size_t i = 0; i < 0x2D0 && i < w->cell.size(); ++i) w->cell[i] &= 0x00FF;
}

void Console::blit13(Win *w, const u8 pics[4]) {
    static const int kCols1[] = {15};
    static const int kCols2[] = {11, 19};
    static const int kCols3[] = {7, 15, 23};
    static const int kCols4[] = {3, 11, 19, 27};
    clearAttr(w);
    std::vector<int> valid;
    for (int k = 0; k < 4; ++k)
        if (pics[k] != 0xFF) valid.push_back(pics[k]);
    const int *cols = nullptr;
    switch (valid.size()) {
        case 1: cols = kCols1; break;
        case 2: cols = kCols2; break;
        case 3: cols = kCols3; break;
        case 4: cols = kCols4; break;
        default: break;
    }
    for (size_t k = 0; cols && k < valid.size(); ++k) {
        int base = 324 + cols[k];                   // row 9
        int al = int(k) << 5;
        for (int row = 0; row < 5; ++row)
            for (int cc = 0; cc < 6; ++cc) {
                ++al;
                size_t i = size_t(base + row * 36 + cc);
                if (i < w->cell.size()) w->cell[i] = u16((w->cell[i] & 0x00FF) | ((al & 0xFF) << 8));
            }
    }
    for (auto &r : rec_) r.fill(0);
    for (size_t k = 0; k < valid.size(); ++k) {
        size_t off = size_t(valid[k] - 1) * 512;
        for (size_t i = 0; i < 512 && off + i < monsters_.size(); ++i) rec_[k][i] = monsters_[off + i];
    }
    for (int si = 0; si < kCells; ++si) {           // force the tiles to redraw
        int hb = cell_[si] >> 8;
        if (hb != 0 && hb < 0x80 && hb != 0x20) cell_[si] = u16((cell_[si] & 0xFF) | 0x1F00);
    }
}

// ---- title picture (selector 23): raw scanlines straight into VRAM --------------------------------
void Console::titleBitmap(const u8 *file, size_t n) {
    int a0 = vaddr(2 * kCols + 2);
    for (int i = 0; i < 28; ++i)
        for (int bank = 0; bank < 2; ++bank) {
            int o = a0 + bank * 0x2000 + i * 80;
            size_t src = 4 + size_t(i * 2 + bank) * 80;
            for (int b = 0; b < 70; ++b)
                vram_[size_t(o + b)] = (file && src + b < n) ? file[src + b] : 0;
        }
}

// ---- window manager ---------------------------------------------------------------------------------
// proc 68: one bubble pass -> descending priority (hidden bit ignored)
void Console::sortWins() {
    for (size_t i = 0; i + 1 < list_.size(); ++i)
        if ((list_[i]->prio & 0x7F) < (list_[i + 1]->prio & 0x7F)) std::swap(list_[i], list_[i + 1]);
}

Win *Console::open(int x, int y, int w, int h, int prio, bool refresh) {   // proc 19
    auto win = std::make_unique<Win>();
    win->x = u8(x); win->y = u8(y); win->w = u8(w); win->h = u8(h);
    win->prio = u8(prio + 1);
    win->cell.assign(size_t(std::max(0, (w - 2) * (h - 2))), 0x2020);
    Win *p = win.get();
    owned_.push_back(std::move(win));
    list_.insert(list_.begin(), p);
    sortWins();
    if (refresh) layout();
    return p;
}

void Console::close(Win *&w, bool refresh) {                               // proc 20
    if (!w || list_.empty()) return;
    list_.erase(std::remove(list_.begin(), list_.end(), w), list_.end());
    owned_.erase(std::remove_if(owned_.begin(), owned_.end(),
                                [&](const std::unique_ptr<Win> &o) { return o.get() == w; }),
                 owned_.end());
    w = nullptr;
    if (refresh) layout();
}

void Console::setPrio(Win *w, int prio, bool refresh) {                    // proc 21
    w->prio = u8(prio + 1);
    if (!list_.empty() && w != list_.front()) {
        list_.erase(std::remove(list_.begin(), list_.end(), w), list_.end());
        list_.insert(list_.begin(), w);
    }
    sortWins();
    if (refresh) layout();
}

void Console::clear(Win *w, bool refresh) {                                // proc 25
    std::fill(w->cell.begin(), w->cell.end(), u16(0x2020));
    w->cx = w->cy = 0;
    if (refresh) layout(); else present(w);
}

void Console::gotoXY(Win *w, int x, int y) {                               // proc 22
    if (x < 0) x = 0;
    else if (x >= w->w - 2) { x = 0; y += 1; }
    if (y < 0) y = 0;
    else if (y >= w->h - 2) y = 0;
    w->cx = u8(x);
    w->cy = u8(y);
}

void Console::newline(Win *w) {                                            // proc 24
    w->cy = u8(w->cy + 1);
    w->cx = 0;
    if (w->cy >= w->h - 2) w->cy = 0;
}

void Console::clearRect(Win *w, int x, int y, int ww, int hh) {            // proc 26
    auto clampX = [&](int &v) { if (v < 0) v = 0; else if (v > w->w - 3) v = w->w - 3; };
    auto clampY = [&](int &v) { if (v < 0) v = 0; else if (v > w->h - 3) v = w->h - 3; };
    clampX(x); clampY(y);
    int x2 = x + ww - 1, y2 = y + hh - 1;
    clampX(x2); clampY(y2);
    for (int r = y; r <= y2; ++r)
        for (int c = x; c <= x2; ++c) w->at(c, r) = 0x2020;
    present(w);
}

void Console::hide(Win *w, bool refresh) {                                 // proc 27
    w->prio = u8(128 + (w->prio % 128));
    if (refresh) layout();
}

void Console::show(Win *w, bool refresh) {                                 // proc 28
    w->prio = u8(w->prio % 128);
    if (refresh) layout();
}

// ---- output -----------------------------------------------------------------------------------------------
const Color *Console::palette() {
    // CGA mode 5 (BIOS palette): black, cyan, red, white at low intensity
    static const Color pal[4] = {{0, 0, 0}, {0, 170, 170}, {170, 0, 0}, {170, 170, 170}};
    return pal;
}

void Console::render(Surface &s) const {
    if (s.width() != 320 || s.height() != 200) s.resize(320, 200);
    for (int y = 0; y < 200; ++y) {
        int base = (y & 1) * 0x2000 + (y >> 1) * 80;
        for (int x = 0; x < 320; ++x) {
            int b = vram_[size_t(base + (x >> 2))];
            s.set(x, y, u8((b >> (6 - 2 * (x & 3))) & 3));
        }
    }
}

std::array<u16, Console::kCells> Console::visible() const {
    std::array<u16, kCells> out{};
    for (int si = 0; si < kCells; ++si) {
        u16 c = cell_[si];
        if ((prio_[si] & 0x3F) == 0 || c == 0x2020 || c == 0x0020) c = 0;
        out[si] = c;
    }
    return out;
}

u32 Console::vramCrc() const {
    static u32 table[256];
    static bool init = false;
    if (!init) {
        for (u32 i = 0; i < 256; ++i) {
            u32 c = i;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            table[i] = c;
        }
        init = true;
    }
    u32 crc = 0xFFFFFFFFu;
    for (u8 b : vram_) crc = table[(crc ^ b) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

} // namespace wiz
