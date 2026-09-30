// Not yet assert-confirmed to a specific file; stays alongside argument.h in
// vscommon/scripting/ since it's used together with UnrefLuaFieldsCache
// (Lua-side bookkeeping).
//
// TId pairs two ints into an identifier - confirmed constructor signature
// only (TGameControl::ReplaceGame, Deponia_Linux.asm line 468783); its own
// fields' meaning and every other member (if any) are not reversed.
#pragma once

class TId {
public:
	TId(int a, int b);

	// Confirmed call shape only (TArgument::ConvertToObject, Deponia_Linux.
	// asm line 1437521) - not reversed beyond that call shape (TId's own
	// fields are still unknown, so this can't meaningfully compare anything
	// yet - stubbed to always report "not equal").
	bool operator==(const TId &other) const;
};

// Confirmed call shape only (TGameControl::ReplaceGame, asm line 468786) -
// invalidates some Lua-side per-object field cache keyed by a TId; not
// reversed beyond that call shape.
void UnrefLuaFieldsCache(const TId &id, int value);

// Confirmed a real, named global (TArgument::ConvertToObject, Deponia_Linux.
// asm line 1437520) - a sentinel TId meaning "any/the current object" in a
// Lua-provided id string. Its own field values aren't recoverable (TId's
// fields are unknown - see the class comment above), so the (-1,-1)
// initializer here is a placeholder, not recovered evidence.
extern TId AnyId;
