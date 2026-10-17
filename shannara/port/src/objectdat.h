// OBJECT.DAT: u16 poolSize followed by poolSize bytes of NUL-separated object names/descriptions. Object records refer to
// strings by 1-based byte offset (name at record+0, description at record+4; 0 = none).
#pragma once
#include "common.h"

namespace shannara {

class ObjectDat {
public:
	explicit ObjectDat(const Bytes &data);
	size_t poolSize() const { return _pool.size(); }
	// string at 1-based offset (0 -> empty); throws on an offset outside the pool
	std::string text(uint32_t offset1) const;
	int stringCount() const; // number of NUL-terminated strings in the pool
private:
	Bytes _pool;
};

} // namespace shannara
