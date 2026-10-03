// Confirmed (asm lines 586976-587170): TId is a 4-byte value - a signed 24-bit
// object id in bytes 0-2 (little-endian) and a one-byte "table" in byte 3
// (see UnpackId/PackId below, which fold the pair into a single 32-bit long).
// The constructor argument names (_id, _table) are invented. It stays alongside
// argument.h in vscommon/scripting/ since it's used together with
// UnrefLuaFieldsCache (Lua-side bookkeeping).
#pragma once

class TId {
public:
	TId() {
		_id[0] = _id[1] = _id[2] = 0;
		_table = 0;
	}
	TId(const TId &other);
	TId(int id, int table);

	bool operator==(const TId &other) const;
	bool operator!=(const TId &other) const;

	/** The signed 24-bit id from bytes 0-2 (the value both comparisons use). */
	int getId() const;
	int getTable() const { return _table; }

private:
	unsigned char _id[3];
	unsigned char _table;
};

/** Packs a TId into one long: (id << 7) + (table & 0x7F), with table 0xFF
 *  mapped to 0x7F. */
long PackId(const TId &id);
/** The inverse of PackId(). */
TId UnpackId(long packed);

// Confirmed call shape only (TGameControl::ReplaceGame, asm line 468786) -
// invalidates some Lua-side per-object field cache keyed by a TId; not
// reversed beyond that call shape.
void UnrefLuaFieldsCache(const TId &id, int value);

// Confirmed a real, named global (TArgument::ConvertToObject, Deponia_Linux.
// asm line 1437520) - a sentinel TId meaning "any/the current object" in a
// Lua-provided id string. Its initial value isn't recovered (the static
// initializer wasn't read), so the (-1,-1) initializer here is a placeholder.
extern TId AnyId;
