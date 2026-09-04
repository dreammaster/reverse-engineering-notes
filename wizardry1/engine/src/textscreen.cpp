#include "wiz/textscreen.h"

#include <algorithm>

namespace wiz {

// ---- framed windows (DOS WIZARDRY proc 19 / CONUNIT sub_159A) -----------

void TextScreen::paintBorder(const Frame &f) {
    if (!f.border || f.w < 2 || f.h < 2) return;
    int x0 = f.x, y0 = f.y, x1 = f.x + f.w - 1, y1 = f.y + f.h - 1;
    u8 tl = BRD_TL, tp = BRD_TOP, tr = BRD_TR, lf = BRD_LEFT,
       rt = BRD_RIGHT, bl = BRD_BL, bt = BRD_BOT, br = BRD_BR;
    if (f.emphasis) { tl = 13; tp = 10; tr = 14; lf = 11; rt = 12; bl = 13; bt = 9; br = 14; }
    putGlyph(x0, y0, tl); putGlyph(x1, y0, tr);
    putGlyph(x0, y1, bl); putGlyph(x1, y1, br);
    for (int x = x0 + 1; x < x1; ++x) { putGlyph(x, y0, tp); putGlyph(x, y1, bt); }
    for (int y = y0 + 1; y < y1; ++y) { putGlyph(x0, y, lf); putGlyph(x1, y, rt); }
}

void TextScreen::frame(int x, int y, int w, int h, bool emphasis) {
    paintBorder(Frame{x, y, w, h, true, emphasis, {}, {}});
}

void TextScreen::setupRoot() {
    for (auto &row : cell_) std::fill(std::begin(row), std::end(row), ' ');
    for (auto &row : attrCell_) std::fill(std::begin(row), std::end(row), u8(ATTR_NORMAL));
    frames_.clear();
    frames_.push_back(Frame{0, 0, kCols, kRows, /*border*/false, false, {}, {}});
    resetWindow();
}

void TextScreen::resetWindow() {
    const Frame &f = frames_.back();
    if (f.border) setWindow(f.x + 1, f.y + 1, f.w - 2, f.h - 2);
    else          setWindow(f.x, f.y, f.w, f.h);
}

void TextScreen::openWindow(int x, int y, int w, int h, bool emphasis) {
    x = std::clamp(x, 0, kCols - 1);
    y = std::clamp(y, 0, kRows - 1);
    w = std::clamp(w, 2, kCols - x);
    h = std::clamp(h, 2, kRows - y);
    Frame f{x, y, w, h, /*border*/true, emphasis, {}, {}};
    f.savedC.reserve(size_t(w) * h);
    f.savedA.reserve(size_t(w) * h);
    for (int r = 0; r < h; ++r)
        for (int c = 0; c < w; ++c) {
            f.savedC.push_back(cell_[y + r][x + c]);
            f.savedA.push_back(attrCell_[y + r][x + c]);
        }
    for (int r = 1; r < h - 1; ++r)                        // blank interior
        for (int c = 1; c < w - 1; ++c) {
            cell_[y + r][x + c] = ' ';
            attrCell_[y + r][x + c] = ATTR_NORMAL;
        }
    paintBorder(f);
    frames_.push_back(std::move(f));
    resetWindow();
}

void TextScreen::closeWindow() {
    if (frames_.size() < 2) return;                        // never pop the root
    Frame f = std::move(frames_.back());
    frames_.pop_back();
    for (int r = 0; r < f.h; ++r)
        for (int c = 0; c < f.w; ++c) {
            cell_[f.y + r][f.x + c] = f.savedC[size_t(r) * f.w + c];
            attrCell_[f.y + r][f.x + c] = f.savedA[size_t(r) * f.w + c];
        }
    resetWindow();
}

void TextScreen::setWindow(int x, int y, int w, int h) {
    const Frame &f = frames_.back();
    int ix = f.x, iy = f.y, iw = f.w, ih = f.h;                   // interior
    if (f.border) { ix += 1; iy += 1; iw -= 2; ih -= 2; }
    win_.x = std::clamp(x, ix, ix + iw);
    win_.y = std::clamp(y, iy, iy + ih);
    win_.w = std::clamp(w, 0, ix + iw - win_.x);
    win_.h = std::clamp(h, 0, iy + ih - win_.y);
    cx_ = cy_ = 0;
}

void TextScreen::gotoXY(int x, int y) {
    cx_ = std::clamp(x, 0, std::max(0, win_.w - 1));
    cy_ = std::clamp(y, 0, std::max(0, win_.h - 1));
}

void TextScreen::newline() {
    cx_ = 0;
    if (++cy_ >= win_.h) {                 // scroll the window up one row
        cy_ = win_.h - 1;
        for (int r = 0; r < win_.h - 1; ++r)
            for (int c = 0; c < win_.w; ++c) {
                cell_[win_.y + r][win_.x + c] = cell_[win_.y + r + 1][win_.x + c];
                attrCell_[win_.y + r][win_.x + c] = attrCell_[win_.y + r + 1][win_.x + c];
            }
        for (int c = 0; c < win_.w; ++c) {
            cell_[win_.y + win_.h - 1][win_.x + c] = ' ';
            attrCell_[win_.y + win_.h - 1][win_.x + c] = ATTR_NORMAL;
        }
    }
}

void TextScreen::putChar(char c) {
    switch ((unsigned char)c) {
        case 12: clearWindow(); return;                 // FF  clear + home
        case 11: clearToEndOfScreen(); return;          // VT
        case 29: clearToEndOfLine(); return;            // GS
        case 13: cx_ = 0; return;                       // CR
        case 10: newline(); return;                     // LF
        case 8:  if (cx_ > 0) --cx_; return;            // BS
        case 7:  return;                                // BEL
    }
    if (cx_ >= win_.w) newline();
    int ax = absX(), ay = absY();
    if (in(ax, ay)) { cell_[ay][ax] = c; attrCell_[ay][ax] = attr_; }
    ++cx_;
}

void TextScreen::write(const std::string &s) { for (char c : s) putChar(c); }

void TextScreen::writeAt(int x, int y, const std::string &s) {
    for (size_t i = 0; i < s.size(); ++i)
        if (in(x + int(i), y)) {
            cell_[y][x + int(i)] = s[i];
            attrCell_[y][x + int(i)] = attr_;
        }
}
void TextScreen::writeln(const std::string &s) { write(s); putChar('\r'); putChar('\n'); }

void TextScreen::writeField(const std::string &s, int width) {
    for (int i = int(s.size()); i < width; ++i) putChar(' ');
    write(s);
}

void TextScreen::writeCentered(const std::string &s, int row) {
    gotoXY(std::max(0, (win_.w - int(s.size())) / 2), row);
    write(s);
}

void TextScreen::clear() {
    for (auto &row : cell_) std::fill(std::begin(row), std::end(row), ' ');
    for (auto &row : attrCell_) std::fill(std::begin(row), std::end(row), u8(ATTR_NORMAL));
    for (const Frame &f : frames_) paintBorder(f);          // frames persist
    cx_ = cy_ = 0;
}

void TextScreen::clearWindow() {
    for (int r = 0; r < win_.h; ++r)
        for (int c = 0; c < win_.w; ++c) {
            cell_[win_.y + r][win_.x + c] = ' ';
            attrCell_[win_.y + r][win_.x + c] = ATTR_NORMAL;
        }
    cx_ = cy_ = 0;
}

void TextScreen::clearRect(int x, int y, int w, int h) {
    for (int r = 0; r < h; ++r)
        for (int c = 0; c < w; ++c) {
            int ax = win_.x + x + c, ay = win_.y + y + r;
            if (in(ax, ay)) { cell_[ay][ax] = ' '; attrCell_[ay][ax] = ATTR_NORMAL; }
        }
}

void TextScreen::clearToEndOfLine() {
    for (int c = cx_; c < win_.w; ++c) {
        int ax = win_.x + c, ay = absY();
        if (in(ax, ay)) { cell_[ay][ax] = ' '; attrCell_[ay][ax] = ATTR_NORMAL; }
    }
}

void TextScreen::clearToEndOfScreen() {
    clearToEndOfLine();
    for (int r = cy_ + 1; r < win_.h; ++r)
        for (int c = 0; c < win_.w; ++c) {
            cell_[win_.y + r][win_.x + c] = ' ';
            attrCell_[win_.y + r][win_.x + c] = ATTR_NORMAL;
        }
}

void TextScreen::render(Surface &s, const Font &font, u8 fg, u8 bg,
                        u8 invFg, u8 invBg) const {
    if (s.width() < kCols * Font::kW || s.height() < kRows * Font::kH)
        s.resize(kCols * Font::kW, kRows * Font::kH);
    s.fill(bg);
    for (int r = 0; r < kRows; ++r)
        for (int c = 0; c < kCols; ++c) {
            bool inv = attrCell_[r][c] == ATTR_INVERSE;
            int px = c * Font::kW, py = r * Font::kH;
            if (inv) s.fillRect(px, py, Font::kW, Font::kH, invBg);
            font.drawGlyph(s, (unsigned char)cell_[r][c], px, py,
                           inv ? invFg : fg);
        }
}

} // namespace wiz
