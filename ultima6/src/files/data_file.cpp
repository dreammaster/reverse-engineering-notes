#include "files/data_file.h"

#include <cstdio>

namespace Ultima6 {

bool readFile(const std::string &path, Bytes &out) {
	FILE *f = std::fopen(path.c_str(), "rb");
	if (!f)
		return false;
	out.clear();
	uint8_t buf[4096];
	size_t n;
	while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0)
		out.insert(out.end(), buf, buf + n);
	std::fclose(f);
	return true;
}

std::string joinPath(const std::string &dir, const std::string &name) {
	if (dir.empty() || dir.back() == '/' || dir.back() == '\\')
		return dir + name;
	return dir + "/" + name;
}

} // namespace Ultima6
