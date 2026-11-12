#ifndef ULTIMA6_ULTIMA6_CONVERSATION_H
#define ULTIMA6_ULTIMA6_CONVERSATION_H

#include "files/data_file.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Ultima6 {
namespace Ultima6 {

/**
 * What a conversation script can ask of the game (the operators and commands of GAME.EXE's
 * conv_read_value / conv_execute_command that touch actors, inventories and the party).
 * "npc" numbers are actor numbers; the script's own 0xEB (the speaker) is resolved by the interpreter.
 */
class ConversationHost {
public:
	virtual ~ConversationHost() {
	}

	virtual void print(const std::string &text) = 0; // a backspace character erases the last character (the page-break star)
	virtual std::string playerName() = 0;
	virtual std::string genderTitle() = 0; // "milord" or "milady"
	virtual std::string timeOfDay() = 0;   // "morning", "afternoon" or "evening"
	virtual void showPortrait(int npc) = 0;
	virtual int roll(int lo, int hi) = 0;
	virtual std::string scriptName(int npc) = 0; // the name in an actor's script header
	virtual std::string partyName(int npc, bool &inParty) = 0;

	virtual int talkFlags(int npc) = 0;
	virtual void setTalkFlags(int npc, int flags) = 0;
	virtual bool inParty(int npc) = 0;
	virtual int partyMember(int index) = 0; // actor number of party member index, 0 if none
	virtual int joinParty(int npc) = 0;     // 0 joined, 1 not on land, 2 party full, 3 already in it
	virtual int leaveParty(int npc) = 0;    // 0 left, 1 not on land, 2 not in the party
	virtual bool npcNearby(int npc) = 0;
	virtual bool wounded(int npc) = 0;
	virtual bool poisoned(int npc) = 0;
	virtual void heal(int npc) = 0;
	virtual void cure(int npc) = 0;
	virtual void resurrect(int npc) = 0;
	virtual void setWorktype(int npc, int worktype) = 0;
	virtual int adjustStat(int npc, int stat, int delta) = 0; // stat 0 exp, 1 level, 2 str, 3 int, 4 dex; returns the new value

	virtual bool hasObject(int npc, int obj, int quality, bool checkQuality) = 0;
	virtual int countObject(int npc, int obj) = 0;
	virtual void newObject(int npc, int obj, int quality, int quantity) = 0;
	virtual void deleteObject(int npc, int obj, int quality, int quantity) = 0;
	virtual void giveObject(int obj, int quality, int from, int to) = 0;
	virtual int whoHasObject(int obj, int quality) = 0; // party member's actor number, or -1
	virtual int maxCarry(int npc) = 0;
	virtual int objectWeight(int obj, int quantity) = 0;
	virtual bool horsed(int npc) = 0;
	virtual void mount(int npc) = 0;
	virtual void addKarma(int n) = 0;
	virtual void subtractKarma(int n) = 0;
	virtual void sleep() = 0;
	/** Martian Dreams' script command 0xD1: a story event by its number (sub_3F869); 0xC8 moves an object by its index into an actor's pack. */
	virtual void event(int) {
	}
	virtual void moveObjectTo(int, int) {
	}
};

/**
 * Interpreter of an Ultima VI conversation script (an actor's entry in CONVERSE.A/B).
 *
 * A script is text mixed with control codes. Text is printed with the special symbols resolved
 * ($P player, $N speaker, $G milord/milady, $T time of day, $Y name set by SETNAME, $Z the last
 * input, $n / #n string and number variables; `/` and `\` choose singular or plural endings,
 * `+...+` is skipped). Commands (0x9C and up) are followed by expressions in postfix, each ended
 * by 0xA7 (see ConverseScan); the keyword blocks `0xEF words 0xF6 answer ... 0xEE` run their
 * answer when the player's input (cut to the keyword's length) matches one of the words.
 *
 * The interpreter runs until it needs the player (a line, a character, a key at a page break) or
 * the conversation ends. The implementation follows Nuvie's interpreter, which reads the same scripts.
 */
class Conversation {
public:
	enum State { NEED_LINE, NEED_CHAR, NEED_KEY, ENDED };

