#include "fod/script.h"
#include <sstream>

namespace fod {

static const int8_t kOpLen[76] = {
	4, 2, 1, 1, 3, 3, 2, 2, 0, 3, 2, 2, 4, 2, 0, 1,
	1, 3, 1, 4, 3, 2, 0, 1, 3, 2, 4, 2, 5, 7, 5, 1,
	2, 0, 3, 2, 2, 1, 0, 1, 0, 4, 2, 2, 2, 1, 0, 3,
	0, 0, 2, 0, 4, 2, 1, 1, 2, 1, 3, 3, 0, 2, 1, 3,
	2, 2, 2, 0, 0, 1, 3, 4, 0, 3, 1, 1};

static const char *kOpName[76] = {
	"hazardOnce", "calc", "move", "set", "getStat", "setStat", "partyMax", "partyMinAlive",
	"getCash", "hasItem", "uniqueItem", "itemOps", "changeMap", "gosub", "nextMember", "inputText",
	"matchText", "damageParty", "healParty", "compare", "if", "else", "endif", "print",
	"dropLoot", "cellTile", "cellFields", "cellScript", "mapTile", "mapFields", "mapScript", "sound",
	"random", "op33", "skillCheck", "weaponIs", "eventArgIs", "shop", "hideParty", "yesNo",
	"restorePos", "hazardOnce2", "setMonsterState", "inParty", "leftParty", "service", "getTime", "menuBegin",
	"menuEnd", "menuReturn", "revealItems", "op51", "moveMonster", "spawnGroup", "npcJoin", "keyIs",
	"giveItem", "message", "monsterHp", "removeItem", "getWeekday", "groupAlive", "pushParty", "testStatus",
	"setStatus", "clearStatus", "freeSlots", "return", "end", "setFacing", "damagePartyNoArmor", "hazardOnce3",
	"storeMember", "setCellHigh", "ending", "moveMonster2"};

static const char *kSig[11] = {"", "B", "BB", "W", "BBB", "BW", "BBBB", "BBW", "WW", "BBBW", "BBBWW"};

const char *scriptSignature(int sig) { return sig >= 0 && sig < 11 ? kSig[sig] : "?"; }
int scriptOpLength(int op) { return op >= 0 && op < 76 ? kOpLen[op] : -1; }
const char *scriptOpName(int op) { return op >= 0 && op < 76 ? kOpName[op] : "op?"; }

ScriptBank::ScriptBank(Bytes decoded) : _d(std::move(decoded)) {
	Reader r(_d);
	for (size_t i = 0; i < kMaxScripts && r.pos() + 2 <= _d.size(); ++i)
		_offsets.push_back(r.u16());
}

bool ScriptBank::has(unsigned idx) const {
	return idx < _offsets.size() && _offsets[idx] != 0 && _offsets[idx] + 4u <= _d.size();
}

ScriptBlock ScriptBank::block(unsigned idx) const {
	ScriptBlock b;
	if (!has(idx)) return b;
	const size_t base = _offsets[idx];
	Reader r(_d, base);
	b.eventMask = r.u16();
	const size_t strOfs = base + r.u16();
	while (r.pos() < strOfs) {
		ScriptRecord rec;
		rec.offset = r.pos() - base;
		const int head = r.u8();
		rec.eventClass = head & 0xF;
		rec.sig = head >> 4;
		rec.opcode = r.u8();
		const char *sg = scriptSignature(rec.sig);
		for (const char *p = sg; *p; ++p) rec.params.push_back(*p == 'B' ? r.u8() : r.u16());
		b.records.push_back(rec);
	}
	// Strings are NUL-terminated and run until the next block starts.
	size_t limit = _d.size();
	for (uint16_t o : _offsets)
		if (o > strOfs && o < limit) limit = o;
	r.seek(strOfs);
	while (r.pos() < limit) {
		std::string s;
		while (r.pos() < limit) {
			const char c = char(r.u8());
			if (c == 0) break;
			s += c;
		}
		b.strings.push_back(s);
	}
	return b;
}

std::string ScriptBank::disassemble(unsigned idx) const {
	std::ostringstream os;
	if (!has(idx)) return "(no script)\n";
	const ScriptBlock b = block(idx);
	os << "script " << idx << " eventMask=0x" << std::hex << b.eventMask << std::dec << "\n";
	for (const auto &r : b.records) {
		os << "  +" << r.offset << " [" << r.eventClass << "] " << scriptOpName(r.opcode) << "(" << r.opcode << ")";
		for (int p : r.params) os << " " << p;
		os << "\n";
	}
	for (size_t i = 0; i < b.strings.size(); ++i) os << "  str" << i << ": \"" << b.strings[i] << "\"\n";
	return os.str();
}

} // namespace fod
