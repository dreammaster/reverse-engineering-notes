// Martian Dreams' SUPERTM.DRV run on the 8086 interpreter: the pictures that turn and scale, checked against the checksums of the same calls run on another 8086 emulator (Unicorn). Skipped
// without the game data.

#include "files/lib_n.h"
#include "ultima6/md_driver.h"

#include <cstdio>
#include <cstring>

using namespace Ultima6;
using ::Ultima6::Ultima6::MdDriver;

static int failures = 0;

#define CHECK(cond)                                                              \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
			++failures;                                                          \
		}                                                                        \
	} while (0)

static uint32_t crc32(const uint8_t *p, size_t n) {
	uint32_t c = 0xFFFFFFFFu;
	for (size_t i = 0; i < n; ++i) {
		c ^= p[i];
		for (int k = 0; k < 8; ++k)
			c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1)));
	}
	return ~c;
}

struct Case {
	const char *file;
	int item, frame, x, y, angle, size;
	uint32_t crc;
};

static const Case CASES[] = {
	{"SCENE1.LZC", 7, 0, 49, 96, 321, 256, 0x98d87262u},
	{"SCENE1.LZC", 7, 0, 241, 116, 180, 256, 0xa14bea27u},
	{"SCENE1.LZC", 7, 0, 208, 199, -90, 256, 0x21fda7e4u},
	{"SCENE1.LZC", 7, 0, 73, 139, 270, 172, 0xf187eccfu},
	{"SCENE1.LZC", 7, 0, 42, 149, -246, 256, 0x713da3e0u},
	{"SCENE1.LZC", 7, 0, 210, 124, 90, 847, 0x4d44d7f0u},
	{"CABIN.LZC", 3, 0, 62, 191, 0, 256, 0x16997acau},
	{"CABIN.LZC", 3, 0, 55, 133, 0, 256, 0x1050ce7du},
	{"CABIN.LZC", 3, 0, 220, 52, 180, 211, 0x366355f4u},
	{"CABIN.LZC", 3, 0, 56, 42, 0, 80, 0xc65dd116u},
	{"CABIN.LZC", 3, 0, 152, 187, -248, 256, 0xd810f8b7u},
	{"CABIN.LZC", 3, 0, 3, 145, -322, 256, 0xb386c5f4u},
	{"CABIN.LZC", 7, 0, 245, 63, -56, 494, 0x2d4cf515u},
	{"CABIN.LZC", 7, 0, 182, 108, 90, 485, 0x298123f1u},
	{"CABIN.LZC", 7, 0, 25, 159, 0, 816, 0xa03cef1du},
	{"CABIN.LZC", 7, 0, 272, 100, 90, 256, 0x885c2824u},
	{"CABIN.LZC", 7, 0, 187, 128, -352, 269, 0x0c9bd3a1u},
	{"CABIN.LZC", 7, 0, 297, 178, 261, 1001, 0x2a661e6du},
	{"ENDGAME.LZC", 1, 0, 134, 138, 90, 675, 0xceede187u},
	{"ENDGAME.LZC", 1, 2, 238, 179, 307, 94, 0xa37c3430u},
	{"ENDGAME.LZC", 1, 0, 34, 92, 270, 256, 0xf30905d9u},
	{"ENDGAME.LZC", 1, 2, 228, 172, -304, 29, 0xba9e6d8au},
	{"ENDGAME.LZC", 1, 0, 298, 82, 270, 256, 0x807632f7u},
	{"ENDGAME.LZC", 1, 1, 280, 56, 0, 256, 0x0b4e1f46u},
	{"FLIGHT.LZC", 2, 0, 39, 113, 180, 468, 0xedf94454u},
	{"FLIGHT.LZC", 2, 0, 234, 142, 49, 256, 0xbe86081cu},
	{"FLIGHT.LZC", 2, 0, 136, 34, 270, 256, 0x86d2655eu},
	{"FLIGHT.LZC", 2, 0, 151, 96, 240, 413, 0x76b76290u},
	{"FLIGHT.LZC", 2, 0, 288, 158, 90, 256, 0x7fd2094fu},
	{"FLIGHT.LZC", 2, 0, 122, 181, 123, 73, 0x9fd2762du},
	{"MARS.LZC", 1, 0, 113, 112, 270, 927, 0xaa303a7au},
	{"MARS.LZC", 1, 3, 225, 109, 270, 65, 0xc070540au},
	{"MARS.LZC", 1, 0, 65, 51, 0, 808, 0xcd921e51u},
	{"MARS.LZC", 1, 1, 86, 63, 90, 107, 0x2f782ff4u},
	{"MARS.LZC", 1, 0, 246, 154, -90, 233, 0x402b868cu},
	{"MARS.LZC", 1, 1, 60, 73, 21, 113, 0xb9e13255u},
};

int main() {
	const char *dir = "md/game/MARTIAN";
	Bytes probe;
	if (!readFile(joinPath(dir, "SUPERTM.DRV"), probe)) {
		std::printf("skipped (no Martian Dreams data)\n");
		return 0;
	}
	MdDriver driver;
	CHECK(driver.load(dir));
	static uint8_t pixels[320 * 200];
	for (const Case &c : CASES) {
		Bytes raw, item;
		LibN lib;
		CHECK(readFile(joinPath(dir, c.file), raw) && lib.load(raw, false) && lib.get((size_t)c.item, item));
		std::memset(pixels, 0, sizeof pixels);
		bool ok = driver.draw(pixels, item, c.frame, c.x, c.y, c.angle, c.size);
		if (!ok || crc32(pixels, sizeof pixels) != c.crc)
			std::printf("FAIL %s item %d frame %d at %d,%d angle %d size %d: %s\n", c.file, c.item, c.frame, c.x, c.y, c.angle, c.size, ok ? "other pixels" : "driver stopped");
		CHECK(ok && crc32(pixels, sizeof pixels) == c.crc);
	}
	// a picture without the screen's other pixels leaves them as they were
	std::memset(pixels, 7, sizeof pixels);
	{
		Bytes raw, item;
		LibN lib;
		readFile(joinPath(dir, "SCENE1.LZC"), raw);
		lib.load(raw, false);
		lib.get(7, item);
		CHECK(driver.draw(pixels, item, 0, 160, 100, 30, 256));
		CHECK(pixels[0] == 7 && pixels[320 * 199 + 319] == 7);
	}
	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}
