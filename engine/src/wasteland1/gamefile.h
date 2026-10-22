#pragma once
// GAME1 / GAME2: sequence of "msq" blocks (maps, one savegame block, shop item lists).
#include "wasteland1/codec.h"
#include <string>

namespace wl {

struct MapInfo {
	uint8_t unknown0, unknown1, encounterFrequency, tileset, lastMonster, maxEncounters, backgroundTile;
	uint16_t timeFactor;
	uint8_t unknown9;
};

struct MapMonster {
	std::string name;
	uint16_t experience;
	uint8_t skill, randomDamage, maxGroupSize, ac, fixedDamage, weaponType, monsterType, picture;
};

struct Npc {                   // 0x100-byte character record (same layout as the savegame party members)
	Bytes raw;
	std::string name() const;
};

struct GameMap {
	int size = 0;                              // 32 or 64
	size_t msqSize = 0;
	size_t tilemapOffset = 0;
	Bytes decoded;                             // block bytes after de-obfuscation (header and key stripped)
	Bytes actionClass;                         // size*size nibbles, one per byte
	Bytes action;                              // size*size action ids
	uint16_t stringsOffset = 0, monsterNamesOffset = 0, monsterDataOffset = 0;
	uint16_t actionClassOffset[16] = {};
	uint16_t nibble6Offset = 0, npcOffset = 0;
	MapInfo info{};
	uint8_t battleStrings[37] = {};
	Bytes tiles;                               // size*size tile ids
	std::vector<std::string> strings;
	std::vector<Npc> npcs;
	std::vector<MapMonster> monsters;
	std::vector<uint16_t> specialActions;      // the 128-entry table at nibble6Offset
};

struct GameFile {
	Bytes data;
	std::vector<MsqBlock> blocks;
	std::vector<GameMap> maps;
	std::vector<size_t> shopBlocks, saveBlocks; // indices into blocks
};

GameFile loadGameFile(const std::string &path);
GameMap parseMapBlock(const Bytes &file, const MsqBlock &blk);

} // namespace wl
