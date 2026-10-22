#include "shared/bytes.h"
#include <fstream>

namespace shared {

Bytes readFile(const std::string &path) {
	std::ifstream f(path, std::ios::binary);
	if (!f) throw std::runtime_error("cannot open " + path);
	return Bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

} // namespace shared
