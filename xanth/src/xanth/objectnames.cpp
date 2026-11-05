#include "xanth/objectnames.h"

namespace Xanth {

bool ObjectNames::load(const Common::Bytes &data) {
	_pool.clear();
	if (data.size() < 2)
		return false;
	size_t size = data[0] | (data[1] << 8);
	if (size == 0 || data.size() < 2 + size)
		return false;
	_pool.assign(data.begin() + 2, data.begin() + 2 + size);
	return true;
}

std::string ObjectNames::name(int offset) const {
	if (offset < 1 || (size_t)offset > _pool.size())
		return std::string();
	std::string s;
	for (size_t i = offset - 1; i < _pool.size() && _pool[i]; i++)
		s += (char)_pool[i];
	return s;
}

} // namespace Xanth
