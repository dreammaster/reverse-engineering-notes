/* The object model: all rooms, items, containers, props and characters of the game and their relations.
 *
 * Every object has a record in a table compiled into the original executable (498 objects, ids 1..498). A record's size and fields depend on the object's type:
 *
 *   type 1 room (18 bytes)       type 2 item (28)             type 3 container (32)        type 4 prop (14)             type 5 character (32)
 *   +0 name offset (OBJECT.DAT)  +0 name                      +0 name                      +0 name                      +0 name
 *   +2 alternative name          +2 alternative name          +2 alternative name          +2 alternative name          +2 alternative name
 *   +4.. attribute bits          +4.. attribute bits          +4.. attribute bits          +4.. attribute bits          +4.. attribute bits
 *   +6 first child (list head)   +8 location (parent)        +8 location                  +8 default verb              +6 location
 *   +8 points                    +0A slot (byte)              +0A slot (byte)              +0C handler index            +8 slot (byte)
 *   +0A handler index            +0C next sibling             +0C next sibling                                          +0A next sibling
 *   +0C prop_a                   +0E first child              +0E first child per slot                                  +0C first child per slot (3)
 *                                +12 points, +14 prop_a       +16 points, +18 prop_a                                    +18 prop_a
 *                                +16 default verb             +1A default verb                                          +1A default verb
 *                                +1A handler index            +1E handler index                                         +1E handler index
 *
 * The children of a room, container or character form singly linked lists through the `next sibling` field; a parent has one list per slot (a container has 2, a
 * character 3, the others 1) and an object records which slot of its parent it is in. `move` removes an object from its old list and puts it at the front of the new one.
 * Names are 1-based offsets into the name pool of OBJECT.DAT (a u16 pool size, then NUL-separated names).
 */
#ifndef XANTH_OBJECTS_H
#define XANTH_OBJECTS_H

#include "common/types.h"

namespace Xanth {

enum ObjectType {
	OT_NONE = 0,
	OT_ROOM = 1,
	OT_ITEM = 2,
	OT_CONTAINER = 3,
	OT_PROP = 4,
	OT_CHARACTER = 5
};

class ObjectModel {
public:
	ObjectModel();

	/** Restores the records of a new game. */
	void reset();

	int count() const;
	bool valid(int id) const { return id > 0 && id <= count(); }
	int type(int id) const { return valid(id) ? _type[id] : 0; }

	// the tree
	int location(int id) const;
	void setLocation(int id, int parent);
	int slot(int id) const;
	void setSlot(int id, int slot);
	int nextSibling(int id) const;
	void setNextSibling(int id, int next);
	int listHead(int parent, int slot) const;
	void setListHead(int parent, int slot, int head);
	/** Moves an object to a new parent and slot (parent 0 = out of the world); it becomes the first child of that slot. */
	void move(int id, int newParent, int newSlot);
	bool parentIs(int id, int parent) const { return location(id) == parent; }
	/** True if the object is in `parent`'s slot `slot` (any slot if negative). */
	bool isAt(int id, int parent, int slot) const;
	/** True if `ancestor` is somewhere above the object. */
	bool isWithin(int id, int ancestor) const;

	// attributes (see xanth_overview.md section 12); an attribute the object's type does not have reads as 0 and cannot be set
	bool hasAttr(int id, int attr) const;
	void setAttr(int id, int attr);
	void clearAttr(int id, int attr);

	// fields
	int points(int id) const;
	void setPoints(int id, int v);
	int propA(int id) const;
	void setPropA(int id, int v);
	int defaultVerb(int id) const;
	void setDefaultVerb(int id, int verb);
	int handlerIndex(int id) const;
	/** 1-based offsets into the OBJECT.DAT name pool. */
	int nameOffset(int id) const;
	int altNameOffset(int id) const;

	// raw records (save games, comparison with the original)
	size_t recordSize(int id) const;
	const byte *record(int id) const { return valid(id) ? &_records[_offset[id]] : nullptr; }
	/** Overwrites a record (restoring a save game); the size must match. */
	bool setRecord(int id, const byte *data, size_t size);

private:
	uint16 get16(int id, int off) const;
	void put16(int id, int off, uint16 v);
	/** Index into _records of the byte holding an attribute and the bit in it, or -1 if the object does not have that attribute. */
	int attrLocate(int id, int attr, int &bit) const;

	std::vector<byte> _records;       // all records, concatenated (the offsets are fixed)
	std::vector<uint16> _offset;      // by id
	std::vector<byte> _type;          // by id
};

} // namespace Xanth

#endif
