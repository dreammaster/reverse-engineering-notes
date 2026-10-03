#include "vscommon/scripting/id.h"

TId::TId(const TId &other) {
	_id[0] = other._id[0];
	_id[1] = other._id[1];
	_id[2] = other._id[2];
	_table = other._table;
}

// Confirmed (asm lines 586997-587018)
TId::TId(int id, int table) {
	_id[0] = (unsigned char)id;
	_id[1] = (unsigned char)(id >> 8);
	_id[2] = (unsigned char)(id >> 16);
	_table = (unsigned char)table;
}

int TId::getId() const {
	int value = _id[0] + (_id[1] << 8) + (_id[2] << 16);
	if (_id[2] & 0x80)
		value -= 0x1000000;
	return value;
}

// Confirmed (asm lines 587019-587065)
bool TId::operator==(const TId &other) const {
	return getId() == other.getId() && _table == other._table;
}

// Confirmed (asm lines 587066-587113)
bool TId::operator!=(const TId &other) const {
	return !(*this == other);
}

// Confirmed (asm lines 587114-587147)
long PackId(const TId &id) {
	int low = (id.getTable() == 0xFF) ? 0x7F : (id.getTable() & 0x7F);
	return (long)((id.getId() << 7) + low);
}

// Confirmed (asm lines 587148-587170)
TId UnpackId(long packed) {
	int low = (int)(packed & 0x7F);
	if (low == 0x7F)
		low = 0xFF;
	return TId((int)(packed >> 7), low);
}

void UnrefLuaFieldsCache(const TId &/*id*/, int /*value*/) {
}

TId AnyId(-1, -1);
