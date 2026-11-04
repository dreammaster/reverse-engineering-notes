// Register adapters for the tiny entry points that sit between the larger routines of WL.EXE (each is a few instructions that the
// bigger functions fall into or are called from): they only move values between the data segment and the registers.
#include "wasteland1/ports/port.h"
#include "wasteland1/character.h"
#include "wasteland1/globals.h"
#include "wasteland1/mapcells.h"
#include "wasteland1/party.h"
#include "wasteland1/roster.h"
#include "wasteland1/text.h"

namespace wl {
namespace rc {

PORT(rosterLocationOn) { m.d8(g::rosterLocOn) = 1; r.a.b.l = 1; }                       // 125B1
PORT(rosterLocationOff) { m.d8(g::rosterLocOn) = 0; r.a.b.l = 0; }                      // 125B7
PORT(textSpeedOn) { m.d8(g::textSpeedFlag) = 1; r.a.b.l = 1; }                           // 12728
PORT(textSpeedOff) { m.d8(g::textSpeedFlag) = 0; r.a.b.l = 0; }                          // 1272E
PORT(loadA4AE) { r.a.b.l = m.d8(0xA4AF); r.d.b.l = m.d8(0xA4AE); }                       // 12955
PORT(loadA5D7) { r.d.b.l = m.d8(0xA5D8); r.b.b.l = m.d8(0xA5D7); }                       // 13EC0
PORT(markMemberRow) { redrawRosterRow(m, m.d8(0xA5E1), 0xFF); }                          // 13C52
PORT(npcOfCurrent) { r.b.b.l = 41; r.a.b.l = m.d8(uint16_t(m.d16(g::currentCharacter) + 41)); r.zf = r.a.b.l == 0; r.di = m.d16(g::currentCharacter); }   // 13E32
PORT(selectNewCharacter) { setCurrentRosterCharacter(m, uint8_t(m.d8(uint16_t(g::saved2 + g::s2::field8)) + 1)); }   // 13435
PORT(monsterStepIndex) { r.d.b.l = r.a.b.l; r.b.b.l = m.d8(uint16_t(0xA622 + r.a.b.l)); }   // 14B93
PORT(addPendingX) { r.a.b.l = uint8_t(m.d8(0xA63F) + m.d8(0xA641)); m.d8(0xA63F) = r.a.b.l; r.d.b.l = r.a.b.l; }   // 14F43
PORT(addPendingY) { r.a.b.l = uint8_t(m.d8(0xA640) + m.d8(0xA642)); m.d8(0xA640) = r.a.b.l; r.b.b.l = r.a.b.l; }   // 14F50
PORT(loadPendingFromSlot) {                                                             // 14F67
	const uint16_t slot = m.d16(g::monsterPtr);
	m.d8(0xA63F) = m.d8(slot);
	m.d8(0xA640) = m.d8(uint16_t(slot + 1));
	r.b.b.l = 1;
	r.a.b.l = m.d8(0xA640);
}
PORT(loadA632) { r.d.b.l = m.d8(0xA633); r.b.b.l = m.d8(0xA632); }                       // 1514B
PORT(loadActionPair) {                                                                  // 15939 (bl = offset)
	const uint16_t c = m.d16(g::currentCharacter);
	m.d8(0xA69C) = m.d8(uint16_t(c + r.b.b.l));
	++r.b.b.l;
	r.a.b.l = m.d8(uint16_t(c + r.b.b.l));
	m.d8(0xA69D) = r.a.b.l;
}
PORT(slotHasMonsters) {                                                                 // 15B9C
	const uint16_t slot = m.d16(g::monsterPtr);
	const uint8_t at = m.d8(0xA671);
	r.b.b.l = uint8_t(at + 1);
	r.a.b.l = uint8_t(m.d8(uint16_t(slot + at)) | m.d8(uint16_t(slot + at + 1)));
	r.zf = r.a.b.l == 0;
}
PORT(restoreRosterFlag) { r.a.b.l = m.d8(0xA6C1); m.d8(g::rosterLocOn) = r.a.b.l; }       // 1634F
PORT(copyB265) { r.a.x = m.d16(0xB265); m.setD16(0x4661, r.a.x); }                        // 16376
PORT(loadStepResult) { r.d.b.l = m.d8(0xAA4B); r.b.b.l = m.d8(0xAA4A); }                  // 164D7
PORT(decBl) { r.b.b.l = uint8_t(r.b.b.l - 1); }                                           // 16508
PORT(incBl) { r.b.b.l = uint8_t(r.b.b.l + 1); }                                           // 1650D
PORT(decDl) { r.d.b.l = uint8_t(r.d.b.l - 1); }                                           // 16512
PORT(incDl) { r.d.b.l = uint8_t(r.d.b.l + 1); }                                           // 16517
PORT(loadAA3E) { r.d.b.l = m.d8(0xAA3F); r.b.b.l = m.d8(0xAA3E); }                        // 16837
PORT(loadAA3C) {                                                                        // 16880
	setPartyIndexesPtr(m, m.d8(0x4654));
	r.cf = false;
	r.d.b.l = m.d8(0xAA3C);
	r.b.b.l = m.d8(0xAA3B);
}
PORT(loadAA38) { r.d.b.l = m.d8(0xAA39); r.b.b.l = m.d8(0xAA38); }                        // 169A8
PORT(changeCellAtParty) {                                                               // 169B1 (al = index)
	r.cf = changeCellFromLocation(m, r.a.b.l, m.d8(0x46A6), m.d8(0x46A7));
	r.d.b.l = m.d8(0x46A6);
	r.b.b.l = m.d8(0x46A7);
}
PORT(selectActiveGroupLeaves) { setPartyIndexesPtr(m, m.d8(0x4654)); r.cf = true; }       // 15DC4
PORT(setPendingFlags) { m.d8(0xAAD3) = 0; m.d8(0xAAD4) = 1; r.d.b.l = 1; r.cf = true; }   // 16B09
PORT(textTableAB3E) { m.setD16(0x4692, 0xAB3E); r.d.x = 0; }                              // 16CBD

PORT(act_aa3d) { r.cf = true; }                                                           // 1AA3D

} // namespace rc
} // namespace wl
