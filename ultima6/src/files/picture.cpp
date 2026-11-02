#include "files/picture.h"
#include "files/lzw.h"

namespace Ultima6 {

bool Picture::load(const std::string &path) {
	Bytes raw, plain;
	if (!readFile(path, raw) || !lzwDecompress(raw, 0, plain) || plain.size() < 4)
		return false;
	width = plain[0] | (plain[1] << 8);
	height = plain[2] | (plain[3] << 8);
	if (plain.size() != (size_t)4 + (size_t)width * height)
		return false;
	pixels.assign(plain.begin() + 4, plain.end());
	return true;
}

} // namespace Ultima6
