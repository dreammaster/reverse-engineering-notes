// Portraits against the real PORTRAIT.A/B/Z (skipped when absent).

#include "files/lib_n.h"
#include "files/portrait.h"
#include "files/shape.h"

#include <cstdio>

using namespace Ultima6;

static int failures = 0;

#define CHECK(cond)                                                              \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
			++failures;                                                          \
		}                                                                        \
	} while (0)

int main() {
	Bytes a, b, z, montage;
	if (!readFile("u6/game/PORTRAIT.A", a) || !readFile("u6/game/PORTRAIT.B", b) || !readFile("u6/game/PORTRAIT.Z", z)) {
		std::printf("skip (no game data)\n");
		return 0;
	}

	// Every item of the three archives is a 56 x 64 bitmap.
	struct { const Bytes *file; int items; } files[] = {{&a, 98}, {&b, 96}, {&z, 12}};
	for (const auto &f : files) {
		for (int i = 0; i < f.items; ++i) {
			Bytes px;
			CHECK(Portrait::decode(*f.file, i, px));
		}
	}

	// Who gets which portrait.
	PortraitRef r;
	CHECK(!Portrait::locate(1, 0, 0, r));              // no character yet
	CHECK(Portrait::locate(1, 0, 5, r) && r.file == 'Z' && r.item == 4);
	CHECK(Portrait::locate(5, 0, 0, r) && r.file == 'A' && r.item == 4);   // Lord British
	CHECK(Portrait::locate(98, 0, 0, r) && r.file == 'A' && r.item == 97);
	CHECK(Portrait::locate(99, 0, 0, r) && r.file == 'B' && r.item == 0);
	CHECK(Portrait::locate(230, 0x175, 0, r) && r.file == 'B' && r.item == 0xBF - 98);
	CHECK(!Portrait::locate(230, 0x100, 0, r));

	// MONTAGE.SHP holds the same twelve Avatar heads: item for item the pictures agree on most pixels.
	std::vector<Shape> shapes;
	CHECK(readFile("u6/game/MONTAGE.SHP", montage) && loadShapeFile(montage, shapes));
	for (int i = 0; i < 12; ++i) {
		Bytes px;
		CHECK(Portrait::decode(z, i, px));
		CHECK(shapes[i].width == Portrait::WIDTH && shapes[i].height == Portrait::HEIGHT);
		int same = 0;
		for (int k = 0; k < Portrait::SIZE; ++k)
			same += px[k] == shapes[i].pixels[k];
		CHECK(same > Portrait::SIZE / 2);
	}

	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}
