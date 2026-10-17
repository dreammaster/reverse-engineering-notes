// The object world model used by every handler: the containment tree (location / slot / next-sibling / child-list heads),
// attribute bits and the 800-bit game flag set. A direct port of the engine's own helpers (object_get_location? ...
// object_move?, object_has_attr, bitset_*), differentially tested against the real machine code (tests/world_fuzz.py).
#pragma once
#include "savestate.h"

namespace shannara {

class World {
public:
	// `exe` supplies the static attribute descriptor and slot-count tables; `state` is the mutable state this model edits.
	World(const LeExe &exe, SaveState &state);

	SaveState &state() { return _s; }
	int objectCount() const { return (int)_s.objects.size(); }
	bool valid(int obj) const { return obj > 0 && obj <= objectCount(); }

	// containment tree (ids are 1-based, 0 = none; accessors return 0 / do nothing for types without the field)
	int location(int obj) const;
	void setLocation(int obj, int loc);
	int slot(int obj) const;
	void setSlot(int obj, int slot);
	int nextSibling(int obj) const;
	void setNextSibling(int obj, int next);
	int listHead(int parent, int slot) const;
	void setListHead(int parent, int slot, int child);
	bool parentIs(int obj, int parent) const { return location(obj) == parent; }
	bool isWithin(int obj, int ancestor) const; // ancestor anywhere up the location chain
	// unlinks obj from its parent's child list and pushes it on the head of `newParent`'s list `slot` (newParent 0 only unlinks)
	void move(int obj, int newParent, int slot);

	// attribute bits (descriptor table at 0x208A0: {u8 bitOffset; u32 typeMask} per attribute, 1-based)
	bool hasAttr(int obj, int attr) const;
	void setAttr(int obj, int attr);
	void clearAttr(int obj, int attr);

	// 800 game flags (1-based; 0 is a no-op / false)
	bool flag(int n) const { return _s.gameFlag(n); }
	void setFlag(int n, bool v);

private:
	uint8_t *attrByte(int obj, int attr, int &bit) const;
	uint16_t rd16(const ObjectRecord &r, size_t o) const { return r.u16(o); }
	static void wr16(ObjectRecord &r, size_t o, uint16_t v) { r.raw[o] = v & 0xFF; r.raw[o + 1] = v >> 8; }

	SaveState &_s;
	uint8_t _slotCount[6] = {};
	uint8_t _attrBit[64] = {};
	uint32_t _attrMask[64] = {};
	int _attrCount = 0;
};

} // namespace shannara
