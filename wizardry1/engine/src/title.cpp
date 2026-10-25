#include "wiz/title.h"

#include <algorithm>

namespace wiz {

// KANJIREA:14 -- GotoXY(BACKGND, 0, row) then `count` centred strings from #first
static void titleLines(Dos &d, Win *dlg, int row, int first, int count) {
    d.gotoXY(dlg, 0, row);
    for (int n = first; n < first + count; ++n) d.putStrC(dlg, n);
}

// KANJIREA:15 -- the portraits dissolve downward, one row at a time
static void dissolve(Dos &d) {
    Win *w1 = d.window1;
    int iw = w1->iw();
    for (int r = 9; r <= 13; ++r) {
        for (int row = 13; row >= r + 1; --row)
            std::copy(w1->cell.begin() + size_t(iw) * (row - 1), w1->cell.begin() + size_t(iw) * row,
                      w1->cell.begin() + size_t(iw) * row);
        std::fill(w1->cell.begin() + size_t(iw) * r, w1->cell.begin() + size_t(iw) * (r + 1), u16(0x2020));
        d.con.present(w1);
    }
}

TitleResult runTitle(Dos &d, Rng &rng, const std::vector<u8> &titleFile) {
    Console &con = d.con;
    Win *dlg = con.open(0, 18, 40, 7, 30, true);                   // BACKGND
    con.clear(d.window1, true);
    con.titleBitmap(titleFile.data(), titleFile.size());           // CONUNIT selector 23
    d.gotoXY(d.window1, 0, 7);
    d.putStrC(d.window1, 2018);
    titleLines(d, dlg, 0, 2019, 2);
    titleLines(d, dlg, 3, 2021, 2);
    std::string startKey = d.str(2021), makeKey = d.str(2022);
    for (;;) {
        while (!d.keyReady()) {
            u8 pics[4] = {0xFF, 0xFF, 0xFF, 0xFF};
            con.clearAttr(d.window1);
            con.present(d.window1);
            int n = rng.mod(4);
            for (int k = 0; k <= n; ++k) pics[k] = u8(4 + rng.mod(30 - 4));
            con.blit13(d.window1, pics);
            con.present(d.window1);
            for (int i = 0; i < 30 && !d.keyReady(); ++i) d.pause(2);
            dissolve(d);
        }
        d.getKey();
        if (d.quit) return TitleResult::Quit;
        if (d.inchar == (unsigned char)startKey[0] || d.inchar == (unsigned char)makeKey[0]) break;
    }
    con.close(dlg, true);
    bool make = d.inchar == (unsigned char)makeKey[0];
    con.titleBitmap(nullptr, 0);
    return make ? TitleResult::MakeDisk : TitleResult::Start;
}

} // namespace wiz
