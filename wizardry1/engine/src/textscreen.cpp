#include "wiz/textscreen.h"

#include <algorithm>

namespace wiz {

void TextScreen::setWindow(int x, int y, int w, int h) {
    win_.x = std::clamp(x, 0, kCols);
    win_.y = std::clamp(y, 0, kRows);
    win_.w = std::clamp(w, 0, kCols - win_.x);
    win_.h = std::clamp(h, 0, kRows - win_.y);
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
