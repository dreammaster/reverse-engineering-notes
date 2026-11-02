// Checks the file loaders against the real Ultima VI data files (skipped when absent).

#include "files/tile_data.h"
#include "files/world_data.h"

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

static const char *DIR = "u6/game";

static bool load(const char *name, Bytes &data) {
	return readFile(joinPath(DIR, name), data);
}

int main() {
	Bytes data;
	if (!load("ANIMDATA", data)) {
		std::printf("skip (no game data in %s)\n", DIR);
		return 0;
	}

	AnimData anim;
	CHECK(anim.load(data));
	CHECK(anim.count == 29);
	CHECK(anim.tileToAnimate[0] == 8 && anim.tileToAnimate[1] == 9);

	TileFlags flags;
	CHECK(load("TILEFLAG", data) && flags.load(data));

	BaseTile base;
	CHECK(load("BASETILE", data) && base.load(data));
	CHECK(base.firstTile[0] == 0);

	Palette pal;
	CHECK(load("U6PAL", data) && pal.load(data));

	Schedule sched;
	CHECK(load("SCHEDULE", data) && sched.load(data));
	CHECK(sched.total() == 563);
	CHECK(sched.entries.size() == 563u * 5);
	CHECK(sched.index[0] == 0);
	int sum = 0;
	for (int i = 0; i < Schedule::ACTORS; ++i) {
		CHECK(sched.entryCount(i) >= 0);
		sum += sched.entryCount(i);
	}
	CHECK(sum == 563); // every entry belongs to exactly one actor

	ObjList obj;
	if (load("SAVEGAME/OBJLIST", data)) {
		CHECK(obj.load(data));
		CHECK(obj.partySize >= 1 && obj.partySize <= ObjList::PARTY_MAX);
		CHECK(obj.partyRoster[0] == 1); // the Avatar is actor 1
		int x, y, z;
		obj.position(obj.partyRoster[0], x, y, z);
		CHECK(x >= 0 && x < 1024 && y >= 0 && y < 1024 && z >= 0 && z <= 5);
		CHECK(obj.objType(obj.partyRoster[0]) != 0);
		std::printf("party of %d, Avatar at %d,%d,%d type %03X frame %d, %zu bytes of globals\n", obj.partySize, x, y, z,
					obj.objType(obj.partyRoster[0]), obj.frame(obj.partyRoster[0]), obj.globals.size());
	}

	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}
