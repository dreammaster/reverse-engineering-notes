#ifndef ULTIMA6_FILES_CONVERSE_H
#define ULTIMA6_FILES_CONVERSE_H

#include "files/data_file.h"

#include <string>

namespace Ultima6 {

/**
 * Conversation scripts, from CONVERSE.A and CONVERSE.B, as GAME.EXE's load_conversation
 * (sub_395D0) selects and reads them:
 *
 *   actor 0..98      CONVERSE.A item = actor number
 *   actor 99..223    CONVERSE.B item = actor number - 99
 *   actor >= 224     only creatures whose object type is 0x175, 0x17E or 0x16B can talk;
 *                    they use CONVERSE.B items 102, 103 and 104 (the scripts of actors
 *                    201-203)
 *
 * Both files are offset-table archives (see LibN, no size header). An item either starts
 * with the u32 size of an LZW stream's output (non-zero and below 0x2800: the item is
 * the LZW stream, see lzw.h) or with a zero u32 followed by the plain script (at most
 * 0x2800 bytes are read). A script begins 0xFF, the actor number, the name, 0xF1 and the
 * description, then the code (0xF2 ...).
 */
struct ConverseRef {
	bool second; // CONVERSE.B
	int item;
};

class ConverseScript {
public:
	enum { MAX_SIZE = 0x2800, SIDENT = 0xFF, SLOOK = 0xF1, SCONVERSE = 0xF2 };

	/** Which archive item holds the script of an actor (object type matters only for actors >= 224); false if it can't talk. */
	static bool locate(int actor, int objectType, ConverseRef &ref);
	/** Reads an item from the archive file's bytes. */
	bool load(const Bytes &archive, int item);

	const Bytes &bytes() const {
		return _bytes;
	}
	int actor() const {
		return _bytes[1];
	}
	const std::string &name() const {
		return _name;
	}
	const std::string &description() const {
		return _description;
	}
	/** Offset of the first byte of code (the 0xF2 that starts the conversation). */
	size_t codeStart() const {
		return _codeStart;
	}

private:
	Bytes _bytes;
	std::string _name, _description;
	size_t _codeStart;
};

} // namespace Ultima6

#endif
