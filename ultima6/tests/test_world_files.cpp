// Checks the LZW decoder, map and object block readers against the real Ultima VI files
// (skipped when the game data is absent).

#include "files/lzw.h"
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

static const char *DIR = "u6/game";

static bool load(const std::string &name, Bytes &data) {
	return readFile(joinPath(DIR, name), data);
}

int main() {
	Bytes map, chunks, lz;
	if (!load("MAP", map) || !load("CHUNKS", chunks)) {
		std::printf("skip (no game data in %s)\n", DIR);
		return 0;
	}

	// LZW: the compressed map is exactly the plain map.
	Bytes unpacked;
	CHECK(load("LZMAP", lz) && lzwDecompress(lz, 0, unpacked));
	CHECK(unpacked == map);

	// LZOBJBLK is 64 consecutive object blocks (in Morton order of the superchunks).
	CHECK(load("LZOBJBLK", lz) && lzwDecompress(lz, 0, unpacked));
	size_t pos = 0;
	int blocks = 0;
	while (pos + 2 <= unpacked.size() && blocks < 64) {
		size_t n = get16(&unpacked[pos]);
		Bytes b(unpacked.begin() + pos, unpacked.begin() + pos + 2 + n * ObjBlock::RECORD_SIZE);
		ObjBlock blk;
		CHECK(blk.load(b));
		pos += b.size();
		++blocks;
	}
	CHECK(blocks == 64 && pos == unpacked.size());

	// A truncated stream is rejected.
	Bytes cut(lz.begin(), lz.begin() + lz.size() / 2);
	CHECK(!lzwDecompress(cut, 0, unpacked));

	// Map: every chunk index is a valid chunk (a wrong packing would not be), and
	// the tile under the Avatar's start position is passable.
	MapData md;
	CHECK(md.load(map, chunks));
	for (int z = 0; z <= MapData::DUNGEON_LEVELS; ++z) {
		int size = z == 0 ? MapData::SURFACE_SIZE : MapData::DUNGEON_SIZE;
		for (int y = 0; y < size; y += 8)
			for (int x = 0; x < size; x += 8)
				CHECK(md.chunkIndex(z, x, y) < MapData::CHUNK_COUNT);
	}
	Bytes d;
	TileFlags flags;
	ObjList obj;
	CHECK(load("TILEFLAG", d) && flags.load(d));
	CHECK(load("SAVEGAME/OBJLIST", d) && obj.load(d));
	int ax, ay, az;
	obj.position(1, ax, ay, az);
	int t = md.tile(az, ax, ay);
	std::printf("Avatar at %d,%d,%d stands on terrain tile %d (flags1 %02X)\n", ax, ay, az, t, flags.flags1[t]);
	CHECK(!(flags.flags1[t] & 0x02));

	// Object blocks: sizes agree with the counts, and every object that lies on the map is in
	// the superchunk its file name says.
	int files = 0, objects = 0, outside = 0;
	for (int row = 0; row < 8; ++row) {
		for (int col = 0; col < 8; ++col) {
			ObjBlock blk;
			CHECK(load("SAVEGAME/" + ObjBlock::surfaceFileName(col, row), d) && blk.load(d));
			++files;
			for (size_t i = 0; i < blk.objects.size(); ++i) {
				const ObjRecord &o = blk.objects[i];
				++objects;
				if (o.location() == ObjRecord::ON_MAP && (o.x >> 7 != col || o.y >> 7 != row))
					++outside;
				if (o.location() == ObjRecord::IN_CONTAINER)
					CHECK(o.containerIndex() < (int)blk.objects.size());
			}
		}
	}
	for (int level = 1; level <= MapData::DUNGEON_LEVELS; ++level) {
		ObjBlock blk;
		CHECK(load("SAVEGAME/" + ObjBlock::dungeonFileName(level), d) && blk.load(d));
		++files;
		objects += (int)blk.objects.size();
	}
	CHECK(outside == 0);
	std::printf("%d object block files, %d objects\n", files, objects);

	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}
