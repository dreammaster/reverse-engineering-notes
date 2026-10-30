// Register adapters: key driven menus (see menu.h).
#include "wasteland1/ports/port.h"
#include "wasteland1/globals.h"
#include "wasteland1/combat.h"
#include "wasteland1/combatmenu.h"
#include "wasteland1/listmenu.h"
#include "wasteland1/menu.h"
#include "wasteland1/roster.h"

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

PORT(drawRosterRow) { drawRosterRow(m); }                                               // 1708B
PORT(redrawRosterRowHere) { redrawRosterRow(m, r.a.b.l, 0xFF); }                          // 17029: al = position, inverse text
PORT(redrawRosterRowPlain) { redrawRosterRow(m, r.a.b.l, 0); }                            // 1702E
PORT(redrawRosterRowKeep) { redrawRosterRow(m, r.a.b.l, m.d8(0x471F)); }                  // 17033
PORT(drawRosterFrame) { drawRosterFrame(m); }                                           // 16F70

PORT(drawRosterHeadersAndMembers) { drawRosterHeadersAndMembers(m); }                  // 1728C (al is preserved)
PORT(showTitleText) { showTitleText(m, r.a.b.l); }                                      // 17923

PORT(countGroupsInRange) { r.a.b.l = countGroupsInRange(m); }                              // 1382B
PORT(showTargetList) { showTargetList(m, r.a.b.l); }                                    // 12262
PORT(chooseTargetGroup) {                                                               // 1236F
	uint8_t group = 0;
	r.cf = !chooseTargetGroup(m, group);
	r.a.b.l = group;
}

namespace {
void setResult(Regs &r, const CommandResult &c) { r.cf = c.cf; r.a.b.l = c.al; }
}
PORT(collectOrders) { r.cf = collectOrders(m); }                                         // 11F76
PORT(commandPause) { setResult(r, commandPause(m, r.a.b.l)); }                          // 1208D
PORT(commandUseItem) { setResult(r, commandUseItem(m, r.a.b.l)); }                      // 1234E
PORT(commandMove) { setResult(r, commandMove(m, r.a.b.l)); }                            // 123A6
PORT(commandReload) { setResult(r, commandReload(m, r.a.b.l)); }                        // 123E2
PORT(commandUseSkill) { setResult(r, commandUseSkill(m, r.a.b.l)); }                    // 1240B
PORT(commandAttack) { setResult(r, commandAttack(m, r.a.b.l)); }                        // 120C1
PORT(pickFirstTarget) { setResult(r, pickFirstTarget(m)); }                             // 121A8

} // namespace rc
} // namespace wl
