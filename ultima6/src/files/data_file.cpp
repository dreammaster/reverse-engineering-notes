#include "files/data_file.h"

#include <cstdio>
#ifdef _WIN32
#include <direct.h>
#define MAKE_DIR(d) _mkdir(d)
#else
#include <sys/stat.h>
#define MAKE_DIR(d) mkdir(d, 0777)
#endif

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

bool writeFile(const std::string &path, const Bytes &data) {
	// create the directories of the path, one level at a time
	for (size_t i = 1; i < path.size(); ++i)
		if (path[i] == '/' || path[i] == '\\')
			MAKE_DIR(path.substr(0, i).c_str());
	FILE *f = std::fopen(path.c_str(), "wb");
	if (!f)
		return false;
	bool ok = data.empty() || std::fwrite(&data[0], 1, data.size(), f) == data.size();
	return std::fclose(f) == 0 && ok;
}

std::string joinPath(const std::string &dir, const std::string &name) {
	if (dir.empty() || dir.back() == '/' || dir.back() == '\\')
		return dir + name;
	return dir + "/" + name;
}

} // namespace Ultima6
