#pragma once
// Dynamic game world: save state + current map + helpers that mirror KEH.EXE routines.
#include "fod/archive.h"
#include "fod/data.h"
#include "fod/map.h"
#include "fod/script.h"
#include "fod/state.h"
#include <functional>
#include <map>
#include <memory>

namespace fod {

// Static game data loaded once from the game directory.
struct GameData {
	std::string dir;
	Globals globals;
	Weapons weapons;
	Archetypes archetypes;
	Bytes packets;               // PACKETS loot tables
	Bytes services;              // SERVICES: blocks of 0x2C header + 0x20 per row
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
	std::map<int, Map> mapCache;              // maps visited this session, with their live state (KEH writes them to the save disk)
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
	struct Hazard {                           // KEH word_2E206/2F018/2D128/2E41C/dword_2EC04: repeating area effect while standing still
		std::string message;
		int min = 0, max = 0, interval = 0;
		bool damage = false;
	} hazard;
	bool lootFlag = false;                    // KEH byte_2EE06: a script put items on the party's cell
	uint16_t timeTicks = 0;                   // KEH _timeTicks: ticks since the game was loaded
	void damageMember(Member &m, int damage); // KEH damageMember: subtract hp, track last-hurt, mark the dead below -60
	bool partyWiped() const;                  // no member above -15 hp (KEH checkPartyWipeAndQuit condition)
	std::function<void()> onHourChange;       // KEH setTime -> checkTimedEffects (outside combat)
	bool passTime(bool combat);               // KEH passTime; false if the whole party died while waiting

	// -- maps --
	void loadMap(int mapNumber);              // loads map + scripts, applies persistent changes
	void setPartyPosition(int x, int y);
	MapCell &cellAt(int x, int y) { return map->cell(x, y); }
	void applyChange(const MapChange &c);
	void recordCellChange(int op, int mapNumber, int x, int y, int val, int extra);
	void applyCellChange(int x, int y, int op, int val, int extra);   // op 0x19..0x1E semantics on one cell
	int mapStackIndex() const { return disk.mapIndex; }

	// -- wandering enemies (KEH isCellPassable / spawnEnemyGroup / tryRandomEncounter) --
	bool isCellPassable(int x, int y) const;
	void markEntityOnMap(int radius, int x, int y, int overlay);
	void spawnEnemyGroup(EnemyGroup &g);
	bool tryRandomEncounter();                // true if a group appeared next to the party
	int partyLevelUnit() const;               // KEH getPartyLevelUnit
	static int directionTowards(int x, int y, int px, int py, bool toward);   // 0,3,..21 (KEH directionTowards)
	bool findFreeCellNear(uint8_t &px, uint8_t &py, int x, int y);
	void setMapCellHighBits(int x, int y, int v);
	void moveEntityStep(uint8_t &px, uint8_t &py, int reach, int detect, int toward, int overlay);
	void moveMapEntities();                   // monsters and groups creep toward the party
	void placeEntitiesOnMap();
	// skills
	int trySkillImprove(int slot, int skill);     // KEH trySkillImprove: 1 if the skill rose
	std::string skillName(int skill) const;       // 0..15 active, 16..31 passive
	std::string attributeName(int attr) const;
	// persistence of visited maps (KEH writes dirty maps back to the save disk)
	std::vector<std::pair<int, Bytes>> exportMaps();
	void importMaps(const std::vector<std::pair<int, Bytes>> &maps);
	// monsters / NPCs / roster
	Monster *findMonsterById(int id, size_t &index);
	bool addNpcToParty(size_t monsterIndex);  // KEH addNpcToParty; false if the party is full
	void removeMonster(size_t index, bool dropLoot);
	void removeMemberFromParty(int slot);
	int findFreeRosterSlot() const;
	// loot (KEH addGroundItem / dropPacketLoot / addScratchItemsAtCell)
	bool addGroundItem(int item, int qty, int x, int y);   // true if it merged into an existing stack
	void dropPacketLoot(int packet, int x, int y, int overrideFirstItem = -1);                // stamp their overlay ids onto the map

	// -- persistence list stored in Disk1.tail --
	void syncChangesToDisk();
	void loadChangesFromDisk();
};

} // namespace fod
