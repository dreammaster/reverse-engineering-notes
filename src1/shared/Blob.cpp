#include "shared/Blob.h"

#include <stdio.h>
#include <string.h>

namespace Yendor1 {
namespace Shared {

Blob::Blob() : _data(0), _size(0) {
}

Blob::~Blob() {
	clear();
}

void Blob::clear() {
	delete[] _data;
	_data = 0;
	_size = 0;
}

void Blob::assign(const byte *data, size_t size) {
	clear();
	if (size == 0)
		return;
	_data = new byte[size];
	memcpy(_data, data, size);
	_size = size;
}

bool Blob::load(const char *path) {
	clear();
	FILE *f = fopen(path, "rb");
	if (!f)
		return false;
	if (fseek(f, 0, SEEK_END) != 0) {
		fclose(f);
		return false;
	}
	long len = ftell(f);
	if (len < 0 || fseek(f, 0, SEEK_SET) != 0) {
		fclose(f);
		return false;
	}
	if (len > 0) {
		_data = new byte[(size_t)len];
		if (fread(_data, 1, (size_t)len, f) != (size_t)len) {
			fclose(f);
			clear();
			return false;
		}
		_size = (size_t)len;
	}
	fclose(f);
	return true;
}

bool joinPath(char *out, size_t outSize, const char *dir, const char *name) {
	size_t d = strlen(dir), n = strlen(name);
	bool sep = d > 0 && dir[d - 1] != '/' && dir[d - 1] != '\\';
	if (d + (sep ? 1 : 0) + n + 1 > outSize)
		return false;
	memcpy(out, dir, d);
	if (sep)
		out[d++] = '/';
	memcpy(out + d, name, n + 1);
	return true;
}

} // namespace Shared
} // namespace Yendor1
