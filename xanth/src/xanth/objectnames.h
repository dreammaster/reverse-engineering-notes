/* The names of the objects: OBJECT.DAT is a u16 size followed by a pool of NUL-separated names ("You", "me", ...). The object records hold 1-based offsets into the pool
 * (the first name is offset 1), one for the name and one for the alternative name ("me" for the player). */
#ifndef XANTH_OBJECTNAMES_H
#define XANTH_OBJECTNAMES_H

#include "common/types.h"

namespace Xanth {

class ObjectNames {
public:
	/** Takes the contents of OBJECT.DAT; false if it is not a valid name pool. */
	bool load(const Common::Bytes &data);
	bool loaded() const { return !_pool.empty(); }
	/** The name at a 1-based pool offset (empty for an invalid offset). */
	std::string name(int offset) const;

private:
	Common::Bytes _pool;
};

} // namespace Xanth

#endif
