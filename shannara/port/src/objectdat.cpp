#include "objectdat.h"

namespace shannara {

ObjectDat::ObjectDat(const Bytes &data) {
	Cursor c(data);
	size_t n = c.u16();
	if (c.left() < n)
		throw FormatError("OBJECT.DAT pool past end of file");
	_pool.assign(c.ptr(), c.ptr() + n);
}

std::string ObjectDat::text(uint32_t offset1) const {
	if (!offset1)
		return std::string();
	size_t o = offset1 - 1;
	if (o >= _pool.size())
		throw FormatError("OBJECT.DAT offset outside the pool");
	size_t e = o;
	while (e < _pool.size() && _pool[e])
		e++;
	return std::string((const char *)&_pool[o], e - o);
}

int ObjectDat::stringCount() const {
	int n = 0;
	for (uint8_t b : _pool)
		n += b == 0;
	return n;
}

} // namespace shannara
