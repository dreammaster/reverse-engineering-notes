// SCNMSG -- SPECIALS' scripted maze messages (SPCMISC / DOMSG, DOS SPECIALS
// procs 10 / 14 / 16).  See docs/maze.md.
//
// A maze cell whose SQRETYPE is ScnMsg carries a descriptor with
//   AUX2 = subtype   (ScnMsgKind)
//   AUX1 = message number
//   AUX0 = a fire counter (kinds 1/4/8) or a payload (e.g. the item index
//          for kind 2)
//
// The text lives in ASCII.KRN, one string per line, at
//   key = 15000 + 50*msgNo + line
// and runs until the first absent key (DOS GetStr answers "**ERR**").
// A leading '@' or '^' centres the line, '$' forces it left; both are
// stripped.
#pragma once
#include "wiz/string_pool.h"
#include <string>
#include <vector>

namespace wiz {

// WIZ1 only uses kinds 1/2/4/5 plus one 8 and one 9 (see `maze-scan`);
// 3/6/7/10/11 never appear in this scenario.
enum ScnMsgKind {
    SCN_NONE     = 0,   // nothing
    SCN_PLAIN    = 1,   // just show the text  (AUX0 = fire count, <0 = always)
    SCN_GIVE     = 2,   // text + give item AUX0 to the first able member (TRYGET)
    SCN_WADE     = 3,   // text + WHOWADE   (status worsening)   -- absent in WIZ1
    SCN_YESNO    = 4,   // text + Y/N -> search (fight AUX0 / give |AUX0|)  (GETYN)
    SCN_NEEDITEM = 5,   // ITM2PASS: pass iff a member holds AUX0, else BOUNCEBK
    SCN_ALIGN    = 6,   // CHKALIGN  (no DOMSG)                  -- absent in WIZ1
    SCN_CHKAUX0  = 7,
    SCN_BACKSHOP = 8,   // text + bounce to a shop              -- not handled (1 sq)
    SCN_LOOKOUT  = 9,   //                                      -- not handled (1 sq)
    SCN_RIDDLE   = 10,  // text + a typed answer                -- absent in WIZ1
    SCN_FEE      = 11,  // text + pay a fee                     -- absent in WIZ1
};

inline int scnMsgKey(int msgNo, int line) { return 15000 + 50 * msgNo + line; }

struct ScnLine { std::string text; bool center = false; };

// Gather a scripted message's lines (prefixes stripped).
inline std::vector<ScnLine> scnMsgLines(const StringPool &sp, int msgNo,
                                        int maxLines = 80) {
    std::vector<ScnLine> out;
    for (int i = 0; i < maxLines; ++i) {
        bool ok = false;
        std::string s = sp.get(scnMsgKey(msgNo, i), &ok);
        if (!ok || s == "**ERR**") break;
        ScnLine ln;
        if (!s.empty() && (s[0] == '@' || s[0] == '^')) { ln.center = true; s.erase(0, 1); }
        else if (!s.empty() && s[0] == '$') s.erase(0, 1);
        ln.text = std::move(s);
        out.push_back(std::move(ln));
    }
    return out;
}

// SPCMISC's AUX0 gate.  Only kinds 1 / 4 / 8 carry a fire counter in AUX0
// (other kinds put a payload there and always fire).  Given how many times
// the descriptor has already fired this session, may it fire again?
//   kind 1/8:  0 dead; N>0 fires N times then the square goes NORMAL;
//              N<0 persistent.
//   kind 4:    0 dead; N>0 persistent (never decremented); -1000<N<0 one-shot;
//              N<=-1000 persistent.
inline bool scnMsgMayFire(int kind, int aux0, int fired) {
    if (kind != SCN_PLAIN && kind != SCN_YESNO && kind != SCN_BACKSHOP)
        return true;
    if (aux0 == 0) return false;
    if (kind == SCN_YESNO)
        return (aux0 > 0 || aux0 <= -1000) ? true : fired < 1;
    return aux0 > 0 ? fired < aux0 : true;
}

inline bool scnMsgCounts(int kind) {
    return kind == SCN_PLAIN || kind == SCN_YESNO || kind == SCN_BACKSHOP;
}

} // namespace wiz