	enum { VAR_SEX = 0x10, VAR_KARMA = 0x14, VAR_GARGF = 0x15, VAR_PARTYLIVE = 0x17, VAR_PARTYALL = 0x18, VAR_HP = 0x19,
		   VAR_QUESTF = 0x1A, VAR_WORKTYPE = 0x20, VAR_YSTRING = 0x22, VAR_INPUT = 0x23, VAR_LAST = 0x25 };

	Conversation(const Bytes &script, int npc, ConversationHost &host);

	void setVar(int v, uint32_t value);
	void setSVar(int v, const std::string &value);
	uint32_t var(int v) const {
		return v <= VAR_LAST ? _vars[v] : 0;
	}

	/** Runs until the script needs something. */
	State run();
	/** The state after run(): characters accepted by NEED_CHAR ("" = any). */
	const std::string &allowed() const {
		return _allowed;
	}
	void giveLine(const std::string &line);
	void giveChar(int c);
	void giveKey();

	const std::string &name() const {
		return _name;
	}
	const std::string &description() const {
		return _description;
	}

private:
	struct Value {
		uint32_t v;
		uint8_t size; // 0, or 0xD2 / 0xD3 / 0xD4 when read with an explicit size
	};
	struct TypedValue {
		uint8_t type;
		uint32_t val;
	};
	struct Frame {
		uint32_t startCode;
		bool run;
		uint32_t breakCode;
	};
	enum Answer { ANSWER_NO, ANSWER_YES, ANSWER_DONE };

	// reading the script
	bool overflow(int ahead = 0) const {
		return _pos + ahead >= _script.size();
	}
	uint32_t peek(size_t ahead = 0) const {
		return _pos + ahead < _script.size() ? _script[_pos + ahead] : 0;
	}
	uint32_t read();
	uint32_t read2();
	uint32_t read4();

	static bool isPrint(uint32_t c);
	static bool isValOp(uint32_t c);
	static bool isDataSize(uint32_t c);
	static bool isCtrl(uint32_t c);

	void step();
	void collectInput();
	void addText();
	void exec();
	void doFrame(uint32_t c);
	Frame *top() {
		return _frames.empty() ? nullptr : &_frames.back();
	}
	bool running() const {
		return _frames.empty() || _frames.back().run;
	}
	void eval(size_t from);
	bool evop(std::vector<TypedValue> &st);
	bool op(std::vector<TypedValue> &st);
	uint32_t pop(std::vector<TypedValue> &st);
	TypedValue popTyped(std::vector<TypedValue> &st);

	std::string format(const std::string &text);
	int npcNumber(uint32_t n) const {
		return n != 0xEB ? (int)n : _npc;
	}
	const std::string &rstr(uint32_t i) const;
	uint32_t addRstr(const std::string &s);
	void setYString(const std::string &s);

	bool matchKeywords(const std::string &keys, const std::string &input) const;
	// the data sections
	std::string dbString(uint32_t loc, uint32_t index) const;
	uint32_t dbInteger(uint32_t loc, uint32_t index) const;
	void setDbInteger(uint32_t loc, uint32_t index, uint32_t value);
	uint32_t findDbString(uint32_t loc, const std::string &s) const;

	Bytes _script;
	size_t _pos = 0;
	int _npc;
	ConversationHost &_host;
	std::string _name, _description;

	uint32_t _vars[VAR_LAST + 1] = {};
	std::string _svars[VAR_LAST + 1];

	std::vector<Value> _in; // the statement being collected
	size_t _inStart = 0;
	std::string _text;      // text being collected
	std::vector<std::string> _rstrings;
	std::string _ystring;
	uint8_t _declVar = 0, _declType = 0;
	bool _dbLvar = false;
	uint32_t _dbLoc = 0, _dbOffset = 0;
	std::vector<Frame> _frames;
	Answer _answer = ANSWER_NO;
	State _state = NEED_KEY;
	bool _waiting = false, _stopped = false;
	std::string _allowed;
	std::string _lastInput;
	std::string _pendingOut; // formatted text not yet printed (a page-break star stopped it)
	size_t _askPos = 0;      // where the last ASK command started, for the "look" input
	uint32_t _lastValue = 0;

	void emit(const std::string &text);
	void flushOutput();
	void block(State s);
};

} // namespace Ultima6
} // namespace Ultima6

#endif
