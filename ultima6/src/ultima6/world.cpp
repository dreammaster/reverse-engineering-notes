#include "ultima6/world.h"

#include "files/lzw.h"

#include <algorithm>

namespace Ultima6 {
namespace Ultima6 {

static uint32_t positionKey(int x, int y, int z) {
	return ((uint32_t)z << 20) | ((uint32_t)y << 10) | (uint32_t)x;
}

bool World::loadBlock(const Bytes &data, int level) {
	ObjBlock block;
	if (!block.load(data))
		return false;
	size_t base = objects.size();
	for (size_t i = 0; i < block.objects.size(); ++i) {
		const ObjRecord &r = block.objects[i];
		Obj o;
		o.status = r.status;
		o.x = r.x;
		o.y = r.y;
		o.z = r.z;
		o.type = r.type();
		o.frame = r.frame();
		o.quantity = r.quantity;
		o.quality = r.quality;
		o.actor = -1;
		o.parent = -1;
		if (r.location() == ObjRecord::IN_CONTAINER)
			o.parent = (int)(base + r.containerIndex());
		else if (r.location() != ObjRecord::ON_MAP)
			o.parent = r.x; // owning actor
		objects.push_back(o);
	}
	(void)level;
	return true;
}

bool World::load(const std::string &dir) {
	Bytes d, d2, d3, d4;
	if (!readFile(joinPath(dir, "TILEFLAG"), d) || !flags.load(d))
		return fail("TILEFLAG");
	if (!readFile(joinPath(dir, "BASETILE"), d) || !baseTile.load(d))
		return fail("BASETILE");
	if (!readFile(joinPath(dir, "U6PAL"), d) || !palette.load(d))
		return fail("U6PAL");
	Bytes mapTiles, maskType, objTiles, index;
	Bytes raw;
	if (!readFile(joinPath(dir, "MAPTILES.VGA"), raw) || !lzwDecompress(raw, 0, mapTiles))
		return fail("MAPTILES.VGA");
	if (!readFile(joinPath(dir, "MASKTYPE.VGA"), raw) || !lzwDecompress(raw, 0, maskType))
		return fail("MASKTYPE.VGA");
	if (!readFile(joinPath(dir, "OBJTILES.VGA"), objTiles) || !readFile(joinPath(dir, "TILEINDX.VGA"), index) ||
		!tiles.load(mapTiles, maskType, objTiles, index))
		return fail("tile set");
	if (!readFile(joinPath(dir, "MAP"), d) || !readFile(joinPath(dir, "CHUNKS"), d2) || !map.load(d, d2))
		return fail("MAP/CHUNKS");

	std::string save = joinPath(dir, "SAVEGAME");
	if (!readFile(joinPath(save, "OBJLIST"), d) || !actors.load(d))
		return fail("SAVEGAME/OBJLIST");

	objects.clear();
	for (int row = 0; row < 8; ++row) {
		for (int col = 0; col < 8; ++col) {
			if (!readFile(joinPath(save, ObjBlock::surfaceFileName(col, row)), d) || !loadBlock(d, 0))
				return fail(ObjBlock::surfaceFileName(col, row));
		}
	}
	for (int level = 1; level <= MapData::DUNGEON_LEVELS; ++level) {
		if (!readFile(joinPath(save, ObjBlock::dungeonFileName(level)), d) || !loadBlock(d, level))
			return fail(ObjBlock::dungeonFileName(level));
	}

	// The actors' bodies: objects 0-255 of the original. An actor without a type is not in the world.
	for (int a = 0; a < ObjList::ACTORS; ++a) {
		if (actors.objType(a) == 0)
			continue;
		Obj o;
		int x, y, z;
		actors.position(a, x, y, z);
		o.status = actors.objStatus[a];
		o.x = x;
		o.y = y;
		o.z = z;
		o.type = actors.objType(a);
		o.frame = actors.frame(a);
		o.quantity = actors.objQuantity[a] & 0xFF;
		o.quality = actors.objQuantity[a] >> 8;
		o.actor = a;
		o.parent = -1;
		objects.push_back(o);
	}

	_byPosition.clear();
	for (size_t i = 0; i < objects.size(); ++i) {
		const Obj &o = objects[i];
		if (o.onMap())
			_byPosition.push_back(std::make_pair(positionKey(o.x, o.y, o.z), (int)i));
	}
	std::stable_sort(_byPosition.begin(), _byPosition.end(),
					 [](const std::pair<uint32_t, int> &a, const std::pair<uint32_t, int> &b) { return a.first < b.first; });
	return true;
}

void World::objectsAt(int x, int y, int z, std::vector<int> &out) const {
	out.clear();
	uint32_t key = positionKey(x & 0x3FF, y & 0x3FF, z);
	auto it = std::lower_bound(_byPosition.begin(), _byPosition.end(), std::make_pair(key, -1));
	for (; it != _byPosition.end() && it->first == key; ++it)
		out.push_back(it->second);
}

} // namespace Ultima6
} // namespace Ultima6
