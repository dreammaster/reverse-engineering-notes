// Register adapters: map cell access.
#include "wasteland1/ports/port.h"
#include "wasteland1/globals.h"
#include "wasteland1/mapcells.h"

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

} // namespace rc
} // namespace wl
