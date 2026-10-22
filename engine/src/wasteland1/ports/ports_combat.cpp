// Register adapters: combat and encounter helpers.
#include "wasteland1/ports/port.h"
#include "wasteland1/combat.h"
#include "wasteland1/character.h"
#include "wasteland1/globals.h"
#include "wasteland1/party.h"

namespace wl {
namespace rc {

namespace {
uint8_t itemByte(Machine &m, uint8_t offset) { return m.d8(uint16_t(m.d16(g::itemDetailsPtr) + offset)); }
void setDlAl(Regs &r, uint16_t v) { r.a.b.l = uint8_t(v); r.d.b.l = uint8_t(v >> 8); }
}

PORT(selectMonsterDetails) {                                                            // 12A40: dl = member (falls into 12A4C)
	r.a.b.l = getMonsterDetails(m, encounterMemberId(m, r.d.b.l));
	r.b.b.l = itemByte(m, 7);
}
PORT(getMonsterDetails) { r.a.b.l = getMonsterDetails(m, r.a.b.l); r.b.b.l = itemByte(m, 7); }   // 12A4C: al = monster id
PORT(itemDamageDice) { r.a.b.l = itemDamageDice(m); }                                   // 12A8D
PORT(itemClipSizeNibble) { r.a.b.l = itemClipSizeNibble(m); }                           // 12A92
PORT(itemAmmunitionType) { r.a.b.l = itemAmmunitionType(m); }                           // 12A9D
PORT(itemArmorClassByte) { r.a.b.l = itemArmorClassByte(m); }                           // 12AA2
PORT(itemBytes01) { r.d.b.l = itemByte(m, 1); r.a.b.l = itemByte(m, 0); }               // 12AAB: al = byte 0, dl = byte 1
PORT(itemByte2) { r.a.b.l = itemByte(m, 2); r.d.b.l = 0; }                              // 12ABA
PORT(locationRecordBits2) { r.a.b.l = locationRecordBits2(m); }                         // 12AC5
PORT(locationRecordBits4) { r.a.b.l = locationRecordBits4(m); }                         // 15C08
PORT(markLocationVisited) { markLocationVisited(m); }                                   // 15C27

PORT(orderLow) { r.a.b.l = orderLow(m, r.b.b.l); }                                      // 13449: bl = member
PORT(orderHigh) { r.a.b.l = orderHigh(m, r.b.b.l); }                                    // 1353C
PORT(clearNumberValue) { for (int i = 0; i < 3; ++i) m.d8(uint16_t(g::numberValue + i)) = 0; r.a.b.l = 0; }   // 13561
PORT(selectSlotCellRecord) { selectSlotCellRecord(m); }                                  // 13787 (falls into 13797 and 1379E)
PORT(selectCellRecord) { selectCellRecord(m, r.d.b.l, r.b.b.l); }                       // 13797: dl = x, bl = y
PORT(saveLocationRecord) { saveLocationRecord(m, r.b.b.l, r.a.b.l); }                   // 1379E: bl = class, al = action
PORT(selectMember) { selectMember(m, r.a.b.l, r.d.b.l, r.b.b.l); }                     // 137CE: al = group, dl = slot, bl = member
PORT(selectSlot) { selectSlot(m, r.a.b.l, r.d.b.l); }                                   // 137F4: al = group, dl = slot
PORT(clearSlot) { clearSlot(m); }                                                       // 138B0
PORT(markReady) { markReady(m); r.a.b.l = 0x0F; }                                       // 1391E
PORT(selectPartyBlock) { selectPartyBlock(m, r.a.b.l); }                                // 13924: al = group
PORT(combatantIndex) { r.a.b.l = combatantIndex(r.a.b.l, r.d.b.l, r.b.b.l); }           // 13A37: al = group, dl = slot, bl = member -> al
PORT(splitCombatantIndex) {                                                             // 13A56: al = index -> al = group, dl = slot, bl = member
	const CombatantId id = splitCombatantIndex(r.a.b.l);
	r.a.b.l = id.group;
	r.d.b.l = id.slot;
	r.b.b.l = id.member;
	r.zf = id.group == 0;
}
PORT(combatReadiness) { r.a.b.l = combatReadiness(m, r.a.b.l); }                        // 13878: al = party number
PORT(useAmmunitionNone) { r.a.b.l = useAmmunition(m, 0); }                              // 138C1
PORT(useAmmunition) { r.a.b.l = useAmmunition(m, r.a.b.l); }                            // 138C3: al = rounds
PORT(setHitFlag) { r.a.b.l = setHitFlag(m, uint16_t(r.a.b.l | (r.d.b.l << 8))); }       // 15C4A: dl:al
PORT(typeGroupAOrB) { r.cf = !(isTypeGroupB(r.a.b.l) || isTypeGroupA(r.a.b.l)); }       // 15C68
PORT(typeGroupA) { r.cf = !isTypeGroupA(r.a.b.l); }                                     // 15C6E
PORT(typeGroupB) { r.cf = !isTypeGroupB(r.a.b.l); }                                     // 15C7E
PORT(typeGroupC) { r.cf = !isTypeGroupC(r.a.b.l); }                                     // 15C8C
PORT(weaponQuantityByte) {                                                              // 15C96: al = quantity byte (flags of test al, al)
	const uint8_t v = weaponQuantityByte(m);
	r.a.b.l = v;
	r.zf = v == 0;
	r.sf = (v & 0x80) != 0;
	r.cf = false;
}
PORT(weaponAmmunitionTypeFlags) {                                                       // 15CA4: al = ammunition type, ZF when none
	getEquippedWeaponDetails(m);
	r.a.b.l = itemByte(m, 7);
	r.zf = r.a.b.l == 0;
}
PORT(timesTen) { r.a.b.l = timesTen(r.b.b.l); }                                         // 15CB2: bl -> al
PORT(selectMonsterType) { selectMonsterType(m, r.a.b.l, r.d.b.l, r.b.b.l); r.a.b.l = m.d8(g::itemSexFlag); r.b.b.l = itemByte(m, 7); }   // 15CC0: al = group, dl = slot, bl = member
PORT(randomDigit) { r.a.b.l = randomDigit(m); r.b.b.l = uint8_t(r.a.b.l << 1); }        // 15CD1
PORT(luckBonus) { setDlAl(r, attributeBonus(m, pm::luck)); }                            // 15705
PORT(dexterityBonus) { setDlAl(r, attributeBonus(m, pm::dexterity)); }                  // 1570A
PORT(agilityBonus) { setDlAl(r, attributeBonus(m, pm::agility)); }                      // 1570F
PORT(strengthBonus) { setDlAl(r, attributeBonus(m, pm::strength)); }                    // 15714
PORT(countAliveMonsters) {                                                              // 157AD: al = group, dl = slot, bl = member -> CF set when none
	const uint8_t n = countAliveMonsters(m, r.a.b.l, r.d.b.l, r.b.b.l);
	r.a.b.l = n;
	r.cf = n == 0;
}

} // namespace rc
} // namespace wl
