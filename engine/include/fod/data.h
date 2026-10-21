#pragma once
#include "fod/bytes.h"

namespace fod {

struct ItemDef {                 // GLOBALS item, 0x18 bytes at file offset 0x2A + 0x18*id
	std::string name;
	uint8_t flags = 0;           // bit0 weapon-capable, bit1 armor, bits2-3 ingest effect, 1Ch = unique
	uint8_t weaponDef = 0;       // index into WEAPONS (weapons) or armor class (armor)
	uint8_t woundChance = 0;     // +0x16 (weapons: % chance to inflict a wound/status)
	uint8_t uniqueBit = 0;       // +0x17 (unique items: bit in Disk1.uniqueItems; weapons: wound/status type)
	bool isWeapon() const { return flags & 1; }
	bool isArmor() const { return flags & 2; }
};

class Globals {
public:
	explicit Globals(const Bytes &file);
	int rangeBands() const { return _rangeBands; }
	const std::string &unit() const { return _unit; }
	const std::vector<ItemDef> &items() const { return _items; }
private:
	int _rangeBands = 0;
	std::string _unit;
	std::vector<ItemDef> _items;
};

struct FireMode {                // 0x12 bytes
	uint8_t hitPct[10];          // to-hit % per range band
	uint16_t dmgMin, dmgMax;
	uint8_t ammoType, clip, burst, b11;
};

struct WeaponDef {               // 0x3C bytes + its fire modes
	Bytes raw;
	uint8_t flags() const { return raw[0x18]; }     // bit0 ammo-using, bit1 ranged/consumable
	uint8_t reliability() const { return raw[0x1A]; }   // % chance the weapon works (checkWeaponMalfunction)
	std::string verb() const { return std::string(reinterpret_cast<const char *>(raw.data()), strnlen(reinterpret_cast<const char *>(raw.data()), 20)); }
	std::vector<FireMode> modes;
};

class Weapons {
public:
	explicit Weapons(const Bytes &file);
	const std::vector<WeaponDef> &weapons() const { return _w; }
private:
	std::vector<WeaponDef> _w;
};

struct Profession {              // 0x80 bytes
	std::string name;
	uint8_t attrMin[7], attrMax[7];
	uint8_t attrPoints = 0;
	uint8_t activeSkills[16], passiveSkills[16];
	uint16_t hpMin = 0, hpMax = 0, hpCap = 0;
	uint8_t baseAC = 0;
	uint32_t xpFactor = 0;           // +0x6C: xp per rank when a monster of this profession is killed
	uint16_t dmgMin = 0, dmgMax = 0; // +0x76/+0x78: unarmed damage
	Bytes raw;
};

struct EnemyDef {                // 0x40 bytes
	std::string name;            // +0x00 (20 bytes)
	uint32_t xpPerLevel = 0;     // +0x14
	std::string verb;            // +0x18 e.g. "stings"
	uint16_t dmgMin = 0, dmgMax = 0;   // +0x2C / +0x2E
	uint8_t speed = 0, reach = 0, radius = 0;   // +0x35 / +0x36 / +0x37 (group movement/visibility)
	uint8_t armor = 0, hitPct = 0, woundChance = 0, woundType = 0, evasion = 0;   // +0x39..+0x3D
	Bytes raw;
};

class Archetypes {
public:
	explicit Archetypes(const Bytes &file);
	const std::vector<Profession> &professions() const { return _p; }
	const std::vector<EnemyDef> &enemies() const { return _e; }
private:
	std::vector<Profession> _p;
	std::vector<EnemyDef> _e;
};

} // namespace fod
