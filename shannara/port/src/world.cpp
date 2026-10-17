#include "world.h"

namespace shannara {

static const uint32_t kSlotCounts = 0x20891; // 5 x 3 bytes, first byte of each triple = child-list slots of that type
static const uint32_t kAttrDesc = 0x208A0;   // 24 x {u8 bitOffset; u32 typeMask}, up to the object table at 0x20918

World::World(const LeExe &exe, SaveState &state) : _s(state) {
	for (int t = 1; t <= 5; t++)
		_slotCount[t] = exe.u8(kSlotCounts + 3 * (t - 1));
	_attrCount = (0x20918 - kAttrDesc) / 5;
	for (int i = 0; i < _attrCount; i++) {
		_attrBit[i] = exe.u8(kAttrDesc + 5 * i);
		_attrMask[i] = exe.u32(kAttrDesc + 5 * i + 1);
	}
}

// ---- tree fields (offsets by type exactly as in sub_1A200 .. sub_1A4E8) ----------------------------------------------------

int World::location(int obj) const {
	if (!valid(obj))
		return 0;
	const ObjectRecord &r = _s.objects[obj - 1];
	return r.type == 2 || r.type == 5 ? rd16(r, 0xA) : r.type == 3 ? rd16(r, 0xB) : 0;
}

void World::setLocation(int obj, int loc) {
	if (!valid(obj))
		return;
	ObjectRecord &r = _s.objects[obj - 1];
	if (r.type == 2 || r.type == 5)
		wr16(r, 0xA, (uint16_t)loc);
	else if (r.type == 3)
		wr16(r, 0xB, (uint16_t)loc);
}

int World::slot(int obj) const {
	if (!valid(obj))
		return 0;
	const ObjectRecord &r = _s.objects[obj - 1];
	return r.type == 2 || r.type == 5 ? r.raw[0xC] : r.type == 3 ? r.raw[0xD] : 0;
}

void World::setSlot(int obj, int slot) {
	if (!valid(obj))
		return;
	ObjectRecord &r = _s.objects[obj - 1];
	if (r.type == 2 || r.type == 5)
		r.raw[0xC] = (uint8_t)slot;
	else if (r.type == 3)
		r.raw[0xD] = (uint8_t)slot;
}

int World::nextSibling(int obj) const {
	if (!valid(obj))
		return 0;
	const ObjectRecord &r = _s.objects[obj - 1];
	return r.type == 2 || r.type == 5 ? rd16(r, 0xD) : r.type == 3 ? rd16(r, 0xE) : 0;
}

void World::setNextSibling(int obj, int next) {
	if (!valid(obj))
		return;
	ObjectRecord &r = _s.objects[obj - 1];
	if (r.type == 2 || r.type == 5)
		wr16(r, 0xD, (uint16_t)next);
	else if (r.type == 3)
		wr16(r, 0xE, (uint16_t)next);
}

// child-list heads: types 1,4 at +0Ah; type 2 at +0Fh; type 3 at +10h + 2*slot; type 5 at +0Fh + 2*slot
int World::listHead(int parent, int slot) const {
	if (!valid(parent))
		return 0;
	const ObjectRecord &r = _s.objects[parent - 1];
	if (slot >= _slotCount[r.type])
		return 0;
	switch (r.type) {
	case 1: case 4: return rd16(r, 0xA);
	case 2: return rd16(r, 0xF);
	case 3: return rd16(r, 0x10 + 2 * slot);
	default: return rd16(r, 0xF + 2 * slot);
	}
}

void World::setListHead(int parent, int slot, int child) {
	if (!valid(parent))
		return;
	ObjectRecord &r = _s.objects[parent - 1];
	if (slot >= _slotCount[r.type])
		return;
	switch (r.type) {
	case 1: case 4: wr16(r, 0xA, (uint16_t)child); break;
	case 2: wr16(r, 0xF, (uint16_t)child); break;
	case 3: wr16(r, 0x10 + 2 * slot, (uint16_t)child); break;
	default: wr16(r, 0xF + 2 * slot, (uint16_t)child); break;
	}
}

bool World::isWithin(int obj, int ancestor) const {
	int a = location(obj);
	for (int guard = 0; a != 0 && guard < 4096; guard++) {
		if (a == ancestor)
			return true;
		a = location(a);
	}
	return false;
}

void World::move(int obj, int newParent, int slot) {
	int oldParent = location(obj);
	int oldSlot = this->slot(obj);
	int p = listHead(oldParent, oldSlot);
	if (p == obj) {
		setListHead(oldParent, oldSlot, nextSibling(obj));
	} else {
		for (int guard = 0; p != 0 && guard < 4096; guard++) {
			int nx = nextSibling(p);
			if (nx == obj) {
				setNextSibling(p, nextSibling(obj));
				break;
			}
			p = nx;
		}
	}
	setLocation(obj, newParent);
	setSlot(obj, slot);
	if (newParent != 0) {
		int oldHead = listHead(newParent, slot);
		setListHead(newParent, slot, obj);
		setNextSibling(obj, oldHead);
	}
	// the original then clears the UI selection if it was `obj` (sub_10F70); there is no UI state in this model
}

// ---- attributes -----------------------------------------------------------------------------------------------------------

uint8_t *World::attrByte(int obj, int attr, int &bit) const {
	if (!valid(obj) || attr < 1 || attr > _attrCount)
		return nullptr;
	const ObjectRecord &r = _s.objects[obj - 1];
	if (!((_attrMask[attr - 1] >> (r.type - 1)) & 1))
		return nullptr; // the attribute does not apply to this object type
	bit = _attrBit[attr - 1] & 7;
	size_t off = 8 + (_attrBit[attr - 1] >> 3);
	if (off >= r.raw.size())
		return nullptr; // outside the record (cannot happen with the shipped descriptors)
	return const_cast<uint8_t *>(&r.raw[off]);
}

bool World::hasAttr(int obj, int attr) const {
	int bit = 0;
	uint8_t *p = attrByte(obj, attr, bit);
	return p && ((*p >> bit) & 1);
}

void World::setAttr(int obj, int attr) {
	int bit = 0;
	if (uint8_t *p = attrByte(obj, attr, bit))
		*p |= (uint8_t)(1 << bit);
}

void World::clearAttr(int obj, int attr) {
	int bit = 0;
	if (uint8_t *p = attrByte(obj, attr, bit))
		*p &= (uint8_t)~(1 << bit);
}

void World::setFlag(int n, bool v) {
	uint8_t *p = _s.at(0xCD8AC);
	if (!p || n < 1 || n > 800)
		return;
	uint8_t m = (uint8_t)(1 << ((n - 1) & 7));
	if (v)
		p[(n - 1) >> 3] |= m;
	else
		p[(n - 1) >> 3] &= (uint8_t)~m;
}

} // namespace shannara
