#include "files/portrait.h"

#include "files/lib_n.h"
#include "files/lzw.h"

namespace Ultima6 {

bool Portrait::locate(int actor, int objectType, int avatarPortraitPlus1, PortraitRef &ref) {
	if (actor < 0)
		return false;
	int number = actor;
	if (actor >= 0xE0) {
		switch (objectType) {
		case 0x175:
			number = 0xC0;
			break;
		case 0x17E:
			number = 0xC1;
			break;
		case 0x16B:
			number = 0xC2;
			break;
		default:
			return false;
		}
	}
	if (number == 1 && actor < 0xE0) {
		if (avatarPortraitPlus1 < 1)
			return false;
		ref.file = 'Z';
		ref.item = avatarPortraitPlus1 - 1;
		return true;
	}
	if (number != 0)
		--number;
	if (number < A_ITEMS) {
		ref.file = 'A';
		ref.item = number;
	} else {
		ref.file = 'B';
		ref.item = number - A_ITEMS;
	}
	return true;
}

bool Portrait::decode(const Bytes &archive, int item, Bytes &pixels) {
	LibN lib;
	size_t size;
	if (!lib.load(archive, false) || !lib.present(item))
		return false;
	const uint8_t *p = lib.item(item, size);
	if (!p)
		return false;
	Bytes raw(p, p + size);
	return lzwDecompress(raw, 0, pixels) && pixels.size() == SIZE;
}

} // namespace Ultima6
