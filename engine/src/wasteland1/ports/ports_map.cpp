// Register adapters: map cell access.
#include "wasteland1/ports/port.h"
#include "wasteland1/globals.h"
#include "wasteland1/mapcells.h"
#include "wasteland1/movement.h"

namespace wl {
namespace rc {

PORT(tileClassAt) { r.a.b.l = tileClassAt(m, r.d.b.l, r.b.b.l); }                // 17C20: dl = x, bl = y -> al
PORT(shr4) { r.a.b.l = uint8_t(r.a.b.l >> 4); }                                  // 17C69
PORT(selectActionRow) { selectActionRow(m, r.b.b.l); }                           // 17C72: bl = y
PORT(selectLocationRecord) { selectLocationRecord(m, r.b.b.l, r.a.b.l); }        // 17CB1: bl = table, al = index
PORT(cellAt) {                                                                   // 17CD2: dl = x, bl = y -> al = class, dl = action
	const Cell c = cellAt(m, r.d.b.l, r.b.b.l);
	r.a.b.l = c.tileClass;
	r.d.b.l = c.action;
}
PORT(actionAt) { r.d.b.l = r.a.b.l = actionAt(m, r.d.b.l, r.b.b.l); }            // 17CEB: -> dl = action
PORT(changeCellFromLocation) { r.cf = changeCellFromLocation(m, r.a.b.l, r.d.b.l, r.b.b.l); }   // 17CFF: al = index, dl = x, bl = y
PORT(applyCellChange) { r.cf = applyCellChange(m, r.d.b.l, r.b.b.l); }           // 17D50: dl = x, bl = y
PORT(setTileClass) { setTileClass(m, r.d.b.l, r.b.b.l, r.a.b.l); }               // 17D7A: al = class, dl = x, bl = y
PORT(storeViewPosition) {                                                        // 17DBB: -> dl, bl
	storeViewPosition(m);
	const XY p = partyMapPosition(m);
	r.d.b.l = p.x;
	r.b.b.l = r.a.b.l = p.y;
}
PORT(partyMapPosition) {                                                         // 17DC7: -> dl = x, bl = y
	const XY p = partyMapPosition(m);
	r.d.b.l = p.x;
	r.b.b.l = r.a.b.l = p.y;
}
PORT(toViewPosition) {                                                           // 17DE0: dl = x, bl = y -> dl, bl
	const XY p = toViewPosition(m, r.d.b.l, r.b.b.l);
	r.d.b.l = p.x;
	r.b.b.l = r.a.b.l = p.y;
}

PORT(drawMapCell) {                                                                     // 18024: dl = x, bl = y
	drawMapCell(m, r.d.b.l, r.b.b.l);
	r.d.b.l = m.d8(g::rectTextX);
	r.b.b.l = m.d8(g::rectTextY);
}

PORT(redrawViewColumn) { redrawViewColumn(m); }                                          // 16619
PORT(redrawViewRow) { redrawViewRow(m); }                                               // 16646
PORT(scrollViewUp) { scrollViewUp(m); }                                                 // 16584
PORT(scrollViewLeft) { scrollViewLeft(m); }                                             // 165AC
PORT(scrollViewRight) { scrollViewRight(m); }                                           // 165D2
PORT(scrollViewDown) { scrollViewDown(m); }                                             // 165FB
PORT(redrawViewObjects) { redrawViewObjects(m); }                                       // 167CE
PORT(partyCell) { uint8_t x, y; partyCell(m, x, y); r.d.b.l = x; r.b.b.l = y; r.a.b.l = y; }   // 17DC7
PORT(selectCellLocation) { r.a.b.l = selectCellLocation(m, r.d.b.l, r.b.b.l); }          // 169EB
PORT(drawOtherParties) { drawOtherParties(m); }                                         // 16428
PORT(drawPartyMarker) { drawPartyMarker(m); }                                           // 16716
PORT(redrawViewAll) { redrawViewAll(m); }                                               // 163A0
PORT(drawStatusBar) { drawStatusBar(m); }                                               // 17DF1
PORT(lineBlocked) { r.cf = lineBlocked(m, r.d.b.l, r.b.b.l); }                          // 14330: dl = x, bl = y
PORT(drawPartyCell) { drawPartyCell(m); }                                               // 1675C

PORT(advanceTime) { advanceTime(m); }                                                   // 1676A
PORT(cellBlockedByObject) { r.cf = cellBlockedByObject(m, r.d.b.l, r.b.b.l); }          // 13E9B: dl = x, bl = y
PORT(confirmGate) { r.cf = confirmGate(m, r.d.b.l, r.b.b.l); }                          // 16AD5
PORT(applyStepOnChange) { r.cf = applyStepOnChange(m, r.d.b.l, r.b.b.l); }              // 15CE0
PORT(stepBlocked) {                                                                     // 1649E: al = direction
	r.cf = stepBlocked(m, r.a.b.l);
	r.d.b.l = m.d8(0xAA4B);
	r.b.b.l = m.d8(0xAA4A);
}
PORT(refreshViewLayout) { refreshViewLayout(m); }                                       // 169BC
PORT(moveParty) { r.cf = !moveParty(m, r.a.b.l); r.b.x = 0; r.d.x = 0; }                 // 1652D
PORT(tryStep) { r.cf = tryStep(m, r.a.b.l); r.b.x = 0; r.d.x = 0; }                      // 1651A

} // namespace rc
} // namespace wl
