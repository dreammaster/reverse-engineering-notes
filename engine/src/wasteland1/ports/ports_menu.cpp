// Register adapters: key driven menus (see menu.h).
#include "wasteland1/ports/port.h"
#include "wasteland1/globals.h"
#include "wasteland1/listmenu.h"
#include "wasteland1/menu.h"

namespace wl {
namespace rc {

PORT(chooseByKey) {                                                                     // 173B0
	const uint8_t at = chooseByKey(m);
	r.a.b.l = at; r.b.b.l = at;
	r.sf = at & 0x80; r.zf = at == 0;
}
PORT(pickGroupMember) { r.a.b.l = pickGroupMember(m, r.a.b.l != 0); r.cf = r.a.b.l == 0; }   // 1721B
PORT(askDirection) {                                                                    // 125BD
	const int d = askDirection(m);
	r.cf = d < 0;
	r.a.b.l = r.b.b.l = uint8_t(d);
}

PORT(chooseUseAction) {                                                                 // 13AE4: al = member
	uint8_t mode = 0, target = 0, what = 0;
	r.cf = !chooseUseAction(m, r.a.b.l, mode, target, what);
	r.a.b.l = mode; r.d.b.l = target; r.b.b.l = what;
}

PORT(listWindowSetup) { listWindowSetup(m); r.a.b.l = 0; }                              // 16EAA
PORT(pagedListDraw) { pagedListDraw(m); }                                               // 16DB4
PORT(pagedListBack) { pagedListBack(m); }                                               // 16E43
PORT(pagedListKeys) { const ListResult l = pagedListKeys(m); r.cf = l.cf; r.a.b.l = l.al; }   // 16D34
PORT(markPickedRow) { markPickedRow(m); }                                               // 16F20
PORT(skillList) {                                                                       // 1997B: al = title
	const ListResult l = skillList(m, r.a.b.l);
	r.cf = l.cf; r.a.b.l = l.al; if (l.empty) r.d.b.l = 1;
}
PORT(skillScreen) { const ListResult l = skillScreen(m); r.cf = l.cf; r.a.b.l = l.al; }   // 198F0
PORT(itemList) { const ListResult l = itemList(m, r.a.b.l); r.cf = l.cf; r.a.b.l = l.al; }   // 19454
PORT(itemScreen) { const ListResult l = itemScreen(m, r.a.b.l); r.cf = l.cf; r.a.b.l = l.al; }   // 19394

PORT(rearrangeParty) { rearrangeParty(m); }                                             // 12AE0

} // namespace rc
} // namespace wl
