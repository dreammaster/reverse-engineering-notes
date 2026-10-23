// Register adapters: combat and encounter helpers.
#include "wasteland1/ports/port.h"
#include "wasteland1/combat.h"
#include "wasteland1/character.h"
#include "wasteland1/globals.h"
#include "wasteland1/party.h"
#include "wasteland1/rules.h"
#include "wasteland1/text.h"

namespace wl {
namespace rc {

namespace {
uint8_t locByteRaw(Machine &m, uint8_t offset) { return m.d8(uint16_t(m.d16(g::savedLocationDetailsPtr) + offset)); }
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
PORT(locationRecordBits2) {
	const uint8_t v = locByteRaw(m, 9);
	r.a.b.l = uint8_t(v >> 2);
	r.cf = (v >> 1) & 1;
	r.zf = r.a.b.l == 0;
	r.sf = false;
}                         // 12AC5
PORT(locationRecordBits4) { r.a.b.l = locationRecordBits4(m); }                         // 15C08
PORT(markLocationVisited) { markLocationVisited(m); }                                   // 15C27

PORT(orderLow) { r.a.b.l = orderLow(m, r.b.b.l); }                                      // 13449: bl = member
PORT(orderHigh) {
	const uint8_t v = m.d8(uint16_t(m.d16(g::partyOrdersPtr) + r.b.b.l));
	r.a.b.l = uint8_t(v >> 6);
	r.cf = (v >> 5) & 1;
	r.zf = r.a.b.l == 0;
	r.sf = false;
}                                    // 1353C
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
PORT(useAmmunitionNone) { r.a.b.l = useAmmunition(m, 0, &r.zf); }                              // 138C1
PORT(useAmmunition) { r.a.b.l = useAmmunition(m, r.a.b.l, &r.zf); }                            // 138C3: al = rounds
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

// ---- combat messages ------------------------------------------------------------------------------------------------------
PORT(printLocationMessage) { printLocationMessage(m, r.b.b.l); }                        // 1393E: bl = index
PORT(printLocationMessageVariant) { printLocationMessageVariant(m, r.a.b.l); }          // 13939: al = base
PORT(randomPlusOne) { r.a.b.l = uint8_t(r.a.b.l + Rules(m).random(2)); }                // 1394D: al = al + random(2)
PORT(printDamageNumber) { printDamageNumber(m, uint16_t(r.a.b.l | (r.d.b.l << 8))); }   // 15A84: dl:al
PORT(printHitMessage) { printHitMessage(m, r.a.b.l); }                                  // 15C36: al
PORT(printAttackMessageA) { printAttackMessageA(m); }                                   // 15BBC
PORT(printAttackMessageB) { printAttackMessageB(m); }                                   // 15BCB
PORT(printAttackMessageC) { printAttackMessageC(m); }                                   // 15BEF
PORT(printMessage2D) { printMessage(m, 0x2D); }                                         // 15BFE
PORT(printMessage61) { printMessage(m, 0x61); }                                         // 15C03
PORT(setHitPointBytes) { setHitPointBytes(m, r.a.b.l); }                                // 1592C
PORT(weaponDamageClass) { r.b.b.l = weaponDamageClass(r.a.b.l); }                       // 156DE: al = type -> bl
PORT(rollMonsterDamage) { setDlAl(r, rollMonsterDamage(m, r.a.b.l, r.d.b.l, r.b.b.l)); }   // 15672
PORT(recordWordToNumber) { recordWordToNumber(m); r.a.b.l = 0; r.b.b.l = 1; }           // 15A18
PORT(reduceByMonsterArmor) { setDlAl(r, reduceByMonsterArmor(m, uint16_t(r.a.b.l | (r.d.b.l << 8)), r.b.b.l)); }   // 1597E: dl:al damage, bl = combatant
PORT(monsterExperience) { monsterExperience(m, r.a.b.l); }                              // 159C7: al = combatant
PORT(defenseRoll) { setDlAl(r, defenseRoll(m, r.a.b.l, r.d.b.l)); }                      // 15755: al = party member, dl = threshold
PORT(attackStrength) { setDlAl(r, attackStrength(m, r.a.b.l, r.d.b.l)); }                // 15738
PORT(hitMonster) {                                                                      // 15500: al = party member, bl = combatant -> al, CF
	const HitResult h = hitMonster(m, r.a.b.l, r.b.b.l);
	r.cf = h.alreadyDead;
	r.a.b.l = h.alreadyDead ? r.a.b.l : (h.killed ? 0 : 0xFF);
}
PORT(weaponSkillRoll) { weaponSkillRoll(m, r.a.b.l); }                                  // 15A30: al = combatant
PORT(attackMonster) {                                                                   // 15A9D: al = combatant, bl = monster number
	bool killed;
	const bool alive = attackMonster(m, r.a.b.l, r.b.b.l, killed);
	r.cf = !alive;
	r.a.b.l = killed ? 1 : 0;
}
PORT(writeMonsterName) { writeMonsterName(m, r.d.b.l); }                                // 129E9: dl = group of monsters (0..2)
PORT(printMonsterMessage43) { printMonsterMessage(m, 0x43, r.d.b.l); }                  // 129D5: dl = group of monsters
PORT(printMonsterMessage1A) { printMonsterMessage(m, 0x1A, r.d.b.l); }                  // 129DA
PORT(writeEncounterGroup) { r.cf = !writeEncounterGroup(m, r.a.b.l); }                   // 13697: al = group; CF when empty
PORT(describeEncounter) { describeEncounter(m, r.a.b.l); }                              // 13651: al = distance
PORT(setSlotStatusOne) { setSlotStatus(m, r.d.b.l, 1); }                                // 13726: dl = slot
PORT(setSlotStatusZero) { setSlotStatus(m, r.d.b.l, 0); }                               // 1372B
PORT(monsterDamage) { setDlAl(r, monsterDamage(m, r.d.b.l)); }                           // 12A76: dl = group -> dl:al
PORT(monsterHitsParty) { monsterHitsParty(m, r.a.b.l, r.d.b.l, r.b.b.l); r.cf = false; }   // 1295D: al = a, dl = b, bl = group
PORT(anyMemberAlive) { r.cf = !anyMemberAlive(m); }                                     // 129B6
PORT(adjustMemberField) { adjustMemberField(m, r.a.b.l, r.d.b.l, r.b.b.l); }            // 141FA: al = member, dl = offset, bl = flags
PORT(actionByte8) {                                                                     // 142D7
	r.a.b.l = actionByte8(m);
	r.zf = r.a.b.l == 0;
	r.sf = false;
}
PORT(actionByte0) {                                                                     // 142E2
	r.a.b.l = actionByte0(m);
	r.zf = r.a.b.l == 0;
	r.sf = (r.a.b.l & 0x80) != 0;
}
PORT(monsterRangeModifier) { r.a.b.l = monsterRangeModifier(m, r.a.b.l, r.d.b.l); r.d.b.l = 0; }   // 156CB: al = distance, dl = group
PORT(ambush) { ambush(m, r.a.b.l); }                                                    // 12783
PORT(partyAttackRound) { partyAttackRound(m, r.a.b.l); }                                // 12D3F: al = party group
PORT(fieldCheck) { r.cf = !fieldCheck(m, r.a.b.l, r.b.b.l); }                           // 1820C: al = difficulty, bl = field offset
PORT(entrySlot) {                                                                       // 14B9C: al = entry offset
	const EntrySlot s = entrySlot(r.a.b.l);
	r.a.b.l = s.group;
	r.d.b.l = s.slot;
	r.zf = s.group == 0;
}
PORT(entrySlotCurrent) {                                                                // 14659: entry offset from ds:A619
	const EntrySlot s = entrySlot(m.d8(0xA619));
	r.a.b.l = s.group;
	r.d.b.l = s.slot;
	r.zf = s.group == 0;
}
PORT(findEntrySlot) {                                                                   // 14B36: bl = entry offset -> al = group, dl = slot, CF found
	EntrySlot s{0, 0};
	r.cf = findEntrySlot(m, r.b.b.l, s);
	r.a.b.l = r.cf ? s.group : 4;
	r.d.b.l = r.cf ? s.slot : 4;
}
PORT(findEntry) {                                                                       // 14AEA: al = party id, dl = x, bl = y
	uint8_t entry = 0;
	r.cf = findEntry(m, r.a.b.l, r.d.b.l, r.b.b.l, entry);
	const uint8_t x = r.d.b.l, y = r.b.b.l;
	r.b.b.l = r.cf ? uint8_t(entry * 4) : 0x40;
	if (!r.cf) {   // what the scan of the last entry leaves behind
		uint8_t al = m.d8(0xA9AB);
		if (al != 0 && al == x) { al = m.d8(0xA9AC); if (al == y) al = m.d8(0xA9AD); }
		r.a.b.l = al;
		r.d.b.l = 0x10;
		r.zf = true;
	}
	if (r.cf) { r.a.b.l = uint8_t(entry >> 2); r.d.b.l = entry; r.zf = r.a.b.l == 0; }
}
PORT(swapEntrySlot) { swapEntrySlot(m, r.a.b.l, r.d.b.l, r.b.b.l); }                  // 14BAD: al = group, dl = slot, bl = entry offset
PORT(placeTrackedMonsters) { placeTrackedMonsters(m); }                                 // 14480
PORT(unjam) { unjam(m, r.a.b.l, r.b.b.l); }                                             // 19ACD: al = party member, bl = weapon offset
PORT(equipOrder) { equipOrder(m, r.a.b.l); }                                            // 130B1
PORT(printMessage52) { printMessage(m, 0x52); }                                         // 13223
PORT(reloadOrder) { reloadOrder(m, r.a.b.l); }                                          // 13228
PORT(executeOrders) { executeOrders(m, r.a.b.l); }                                      // 12CC3: al = party group
PORT(forgetTrackedGroup) { forgetTrackedGroup(m); r.a.b.l = 1; }                        // 14AAA
PORT(recordTrackedGroup) { recordTrackedGroup(m, r.b.b.l); r.a.b.l = 1; }               // 14A88: bl = entry offset
PORT(continuePendingMove) { r.cf = !continuePendingMove(m); }                           // 14AC7
PORT(updateSlotDistances) { updateSlotDistances(m); }                                    // 136C7
PORT(outsideView) { r.cf = outsideView(m, r.d.b.l, r.b.b.l); }                          // 169CF: dl = x, bl = y
PORT(updateGroupDistances) { updateGroupDistances(m); }                                  // 149F7
PORT(scanForMonsters) { r.cf = scanForMonsters(m); }                                     // 14664
PORT(combatRound) { combatRound(m, r.a.b.l); }                                          // 1AD73
PORT(partyAtCell) {                                                                     // 16840: dl = x, bl = y
	const uint8_t x = r.d.b.l, y = r.b.b.l;
	r.cf = partyAtCell(m, x, y);
	r.d.b.l = x;
	r.b.b.l = y;
}
PORT(monsterCellBlocked) { r.cf = monsterCellBlocked(m, r.d.b.l, r.b.b.l); }            // 14F26
PORT(monsterStepDirection) { uint8_t d = 0; r.cf = !monsterStepDirection(m, d); if (!r.cf) r.a.b.l = d; }   // 14ECB
PORT(monsterApproachTowards) { uint8_t d = 0; r.cf = !monsterApproach(m, false, d); if (!r.cf) r.a.b.l = d; }   // 14E4A
PORT(monsterApproachAway) { uint8_t d = 0; r.cf = !monsterApproach(m, true, d); if (!r.cf) r.a.b.l = d; }   // 14E45
PORT(averageMonsterHitPoints) { averageMonsterHitPoints(m); }                           // 14FDE
PORT(averagePartyHitPoints) { averagePartyHitPoints(m); }                               // 14F9D
PORT(monsterHoldsBack) { r.cf = monsterHoldsBack(m, r.d.b.l); }                         // 14F7A: dl = shift
PORT(moveMonsters) { moveMonsters(m); }                                                 // 15036
PORT(planMonsterMoves) { planMonsterMoves(m); }                                          // 14BF0
PORT(scanForMonstersQuiet) { r.cf = scanForMonsters(m, false); }                         // 1465F
PORT(restoreLocationAtSlot) { r.cf = restoreLocationAtSlot(m, r.a.b.l); }                // 13762
PORT(updateSlotSurvivors) { r.cf = updateSlotSurvivors(m, r.a.b.l, r.d.b.l); }          // 135C6: al = group, dl = slot
PORT(updateAllSlotSurvivors) { updateAllSlotSurvivors(m); }                             // 13580
PORT(setItemQuantityOne) { setItemQuantityOne(m); r.a.b.l = 1; }                       // 17AB7
PORT(memberTargetsAlive) { const uint8_t n = memberTargetsAlive(m, r.b.b.l); r.a.b.l = n; r.cf = n == 0; }   // 13440: bl = party member
PORT(targetMembersAlive) { const uint8_t n = targetMembersAlive(m); r.a.b.l = n; r.cf = n == 0; }           // 13079
PORT(printWeaponNote) { printWeaponNote(m); }                                           // 1306C
PORT(countHitMonsters) { r.d.b.l = countHitMonsters(m); r.a.b.l = r.d.b.l; }            // 13086
PORT(cellDistance) { r.a.b.l = cellDistance(m, r.d.b.l, r.b.b.l); }                    // 19D4D: dl = x, bl = y
PORT(burstHits) { r.a.b.l = burstHits(m, r.a.b.l); r.d.b.l = 0; }                      // 1595C: al = n
PORT(burstSize) { r.a.b.l = burstSize(m, r.a.b.l); r.d.b.l = 0; }                      // 15956: al = party member
PORT(rollToFire) { r.cf = rollToFire(m, r.a.b.l); }                                     // 134B2: al = party member
PORT(rangeModifier) { r.a.b.l = rangeModifier(m, r.a.b.l, r.d.b.l, r.b.b.l); }          // 139CE: al = distance, dl = ammo flag, bl = class
PORT(attackDifficulty) { r.a.b.l = attackDifficulty(m, r.a.b.l); r.d.b.l = 0; }          // 1567F: al = party member
PORT(partyMemberAttack) { partyMemberAttack(m, r.a.b.l); }                              // 12D84: al = party member
PORT(damagePartyMember) { r.cf = damagePartyMember(m, uint16_t(r.a.b.l | (r.d.b.l << 8)), r.b.b.l); }   // 157D6: dl:al damage, bl = member

} // namespace rc
} // namespace wl
