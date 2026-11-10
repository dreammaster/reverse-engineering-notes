#ifndef ULTIMA6_ULTIMA6_WORLD_H
#define ULTIMA6_ULTIMA6_WORLD_H

#include "engine/game_id.h"
#include "files/converse.h"
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

struct NewCharacter;

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
		bool deleted = false;
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
	/** With `newGame`, builds a new game from LZOBJBLK/LZDNGBLK with that character instead of reading SAVEGAME. */
	bool load(const std::string &dir, const NewCharacter *newGame = nullptr, const std::string &saveDir = "");
	/**
	 * Writes the world as a SAVEGAME directory: OBJLIST (the actors' arrays and the globals) and one OBJBLK file per
	 * block holding the other objects. Inventories go with the block their owner stands in, contents with their container.
	 * `gameClock` etc. are in the globals, which the caller keeps current.
	 */
	bool save(const std::string &saveDir);

	const std::string &error() const {
		return _error;
	}

	/** BOOK.DAT: 128 word offsets, then the null-terminated texts (read_book, 6B7A). The text of quality `n` (1 to 128), or "" if there is none. */
	std::string bookText(int n) const;
	/** Brings a dead actor back with full hit points; its dead body (quality = the actor) gives back what it held and goes. False if it is not dead. */
	bool resurrect(int actor);
	/** sub_1EF66 (join_party): 0 when the actor has joined (last in the roster, with the name from its script, on the party's side as a follower), 2 when the party is full, 3 if it is in it already. */
	int joinParty(int actor);
	Bytes book;

	GameId game = GameId::Ultima6; // set before load(): which of the three games' data the directory holds
	TileSet tiles;
	TileFlags flags;
	AnimData anim; // ANIMDATA: the tiles that change with the frame counter (sub_12023)
	BaseTile baseTile;
	Palette palette;
	Font font;
	LookData look;
	Picture paper;  // PAPER.BMP: the play screen's frame
	Bytes portraits[3]; // PORTRAIT.A, .B and .Z
	Schedule schedule;  // SCHEDULE
	/** The conversation script of an actor (CONVERSE.A/B for Ultima VI, TALK.LZC for the spin-offs); false if it cannot talk. */
	bool script(int actor, ConverseScript &out) const;
	Bytes converse[2];
	Bytes faces; // MDFACES.LZC / FACES.LZC
	mutable int portraitW = 56, portraitH = 64; // the size of the portrait the last call of portrait() produced
	Bytes talk; // TALK.LZC of Martian Dreams / Savage Empire
	Bytes mdText; // MDTEXT.LZC of Martian Dreams: the books, notes and signs (item = quality - 1)
	/** The text of item `n` of MDTEXT.LZC, "" if there is none (sub_3AAAD). */
	std::string mdTextItem(int n) const;
	mutable std::vector<int> _talkItem; // actor -> item of TALK.LZC  // CONVERSE.A and CONVERSE.B
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

	/** Whether objects of this type stack: their description has a plural form. */
	bool isStackable(int type) const;
	/** The object's quantity (gold has a word, the other stackable types a byte). */
	int quantityOf(const Obj &o) const {
		if (o.type == 0x58)
			return o.quantity | (o.quality << 8); // gold keeps a word
		if (o.type == 0x5A && o.frame == 1)
			return 1; // a lit torch is one object; its quantity is the burn time
		return isStackable(o.type) ? o.quantity : 1;
	}
	void setQuantity(Obj &o, int q) const;
	/** Indices of the objects an actor carries; with `nested`, also the contents of its containers. */
	void inventory(int actor, std::vector<int> &out, bool nested = true) const;
	/** Weight of an object with what it holds, in the game's tenth-unit scale (1/10 kg times 10). */
	int weightOf(int index) const;
	int weightOfType(int type, int quantity) const;
	int carriedWeight(int actor) const;
	/** The squares a missile of sub_37CBD passes from (cx, cy) to (gx, gy), stopping at the reach or a blocking square; true if the goal was reached. */
	bool boltPath(int z, int cx, int cy, int gx, int gy, int reach, std::vector<std::pair<int, int>> &squares) const;
	/** sub_28F67: what leaving a square costs in moves: 5 plus the high nibble of the TILEFLAG byte of the terrain and of every object there that is not impassable. */
	int moveCost(int x, int y, int z) const;
	/** sub_26CC4: takes moves off an actor (halved on a horse, at least 1). */
	void spendMoves(int actor, int cost);
	int contentsWeight(int index) const;
	/** Creates an object in an actor's inventory (merging into a stack of the same kind); returns its index. */
	int addToInventory(int actor, int type, int quality, int quantity);
	/** Removes up to `quantity` of a type from an inventory (quality 0 = any); returns how many were taken. */
	int removeFromInventory(int actor, int type, int quality, int quantity);
	void deleteObject(int index);
	/** Puts an object that is not on the map yet (a new one) onto the map position it holds. */
	void addToMap(int index);
	/** Creates the body of a (temporary) actor that has none, on the map; returns the object index. */
	int addActor(int actor, int type, int x, int y, int z);
	/** Whether an actor number has no body in the world. */
	bool actorFree(int actor) const {
		int k = actorObject(actor);
		return k < 0 || objects[k].deleted;
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
	Bytes _spinoffExtra; // the array the spin-offs' OBJLIST has at 0x14F1 and Ultima VI's does not
};

} // namespace Ultima6
} // namespace Ultima6

#endif
