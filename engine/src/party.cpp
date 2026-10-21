// Party inventory handling: ports of KEH itemActionMenu, ingestItem, giveItemToMember, chooseAmmoToLoad,
// tryUnjamWeapon, applyItemEffect, exposeToMutagens and doctorTreat.
#include "fod/party.h"
#include <algorithm>

namespace fod {

namespace {
const char *kPoss[4] = {"its", "his", "her", "its"};
}

void exposeToMutagens(World &w, Member &m, int percent) {
	if (percent < w.rng.range(1, 100)) return;
	m.b[0x53] = uint8_t(w.rng.range(1, 6));
	m.b[0x55] = uint8_t((w.disk.weekday + 1) % 7);
	m.b[0x56] = uint8_t(w.rng.range(6, 11));
}

void applyItemEffect(World &w, Member &m, const ItemDef &item, bool apply) {
	if (item.woundChance == 0) return;
	if (item.uniqueBit == 7) exposeToMutagens(w, m, item.woundChance);
	else if (apply) m.b[0x58] |= uint8_t(1u << item.uniqueBit);
}

bool giveItemToMember(World &w, Host &h, int from, int to, int itemSlot, bool report, bool dryRun) {
	Member &src = w.party(from), &dst = w.party(to);
	if (dst.hp() <= 0) return false;
	if (dst.itemCount() >= 32) {
		if (report) h.message(dst.name() + "\rhas too much to carry already.");
		return false;
	}
	if (!dryRun) {
		applyItemEffect(w, dst, w.item(src.itemId(itemSlot)), false);
		std::memcpy(&dst.b[0x62 + 6 * size_t(dst.itemCount())], &src.b[0x62 + 6 * size_t(itemSlot)], 6);
		++dst.itemCount();
		w.removeItemSlot(src, itemSlot, 0xFF);
	}
	return true;
}

int chooseAmmoToLoad(World &w, Host &h, int member, int itemSlot) {
	Member &m = w.party(member);
	const WeaponDef &wd = w.weaponOf(m.itemId(itemSlot));
	std::vector<int> slots;
	std::vector<std::string> names;
	for (size_t mode = 0; mode < wd.modes.size(); ++mode) {
		for (int i = 0; i < m.itemCount(); ++i) {
			if (wd.modes[mode].ammoType != m.itemId(i)) continue;
			m.itemAmmoType(i) = uint8_t(mode);
			m.itemAmmo(i) = wd.modes[mode].clip;
			slots.push_back(i);
			names.push_back(w.item(m.itemId(i)).name);
		}
	}
	if (slots.empty()) return -2;
	if (slots.size() == 1) return slots[0];
	const int pick = h.pickFromList("Load Which Ammo?", names);
	return pick < 0 ? -1 : slots[size_t(pick)];
}

void tryUnjamWeapon(World &w, Host &h, int member) {
	Member &m = w.party(member);
	const int slot = m.b[0x145];
	const int item = m.b[0x62 + 6 * slot];
	int chance = std::min(100, 20 * m.b[0x3E]);
	const std::string name = w.item(item).name;
	const std::string poss = kPoss[m.b[0x50] & 3];
	bool improved = false;
	if (chance >= w.rng.range(1, 100)) {
		m.b[0x65 + 6 * slot] &= 0xFE;
		h.message(m.name() + " successfully unjams " + poss + " " + name + ".");
		improved = w.trySkillImprove(member, 0x1A) != 0;
	} else {
		h.message(m.name() + " trashes " + poss + " " + name + " while trying to fix it.");
		w.removeItemSlot(m, slot, 0xFF);
	}
	if (improved) h.skillRaised(m.name(), 0x1A, m.b[0x24 + 0x1A]);
}

void ingestItem(World &w, Host &h, int member, int ingestType, int itemSlot) {
	std::vector<std::string> names;
	for (int i = 0; i < w.partyCount(); ++i) names.push_back(w.party(i).name());
	for (;;) {
		const int pick = h.pickFromList("Who will ingest this?", names);
		if (pick < 0) return;
		Member &t = w.party(pick);
		if (t.hp() < 1) { h.message(t.name() + " is incapacitated."); continue; }
		w.removeItemSlot(w.party(member), itemSlot, 0xFF);
		t.b[0x55] = uint8_t((w.disk.weekday + 2) % 7);
		if (ingestType == 1) {
			t.b[0x57] = 0x18;
			h.message(t.name() + ", although plagued by pink elephants, will not be bothered by any mutations for 2 days.");
		} else {
			h.message(t.name() + " will not be bothered by any mutations for 2 days.");
		}
		if (t.b[0x58] & 0x80) {
			t.b[0x21] = 0x30;
			t.b[0x55] = uint8_t((w.disk.weekday + 2) % 7);
			t.b[0x56] = w.disk.hours;
		}
		t.b[0x53] = 0;
		return;
	}
}

bool itemActionMenu(World &w, Host &h, int member, int itemSlot, bool inCombat) {
	Member &m = w.party(member);
	const ItemDef &item = w.item(m.itemId(itemSlot));
	const bool active = inCombat ? w.isActive(m) : m.hp() > 0;
	int ingest = 0;
	for (;;) {
		m.b[0x147] = m.itemId(itemSlot);
		bool equipped = false, canEquip = false, reloadable = false;
		int armorSlot = 0;
		std::string menu = "Do what with your\r" + item.name + ":\r\r";
		if (active) {
			if (!inCombat || !(item.flags & 1) || m.b[0x4A] == itemSlot) menu += "U)se\r";
			if (!inCombat) {
				ingest = (item.flags & 3) ? 0 : (item.flags & 0xC) >> 2;
				if (ingest > 0) menu += "I)ngest\r";
			}
			menu += "D)rop\r";
		}
		int others = 0;
		for (int i = 0; i < w.partyCount(); ++i)
			if (i != member && w.party(i).hp() > 0) ++others;
		if (others > 0) menu += "T)rade\r";
		if (active) {
			if (item.flags & 1) {
				if (!inCombat) {
					menu += "E)quip-unequip weapon\r";
					if (m.b[0x4A] == itemSlot) equipped = true; else canEquip = true;
				}
				reloadable = (w.weaponOf(m.itemId(itemSlot)).flags() & 1) != 0;
			} else if (!inCombat && (item.flags & 2)) {
				menu += "E)quip-unequip armor\r";
				for (int i = 0; i < 3; ++i) if (m.b[0x4B + i] == itemSlot) equipped = true;
				canEquip = !equipped;
				if (canEquip) armorSlot = (item.flags & 0xC) >> 2;
			}
			if (!inCombat && reloadable) menu += "L)oad/unjam";
		}
		h.showMenu(menu, false);
		const int k = h.readKey();
		if (k == 0xFF) return false;
		if (k == 'D') {
			if (!active) continue;
			if (!inCombat) w.removeItemSlot(m, itemSlot, 0xFF); else m.b[0x144] = 0x0D;
			return inCombat;
		}
		if (k == 'E') {
			if (!canEquip && !equipped) continue;
			if (item.flags & 1) m.b[0x4A] = canEquip ? uint8_t(itemSlot) : 0xFF;
			else m.b[0x4B + armorSlot] = canEquip ? uint8_t(itemSlot) : 0xFF;
			if (canEquip) applyItemEffect(w, m, item, true);
			return false;
		}
		if (k == 'I') {
			if (ingest <= 0) continue;
			ingestItem(w, h, member, ingest, itemSlot);
			return false;
		}
		if (k == 'L') {
			if (inCombat || !reloadable) continue;
			if (m.b[0x65 + 6 * itemSlot] & 1) {
				if (!h.yesNo("This weapon is jammed.\r\rDo you wish to fix it?")) continue;
				tryUnjamWeapon(w, h, member);
				return false;
			}
			const int ammo = chooseAmmoToLoad(w, h, member, itemSlot);
			if (ammo >= 0) {
				m.itemAmmoType(itemSlot) = m.itemAmmoType(ammo);
				m.itemAmmo(itemSlot) = m.itemAmmo(ammo);
				w.removeItemSlot(m, ammo, 0xFF);
				return false;
			}
			if (ammo == -2) h.message("You have no ammo for this weapon.");
			return false;
		}
		if (k == 'T') {
			if (others <= 0) continue;
			std::vector<int> idx;
			std::vector<std::string> names;
			for (int i = 0; i < w.partyCount(); ++i)
				if (i != member && w.party(i).hp() > 0) { idx.push_back(i); names.push_back(w.party(i).name()); }
			const int pick = h.pickFromList("Who will take your\r" + item.name.substr(0, 19) + "?", names);
			if (pick < 0) continue;
			const int to = idx[size_t(pick)];
			if (w.party(to).hp() < 1) { h.message(w.party(to).name() + " is incapacitated."); continue; }
			if (!giveItemToMember(w, h, member, to, itemSlot, true, true)) continue;
			if (inCombat) {
				m.b[0x144] = 0x0C;
				m.b[0x146] = uint8_t(to);
			} else {
				giveItemToMember(w, h, member, to, itemSlot, true, false);
			}
			return inCombat;
		}
		if (k == 'U') {
			if (!active) continue;
			if (m.b[0x65 + 6 * itemSlot] & 1) {
				if (!h.yesNo("This weapon is jammed.\r\rDo you wish to fix it?")) continue;
				if (!inCombat) { tryUnjamWeapon(w, h, member); continue; }
				m.b[0x144] = 6;
				return true;
			}
			if (reloadable && m.itemAmmo(itemSlot) == 0) { h.message("You have no ammo for\rthis Weapon."); continue; }
			m.b[0x144] = 0x0B;
			return true;
		}
	}
}

bool doctorTreat(World &w, Host &h, int doctor, bool interactive, bool perform) {
	static const int kNeed[4] = {1, 4, 7, 10};
	static const int kHealTo[4] = {0, -7, -22, -37};
	Member &d = w.party(doctor);
	const int skill = d.b[0x145];
	const int level = d.b[0x24 + skill];
	int target;
	for (;;) {
		if (interactive) {
			target = 0;
			if (w.partyCount() > 1) {
				std::vector<std::string> names;
				for (int i = 0; i < w.partyCount(); ++i) names.push_back(w.party(i).name());
				target = h.pickFromList(skill == 0 ? "Heal whom?" : "Cure whom?", names);
			}
			if (target < 0) return false;
			d.b[0x146] = uint8_t(target);
		} else {
			target = d.b[0x146];
		}
		break;
	}
	Member &t = w.party(target);
	auto beyond = [&]() { if (interactive) h.message(t.name() + " is beyond your help."); return false; };
	if (skill == 0) {
		d.b[0x144] = 8;
		int c = World::conState(t);
		if (c == 0) {
			if (interactive) h.message(t.name() + " does not need any attention.");
			return false;
		}
		if (c == 5) return beyond();
		const int idx = c - 1;
		if (kNeed[idx] > level) return beyond();
		int chance;
		if (level > 20 || level / 5 > idx) chance = 95;
		else chance = (level - kNeed[idx]) * 30 + 30;
		if (!perform) return true;
		d.b[0x144] = 0x0E;
		if (chance >= w.rng.range(1, 100)) {
			if (interactive) h.message("Success! " + t.name() + " is getting better.");
			t.setHp(idx == 0 ? int16_t(t.u16(0x122)) : kHealTo[idx]);
			if (w.trySkillImprove(doctor, skill)) h.skillRaised(d.name(), skill, d.b[0x24 + skill]);
			return true;
		}
		if (interactive) h.message("Your efforts were commendable, but unsuccessful.");
		return false;
	}

	// afflictions
	d.b[0x144] = 9;
	std::vector<int> ail;
	for (int i = 0; i < 7; ++i) if (t.b[0x58] & (1u << i)) ail.push_back(i);
	if (t.b[0x54]) ail.push_back(7);
	if (ail.empty()) {
		if (interactive) h.message(t.name() + " is not afflicted by anything!");
		return false;
	}
	int which;
	if (interactive) {
		std::vector<std::string> names;
		for (int a : ail) {
			const char *n = reinterpret_cast<const char *>(w.data.hdspct.data() + 0x3EE + 0x16 * size_t(a));
			names.push_back(std::string(n, strnlen(n, 20)));
		}
		const int pick = h.pickFromList("Cure what?", names);
		if (pick < 0) return false;
		which = ail[size_t(pick)];
	} else {
		which = d.b[0x147];
	}
	if (which == 7) return beyond();
	d.b[0x147] = uint8_t(which);
	if ((level - 1) / 2 < which) return beyond();
	int chance = (level > 14 || (level - 1) / 2 > which) ? 95 : ((level & 1) ? 60 : 90);
	if (!perform) return true;
	d.b[0x144] = 0x0E;
	if (chance >= w.rng.range(1, 100)) {
		if (interactive) h.message("Success! Your efforts have cured " + t.name() + ".");
		t.b[0x58] &= uint8_t(~(1u << which));
		if (w.trySkillImprove(doctor, skill)) h.skillRaised(d.name(), skill, d.b[0x24 + skill]);
		return true;
	}
	if (interactive) h.message("Your efforts were commendable, but unsuccessful.");
	return false;
}

} // namespace fod
