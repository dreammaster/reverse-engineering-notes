// SEGMENT CAMP -- see camp_dos.h.
#include "wiz/camp_dos.h"

namespace wiz {

// CAMP:3
void Camp::row(int i) {
    const Character &c = w_.party.member(i);
    Win *g = w_.g10;
    d_.gotoXY(g, 0, i + 1);
    d_.putNum(g, i + 1, 1);
    d_.putC(g, ' ');
    d_.putS(g, c.name);
    while (g->cx < 18) d_.putC(g, ' ');
    d_.putS(g, d_.str(10009 + int(c.align)).substr(0, 1));
    d_.putC(g, '-');
    d_.putS(g, d_.str(10030 + int(c.cls)).substr(0, 3));
    d_.putC(g, ' ');
    int ac = c.armorClass - w_.protect;
    if (ac > -10) d_.putNum(g, ac, 2);
    else d_.putStr(g, 601);
    d_.putNum(g, c.hpLeft, 5);
    d_.putC(g, ' ');
    if (c.status == Status::OK) {
        if (c.poison != 0) d_.putStr(g, 602);
        else d_.putNum(g, c.hpMax, 4);
    } else {
        d_.putS(g, d_.str(10040 + int(c.status)).substr(0, 6));
    }
    while (g->cx != 0) d_.putC(g, ' ');
}

// CAMP:1
CampResult Camp::run(int xgoto) {
    CampResult res;
    for (int i = 0; i < w_.party.count(); ++i) row(i);
    if (!title_) title_ = d_.boxMsg(652, 0, 1, 1);
    for (;;) {
        d_.partyCount = w_.party.count();
        std::string spec = d_.str(653);
        Win *m = d_.centerBox(d_.menuWidth(spec), 4, 6, 1, true);
        int sel = d_.menu(m, spec);
        d_.con.close(m, false);
        if (d_.quit) { res.windowClosed = true; return res; }
        if (sel >= 0 && sel <= 5) {                  // #)INSPECT: the character sheet (not ported)
            res.inspect = true;
            res.member = sel;
            res.xgoto = 12;
            return res;
        }
        if (sel == 6) { res.xgoto = 15; return res; }   // R)EORDER
        if (sel == 7) { res.xgoto = 14; return res; }   // E)QUIP
        if (sel == 8) {                                 // L)EAVE
            d_.con.refresh();
            closeTitle();
            w_.g41 = 23;
            res.xgoto = 20;
            return res;
        }
    }
}

} // namespace wiz
