// Which Ultima VI file formats the two spin-offs share (skipped when their data is absent).

#include "files/map_data.h"
#include "files/obj_block.h"
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

static void check(const char *dir, int schedules) {
	Bytes d;
	if (!readFile(joinPath(dir, "ANIMDATA"), d)) {
		std::printf("skip %s (no game data)\n", dir);
		return;
	}
	AnimData anim;
	CHECK(anim.load(d));
	TileFlags flags;
	CHECK(readFile(joinPath(dir, "TILEFLAG"), d) && flags.load(d));
	BaseTile base;
	CHECK(readFile(joinPath(dir, "BASETILE"), d) && base.load(d));
	Schedule sched;
	CHECK(readFile(joinPath(dir, "SCHEDULE"), d) && sched.load(d));
	CHECK(sched.total() == schedules);

	Bytes map, chunks;
	MapData md;
	CHECK(readFile(joinPath(dir, "MAP"), map) && readFile(joinPath(dir, "CHUNKS"), chunks) && md.load(map, chunks));
	for (int z = 0; z <= MapData::DUNGEON_LEVELS; ++z) {
		int size = z == 0 ? MapData::SURFACE_SIZE : MapData::DUNGEON_SIZE;
		for (int y = 0; y < size; y += 8)
			for (int x = 0; x < size; x += 8)
				CHECK(md.chunkIndex(z, x, y) < MapData::CHUNK_COUNT);
	}

	int objects = 0, outside = 0;
	for (int row = 0; row < 8; ++row) {
		for (int col = 0; col < 8; ++col) {
			ObjBlock blk;
			CHECK(readFile(joinPath(joinPath(dir, "SAVEGAME"), ObjBlock::surfaceFileName(col, row)), d) && blk.load(d));
			for (const ObjRecord &o : blk.objects) {
				++objects;
				outside += o.location() == ObjRecord::ON_MAP && (o.x >> 7 != col || o.y >> 7 != row);
			}
		}
	}
	CHECK(outside == 0);
	std::printf("%s: %d schedules, %d surface objects, all in their superchunks\n", dir, sched.total(), objects);
}

int main() {
	check("md/game/MARTIAN", 192);
	check("se/game/SAVAGE", 354);
	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}
