/* A whole file held in memory. File access is isolated here so the port can
 * replace it with ScummVM's Common::File without touching the game code. */

#ifndef YENDOR1_SHARED_BLOB_H
#define YENDOR1_SHARED_BLOB_H

#include "shared/Types.h"

namespace Yendor1 {
namespace Shared {

class Blob {
public:
	Blob();
	~Blob();

	/** Read the whole file. Returns false (and leaves the blob empty) on failure. */
	bool load(const char *path);

	/** Take ownership of a copy of `size` bytes. */
	void assign(const byte *data, size_t size);

	void clear();

	const byte *data() const { return _data; }
	byte *data() { return _data; }
	size_t size() const { return _size; }
	bool empty() const { return _size == 0; }

private:
	Blob(const Blob &);
	Blob &operator=(const Blob &);

	byte *_data;
	size_t _size;
};

/** Join a directory and a file name with a '/' (no normalisation). */
bool joinPath(char *out, size_t outSize, const char *dir, const char *name);

} // namespace Shared
} // namespace Yendor1

#endif
