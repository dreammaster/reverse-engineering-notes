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
};

// Confirmed call shape only (TGameControl::ReplaceGame, asm line 468786) -
// invalidates some Lua-side per-object field cache keyed by a TId; not
// reversed beyond that call shape.
void UnrefLuaFieldsCache(const TId &id, int value);
