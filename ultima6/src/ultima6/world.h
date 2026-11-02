#ifndef ULTIMA6_ULTIMA6_WORLD_H
#define ULTIMA6_ULTIMA6_WORLD_H

#include "files/map_data.h"
#include "files/obj_block.h"
#include "files/picture.h"
#include "files/tile_data.h"
#include "files/tile_set.h"
#include "files/world_data.h"

#include <string>
#include <vector>

namespace Ultima6 {
namespace Ultima6 {

/**
 * Everything of Ultima VI's world that is loaded from the game directory and the saved game: the
 * tiles and their attributes, the map, every object of every object block and the actors.
 *
 * Objects are kept as the original keeps them: a flat list of records (status, position, type,
 * frame, quantity, quality) in which an object that is inside a container stores the container's
 * index, and an object in an inventory the owner actor's number, in place of its x coordinate.
 * The original holds at most 0xC00 objects in memory at a time and streams object blocks as the
 * party moves; this implementation loads them all, which is equivalent for what can be seen.
 */
class World {
public:
	struct Obj {
		uint8_t status;
		uint16_t x, y;
		uint8_t z;
		uint16_t type;
		uint8_t frame;
		uint8_t quantity, quality;
		int actor; // actor number if this object is an actor's body, else -1
		int parent; // index of the container (status location 8) or owning actor number (location 0x10 / 0x18), else -1

		int location() const {
			return status & ObjRecord::LOCATION_MASK;
		}
		bool onMap() const {
			return location() == ObjRecord::ON_MAP;
		}
		bool invisible() const {
			return status & 0x02;
		}
	};

	/** dir = the game directory (the one with MAP, CHUNKS, TILEFLAG ... and SAVEGAME\). */
	bool load(const std::string &dir);

	const std::string &error() const {
		return _error;
	}

	TileSet tiles;
	TileFlags flags;
	BaseTile baseTile;
	Palette palette;
	Font font;
	Picture paper;  // PAPER.BMP: the play screen's frame
	Bytes portraits[3]; // PORTRAIT.A, .B and .Z
	MapData map;
	ObjList actors;
	std::vector<Obj> objects;

	/** The 56 x 64 portrait of an actor (see Portrait::locate); false if it has none. */
	bool portrait(int actor, Bytes &pixels) const;

	/** The party's leader (actor 1, the Avatar) position. */
	void avatarPosition(int &x, int &y, int &z) const {
		actors.position(1, x, y, z);
	}

	/** Indices of the objects standing on a map position, in drawing order (bottom first). */
	void objectsAt(int x, int y, int z, std::vector<int> &out) const;

	/** Index into objects of an actor's body, or -1 if the actor is not in the world. */
	int actorObject(int actor) const {
		return actor >= 0 && actor < (int)_actorObject.size() ? _actorObject[actor] : -1;
	}

	/** Puts an object on another map position (keeping the position index in step). */
	void moveObject(int index, int x, int y, int z);

private:
	bool fail(const std::string &msg) {
		_error = msg;
		return false;
	}
	bool loadBlock(const Bytes &data, int firstLevelZ);

	std::string _error;
	// index of objects by position: key = (z << 20) | (y << 10) | x, list kept in load order
	std::vector<std::pair<uint32_t, int>> _byPosition; // sorted by key (stable)
	std::vector<int> _actorObject;
};

} // namespace Ultima6
} // namespace Ultima6

#endif
