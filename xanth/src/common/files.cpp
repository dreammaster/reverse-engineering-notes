#include "common/files.h"

#include <cctype>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

namespace Common {

void File::close() {
	if (_fp)
		fclose(static_cast<FILE *>(_fp));
	_fp = nullptr;
	_size = 0;
}

size_t File::readAt(size_t offset, void *dst, size_t n) {
	if (!_fp || offset > _size)
		return 0;
	if (offset + n > _size)
		n = _size - offset;
	if (fseek(static_cast<FILE *>(_fp), (long)offset, SEEK_SET) != 0)
		return 0;
	return fread(dst, 1, n, static_cast<FILE *>(_fp));
}

std::string GameFiles::lower(const std::string &s) {
	std::string r(s);
	for (size_t i = 0; i < r.size(); i++)
		r[i] = (char)tolower((unsigned char)r[i]);
	return r;
}

bool GameFiles::setFolder(const std::string &folder) {
	_folder = folder;
	if (!_folder.empty() && _folder[_folder.size() - 1] != '/' && _folder[_folder.size() - 1] != '\\')
		_folder += '/';
	_names.clear();
#ifdef _WIN32
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA((_folder + "*").c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE)
		return false;
	do {
		_names[lower(fd.cFileName)] = fd.cFileName;
	} while (FindNextFileA(h, &fd));
	FindClose(h);
#else
	DIR *d = opendir(_folder.c_str());
	if (!d)
		return false;
	while (struct dirent *e = readdir(d))
		_names[lower(e->d_name)] = e->d_name;
	closedir(d);
#endif
	return true;
}

std::string GameFiles::pathOf(const std::string &name) const {
	std::map<std::string, std::string>::const_iterator it = _names.find(lower(name));
	return _folder + (it == _names.end() ? name : it->second);
}

bool GameFiles::exists(const std::string &name) const {
	return _names.find(lower(name)) != _names.end();
}

bool GameFiles::open(const std::string &name, File &file) const {
	file.close();
	if (!exists(name))
		return false;
	FILE *fp = fopen(pathOf(name).c_str(), "rb");
	if (!fp)
		return false;
	fseek(fp, 0, SEEK_END);
	file._size = (size_t)ftell(fp);
	file._fp = fp;
	return true;
}

size_t GameFiles::fileSize(const std::string &name) const {
	File f;
	return open(name, f) ? f.size() : 0;
}

bool GameFiles::load(const std::string &name, Bytes &out) const {
	File f;
	if (!open(name, f))
		return false;
	out.resize(f.size());
	return out.empty() || f.readAt(0, &out[0], out.size()) == out.size();
}

bool GameFiles::loadPart(const std::string &name, size_t offset, size_t size, Bytes &out) const {
	File f;
	if (!open(name, f))
		return false;
	out.resize(size);
	size_t got = size ? f.readAt(offset, &out[0], size) : 0;
	out.resize(got);
	return got == size;
}

bool GameFiles::loadSaved(const std::string &name, Bytes &out) const {
	std::string dir = _saveFolder.empty() ? _folder : _saveFolder;
	if (!dir.empty() && dir[dir.size() - 1] != '/' && dir[dir.size() - 1] != '\\')
		dir += '/';
	FILE *fp = fopen((dir + name).c_str(), "rb");
	if (!fp)
		return false;
	fseek(fp, 0, SEEK_END);
	long n = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	out.resize(n > 0 ? (size_t)n : 0);
	bool ok = out.empty() || fread(&out[0], 1, out.size(), fp) == out.size();
	fclose(fp);
	return ok;
}

bool GameFiles::save(const std::string &name, const Bytes &data) const {
	std::string dir = _saveFolder.empty() ? _folder : _saveFolder;
	if (!dir.empty() && dir[dir.size() - 1] != '/' && dir[dir.size() - 1] != '\\')
		dir += '/';
	FILE *fp = fopen((dir + name).c_str(), "wb");
	if (!fp)
		return false;
	bool ok = data.empty() || fwrite(&data[0], 1, data.size(), fp) == data.size();
	fclose(fp);
	return ok;
}

} // namespace Common
