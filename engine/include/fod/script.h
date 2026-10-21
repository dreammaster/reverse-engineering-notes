#pragma once
#include "fod/bytes.h"

namespace fod {

// Parameter signature (record head high nibble) -> layout of B(yte)/W(ord) parameters.
const char *scriptSignature(int sig);
// Number of parameter bytes of an opcode (KEH dseg:0F81h table); -1 if unknown opcode.
int scriptOpLength(int opcode);
const char *scriptOpName(int opcode);

struct ScriptRecord {
	int eventClass;
	int opcode;
	int sig;
	std::vector<int> params;
	size_t offset;            // byte offset inside the block
};

struct ScriptBlock {
	uint16_t eventMask = 0;
	std::vector<ScriptRecord> records;
	size_t stringBase = 0;               // absolute offset of string 0 in the decoded bank
	std::vector<std::string> strings;    // first few strings (for disassembly only)
};

class ScriptBank {
public:
	explicit ScriptBank(Bytes decoded);
	static constexpr size_t kMaxScripts = 1500;
	bool has(unsigned idx) const;
	ScriptBlock block(unsigned idx) const;
	// KEH getScriptString: skip n NUL-terminated strings from the block's string area (no explicit length).
	std::string string(unsigned idx, unsigned n) const;
	std::string disassemble(unsigned idx) const;
private:
	Bytes _d;
	std::vector<uint16_t> _offsets;
};

} // namespace fod
