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
/**
 * The structure of a script's code, as far as it is needed to find things in it without
 * running it. Derived from the original's interpreter (GAME.EXE conv_execute_command /
 * conv_read_value) and checked by scanning every shipped script:
 *
 *   - an *expression* is a postfix token string ending with 0xA7 (or 0xA8): tokens 0xD2 / 0xD3 / 0xD4
 *     are followed by a u32 / u8 / u16 constant, other tokens below 0x81 are small constants, the
 *     rest are operators (comparisons 81-86, arithmetic 90-95, VAR b2, SVAR b3, FLAG ab, ...)
 *   - commands (0x9C-0xDF) take a fixed number of expressions (SETF 2, GIVE 4, ...), JUMP (0xB0) a u32
 *     absolute offset, the input commands 0xF9/0xFB/0xFC a variable (tokens up to a 0xB2/0xB3)
 *   - 0xEF starts a comma-separated keyword list that runs to the 0xF6 which starts its answer
 *   - everything else printable is text; the code ends at a zero pad or where the data tables begin
 */
struct ConverseKeywordEntry {
	std::string keywords;  // "name" or "heav,burd,trou,time"
	size_t answerOffset;   // offset of the byte after 0xF6
};

struct ConverseScan {
	enum Kind { TEXT, COMMAND, JUMP, KEYWORDS, ANSWER, PAD, DATA };
	struct Item {
		Kind kind;
		size_t offset, length;
		int op;
		uint32_t target; // JUMP only
	};
	std::vector<Item> items;
	std::vector<ConverseKeywordEntry> keywords;
	bool clean; // the whole code was understood up to its end (a pad or the data tables)
	size_t end; // offset where the scan stopped
};

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
	/** A script that is already decoded (the spin-offs' TALK.LZC items): the header is parsed as `load` does. */
	bool fromBytes(const Bytes &script);

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
	/** Scans the code (see ConverseScan). */
	void scan(ConverseScan &result) const;

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
