#include "ultima6/md_driver.h"

#include "files/lib_n.h"

#include <cstring>

namespace Ultima6 {
namespace Ultima6 {

namespace {
// where the pieces lie in the megabyte of the interpreter
const uint16_t CODE_SEG = 0x2000;   // the driver's block (its own data are in the code segment)
const uint16_t DATA_SEG = 0x1000;   // the data and the stack of the caller
const uint16_t SCREEN_SEG = 0x3000; // the 320 x 200 pixels
const uint16_t STUB_SEG = 0x4000;   // the callback that answers where the archive is
const uint16_t ARCHIVE_SEG = 0x5000;
const uint16_t RETURN_SEG = 0x9000; // where the entry returns to
const uint16_t ROWS = 0x100;        // in DATA_SEG: the offset of each row of the screen
const uint16_t SCREEN = 0x800;      // in DATA_SEG: the screen's table pointer and clipping rectangle
const uint16_t ENTRY = 0x15E9;
const size_t ARCHIVE_MAX = 0x10000 * 4 - 8;
} // namespace

MdDriver::MdDriver() : _cpu(new Cpu8086()) {
}

bool MdDriver::load(const std::string &dir) {
	_ok = false;
	Bytes raw, block;
	LibN lib;
	if (!readFile(joinPath(dir, "SUPERTM.DRV"), raw) || !lib.load(raw, true) || !lib.get(0, block) || block.size() < ENTRY + 16 || block.size() > 0xFFF0)
		return false;
	Cpu8086 &c = *_cpu;
	std::memcpy(c.memory() + Cpu8086::linear(CODE_SEG, 0), block.data(), block.size());
	c.write16(CODE_SEG, 0xC, 0); // the callback the game puts there: mov ax, 0 / mov dx, ARCHIVE_SEG / retf
	c.write16(CODE_SEG, 0xE, STUB_SEG);
	const uint8_t stub[] = {0xB8, 0x00, 0x00, 0xBA, (uint8_t)(ARCHIVE_SEG & 0xFF), (uint8_t)(ARCHIVE_SEG >> 8), 0xCB};
	std::memcpy(c.memory() + Cpu8086::linear(STUB_SEG, 0), stub, sizeof stub);
	for (int y = 0; y <= 200; ++y)
		c.write16(DATA_SEG, (uint16_t)(ROWS + 2 * y), (uint16_t)(y * 320));
	c.write16(DATA_SEG, SCREEN + 0, SCREEN_SEG);
	c.write16(DATA_SEG, SCREEN + 2, ROWS);
	c.write16(DATA_SEG, SCREEN + 4, 0);   // left
	c.write16(DATA_SEG, SCREEN + 6, 0);   // top
	c.write16(DATA_SEG, SCREEN + 8, 319); // right
	c.write16(DATA_SEG, SCREEN + 10, 199); // bottom
	c.write8(RETURN_SEG, 0, 0xF4);
	_ok = true;
	return true;
}

bool MdDriver::draw(uint8_t *pixels, const Bytes &archive, int frame, int x, int y, int angle, int size, int flags) {
	if (!_ok || archive.size() + 4 > ARCHIVE_MAX || size < 0)
		return false;
	Cpu8086 &c = *_cpu;
	uint8_t *m = c.memory();
	// the archive as the game keeps it: its size, then the table and the shapes
	uint32_t total = (uint32_t)archive.size() + 4;
	uint8_t *a = m + Cpu8086::linear(ARCHIVE_SEG, 0);
	a[0] = (uint8_t)total;
	a[1] = (uint8_t)(total >> 8);
	a[2] = (uint8_t)(total >> 16);
	a[3] = (uint8_t)(total >> 24);
	std::memcpy(a + 4, archive.data(), archive.size());
	std::memcpy(m + Cpu8086::linear(SCREEN_SEG, 0), pixels, 320 * 200);
	c.write16(CODE_SEG, 0, 0); // not in the middle of a call
	c.ss = DATA_SEG;
	c.ds = DATA_SEG;
	c.es = DATA_SEG;
	c.sp = 0xF000;
	const uint16_t args[9] = {(uint16_t)flags, (uint16_t)size, (uint16_t)angle, (uint16_t)frame, ARCHIVE_SEG, 0, (uint16_t)y, (uint16_t)x, SCREEN};
	for (uint16_t v : args)
		c.push16(v);
	c.push16(RETURN_SEG);
	c.push16(0); // (the far return address: the offset is on top)
	c.cs = CODE_SEG;
	c.ip = ENTRY;
	bool ok = c.run(Cpu8086::linear(RETURN_SEG, 0), 20000000);
	if (ok)
		std::memcpy(pixels, m + Cpu8086::linear(SCREEN_SEG, 0), 320 * 200);
	return ok;
}

} // namespace Ultima6
} // namespace Ultima6
