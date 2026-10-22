// Register adapters: inventory, equipment, money, experience, hit points and the skill check accumulators.
#include "wasteland1/ports/port.h"
#include "wasteland1/character.h"
#include "wasteland1/globals.h"
#include "wasteland1/party.h"
#include "wasteland1/rules.h"
#include "wasteland1/text.h"

namespace wl {
namespace rc {

namespace {
uint16_t curChar(Machine &m) { return m.d16(g::currentCharacter); }
}

// ---- inventory and equipment ----------------------------------------------------------------------------------------
PORT(removeItem) {   // 19AB6: bl = offset of the item id byte
	removeItem(m, r.b.b.l);
	r.a.b.l = 0;
	r.b.b.l = uint8_t(r.b.b.l + 1);
	r.di = curChar(m);
}
PORT(getInventoryItemIdPtr) { r.a.b.l = getInventoryItemOffset(r.a.b.l); }   // 19AC8: al = slot -> al = offset of the item id
PORT(getMemberWeaponIdPtr) {
	r.b.b.l = r.a.b.l = equippedItemOffset(m, pm::weapon);
	r.di = curChar(m);
	r.zf = r.a.b.l == 0;
	r.sf = (r.a.b.l & 0x80) != 0;
}   // 196C9
PORT(getMemberArmorIdPtr) {
	r.b.b.l = r.a.b.l = equippedItemOffset(m, pm::armor);
	r.di = curChar(m);
	r.zf = r.a.b.l == 0;
	r.sf = (r.a.b.l & 0x80) != 0;
}     // 196C4
PORT(getEquippedWeaponDetails) { getEquippedWeaponDetails(m); }   // 196B2
PORT(getArmorAC) { r.a.b.l = getArmorClass(m); r.b.b.l = 6; r.di = m.d16(g::itemDetailsPtr); }   // 19A0F
PORT(equip) { equip(m, r.a.b.l); r.di = curChar(m); }                         // 1949E: al = inventory slot
PORT(removeWeaponArmor) {
	const uint8_t slot = r.a.b.l;
	const bool weapon = slot == m.d8(uint16_t(curChar(m) + pm::weapon)), armor = !weapon && slot == m.d8(uint16_t(curChar(m) + pm::armor));
	removeWeaponArmor(m, slot);
	r.b.b.l = weapon ? pm::weapon : armor ? pm::armorClass : pm::armor;
	if (weapon || armor) r.a.b.l = 0;
	r.di = curChar(m);
} // 194E8: al = inventory slot
PORT(reload) { reload(m, r.b.b.l); }                       // 196DB: bl = offset of the ammunition's item id
PORT(hasNoAmmunition) { r.cf = hasNoAmmunition(m, r.a.b.l); }   // 19D2F

// ---- money, experience ---------------------------------------------------------------------------------------------------
PORT(resetMoney) { setMember24(m, pm::money, r.a.b.l); r.b.b.l = pm::money + 2; r.di = curChar(m); }   // 198BC: al = value written to all three bytes
PORT(getMemberMoney) { getMemberMoney(m); r.a.b.l = m.d8(uint16_t(g::numberValue + 2)); r.b.b.l = pm::money + 2; r.di = curChar(m); }   // 19A9C
PORT(addMoneyToMember) {
	addMoneyToMember(m);
	r.a.b.l = m.d8(uint16_t(curChar(m) + pm::money + 2));   // the byte last written (FFh after saturation)
	r.b.b.l = pm::money + 2;
	r.di = curChar(m);
}            // 19895
PORT(getMemberExperience) { getMemberExperience(m); r.a.b.l = m.d8(uint16_t(g::numberValue + 2)); r.b.b.l = pm::experience + 2; r.di = curChar(m); }   // 19B67
PORT(addExperience) {
	addExperience(m);
	r.a.b.l = m.d8(uint16_t(curChar(m) + pm::experience + 2));
	r.b.b.l = pm::experience + 2;
	r.di = curChar(m);
}                  // 19BC0
PORT(canAfford) { r.cf = canAfford(m); }                   // 17B3E
PORT(pay) { r.cf = pay(m); }                               // 17B15
PORT(getItemPrice) { getItemPrice(m); r.a.b.l = 0; r.b.b.l = 1; r.di = m.d16(g::itemDetailsPtr); }   // 17B5C

// ---- hit points ----------------------------------------------------------------------------------------------------------
PORT(applyDamage) {   // 19CE9: dl:al = damage
	const uint16_t con = uint16_t(m.d8(uint16_t(curChar(m) + pm::con)) | (m.d8(uint16_t(curChar(m) + pm::con + 1)) << 8));
	const uint16_t damage = uint16_t(r.a.b.l | (r.d.b.l << 8));
	r.a.b.l = applyDamage(m, damage);
	r.c.x = uint16_t(con - damage);
	r.b.b.l = pm::con + 1;
	r.di = curChar(m);
	r.zf = r.c.x == 0;
	r.sf = (r.c.x & 0x8000) != 0;
	r.cf = false;
}
PORT(healOneHitPoint) { r.cf = healOneHitPoint(m); }       // 19CFE
PORT(isAtFullHealth) { r.zf = isAtFullHealth(m); r.di = curChar(m); r.b.b.l = 0x1C; r.a.b.l = m.d8(uint16_t(curChar(m) + pm::con + 1)); }         // 19A41
PORT(countConsciousMembers) { r.a.b.l = countConsciousMembers(m); r.zf = r.a.b.l == 0; }   // 19D0E

// ---- skill check accumulators (seed1 = target, seed2 = roll; 16 bit values passed in dl:al) ---------------------------
namespace {
uint16_t dlal(const Regs &r) { return uint16_t(r.a.b.l | (r.d.b.l << 8)); }
void setDlAl(Regs &r, uint16_t v) { r.a.b.l = uint8_t(v); r.d.b.l = uint8_t(v >> 8); }
}
PORT(resetRandomSeed2) { r.a.b.l = r.d.b.l = 0; Rules(m).setSeed2(0); }   // 19BEC
PORT(setRandomSeed2) { Rules(m).setSeed2(dlal(r)); }       // 19BF0
PORT(resetRandomSeed1) { r.a.b.l = r.d.b.l = 0; Rules(m).setSeed1(0); }   // 19BF8
PORT(setRandomSeed1) { Rules(m).setSeed1(dlal(r)); }       // 19BFC
PORT(addRoll) {                                            // 19C04: seed2 += signed dl:al, clamped to 0..FFFFh
	Rules q(m);
	q.addRoll(int16_t(dlal(r)));
	setDlAl(r, q.seed2());
}
PORT(addTarget) {                                          // 19C2C
	Rules q(m);
	q.addTarget(int16_t(dlal(r)));
	setDlAl(r, q.seed1());
}
PORT(rollMinusTarget) {                                    // 19C54: ax = seed2 - seed1, dl = ah
	Rules q(m);
	const uint16_t v = uint16_t(q.seed2() - q.seed1());
	r.a.x = v;
	r.d.b.l = r.a.b.h;
	r.cf = q.seed2() < q.seed1();
}
PORT(rollBeatsTarget) {
	Rules q(m);
	r.a.x = q.seed2();
	r.cf = q.seed2() >= q.seed1();
	r.zf = q.seed2() == q.seed1();
	r.sf = ((q.seed2() - q.seed1()) & 0x8000) != 0;
}   // 19C69
PORT(targetBeatsRoll) { Rules q(m); r.a.x = q.seed1(); r.cf = q.seed1() >= q.seed2(); }   // 19C72
PORT(axBeatsTarget) { r.cf = r.a.x >= Rules(m).seed1(); }  // 19C7E
PORT(targetBase) {                                         // 19CAC: dl = count -> dl:al = 15 + 5 * count
	const uint16_t v = uint16_t(Rules::targetBase(r.d.b.l));
	r.b.b.l = uint8_t(v >> 8);
	setDlAl(r, v);
}
PORT(times3) {                                             // 19CCC: dl:al = 3 * al
	const uint16_t v = uint16_t(r.a.b.l * 3);
	setDlAl(r, v);
}
PORT(sumOfD6) {                                            // 19D86: al = base, dl = number of dice
	uint8_t al;
	const uint16_t v = Rules(m).sumOfD6(r.a.b.l, r.d.b.l, al);
	r.a.b.l = uint8_t(v);
	r.d.b.l = uint8_t(v >> 8);
	r.b.b.l = uint8_t(v >> 8);
}

// ---- text ---------------------------------------------------------------------------------------------------------------
PORT(writeItemText) { r.d.b.h = 0; writeItemText(m, r.a.b.l); }          // 17AD3 (also reached through 17AC8)
PORT(writeItemTextNormal) { r.d.b.h = 0; writeItemTextNormal(m, r.a.b.l); }   // 17ACE
PORT(writeItemName) { r.d.b.h = 0; writeItemName(m, r.a.b.l); }          // 17AF0
PORT(writeSingleCharacter) { setWriteCharacterFn2(m); r.d.b.h = 0; writeItemText(m, r.a.b.l); }   // 17AC3 (falls into 17AC8)

} // namespace rc
} // namespace wl
