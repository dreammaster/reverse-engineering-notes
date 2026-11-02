#ifndef ULTIMA6_FILES_DATA_FILE_H
#define ULTIMA6_FILES_DATA_FILE_H

#include <cstdint>
#include <string>
#include <vector>

namespace Ultima6 {

typedef std::vector<uint8_t> Bytes;

/** Reads a whole file; false when it can't be opened. */
bool readFile(const std::string &path, Bytes &out);

/** Writes a whole file (creating the directory if needed); false on failure. */
bool writeFile(const std::string &path, const Bytes &data);

/** dir + '/' + name (DOS file names are case-insensitive, so the original spelling is kept). */
std::string joinPath(const std::string &dir, const std::string &name);

/** Little-endian accessors; callers check sizes first. */
inline uint16_t get16(const uint8_t *p) {
	return (uint16_t)(p[0] | (p[1] << 8));
}

} // namespace Ultima6

#endif
