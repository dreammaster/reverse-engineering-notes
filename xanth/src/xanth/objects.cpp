#include "xanth/objects.h"

#include "xanth/tables/object_tables.inc"

namespace Xanth {

namespace {

// Field offsets by object type (index 1..5); -1 = the type has no such field. See the table in objects.h.
const int kLocationAt[6] = {-1, -1, 8, 8, -1, 6};
const int kSlotAt[6] = {-1, -1, 0xA, 0xA, -1, 8};
const int kNextAt[6] = {-1, -1, 0xC, 0xC, -1, 0xA};
const int kPointsAt[6] = {-1, 8, 0x12, 0x16, -1, -1};
const int kPropAAt[6] = {-1, 0xC, 0x14, 0x18, -1, 0x18};
const int kDefaultVerbAt[6] = {-1, -1, 0x16, 0x1A, 8, 0x1A};
const int kHandlerAt[6] = {-1, 0xA, 0x1A, 0x1E, 0xC, 0x1E};

} // namespace

ObjectModel::ObjectModel() {
	reset();
}

void ObjectModel::reset() {
	_records.assign(kObjectRecords, kObjectRecords + ARRAYSIZE(kObjectRecords));
	_offset.assign(kObjectRecordOffset, kObjectRecordOffset + ARRAYSIZE(kObjectRecordOffset));
	_type.assign(kObjectType, kObjectType + ARRAYSIZE(kObjectType));
}

int ObjectModel::count() const {
	return kObjectCount;
}

size_t ObjectModel::recordSize(int id) const {
	return valid(id) ? kObjectTypeTable[_type[id] - 1][2] : 0;
}

uint16 ObjectModel::get16(int id, int off) const {
	const byte *r = &_records[_offset[id]] + off;
	return (uint16)(r[0] | (r[1] << 8));
}

void ObjectModel::put16(int id, int off, uint16 v) {
	byte *r = &_records[_offset[id]] + off;
	r[0] = (byte)v;
	r[1] = (byte)(v >> 8);
}

bool ObjectModel::setRecord(int id, const byte *data, size_t size) {
	if (!valid(id) || size != recordSize(id))
		return false;
	memcpy(&_records[_offset[id]], data, size);
	return true;
}

// ---- the tree ------------------------------------------------------------------------------------------------------------------------------------------------

int ObjectModel::location(int id) const {
	if (!valid(id) || kLocationAt[_type[id]] < 0)
		return 0;
	return get16(id, kLocationAt[_type[id]]);
}

void ObjectModel::setLocation(int id, int parent) {
	if (valid(id) && kLocationAt[_type[id]] >= 0)
		put16(id, kLocationAt[_type[id]], (uint16)parent);
}

int ObjectModel::slot(int id) const {
	if (!valid(id) || kSlotAt[_type[id]] < 0)
		return 0;
	return _records[_offset[id] + kSlotAt[_type[id]]];
}

void ObjectModel::setSlot(int id, int slot) {
	if (valid(id) && kSlotAt[_type[id]] >= 0)
		_records[_offset[id] + kSlotAt[_type[id]]] = (byte)slot;
}

int ObjectModel::nextSibling(int id) const {
	if (!valid(id) || kNextAt[_type[id]] < 0)
		return 0;
	return get16(id, kNextAt[_type[id]]);
}

void ObjectModel::setNextSibling(int id, int next) {
	if (valid(id) && kNextAt[_type[id]] >= 0)
		put16(id, kNextAt[_type[id]], (uint16)next);
}

int ObjectModel::listHead(int parent, int slot) const {
	if (!valid(parent) || slot < 0 || slot >= kObjectTypeTable[_type[parent] - 1][1])
		return 0;
	switch (_type[parent]) {
	case OT_ROOM:
	case OT_PROP:
		return get16(parent, 6);
	case OT_ITEM:
		return get16(parent, 0xE);
	case OT_CONTAINER:
		return get16(parent, 0xE + 2 * slot);
	case OT_CHARACTER:
		return get16(parent, 0xC + 2 * slot);
	}
	return 0;
}

void ObjectModel::setListHead(int parent, int slot, int head) {
	if (!valid(parent) || slot < 0 || slot >= kObjectTypeTable[_type[parent] - 1][1])
		return;
	switch (_type[parent]) {
	case OT_ROOM:
	case OT_PROP:
		put16(parent, 6, (uint16)head);
		break;
	case OT_ITEM:
		put16(parent, 0xE, (uint16)head);
		break;
	case OT_CONTAINER:
		put16(parent, 0xE + 2 * slot, (uint16)head);
		break;
	case OT_CHARACTER:
		put16(parent, 0xC + 2 * slot, (uint16)head);
		break;
	}
}

void ObjectModel::move(int id, int newParent, int newSlot) {
	// unlink from the old list
	int oldParent = location(id), oldSlot = slot(id);
	int head = listHead(oldParent, oldSlot);
	if (head == id) {
		setListHead(oldParent, oldSlot, nextSibling(id));
	} else if (head != 0) {
		int p = head;
		for (;;) {
			int n = nextSibling(p);
			if (n == id) {
				setNextSibling(p, nextSibling(id));
				break;
			}
			p = n;
			if (p == 0)
				break;
		}
	}
	// link at the front of the new list
	setLocation(id, newParent);
	setSlot(id, newSlot);
	if (newParent != 0) {
		int first = listHead(newParent, newSlot);
		setListHead(newParent, newSlot, id);
		setNextSibling(id, first);
	}
}

bool ObjectModel::isAt(int id, int parent, int slot) const {
	return (slot < 0 || this->slot(id) == slot) && location(id) == parent;
}

bool ObjectModel::isWithin(int id, int ancestor) const {
	for (int p = location(id); p != 0; p = location(p))
		if (p == ancestor)
			return true;
	return false;
}

// ---- attributes ----------------------------------------------------------------------------------------------------------------------------------------------

int ObjectModel::attrLocate(int id, int attr, int &bit) const {
	if (!valid(id) || attr <= 0 || attr >= ARRAYSIZE(kAttributeBit))
		return -1;
	if (!((kAttributeTypes[attr] >> (_type[id] - 1)) & 1))
		return -1;
	bit = kAttributeBit[attr] & 7;
	return _offset[id] + 4 + (kAttributeBit[attr] >> 3);
}

bool ObjectModel::hasAttr(int id, int attr) const {
	int bit = 0;
	int at = attrLocate(id, attr, bit);
	return at >= 0 && ((_records[at] >> bit) & 1);
}

void ObjectModel::setAttr(int id, int attr) {
	int bit = 0;
	int at = attrLocate(id, attr, bit);
	if (at >= 0)
		_records[at] |= (byte)(1 << bit);
}

void ObjectModel::clearAttr(int id, int attr) {
	int bit = 0;
	int at = attrLocate(id, attr, bit);
	if (at >= 0)
		_records[at] &= (byte)~(1 << bit);
}

// ---- fields --------------------------------------------------------------------------------------------------------------------------------------------------

int ObjectModel::points(int id) const {
	return valid(id) && kPointsAt[_type[id]] >= 0 ? get16(id, kPointsAt[_type[id]]) : 0;
}

void ObjectModel::setPoints(int id, int v) {
	if (valid(id) && kPointsAt[_type[id]] >= 0)
		put16(id, kPointsAt[_type[id]], (uint16)v);
}

int ObjectModel::propA(int id) const {
	return valid(id) && kPropAAt[_type[id]] >= 0 ? get16(id, kPropAAt[_type[id]]) : 0;
}

void ObjectModel::setPropA(int id, int v) {
	if (valid(id) && kPropAAt[_type[id]] >= 0)
		put16(id, kPropAAt[_type[id]], (uint16)v);
}

int ObjectModel::defaultVerb(int id) const {
	return valid(id) && kDefaultVerbAt[_type[id]] >= 0 ? get16(id, kDefaultVerbAt[_type[id]]) : 0;
}

void ObjectModel::setDefaultVerb(int id, int verb) {
	if (valid(id) && kDefaultVerbAt[_type[id]] >= 0)
		put16(id, kDefaultVerbAt[_type[id]], (uint16)verb);
}

int ObjectModel::handlerIndex(int id) const {
	return valid(id) && kHandlerAt[_type[id]] >= 0 ? get16(id, kHandlerAt[_type[id]]) : 0;
}

int ObjectModel::nameOffset(int id) const {
	return valid(id) ? get16(id, 0) : 0;
}

int ObjectModel::altNameOffset(int id) const {
	return valid(id) ? get16(id, 2) : 0;
}

} // namespace Xanth
