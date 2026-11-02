#include "files/converse.h"

#include "files/lib_n.h"
#include "files/lzw.h"

namespace Ultima6 {

bool ConverseScript::locate(int actor, int objectType, ConverseRef &ref) {
	if (actor < 0)
		return false;
	if (actor < 0xE0) {
		ref.second = actor > 98;
		ref.item = actor > 98 ? actor - 99 : actor;
		return true;
	}
	ref.second = true;
	switch (objectType) {
	case 0x175:
		ref.item = 0x66;
		return true;
	case 0x17E:
		ref.item = 0x67;
		return true;
	case 0x16B:
		ref.item = 0x68;
		return true;
	default:
		return false;
	}
}

bool ConverseScript::load(const Bytes &archive, int item) {
	_bytes.clear();
	_name.clear();
	_description.clear();
	LibN lib;
	if (!lib.load(archive, false) || !lib.present(item))
		return false;
	size_t offset = 0, size;
	if (!lib.item(item, size) || size < 4)
		return false;
	const uint8_t *p = lib.item(item, size);
	uint32_t header = p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);

	if (header != 0 && header < MAX_SIZE) {
		// LZW: the stream starts at the item itself
		Bytes raw(p, p + size);
		if (!lzwDecompress(raw, offset, _bytes))
			return false;
	} else {
		size_t n = size - 4 < (size_t)MAX_SIZE ? size - 4 : (size_t)MAX_SIZE;
		_bytes.assign(p + 4, p + 4 + n);
	}

	// 0xFF, actor, name, 0xF1, description, then the code
	if (_bytes.size() < 4 || _bytes[0] != SIDENT)
		return false;
	size_t i = 2;
	while (i < _bytes.size() && _bytes[i] != SLOOK)
		_name += (char)_bytes[i++];
	if (i >= _bytes.size())
		return false;
	++i;
	while (i < _bytes.size() && _bytes[i] != SCONVERSE)
		_description += (char)_bytes[i++];
	_codeStart = i;
	return i < _bytes.size();
}

namespace {

// Number of expressions each command takes (from the read_value calls of its handler).
int commandArity(int op) {
	switch (op) {
	case 0x9C: case 0xA1: case 0xBE: case 0xBF: case 0xC4: case 0xC5: case 0xD0: case 0xD6:
	case 0xD8: case 0xD9: case 0xDB: case 0xDF:
		return 1;
	case 0xA4: case 0xA5: case 0xA6: case 0xC8: case 0xCD:
		return 2;
	case 0xC9: case 0xB9: case 0xBA:
		return 4;
	case 0x9E: case 0xA2: case 0xA3: case 0xB6: case 0xCB: case 0xEE: case 0xF7: case 0xF8:
		return 0;
	default:
		return -1;
	}
}

bool isTextByte(uint8_t c) {
	return c == 0x0A || c == 0x0D || (c >= 0x20 && c <= 0x7E);
}

// Skips tokens up to and including a terminator; false if the data ends first.
bool skipTokens(const Bytes &b, size_t &j, bool toVariable) {
	while (j < b.size()) {
		uint8_t t = b[j++];
		if (t == 0xD2)
			j += 4;
		else if (t == 0xD3)
			j += 1;
		else if (t == 0xD4)
			j += 2;
		else if (toVariable ? (t == 0xB2 || t == 0xB3) : (t == 0xA7 || t == 0xA8))
			return j <= b.size();
	}
	return false;
}

} // namespace

void ConverseScript::scan(ConverseScan &r) const {
	r.items.clear();
	r.keywords.clear();
	r.clean = true;
	const Bytes &b = _bytes;
	size_t n = b.size(), i = _codeStart + 1;
	auto add = [&](ConverseScan::Kind k, size_t off, size_t len, int op = 0, uint32_t target = 0) {
		ConverseScan::Item it = {k, off, len, op, target};
		r.items.push_back(it);
	};

	while (i < n) {
		uint8_t c = b[i];
		int arity = commandArity(c);
		if (c == 0xB0) {
			if (i + 5 > n) {
				r.clean = false;
				break;
			}
			uint32_t t = b[i + 1] | (b[i + 2] << 8) | (b[i + 3] << 16) | ((uint32_t)b[i + 4] << 24);
			add(ConverseScan::JUMP, i, 5, c, t);
			i += 5;
		} else if (arity >= 0) {
			size_t j = i + 1;
			bool ok = true;
			for (int a = 0; a < arity && ok; ++a)
				ok = skipTokens(b, j, false);
			if (!ok) { // not code: the data tables begin here
				add(ConverseScan::PAD, i, n - i);
				r.clean = false;
				i = n;
				break;
			}
			add(ConverseScan::COMMAND, i, j - i, c);
			i = j;
		} else if (c == 0xB5) { // DPRINT d2 <u32> then B1 or an expression, or d5
			if (i + 1 < n && b[i + 1] == 0xD5) {
				add(ConverseScan::COMMAND, i, 2, c);
				i += 2;
				continue;
			}
			size_t j = i + 6;
			if (i + 1 >= n || b[i + 1] != 0xD2 || j >= n) {
				r.clean = false;
				break;
			}
			if (b[j] == 0xB1)
				++j;
			else if (!skipTokens(b, j, false)) {
				r.clean = false;
				break;
			}
			add(ConverseScan::COMMAND, i, j - i, c);
			i = j;
		} else if (c == 0xF9 || c == 0xFB || c == 0xFC) {
			size_t j = i + 1;
			if (!skipTokens(b, j, true)) {
				r.clean = false;
				break;
			}
			add(ConverseScan::COMMAND, i, j - i, c);
			i = j;
		} else if (c == 0xEF) {
			size_t j = i + 1;
			while (j < n && b[j] != 0xF6)
				++j;
			add(ConverseScan::KEYWORDS, i, j - i, c);
			if (j < n) {
				ConverseKeywordEntry e;
				e.keywords.assign((const char *)&b[i + 1], j - i - 1);
				e.answerOffset = j + 1;
				r.keywords.push_back(e);
			}
			i = j;
		} else if (c == 0xF6) {
			add(ConverseScan::ANSWER, i, 1, c);
			++i;
		} else if (isTextByte(c)) {
			size_t j = i;
			while (j < n && isTextByte(b[j]))
				++j;
			add(ConverseScan::TEXT, i, j - i);
			i = j;
		} else if (c == 0x00) {
			add(ConverseScan::PAD, i, n - i);
			i = n;
			break;
		} else { // the data tables
			add(ConverseScan::DATA, i, n - i);
			i = n;
			break;
		}
	}
	r.end = i;
}

} // namespace Ultima6
