#pragma once
// Dynamic game world: save state + current map + helpers that mirror KEH.EXE routines.
#include "fod/archive.h"
#include "fod/data.h"
#include "fod/map.h"
#include "fod/script.h"
#include "fod/state.h"
#include <functional>
#include <memory>

namespace fod {

// Static game data loaded once from the game directory.
struct GameData {
	std::string dir;
	Globals globals;
	Weapons weapons;
	Archetypes archetypes;
	Bytes hdspct;                // skill use thresholds at +0x13A + 0x16*skill
	GameData(const std::string &gameDir);
};

// Deterministic PRNG matching KEH rngByte/randomRange semantics (seeded externally).
class Rng {
public:
	explicit Rng(uint32_t seed = 1) { reseed(seed); }
	void reseed(uint32_t seed);
	uint8_t next();                         // KEH rngByte: 16-byte additive-carry generator
	int range(int lo, int hi);              // KEH randomRange: rejection sampling on bytes
private:
	uint8_t _s[16];
};

// Persistent change to a map cell (KEH _mapChangeList record: 12 bytes at Disk1+0x7B6+2).
struct MapChange {
	uint8_t map = 0, x = 0, y = 0, kind = 0;    // kind bits: 0x80 tile, 0x40 fields, 0x20 script
	uint16_t tile = 0;
	uint8_t f2 = 0, b3 = 0;
	uint16_t w4 = 0, script = 0;
};

class World {
public:
	World(const GameData &data, Disk1 disk, uint32_t seed = 1);

	const GameData &data;
	Disk1 disk;
	Rng rng;
	int mapNo = 0;                            // current 1-based map number
	std::unique_ptr<Map> map;
	std::unique_ptr<ScriptBank> scripts;
	std::vector<MapChange> changes;

	// -- party --
	int partyCount() const { return disk.partyCount; }
	Member &party(int slot) { return disk.roster[disk.partyIds[slot]]; }
	const Member &party(int slot) const { return disk.roster[disk.partyIds[slot]]; }
	// 0 = healthy (hp>=1); 1..4 = unconscious..comatose; 5 = dead (KEH getMemberCon)
	static int conState(const Member &m);
	static int attrBonus(int v);              // KEH attrBonus (can be negative)
	bool isActive(const Member &m) const;     // KEH isMemberActive
	int armorClass(const Member &m) const;    // KEH getArmorClass
	const WeaponDef &weaponOf(int itemId) const { return data.weapons.weapons().at(item(itemId).weaponDef); }

	// -- items --
	void initItemSlot(Member &m, int slot, int item);
	void removeItemSlot(Member &m, int slot, int replaceWith);   // replaceWith 0xFF = delete (KEH removeItemSlot)
	bool addItem(Member &m, int item);
	bool hasItem(const Member &m, int item) const;
	bool isUnique(int item) const;
	const ItemDef &item(int id) const { return data.globals.items().at(id); }

	// -- script stat access (KEH getMemberStat / setMemberStat) --
	int32_t getStat(int memberSel, int kind, int idx, int curMember);
	void setStat(int memberSel, int kind, int idx, int32_t value, int curMember);
	int resolveMember(int sel, int cur) const { return sel == 0 ? cur : sel - 1; }

	// -- time --
	void advanceMinutes(int minutes);

	// -- maps --
	void loadMap(int mapNumber);              // loads map + scripts, applies persistent changes
	void setPartyPosition(int x, int y);
	MapCell &cellAt(int x, int y) { return map->cell(x, y); }
	void applyChange(const MapChange &c);
	void recordCellChange(int op, int mapNumber, int x, int y, int val, int extra);
	void applyCellChange(int x, int y, int op, int val, int extra);   // op 0x19..0x1E semantics on one cell
	int mapStackIndex() const { return disk.mapIndex; }

	// -- persistence list stored in Disk1.tail --
	void syncChangesToDisk();
	void loadChangesFromDisk();
};

} // namespace fod
