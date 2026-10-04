// Original path not yet confirmed; stays alongside type.h in datastruct/.
//
// Confirmed in full (Deponia_Linux.asm lines 635588-636060, all 15
// manifest-listed methods): a reference from one game-data record to another.
// Eight bytes - the target's TId (+0), the field of the target it links
// through (+4, a short, -1 for none) and a flag byte (+6) whose bit 0 marks a
// parent link and bit 1 an "any object" link, which matches every other "any"
// link and nothing else. A default link points at nothing (TId(-1,-1), field
// -1). Member names are invented.
#pragma once

#include <vector>

#include "vscommon/scripting/id.h"

class TProjectFileWriter;

class TLink {
public:
	/** A link to nothing; `any` makes it an "any object" link instead. */
	explicit TLink(bool any);
	TLink(const TId &id, int field, bool parent);
	TLink();
	TLink(const TLink &other) = default;
	~TLink() = default;
	TLink &operator=(const TLink &other) = default;

	bool operator==(const TLink &other) const;

	/** Back to a link to nothing. */
	void Clear();

	const TId &GetId() const {
		return _id;
	}
	/** Re-points the link at another object (the loader's id remapping). */
	void SetId(const TId &id) {
		_id = id;
	}
	int GetField() const {
		return _field;
	}
	bool IsParentLink() const {
		return (_flags & kParent) != 0;
	}
	bool IsAnyLink() const {
		return (_flags & kAny) != 0;
	}

	/** The loader's way of filling a link in: id (low 24 bits) and table. */
	void Serialize(int id, int table, int field, bool parent, bool any);
	/** Writes the link as an element named `name` (a list writes each of
	 *  its links as a `Link`). */
	void Serialize(TProjectFileWriter &writer, int name);
	/** Writes a list of links under one element. */
	static bool Serialize(TProjectFileWriter &writer, int name, std::vector<TLink> &links);

private:
	enum {
		kParent = 1,
		kAny = 2
	};

	TId _id;
	short _field;
	unsigned char _flags;
};
