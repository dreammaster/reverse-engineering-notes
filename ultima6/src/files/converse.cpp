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

} // namespace Ultima6
