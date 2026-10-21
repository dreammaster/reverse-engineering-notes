#include "fod/data.h"

namespace fod {

Globals::Globals(const Bytes &file) {
	Reader r(file);
	r.seek(2);
	_rangeBands = r.u16();
	r.seek(8);
	_unit = r.cstr(0x2A - 8);
	const size_t count = (file.size() - 0x2A) / 0x18;
	for (size_t i = 0; i < count; ++i) {
		r.seek(0x2A + i * 0x18);
		ItemDef it;
		it.name = r.cstr(0x14);
		it.flags = r.u8();
		it.weaponDef = r.u8();
		it.woundChance = r.u8();
		it.uniqueBit = r.u8();
		_items.push_back(std::move(it));
	}
}

Weapons::Weapons(const Bytes &file) {
	Reader r(file);
	const unsigned count = r.u16();
	for (unsigned i = 0; i < count; ++i) {
		WeaponDef w;
		w.raw.resize(0x3C);
		r.seek(2 + i * 0x3C);
		r.bytes(w.raw.data(), 0x3C);
		const unsigned modeCount = w.raw[0x38] | (w.raw[0x39] << 8);
		const unsigned modesOfs = w.raw[0x3A] | (w.raw[0x3B] << 8);
		for (unsigned m = 0; m < modeCount; ++m) {
			if (modesOfs + (m + 1) * 0x12 > file.size()) break;
			r.seek(modesOfs + m * 0x12);
			FireMode fm;
			r.bytes(fm.hitPct, 10);
			fm.dmgMin = r.u16();
			fm.dmgMax = r.u16();
			fm.ammoType = r.u8();
			fm.clip = r.u8();
			fm.burst = r.u8();
			fm.b11 = r.u8();
			w.modes.push_back(fm);
		}
		_w.push_back(std::move(w));
	}
}

Archetypes::Archetypes(const Bytes &file) {
	Reader r(file);
	const unsigned profCount = r.u16();
	const unsigned enemyCount = r.u16();
	const unsigned profOfs = r.u16();
	const unsigned enemyOfs = r.u16();
	for (unsigned i = 0; i < profCount; ++i) {
		Profession p;
		p.raw.resize(0x80);
		r.seek(profOfs + i * 0x80);
		r.bytes(p.raw.data(), 0x80);
		Reader pr(p.raw);
		p.name = pr.cstr(16);
		pr.seek(0x14);
		pr.bytes(p.attrMin, 7);
		pr.seek(0x20);
		pr.bytes(p.attrMax, 7);
		pr.seek(0x33);
		p.attrPoints = pr.u8();
		pr.seek(0x38);
		pr.bytes(p.activeSkills, 16);
		pr.bytes(p.passiveSkills, 16);
		pr.seek(0x6C);
		p.xpFactor = pr.u32();
		pr.seek(0x76);
		p.dmgMin = pr.u16();
		p.dmgMax = pr.u16();
		pr.seek(0x70);
		p.hpMin = pr.u16();
		p.hpMax = pr.u16();
		p.hpCap = pr.u16();
		pr.seek(0x7A);
		p.baseAC = pr.u8();
		_p.push_back(std::move(p));
	}
	for (unsigned i = 0; i < enemyCount; ++i) {
		EnemyDef e;
		e.raw.resize(0x40);
		r.seek(enemyOfs + i * 0x40);
		r.bytes(e.raw.data(), 0x40);
		Reader er(e.raw);
		e.name = er.cstr(0x14);
		e.xpPerLevel = er.u32();
		e.verb = er.cstr(0x14);
		er.seek(0x2C);
		e.dmgMin = er.u16();
		e.dmgMax = er.u16();
		er.seek(0x35);
		e.speed = er.u8(); e.reach = er.u8(); e.radius = er.u8();
		er.seek(0x39);
		e.armor = er.u8(); e.hitPct = er.u8(); e.woundChance = er.u8(); e.woundType = er.u8(); e.evasion = er.u8();
		_e.push_back(std::move(e));
	}
}

} // namespace fod
