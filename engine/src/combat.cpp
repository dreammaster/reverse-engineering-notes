// Ports of the KEH.EXE combat primitives. Names in comments refer to the IDB function names.
#include "fod/combat.h"
#include <algorithm>
#include <cstdlib>

namespace fod {

namespace {
int16_t rd16(const uint8_t *p) { return int16_t(p[0] | (p[1] << 8)); }
void wr16(uint8_t *p, int v) { p[0] = uint8_t(v); p[1] = uint8_t(v >> 8); }
}

int16_t Combat::targetHpValue() const { return _hpPtr ? rd16(_hpPtr) : 0; }
void Combat::addTargetHp(int delta) { if (_hpPtr) wr16(_hpPtr, rd16(_hpPtr) + delta); }

int Combat::distanceToParty(int x, int y) const {
	// KEH distanceToParty: smallest square radius 1..63 around the party containing (x, y)
	for (int r = 1; r < 64; ++r)
		if (std::abs(x - _w.disk.mapX) <= r && std::abs(y - _w.disk.mapY) <= r) return r;
	return 63;
}

// ---------------------------------------------------------------- targets
bool Combat::pickWeakestInGroup(const Combatant &c) {
	EnemyGroup &g = _w.map->groups().at(c.tIndex);
	uint8_t *slot = &g.raw[0xE + 0x16 * c.tSub];
	const int count = slot[0xF];
	bool any = false;
	for (int i = 0; i < count; ++i)
		if (rd16(slot + 2 + 2 * i) > 0) any = true;
	if (!any) return false;
	int best = 0;
	for (int i = 0; i < count; ++i) {
		const int v = rd16(slot + 2 + 2 * i);
		if (v > 0 && rd16(slot + 2 + 2 * best) > v) best = i;
	}
	// KEH starts with index 0 even when its hp is <= 0; mirror that by re-checking
	if (rd16(slot + 2 + 2 * best) <= 0) {
		for (int i = 0; i < count; ++i) if (rd16(slot + 2 + 2 * i) > 0) { best = i; break; }
	}
	_weakestIdx = best;
	_hpPtr = slot + 2 + 2 * best;
	_targetNameType = 0;
	const EnemyDef &e = _w.data.archetypes.enemies().at(slot[0]);
	_xpReward = e.xpPerLevel * slot[1];
	return true;
}

void Combat::selectTarget(const Combatant &c) {
	_targetNameType = 0;
	_xpReward = 0;
	if (c.tType == 1) {
		Monster &m = _w.map->monsters().at(c.tIndex);
		_hpPtr = &m.raw[0x44];
		const Profession &p = _w.data.archetypes.professions().at(m.raw[0x48]);
		_xpReward = p.xpFactor * m.raw[0x4E];
		_targetNameType = m.raw[0x50] & 3;
	} else if (c.tType == 2) {
		if (!pickWeakestInGroup(c)) { _dummyHp[0] = _dummyHp[1] = 0; _hpPtr = _dummyHp; }
	} else {
		_hpPtr = &_w.party(c.tIndex).b[0x44];
	}
}

bool Combat::combatantPos(const Combatant &c, int &x, int &y, int &hp) const {
	if (c.type == 1) {
		const Monster &m = _w.map->monsters().at(c.index);
		x = int8_t(m.raw[0x60]); y = int8_t(m.raw[0x61]);
		hp = rd16(&m.raw[0x44]);
		return true;
	}
	if (c.type == 2) {
		const EnemyGroup &g = _w.map->groups().at(c.index);
		const uint8_t *slot = &g.raw[0xE + 0x16 * c.sub];
		x = g.x(); y = g.y();
		hp = 0;
		for (int i = 0; i < slot[0xF]; ++i) {
			const int v = rd16(slot + 2 + 2 * i);
			if (v > 0) { hp = v; break; }
		}
		return true;
	}
	return false;
}

std::string Combat::attackerName(const Combatant &c) const {
	if (c.type == 0) return _w.party(c.index).name();
	if (c.type == 1) return _w.map->monsters().at(c.index).name();
	const EnemyGroup &g = _w.map->groups().at(c.index);
	return _w.data.archetypes.enemies().at(g.raw[0xE + 0x16 * c.sub]).name;
}

std::string Combat::targetName(const Combatant &c) const {
	if (c.tType == 0) return _w.party(c.tIndex).name();
	if (c.tType == 1) return _w.map->monsters().at(c.tIndex).name();
	const EnemyGroup &g = _w.map->groups().at(c.tIndex);
	return _w.data.archetypes.enemies().at(g.raw[0xE + 0x16 * c.tSub]).name;
}

int Combat::armorClassOf(const Combatant &c) const {
	if (c.tType == 0) return _w.armorClass(_w.party(c.tIndex));
	return 0;
}

// ---------------------------------------------------------------- rollAttack (0x12F62)
int Combat::rollAttack(const Combatant &c, int &totalDamage, int &wound) {
	const auto &profs = _w.data.archetypes.professions();
	const auto &enemies = _w.data.archetypes.enemies();
	const int aScale = 1, bScale = 2;   // GLOBALS header scale bytes (scaleA=1, scaleB=2)

	int weaponItem = 0xFF, ammoType = 0;
	int dmgMin = 0, dmgMax = 0, hit = 0, strBonus = 0;
	int woundChance = 0, woundType = 1;
	const Member *attackerMember = nullptr;
	const uint8_t *attackerRaw = nullptr;   // member or monster bytes (shared layout)
	int tx = 0, ty = 0;
	int enemyId = 0;

	if (c.type == 0) {
		const Member &m = _w.party(c.index);
		attackerMember = &m;
		attackerRaw = m.b.data();
		const uint8_t readied = m.b[0x4A];
		weaponItem = readied != 0xFF ? m.b[0x62 + 6 * readied] : 0xFF;
		ammoType = weaponItem != 0xFF ? m.b[0x63 + 6 * readied] : 0;
		const Profession &p = profs.at(m.profession());
		dmgMin = p.dmgMin; dmgMax = p.dmgMax;
		strBonus = World::attrBonus(m.attr(0)) * 2;
		hit = m.b[0x34 + 4] * 2 + m.b[0x51] + (World::attrBonus(m.attr(2)) + World::attrBonus(m.attr(6))) * 2;
		fireMode = m.b[0x144];
		shots = std::max<int>(1, m.b[0x146]);
	} else if (c.type == 1) {
		shots = 1;
		const Monster &m = _w.map->monsters().at(c.index);
		attackerRaw = m.raw.data();
		const uint8_t readied = m.raw[0x4A];
		weaponItem = readied != 0xFF ? m.raw[0x53 + readied] : 0xFF;
		const Profession &p = profs.at(m.raw[0x48]);
		dmgMin = p.dmgMin; dmgMax = p.dmgMax;
		strBonus = World::attrBonus(m.raw[0x18]) * 2;
		hit = m.raw[0x38] * 2 + m.raw[0x51] + (World::attrBonus(m.raw[0x1A]) + World::attrBonus(m.raw[0x1E])) * 2;
		tx = int8_t(m.raw[0x60]); ty = int8_t(m.raw[0x61]);
	} else {
		shots = 1;
		const EnemyGroup &g = _w.map->groups().at(c.index);
		const uint8_t *slot = &g.raw[0xE + 0x16 * c.sub];
		weaponItem = slot[0x13];
		tx = g.x(); ty = g.y();
		enemyId = slot[0];
		const EnemyDef &e = enemies.at(enemyId);
		dmgMin = e.dmgMin; dmgMax = e.dmgMax;
		hit = e.hitPct;
		woundChance = e.woundChance;
		woundType = e.woundType;
	}

	// ---- defender ----
	int evasion = 0, defArmorItem = 0xFF, defBaseAC = 0, deathThreshold = 0;
	if (c.tType == 0) {
		const Member &t = _w.party(c.tIndex);
		evasion = (World::attrBonus(t.attr(2)) + World::attrBonus(t.attr(6))) * 2;
		deathThreshold = -60;
	} else if (c.tType == 1) {
		const Monster &t = _w.map->monsters().at(c.tIndex);
		defArmorItem = t.raw[0x4B] != 0xFF ? t.raw[0x53 + t.raw[0x4B]] : 0xFF;
		defBaseAC = profs.at(t.raw[0x48]).baseAC;
		evasion = (t.raw[0x1A] + World::attrBonus(t.raw[0x1E])) * 2;
		tx = int8_t(t.raw[0x60]); ty = int8_t(t.raw[0x61]);
	} else {
		const EnemyGroup &g = _w.map->groups().at(c.tIndex);
		const uint8_t *slot = &g.raw[0xE + 0x16 * c.tSub];
		const EnemyDef &e = enemies.at(slot[0]);
		defBaseAC = e.armor;
		evasion = e.evasion;
		tx = g.x(); ty = g.y();
	}

	// ---- weapon ----
	if (weaponItem != 0xFF) {
		const ItemDef &it = _w.item(weaponItem);
		const WeaponDef &wd = _w.weaponOf(weaponItem);
		woundChance = it.woundChance;
		woundType = it.uniqueBit;
		strBonus = 0;
		if (wd.flags() & 2) {
			const FireMode &fm = wd.modes.at(std::min<size_t>(size_t(ammoType), wd.modes.size() - 1));
			dmgMin = fm.dmgMin; dmgMax = fm.dmgMax;
			const int dist = distanceToParty(tx, ty);
			hit = fm.hitPct[std::min(dist, 10) - 1];
		} else {
			dmgMin = wd.raw[0x14] | (wd.raw[0x15] << 8);
			dmgMax = wd.raw[0x16] | (wd.raw[0x17] << 8);
			hit = wd.raw[0x19];
			if (attackerRaw) strBonus = wd.raw[0x1C] * World::attrBonus(attackerRaw[0x18]);
		}
		if (attackerRaw) {
			hit += wd.raw[0x1E] * World::attrBonus(attackerRaw[0x1A]);
			hit += wd.raw[0x22] * World::attrBonus(attackerRaw[0x1E]);
			for (int i = 0; i < 16; ++i) hit += wd.raw[0x28 + i] * attackerRaw[0x34 + i];
		}
	}
	if (c.type == 2) hit += enemies.at(enemyId).evasion;
	if (fireMode == 1) hit += 10;
	else if (fireMode == 2) hit += 20;
	hit -= evasion;
	if (hit < 0) hit = 0;
	if (calcOnly) return hit;
	if (sureHit > 0) hit = 100;

	// ---- armour ----
	int ac;
	if (c.tType == 0) ac = _w.armorClass(_w.party(c.tIndex));
	else if (defArmorItem != 0xFF) ac = _w.item(defArmorItem).weaponDef;
	else ac = defBaseAC;
	if (ac * aScale >= dmgMax + strBonus) return 3;     // cannot penetrate

	totalDamage = 0;
	int hits = 0;
	for (int shot = 0; shot < shots; ++shot) {
		if (c.type == 0 && weaponItem != 0xFF && sureHit == 0) {
			useUpAmmo(c.index, _w.party(c.index).b[0x4A]);
		}
		if (hit < _w.rng.range(1, 100)) continue;       // missed this shot
		if (c.type == 0) {
			Member &m = _w.party(c.index);
			if (weaponItem == 0xFF) {
				const int r = trySkillImprove(c.index, 0x14);
				m.b[0x22] = uint8_t(r); m.b[0x23] = 0x14;
			} else {
				const WeaponDef &wd = _w.weaponOf(weaponItem);
				for (int i = 0; i < 16; ++i)
					if (m.b[0x34 + i] && wd.raw[0x28 + i]) {
						const int r = trySkillImprove(c.index, i + 0x10);
						m.b[0x22] = uint8_t(r); m.b[0x23] = uint8_t(i + 0x10);
					}
			}
		}
		++hits;
		const int absorb = _w.rng.range(aScale * ac, bScale * ac);
		int dmg = (_w.rng.range(dmgMin, dmgMax) + strBonus) / std::max(divisor, 1) - absorb;
		if (dmg < 0) dmg = 0;
		if (dmg > 0) {
			if (woundChance >= _w.rng.range(1, 100)) wound = woundType + 1;
			totalDamage += dmg;
			if (targetHpValue() - totalDamage <= deathThreshold) return 4;
		}
	}
	if (hits == 0) return 0;
	return totalDamage == 0 ? 1 : 2;
}

// KEH trySkillImprove (0x1969F): bump the use counter of skill idx; on overflow raise the skill if the attribute allows.
int Combat::trySkillImprove(int slot, int skill) {
	Member &m = _w.party(slot);
	const int attrIdx = skill > 15 ? 1 : 4;
	m.b[0x124 + skill] += 1;
	const int threshold = 20;   // TODO: port the real threshold table from trySkillImprove
	int raised = 0;
	if (m.b[0x124 + skill] >= threshold) {
		const int attr = m.b[0x18 + attrIdx];
		if (attr > 15 && attr - 15 > m.b[0x24 + skill]) { ++m.b[0x24 + skill]; raised = 1; }
		m.b[0x124 + skill] = 0;
	}
	return raised;
}

void Combat::useUpAmmo(int slot, int itemSlot) {
	Member &m = _w.party(slot);
	const int item = m.b[0x62 + 6 * itemSlot];
	const WeaponDef &wd = _w.weaponOf(item);
	if (wd.flags() & 2) {
		if (wd.flags() & 1) {
			if (m.b[0x64 + 6 * itemSlot] > 0) --m.b[0x64 + 6 * itemSlot];
		} else {
			int next = 0xFF;
			if (m.b[0x4A] != 0xFF) next = m.b[0x62 + 6 * m.b[0x4A]];
			_w.removeItemSlot(m, itemSlot, 0xFF);
			if (next != 0xFF)
				for (int i = 0; i < m.itemCount(); ++i)
					if (m.b[0x62 + 6 * i] == next) { m.b[0x4A] = uint8_t(i); break; }
		}
	}
}

// ---------------------------------------------------------------- resolveAttack (0x13C29)
int Combat::resolveAttack(const Combatant &c) {
	int damage = 0, wound = 0;
	const int result = rollAttack(c, damage, wound);
	if (calcOnly) return result;

	std::string verb = "attacks";
	if (c.type == 0) {
		const Member &m = _w.party(c.index);
		const uint8_t readied = m.b[0x4A];
		if (readied != 0xFF) verb = _w.weaponOf(m.b[0x62 + 6 * readied]).verb();
		if (fireMode == 1) verb = "triggers a burst at";
		else if (fireMode == 2) verb = "rips off a clip at";
	} else if (c.type == 2) {
		const EnemyGroup &g = _w.map->groups().at(c.index);
		verb = _w.data.archetypes.enemies().at(g.raw[0xE + 0x16 * c.sub]).verb;
	} else {
		verb = "attacks";
	}
	const std::string line = attackerName(c) + " " + verb + " " + targetName(c);

	static const char *kWhom[4] = {"it", "him", "her", "it"};
	switch (result) {
	case 0:
		_h.print(line + " and misses.\r\r");
		if (c.tType == 0 && _w.party(c.tIndex).hp() > 0 && trySkillImprove(c.tIndex, 0x1C))
			_h.skillRaised(_w.party(c.tIndex).name(), 0x1C, 0);
		break;
	case 1:
		_h.print(line + " but fails to inflict any damage.\r\r");
		break;
	case 2: case 4:
		_h.print(line);
		if (c.tType == 0) {
			Member &t = _w.party(c.tIndex);
			t.setHp(t.hp() - damage);
			t.setU16(0x122, uint16_t(t.hp()));
			if (t.hp() <= -60) _w.disk.tail[t.rosterId()] |= 2;
			if (result == 4) {
				_h.print(std::string(" and kills ") + kWhom[t.u8(0x50) & 3] + "!\r\r");
			} else {
				_h.print(" for " + std::to_string(damage) + " pts damage.\r\r");
				if (wound) {
					--wound;
					if (wound == 7) _h.print(t.name() + " has been exposed to mutagens!\r");
					else if (!(t.status() & (1u << wound))) {
						t.status() |= uint8_t(1u << wound);
						_h.print(t.name() + " has become afflicted!\r");
					}
				}
			}
		} else {
			addTargetHp(-damage);
			if (result == 4) {
				_h.print(std::string(" and wastes ") + kWhom[_targetNameType & 3] + "!\r\r");
				if (c.type == 0) xpAccum[c.index] += _xpReward;
				removeDefeated(c);
			} else {
				_h.print(" for " + std::to_string(damage) + " pts damage.\r\r");
			}
		}
		break;
	case 3:
		_h.print(attackerName(c) + "'s weapon is useless against " + targetName(c) + ".\r\r");
		break;
	}
	return result;
}

// KEH removeDefeatedFromMap (0x13B70): a defeated monster is marked dead; a group slot only clears its map bit once empty.
void Combat::removeDefeated(const Combatant &c) {
	if (c.tType == 1) {
		Monster &m = _w.map->monsters().at(c.tIndex);
		_w.disk.tail[m.raw[0x49]] |= 2;
		if (_w.map->inBounds(int8_t(m.raw[0x5B]), int8_t(m.raw[0x5C]))) {
			MapCell &cell = _w.map->cell(int8_t(m.raw[0x5B]), int8_t(m.raw[0x5C]));
			cell.tile &= 0x07FF;
		}
	}
}

bool Combat::checkWeaponMalfunction(int slot) {
	Member &m = _w.party(slot);
	if (m.b[0x4A] == 0xFF) return false;
	const int item = m.b[0x62 + 6 * m.b[0x4A]];
	const WeaponDef &wd = _w.weaponOf(item);
	if (wd.reliability() >= _w.rng.range(1, 100)) return false;
	if (wd.flags() & 2) {
		if (wd.flags() & 1) {
			m.b[0x65 + 6 * m.b[0x4A]] |= 1;
			_h.print(m.name() + "'s weapon has jammed!\r\r");
			return true;
		}
		if (sureHit > 0) {
			_h.print(" but it fails to detonate.\r\r");
			useUpAmmo(slot, m.b[0x4A]);
			return true;
		}
		return false;
	}
	_h.print(m.name() + "'s weapon has broken!\r\r");
	_w.removeItemSlot(m, m.b[0x4A], 0xFF);
	return true;
}

// KEH memberAttack (0x1459F), without the blast-weapon branch.
void Combat::memberAttack(int slot, const Combatant &rec) {
	Member &m = _w.party(slot);
	if (!_w.isActive(m)) return;
	selectTarget(rec);
	if (targetHpValue() <= 0) {
		_h.print(targetName(rec) + " manages to hold back.\r\r");
		return;
	}
	if (checkWeaponMalfunction(slot)) return;
	Combatant r = rec;
	bool again = true;
	bool valid = true;
	while (again) {
		again = false;
		if (!valid) break;
		const int res = resolveAttack(r);
		const bool killed = res == 4 && m.b[0x146] != 0;
		if (killed) {
			again = retarget(r);
			valid = again;
		}
	}
}

bool Combat::retarget(Combatant &r) {
	const bool autoFire = _w.party(r.index).b[0x144] == 2;
	if (r.tType == 2) {
		if (pickWeakestInGroup(r)) return true;
	}
	if (!autoFire) return false;
	return findNearbyTarget(r);
}

bool Combat::findNearbyTarget(Combatant &r) {
	int tx = 0, ty = 0, hp = 0;
	Combatant cur = r;
	cur.type = r.tType; cur.index = r.tIndex; cur.sub = r.tSub;
	if (!combatantPos(cur, tx, ty, hp)) return false;
	for (int radius = 1; radius < 4; ++radius) {
		for (size_t j = 0; j < list.size(); ++j) {
			const Combatant &o = list[j];
			if (o.type == 0) continue;
			if (o.type == r.tType && o.index == r.tIndex && o.sub == r.tSub) continue;
			int ox, oy, ohp;
			if (!combatantPos(o, ox, oy, ohp) || ohp <= 0) continue;
			if (std::abs(ox - tx) <= radius && std::abs(oy - ty) <= radius) {
				r.tType = o.type; r.tIndex = o.index; r.tSub = o.sub;
				selectTarget(r);
				return true;
			}
		}
	}
	return false;
}

// Enemy attack: KEH attackMultiShot without the blast branch.
void Combat::monsterAttack(const Combatant &rec, int attacks) {
	if (rec.tType != 0) return;
	Member &t = _w.party(rec.tIndex);
	if (World::conState(t) == 5) return;
	for (int i = 0; i < attacks; ++i) {
		selectTarget(rec);
		resolveAttack(rec);
		if (World::conState(t) == 5) break;
	}
}

} // namespace fod
