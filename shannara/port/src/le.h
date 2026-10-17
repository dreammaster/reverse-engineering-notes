// Minimal DOS/4GW "LE" executable reader: maps linear addresses to file bytes so the engine can read the *initial values* of
// the game's static tables straight from the user's own SHANNARA.EXE (object records, room picture/music tables, ...).
// Fixups are not applied, so pointer-valued fields (e.g. the verb name pointers) are not usable; plain data is.
#pragma once
#include "common.h"

namespace shannara {

class LeExe {
public:
	explicit LeExe(Bytes file);
	// Copies n bytes at linear address `addr` into dst (zeros for the uninitialised tail of an object); throws FormatError if any byte is
	// outside the loaded objects.
	void read(uint32_t addr, void *dst, size_t n) const;
	uint8_t u8(uint32_t addr) const { uint8_t v; read(addr, &v, 1); return v; }
	uint16_t u16(uint32_t addr) const { uint8_t b[2]; read(addr, b, 2); return b[0] | (b[1] << 8); }
	uint32_t u32(uint32_t addr) const { return u16(addr) | ((uint32_t)u16(addr + 2) << 16); }
	// Builds the loaded image: every object's pages copied to its link address in `mem` (zero-filled BSS), then all internal-reference
	// fixups applied (value = base of the target object + offset), so pointers in tables hold real addresses as after DOS/4GW loading.
	void mapInto(uint8_t *mem, size_t memSize) const;
	uint32_t entryPoint() const { return _eip; }
	size_t objectCount() const { return _objects.size(); }

private:
	struct Object {
		uint32_t base, size, firstPage, pages;
	};
	Bytes _d;
	size_t _dataPages = 0;
	uint32_t _pageSize = 0, _eip = 0;
	std::vector<Object> _objects;
	std::vector<uint32_t> _pageMap; // file page number (1-based) per logical page index
	size_t _hdr = 0, _fixupPageTab = 0, _fixupRecTab = 0;
	uint32_t _pages = 0;
};

} // namespace shannara
