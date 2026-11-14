/* Access to the game's data files: a folder searched case-insensitively (the original files are upper case 8.3 names).
 *
 * ScummVM port: replace with Common::File / SearchMan (same open-by-name semantics) - only this header and files.cpp change.
 */
#ifndef COMMON_FILES_H
#define COMMON_FILES_H

#include <map>

#include "common/types.h"

namespace Common {

/** A file opened for random access (large files such as the 190 MB voice archive are never loaded whole). */
class File {
public:
	File() : _fp(nullptr), _size(0) {}
	~File() { close(); }

	bool isOpen() const { return _fp != nullptr; }
	size_t size() const { return _size; }
	void close();
	/** Reads n bytes at an absolute offset; returns the number of bytes actually read. */
	size_t readAt(size_t offset, void *dst, size_t n);

private:
	friend class GameFiles;
	File(const File &);
	File &operator=(const File &);
	void *_fp;
	size_t _size;
};

class GameFiles {
public:
	/** Scans the folder once; returns false if it does not exist. */
	bool setFolder(const std::string &folder);
	const std::string &folder() const { return _folder; }

	/** True if a file of that name exists (any letter case). */
	bool exists(const std::string &name) const;
	/** Loads a whole file. */
	bool load(const std::string &name, Bytes &out) const;
	/** Loads `size` bytes starting at `offset` (the directory of a huge file, one voice track ...). */
	bool loadPart(const std::string &name, size_t offset, size_t size, Bytes &out) const;
	/** Opens a file for random access. */
	bool open(const std::string &name, File &file) const;
	/** Size of a file, or 0 if it does not exist. */
	size_t fileSize(const std::string &name) const;

	/** Writes a file in the save folder (the game folder unless setSaveFolder was called). */
	bool save(const std::string &name, const Bytes &data) const;
	/** Reads a file from the save folder (it need not have existed when the folder was scanned). */
	bool loadSaved(const std::string &name, Bytes &out) const;
	void setSaveFolder(const std::string &folder) { _saveFolder = folder; }

private:
	std::string pathOf(const std::string &name) const;
	static std::string lower(const std::string &s);

	std::string _folder, _saveFolder;
	std::map<std::string, std::string> _names;   // lower-case name -> real name
};

} // namespace Common

#endif
