#ifndef ULTIMA6_FILES_WORLD_DATA_H
#define ULTIMA6_FILES_WORLD_DATA_H

#include "files/data_file.h"

namespace Ultima6 {

/**
 * SCHEDULE: u16 index[257] (first schedule entry of each of the 256 actors, the
 * last word being the total) followed by total * 5-byte entries. The entry
 * fields are not decoded yet.
 */
struct Schedule {
	enum { ACTORS = 256, HEADER_SIZE = (ACTORS + 1) * 2, ENTRY_SIZE = 5 };

	uint16_t index[ACTORS + 1];
	std::vector<uint8_t> entries; // total * ENTRY_SIZE bytes

	uint16_t total() const {
		return index[ACTORS];
	}
	/**
	 * Number of entries of an actor. Actors without a schedule point past the end
	 * (the file even has 564 for actor 255 with a total of 563), and an actor's
	 * entries run up to the next actor's start. This is how Nuvie interprets the
	 * index; GAME.EXE's own use of it is not yet traced.
	 */
	int entryCount(int actor) const;

	/**
	 * One 5-byte entry: byte 0 = hour (low 5 bits) and day of week (high 3 bits, 0 = every
	 * day); byte 1 = the worktype the actor takes up; bytes 2-4 = x, y (10 bits each) and z
	 * (4 bits) packed like an object position. GAME.EXE's schedule check (sub_2AED5, from
	 * the actors' hourly update) compares the hour with the game hour and the day of week
	 * with (day - 1) % 7 + 1, scanning an actor's entries from the last one.
	 */
	struct Entry {
		int hour, dayOfWeek, worktype, x, y, z;
	};
	Entry entry(int index) const;
	bool load(const Bytes &data);
};

/**
 * savegame\objlist, in the order and with the sizes GAME.EXE's load_world_data reads
 * and save_objlist writes it. The per-actor arrays have one entry per actor (256).
 *
 * The first five arrays (status, position, type/frame, quantity) are the first 256 entries
 * of the game's global object arrays, the ones object blocks are loaded into (actors are
 * objects 0-255).
 *
 * The program asks for 0x2CCC bytes for the final block but the file only holds the
 * remaining globals (0x182 bytes in a fresh game); loading therefore accepts a short
 * tail.
 */
struct ObjList {
	enum { ACTORS = 256, PARTY_MAX = 16, NAME_SIZE = 14, PARTY_NAMES = 16 };

	// File offsets of each block.
	enum {
		OFF_OBJ_STATUS = 0x000,
		OFF_POS = 0x100,
		OFF_OBJ_FRAME = 0x400,
		OFF_OBJ_QUANTITY = 0x600,
		OFF_STATUS = 0x800,
		OFF_STRENGTH = 0x900,
		OFF_DEXTERITY = 0xA00,
		OFF_INTELLIGENCE = 0xB00,
		OFF_EXP = 0xC00,
		OFF_HP = 0xE00,
		OFF_PARTY_NAMES = 0xF00,
		OFF_PARTY_ROSTER = 0xFE0,
		OFF_PARTY_SIZE = 0xFF0,
		OFF_LEVEL = 0xFF1,
		OFF_SCHEDULE_INDEX = 0x10F1,
		OFF_WORKTYPE = 0x11F1,
		OFF_UNKNOWN_12F1 = 0x12F1, // combat mode / magic points
		OFF_UNKNOWN_13F1 = 0x13F1, // magic points / combat mode
		OFF_MOVES = 0x14F1,
		OFF_BASE_OBJ_FRAME = 0x15F1,
		OFF_TALK_FLAGS = 0x17F1,
		OFF_UNKNOWN_18F1 = 0x18F1,
		OFF_MOVEMENT_FLAGS = 0x19F1,
		OFF_UNKNOWN_1AF1 = 0x1AF1,
		OFF_GLOBALS = 0x1BF1,
		MIN_SIZE = OFF_GLOBALS
	};

	uint8_t objStatus[ACTORS];    // object status/location bits of the actors (see ObjRecord)
	uint8_t pos[ACTORS][3];       // x: 10 bits, y: 10 bits, z: 4 bits
	uint16_t objFrame[ACTORS];    // object type (10 bits) | frame << 10
	uint16_t objQuantity[ACTORS]; // quantity (low byte) and quality (high byte) of the actors' objects
	uint8_t status[ACTORS];
	uint8_t strength[ACTORS];
	uint8_t dexterity[ACTORS];
	uint8_t intelligence[ACTORS];
	uint16_t exp[ACTORS];
	uint8_t hp[ACTORS];
	uint8_t partyNames[PARTY_NAMES][NAME_SIZE];
	uint8_t partyRoster[PARTY_MAX];
	uint8_t partySize;
	uint8_t level[ACTORS];
	uint8_t scheduleIndex[ACTORS]; // index (within the actor's entries) of the schedule entry in force
	uint8_t worktype[ACTORS];
	uint8_t unknown12f1[ACTORS];
	uint8_t unknown13f1[ACTORS];
	uint8_t moves[ACTORS];
	uint16_t baseObjFrame[ACTORS];
	uint8_t talkFlags[ACTORS];
	uint8_t unknown18f1[ACTORS];
	uint8_t movementFlags[ACTORS];
	uint8_t unknown1af1[ACTORS];
	std::vector<uint8_t> globals;

	bool load(const Bytes &data);
	/** The inverse of load: the file as GAME.EXE's save_objlist writes it. */
	void save(Bytes &data) const;
	void setPosition(int actor, int x, int y, int z);

	void position(int actor, int &x, int &y, int &z) const;
	int objType(int actor) const {
		return objFrame[actor] & 0x3FF;
	}
	int frame(int actor) const {
		return objFrame[actor] >> 10;
	}
};

} // namespace Ultima6

#endif
