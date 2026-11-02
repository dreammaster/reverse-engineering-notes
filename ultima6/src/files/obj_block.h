#ifndef ULTIMA6_FILES_OBJ_BLOCK_H
#define ULTIMA6_FILES_OBJ_BLOCK_H

#include "files/data_file.h"

#include <string>

namespace Ultima6 {

/**
 * One object as stored in an object block (and in memory, where GAME.EXE splits the
 * fields into parallel arrays of at most 0xC00 objects: status at ds:A0AB, position
 * at ds:6828, type/frame at ds:3548, quantity word at ds:4EE4, "next object" link at
 * ds:BDDA). The first 256 entries of those arrays are the actors, which is why
 * savegame\objlist holds exactly those arrays.
 */
struct ObjRecord {
	enum Location { ON_MAP = 0x00, IN_CONTAINER = 0x08, IN_INVENTORY = 0x10, READIED = 0x18, LOCATION_MASK = 0x18 };

	uint8_t status;     // +0
	uint16_t x, y;      // +1..+3, 10 + 10 bits
	uint8_t z;          //         4 bits
	uint16_t typeFrame; // +4, type in the low 10 bits, frame in the high 6
	uint8_t quantity;   // +6
	uint8_t quality;    // +7

	int location() const {
		return status & LOCATION_MASK;
	}
	int type() const {
		return typeFrame & 0x3FF;
	}
	int frame() const {
		return typeFrame >> 10;
	}
	/** A contained object stores the index (within the block) of its container in x. */
	int containerIndex() const {
		return x;
	}
};

/**
 * An object block file: u16 count (the game rejects more than 0xC00) followed by
 * count 8-byte records. Surface blocks are named OBJBLK<col><row> with col = x >> 7 and
 * row = y >> 7 as letters from 'A'; the five dungeon levels are OBJBLK<level>I with
 * level 1..5 as 'A'..'E'. Which letters go where is read off sub_1BBA2/sub_1C2AF/sub_1C566
 * and checked against every object of the shipped files.
 */
struct ObjBlock {
	enum { MAX_OBJECTS = 0xC00, RECORD_SIZE = 8 };

	std::vector<ObjRecord> objects;

	bool load(const Bytes &data);

	static std::string surfaceFileName(int col, int row);
	static std::string dungeonFileName(int level);
};

} // namespace Ultima6

#endif
