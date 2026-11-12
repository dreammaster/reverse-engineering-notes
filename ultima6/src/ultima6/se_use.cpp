// Savage Empire's Use command (cmd_use = sub_35E46 of its GAME.EXE, see docs/spinoffs.md): built like Ultima VI's, a table of object types whose handlers are blocks of code that end in a
// jump to loc_36508 (five moves, redraw). The table is 80 types at cs:8967 with the handler offsets 0xA2 bytes after it; the handlers are named here by the address of the routine they call.

#include "ultima6/describe.h"
#include "ultima6/equipment.h"
#include "ultima6/session.h"

#include <cstdlib>

namespace Ultima6 {
namespace Ultima6 {

namespace {

// sub_2E85C: the types a person may try to use (84 words at ds:11AA); the others are "Not possible."
const uint16_t SE_USABLE[84] = {0x3E, 0x2F, 0x12E, 0x133, 0xD6, 0x69, 0x6E, 0xD5, 0x67, 0xA, 0x6A, 0xEA, 0x5D, 0x6F, 0xC5, 0xA1, 0x66, 0x68, 0x6B, 0x6C, 0x6D, 0x2B, 0x71, 0x52, 0x4F, 0x50, 0x51,
								0x72, 0xF0, 0x78, 0x3F, 0x113, 0xCF, 0xC0, 0x3D, 0xBC, 0xD8, 0x83, 0xAD, 0xD4, 0x5E, 0x6, 0x3C, 0xCC, 0xF7, 0xFC, 0xDD, 0x94, 0x2C, 0x34, 0x19, 0xD9, 0x5F, 0xA5,
								0x6E, 0x88, 0x77, 0xBE, 0x3B, 0x31, 0x33, 0x14, 0x2A, 0x16, 0x89, 0xE9, 0xED, 0x32, 0x1D, 0x5C, 0x86, 0xBF, 0xD0, 0xD1, 0x56, 0x58, 0x57, 0x8F, 0xB1, 0xE5, 0x101,
								0xF2, 0xBA, 0xFE};

// The switch of cmd_use: object type -> the routine its handler calls (0 where the handler is a block of its own, named in seAction).
struct Handler {
	uint16_t type;
	uint32_t sub;
};
const Handler SE_HANDLERS[] = {
	{0x0A, 0x34F6B}, {0x14, 0x362F7}, {0x16, 0x36312}, {0x19, 0x32488}, {0x1D, 0x31436}, {0x2A, 0x34FDE}, {0x2B, 0x31293}, {0x2C, 0x32488}, {0x2F, 0x34783}, {0x31, 0x34D88}, {0x32, 0x31E67},
	{0x33, 0x34EDF}, {0x34, 0x32488}, {0x3B, 0x310AD}, {0x3C, 0x310AD}, {0x3D, 0x33EA5}, {0x3E, 0x33E52}, {0x3F, 0x354BC}, {0x4F, 0x33F4B}, {0x50, 0x33F4B}, {0x51, 0x33F4B}, {0x52, 0x33F4B},
	{0x56, 0x3646E}, {0x57, 0x3646E}, {0x58, 0x3646E}, {0x5C, 0x35C2E}, {0x5D, 0x31DF5}, {0x5E, 0x343E9}, {0x5F, 0x31C2D}, {0x66, 0x31DF5}, {0x67, 0x31DF5}, {0x68, 0x31DF5}, {0x69, 0x31DF5},
	{0x6A, 0x31DF5}, {0x6B, 0x31DF5}, {0x6C, 0x31DF5}, {0x6D, 0x31DF5}, {0x6E, 0x31DF5}, {0x6F, 0x31DF5}, {0x71, 0x31DF5}, {0x72, 0x31DF5}, {0x77, 0x31436}, {0x78, 0x31DF5}, {0x83, 0x31F60},
	{0x86, 0x33C06}, {0x88, 0x335A9}, {0x89, 0x31671}, {0x8F, 0x35B44}, {0x94, 0x3542B}, {0xA1, 0x353D0}, {0xA5, 0x33F9E}, {0xAD, 0x3215F}, {0xB1, 0x33B10}, {0xBA, 0x36206}, {0xBC, 0x361BA},
	{0xBE, 0x34147}, {0xBF, 0x33777}, {0xC0, 0x3204A}, {0xC5, 0x31DF5}, {0xCC, 0x34C0A}, {0xCF, 0x3623D}, {0xD0, 0x30ECC}, {0xD1, 0x35686}, {0xD4, 0x31B48}, {0xD5, 0x31DF5}, {0xD6, 0x31DF5},
	{0xD8, 0x35D54}, {0xE5, 0x33211}, {0xE9, 0x31C98}, {0xEA, 0x35150}, {0xED, 0x31C98}, {0xF0, 0x31671}, {0xF2, 0x3215F}, {0xF7, 0x3423A}, {0xFC, 0x35972}, {0xFE, 0x3215F}, {0x101, 0x3215F},
	{0x113, 0x346A8}, {0x12E, 0x34B00}, {0x133, 0x349A9}};

uint32_t handlerOf(int type) {
	for (const Handler &h : SE_HANDLERS)
		if (h.type == type)
			return h.sub;
	return 0;
}

} // namespace

bool Game::seUsable(int type) const {
	for (uint16_t t : SE_USABLE)
		if (t == type)
			return true;
	return false;
}

// sub_1ABE3: whether an actor is within five squares east or west and four north or south of the party's leader (the 11 x 9 squares of the view)
bool Game::seInView(int actor) const {
	int a = _world.actorObject(actor);
	int l = _world.actorObject(_world.actors.partyRoster[0]);
	if (a < 0 || l < 0)
		return false;
	return std::abs((int)_world.objects[a].x - (int)_world.objects[l].x) <= 5 && std::abs((int)_world.objects[a].y - (int)_world.objects[l].y) <= 4;
}

// sub_34DD4: a flash at a square (of the camera or a magnesium ribbon). Once the story has not yet come to it (flag 2 of actor 3 not set) and the ghost (actor 0x19) has not been seen (flag 0x80 of actor 0x35),
// a ghost within two squares to the west and one to the east (and the same north and south) of the flash starts to talk (its flag is 0x28), a ghost that is only in view whispers "Closer...".
// APPROX(79): the picture of the flash (the palette goes white for a moment) is not drawn
void Game::seFlash(int fx, int fy) {
	ObjList &al = _world.actors;
	if ((al.talkFlags[3] & 2) || (al.talkFlags[0x35] & 0x80) || !seInView(0x19))
		return;
	int g = _world.actorObject(0x19);
	int dx = fx - (int)_world.objects[g].x, dy = fy - (int)_world.objects[g].y;
	if (dx > 1 || dy > 1 || dx < -2 || dy < -2) {
		message("\nYou hear a barely audible voice whisper, \"Closer...\".\n");
		return;
	}
	al.talkFlags[0x19] = 0x28;
	al.talkFlags[0x35] |= 0x80;
	startTalk(0x19);
}

// sub_1B486: a new object lies on the map
int Game::seMake(int type, int x, int y, int z, int quantity, int quality) {
	World::Obj o;
	o.status = 0;
	o.x = (uint16_t)x;
	o.y = (uint16_t)y;
	o.z = (uint8_t)z;
	o.type = (uint16_t)type;
	o.frame = 0;
	o.quantity = (uint8_t)quantity;
	o.quality = (uint8_t)quality;
	o.actor = -1;
	o.parent = -1;
	_world.objects.push_back(o);
	_world.addToMap((int)_world.objects.size() - 1);
	return (int)_world.objects.size() - 1;
}

// sub_1AC47: the first object of the type on a square of the level the party is on, -1 if there is none
int Game::seFind(int type, int x, int y) const {
	std::vector<int> here;
	_world.objectsAt(x, y, z, here);
	for (int i : here)
		if (_world.objects[i].type == type && !_world.objects[i].deleted)
			return i;
	return -1;
}

// sub_1B5FA into a container: a new thing inside
void Game::seFill(int container, int type, int quantity) {
	World::Obj o;
	o.status = ObjRecord::IN_CONTAINER;
	o.x = o.y = 0;
	o.z = 0;
	o.type = (uint16_t)type;
	o.frame = 0;
	o.quantity = (uint8_t)quantity;
	o.quality = 0;
	o.actor = -1;
	o.parent = container;
	_world.objects.push_back(o);
}

// sub_1B5FA with a quantity: the thing goes into the pack of the actor unless it would be too heavy (the original's "carrying too much already")
bool Game::seGive(int actor, int type, int quantity) {
	if (_world.carriedWeight(actor) + _world.weightOfType(type, quantity) > _world.actors.strength[actor] * 20)
		return false;
	_world.addToInventory(actor, type, 0, quantity);
	return true;
}

// cmd_use of Savage Empire after the object is chosen (the name is printed): "Not possible." for a type that is not usable at all, "Out of range!" beyond one square (a carried thing is
// always in reach), then the handler of the type; every handler ends with five moves. APPROX(79): the choice of the object by the cursor is the engine's, the clicks on the party panel and
// the terrain (a tree, lava, a yucca: sub_33DFF, sub_3215F, sub_33EF8) are not used; many handlers are not ported yet (seAction).
void Game::seUseObject(int idx) {
	_seAsync = false;
	World::Obj &o = _world.objects[idx];
	bool usable = false;
	for (uint16_t t : SE_USABLE)
		usable |= t == o.type;
	if (!usable) {
		message("Not possible.\n");
		return;
	}
	uint32_t sub = handlerOf(o.type);
	if (sub)
		seAction(sub, idx);
	if (_pending != 'Y') // (a handler that asks "On " ends when the object is chosen)
		seEnd();
}

// loc_36508: five moves; the prompt comes after a handler that waited for a key
void Game::seEnd() {
	_world.spendMoves(actingActor(), 5);
	if (_seAsync) {
		_seAsync = false;
		prompt();
	}
}

// "On ": the cursor chooses an object; `then` is called with the object (-1 for an empty square, -2 when none was chosen) and the square
void Game::seAsk(std::function<void(int, int, int)> then) {
	message("On ");
	_seThen = then;
	_seAsync = true;
	_pending = 'Y';
	_cursor = true;
	_cx = _cy = 5;
}

// the object the cursor chose: the one on top of the square (a person too); its name is printed
void Game::seChosen(int tx, int ty) {
	std::vector<int> here;
	_world.objectsAt(tx, ty, z, here);
	int pick = -1;
	for (size_t i = here.size(); i-- > 0;)
		if (!_world.objects[here[i]].invisible()) {
			pick = here[i];
			break;
		}
	if (pick >= 0)
		message(Describe::object(_world, _world.objects[pick]) + "\n");
	std::function<void(int, int, int)> then = _seThen;
	_seThen = nullptr;
	if (then)
		then(pick, tx, ty);
	seEnd();
}

// the handler named by the routine it calls
void Game::seAction(uint32_t sub, int idx) {
	World::Obj &o = _world.objects[idx];
	int actor = actingActor();
	switch (sub) {
	case 0x31C2D: // drink the liquor: gone, one more on the drunkenness counter (byte_4226E, which counts down a minute at a time)
		_world.deleteObject(idx);
		if (_world.actors.globals.size() > 0x26)
			++_world.actors.globals[0x26];
		message("\nYou drink it.\n");
		break;
	case 0x31DF5: // eat: one of a stack goes, anything else is deleted
		message("\nYou eat the food.\n");
		consumeOne(idx);
		break;
	case 0x31C98: { // a raft (0xE9 lying about, or 0xED under the party: sub_31C98): not in solo mode, at least four members of whom four have a paddle (0x0C), else the refusals; boarded the party is the raft (the leader's body 0xED, frame 3, the others out of sight), left (Use on it) next to land (the raft's square or one of the three squares to its west and north that can be walked on: "Only next to land.") the raft (0xE9, with its hit points) is put down where it was
		// APPROX(79): the original keeps the raft as actor 0 beyond the roster, with the members inside it (sub_252BB, sub_25433, sub_1C4AB); here the leader's body takes its type as on Ultima VI's boats, and the party steps ashore onto the land square found
		ObjList &al = _world.actors;
		int mask = z == 0 ? 0x3FF : 0xFF;
		if (_aboard && o.actor >= 0) {
			int ox = o.x, oy = o.y, lx = -1, ly = -1;
			for (int dx = 0; dx < 2 && lx < 0; ++dx)
				for (int dy = 0; dy < 2 && lx < 0; ++dy) {
					int sx = (ox - dx) & mask, sy = (oy - dy) & mask;
					if (!(_world.flags.flags1[_world.map.tile(z, sx, sy) & (TileFlags::TILES - 1)] & 2)) {
						lx = sx;
						ly = sy;
					}
				}
			if (lx < 0) {
				message("\nOnly next to land.\n");
				break;
			}
			o.type = (uint16_t)_footType;
			o.frame = (uint8_t)_footFrame;
			int raft = seMake(0xE9, ox, oy, z, _shipQuantity, 0);
			_world.objects[raft].frame = 3;
			_aboard = false;
			for (int m = 0; m < al.partySize; ++m) {
				int k = _world.actorObject(al.partyRoster[m]);
				if (k >= 0) {
					_world.objects[k].status &= ~0x02;
					_world.moveObject(k, lx, ly, z);
				}
			}
			x = lx;
			y = ly;
			chooseMusic();
			break;
		}
		if (_solo >= 0) {
			message("Not in solo mode!\n");
			break;
		}
		if (al.partySize < 4) {
			message("You don't have enough people to paddle.\n");
			break;
		}
		int paddlers = 0;
		for (int m = 0; m < al.partySize && paddlers < 4; ++m)
			if (sePackItem(al.partyRoster[m], 0x0C) >= 0)
				++paddlers;
		if (paddlers < 4) {
			message("You would need at least four people carrying paddles to operate the raft effectively.\n");
			break;
		}
		World::Obj &me = _world.objects[_leaderObject];
		_footType = me.type;
		_footFrame = me.frame;
		me.type = 0xED;
		me.frame = 3;
		_shipQuantity = o.quantity;
		int vx = o.x, vy = o.y;
		_world.deleteObject(idx);
		_aboard = true;
		for (int m = 1; m < al.partySize; ++m) {
			int k = _world.actorObject(al.partyRoster[m]);
			if (k >= 0)
				_world.objects[k].status |= 0x02;
		}
		_world.moveObject(_leaderObject, vx, vy, z);
		x = vx;
		y = vy;
		chooseMusic();
		break;
	}
	case 0x34B00: { // a turtle (the creature 0x12E, sub_34B00): the acting member climbs on (its body becomes 0x133 with the turtle's frame, on the turtle's square, sub_34965); if that member is on a turtle already, another member next to the turtle does; none: "You're already on a turtle!"
		int rider = actor, tx = o.x, ty = o.y, turtleActor = o.actor, frame = o.frame;
		int rk = _world.actorObject(rider);
		if (rk >= 0 && _world.objects[rk].type == 0x133) {
			rider = -1;
			for (int m = 0; m < _world.actors.partySize && rider < 0; ++m) {
				int a = _world.actors.partyRoster[m], k = _world.actorObject(a);
				if (k >= 0 && std::abs((int)_world.objects[k].x - tx) <= 1 && std::abs((int)_world.objects[k].y - ty) <= 1 && _world.objects[k].type != 0x133)
					rider = a;
			}
			if (rider < 0) {
				message("\nYou're already on a turtle!\n");
				break;
			}
		}
		_world.deleteObject(idx);
		if (turtleActor >= 0)
			_world.actors.hp[turtleActor] = 0;
		int k = _world.actorObject(rider);
		_world.moveObject(k, tx, ty, z);
		_world.objects[k].type = 0x133;
		_world.objects[k].frame = (uint8_t)frame;
		if (k == _leaderObject) {
			x = tx;
			y = ty;
		}
		break;
	}
	case 0x349A9: { // a member on a turtle (0x133, sub_349A9): steps to a square of land next to it (the first of the eight round it that can be walked on), and the turtle (0x12E, friendly) is left in the water; "Only next to land!" if there is none
		if (o.actor < 0)
			break;
		int who = o.actor, ox = o.x, oy = o.y, mask = z == 0 ? 0x3FF : 0xFF, lx = -1, ly = -1;
		for (int dx = 0; dx < 3 && lx < 0; ++dx)
			for (int dy = 0; dy < 3 && lx < 0; ++dy) {
				if (dx == 1 && dy == 1)
					continue;
				int sx = (ox + dx - 1) & mask, sy = (oy + dy - 1) & mask;
				if (!(_world.flags.flags1[_world.map.tile(z, sx, sy) & (TileFlags::TILES - 1)] & 2)) {
					lx = sx;
					ly = sy;
				}
			}
		if (lx < 0) {
			message("\nOnly next to land!\n");
			break;
		}
		int k = _world.actorObject(who);
		_world.moveObject(k, lx, ly, z);
		_world.objects[k].type = _world.actors.baseObjFrame[who] & 0x3FF;
		_world.objects[k].frame = (uint8_t)(_world.actors.baseObjFrame[who] >> 10);
		if (k == _leaderObject) {
			x = lx;
			y = ly;
		}
		int turtle = _creatures.spawn(0x12E, 0x8F, ox, oy, z, false);
		if (turtle >= 0)
			_world.actors.status[turtle] = (uint8_t)((_world.actors.status[turtle] & 0x9F) | 0x40);
		break;
	}
	case 0x3542B: { // the jungle drum (sub_3542B): drumsticks (0x5B) must be in the pack, else "You have nothing with which to play the drum."; once the tribes are friends (flag 0x80 of the eleven actors 0x10, 0x11, 0x13, 0x14, 0x1B, 0x1D, 0x20, 0x22, 0x28, 0x30 and 0x35) and the drum has not been played (flag 8 of actor 1) it starts the story's last scene
		// APPROX(79): the scene itself (sub_2E1AC: its pictures and conversation) is not done; only its flag (8 of actor 1) is set
		if (sePackItem(actor, 0x5B) < 0) {
			message("You have nothing with which to play the drum.\n");
			break;
		}
		ObjList &al = _world.actors;
		static const int FRIENDS[11] = {0x10, 0x11, 0x13, 0x14, 0x1B, 0x1D, 0x20, 0x22, 0x28, 0x30, 0x35};
		bool all = !(al.talkFlags[1] & 8);
		for (int f : FRIENDS)
			all &= (al.talkFlags[f] & 0x80) != 0;
		if (all)
			al.talkFlags[1] |= 8;
		break;
	}
	case 0x34783: { // turtle bait (sub_34783): a turtle (0x12E, friendly, standing still) comes up on a water square (tiles with flag 1 and 2) next to the member: "You must be near water." if there is none (the bait is kept); in the lake (x 0x335-0x3ED, y 0xE7-0x1E7) it stays, anywhere else, or when no square is free, "Nothing takes the bait."; the bait is used up
		int me = _world.actorObject(actor);
		if (me < 0)
			break;
		int hx = _world.objects[me].x, hy = _world.objects[me].y, mask = z == 0 ? 0x3FF : 0xFF;
		bool water = false;
		int px = -1, py = -1;
		for (int dx = 0; dx < 3 && px < 0; ++dx)
			for (int dy = 0; dy < 3 && px < 0; ++dy) {
				if (dx == 1 && dy == 1)
					continue;
				int sx = (hx + dx - 1) & mask, sy = (hy + dy - 1) & mask;
				int f = _world.flags.flags1[_world.map.tile(z, sx, sy) & (TileFlags::TILES - 1)];
				water |= (f & 1) != 0;
				if (!(f & 1) || !(f & 2))
					continue;
				std::vector<int> there;
				_world.objectsAt(sx, sy, z, there);
				bool free = true;
				for (int i : there)
					free &= _world.objects[i].deleted || _world.objects[i].actor < 0; // (nobody stands there)
				if (free) {
					px = sx;
					py = sy;
				}
			}
		if (px < 0 && !water) {
			message("\nYou must be near water.\n");
			break;
		}
		bool lake = hx >= 0x335 && hx <= 0x3ED && hy >= 0xE7 && hy <= 0x1E7;
		if (px >= 0 && lake) {
			int turtle = _creatures.spawn(0x12E, 0, px, py, z, false);
			if (turtle >= 0)
				_world.actors.status[turtle] = (uint8_t)((_world.actors.status[turtle] & 0x9F) | 0x40);
		} else {
			message("\nNothing takes the bait.\n");
		}
		consumeOne(idx);
		break;
	}
	case 0x353D0: { // the sealed door: Yunapotli (actor 11) in the party and in view makes it slide open
		bool open = (_world.actors.status[0xB] & 0x80) && seInView(0xB);
		if (open) {
			message("\nYunapotli closes its eyes and appears to concentrate.");
			message("  There is a grinding noise froom the direction of the doors... ");
			_world.deleteObject(idx);
			message(" and they slide open!\n");
		} else {
			message("You can't budge it!\n");
		}
		break;
	}
	case 0x30ECC: { // a torch is lit in a free hand: one of the stack goes, a lit torch (0xD1) with 100 minutes in the quality is readied
		Equipment::Slots slots = Equipment::of(_world, actor);
		if (slots.twoHanded || (slots.object[2] >= 0 && slots.object[5] >= 0)) {
			message("\nYour hands are full.\n"); // (the original goes on to set the quality of whatever object its register held: left out)
			break;
		}
		consumeOne(idx);
		int n = _world.addToInventory(actor, 0xD1, 0, 1);
		World::Obj &lit = _world.objects[n];
		lit.status = (uint8_t)((lit.status & ~ObjRecord::LOCATION_MASK) | ObjRecord::READIED);
		lit.quantity = 1;
		lit.quality = 100;
		break;
	}
	case 0x361BA: { // a crate: shut (frame 1) it opens, else "Not possible."; then what is in it is looked for (sub_2EC8A)
		if (o.frame == 1)
			o.frame = 0;
		else
			message("Not possible.\n");
		seSearch(idx);
		break;
	}
	case 0x33F4B: // bones: a rib bone (0xD3)
	case 0x33E52: // a bean stalk: beans (0x6D)
	case 0x33EA5: { // a corn stalk: corn (0x6C)
		int type = sub == 0x33F4B ? 0xD3 : sub == 0x33E52 ? 0x6D : 0x6C;
		if (seGive(actor, type, 1))
			message(sub == 0x33F4B ? "\nYou got some rib bones.\n" : sub == 0x33E52 ? "\nYou got some beans from the plant.\n" : "\nYou got some corn from the plant.\n");
		else
			message("You are carrying too much already.\n");
		break;
	}
	case 0x34D88: // the notebook: Jimmy reads his shorthand, nobody else can
		if (_world.actors.baseObjFrame[actor] == 0x138) {
			message("\nJimmy says, \"Hey, boss, you'll never be able to figure out my ");
			message("shorthand.\"\n\n\"Why don't you just ask me what ");
			message("you want to know?\"\n");
		} else {
			message("\nYou can't read Jimmy's shorthand.\n");
		}
		break;
	case 0x34EDF: { // the camera: the film (the quality) counts down, the flash works without it
		bool carried = carriedObject(o);
		if (o.quality > 0) {
			--o.quality;
			message("\nYou take a picture.\n");
		} else {
			message("\nThe camera's out of film, ");
			message("but the flash still works.\n");
		}
		{
			int hk = carried ? _world.actorObject(actor) : idx;
			if (hk >= 0)
				seFlash(_world.objects[hk].x, _world.objects[hk].y);
		}
		break;
	}
	case 0x33B10: { // the wire screen: sifting at a square of the eight around the member where there is a 0xE7 gives ten of 0x7B (sulphur powder)
		int a = _world.actorObject(actor);
		if (a < 0)
			break;
		int px = _world.objects[a].x, py = _world.objects[a].y;
		bool made = false;
		for (int dx = 0; dx < 3 && !made; ++dx) {
			for (int dy = 0; dy < 3 && !made; ++dy) {
				if (dx == 1 && dy == 1)
					continue;
				if (seFind(0xE7, px + dx - 1, py + dy - 1) < 0)
					continue;
				message("\nYou made some sulphur powder.\n");
				if (!seGive(actor, 0x7B, 10))
					seMake(0x7B, px, py, z, 10);
				made = true;
			}
		}
		if (!made)
			message("\nThere is nothing to sift.\n");
		break;
	}
	case 0x362F7: // a rope: only in party mode (else "Not in solo mode!"), then sub_19505 takes the party to the square its quality names (table ds:4D4 + 4 x (9 + quality)) on level 1, where the hole gets its rope (frame 1)
		if (_solo != -1)
			message("Not in solo mode!\n");
		else
			seClimb(idx);
		break;
	case 0x36312: // a hole in the ceiling: frame 1 with the party together is the way up (sub_19505: table ds:4D4 by quality, level 0, or 2 for quality 8), frame 0 needs a rope from above
		if (_solo == -1 && o.frame == 1) {
			seClimb(idx);
		} else if (o.frame == 0) {
			message("\nYou need a rope dropped from above.\n");
		} else {
			message("Not in solo mode!\n");
		}
		break;
	case 0x36206: // a brazier with frame bit 2: "Not possible."; else the fire routine (sub_3215F)
		if (o.frame & 2)
			message("Not possible.\n");
		else
			seFire();
		break;
	case 0x3623D: // burnt logs: frame 0 "Not possible.", 1 starts a fire (frame 2), 2 the fire routine (sub_3215F)
		if (o.frame == 0) {
			message("Not possible.\n");
		} else if (o.frame == 1) {
			o.frame = 2;
			message("\nYou started a fire.\n");
		} else if (o.frame == 2) {
			seFire();
		}
		break;
	case 0x3215F: // an oven or a fire
		seFire();
		break;
	case 0x31F60: { // the digging stick: of the eight squares around the member the first with clay in the ground (terrain tile 0x20-0x2F or 0xBE: sub_2E5F9) gives a lump
		int a = _world.actorObject(actor);
		if (a < 0)
			break;
		int px = _world.objects[a].x, py = _world.objects[a].y;
		bool found = false;
		for (int dx = 0; dx < 3 && !found; ++dx) {
			for (int dy = 0; dy < 3 && !found; ++dy) {
				if (dx == 1 && dy == 1)
					continue;
				int tx = px + dx - 1, ty = py + dy - 1;
				int tile = _world.map.tile(z, tx & (z == 0 ? 0x3FF : 0xFF), ty & (z == 0 ? 0x3FF : 0xFF));
				if (!((tile >= 0x20 && tile <= 0x2F) || tile == 0xBE))
					continue;
				message("\nYou get some clay.\n");
				found = true;
				if (!seGive(actor, 0xC0, 1))
					seMake(0xC0, tx, ty, z, 1);
			}
		}
		if (!found)
			message("\nThere is nothing to dig!\n");
		break;
	}
	case 0x3204A: { // clay: one lump becomes a pot (0x84), in the pack if the clay was carried, else on the square of the clay (a carried one that does not fit is put at the member's feet)
		int a = _world.actorObject(actor);
		bool carried = carriedObject(o);
		int ox = o.x, oy = o.y;
		consumeOne(idx);
		if (carried) {
			if (!seGive(actor, 0x84, 1) && a >= 0)
				seMake(0x84, _world.objects[a].x, _world.objects[a].y, z, 1);
		} else {
			seMake(0x84, ox, oy, z, 1);
		}
		message("\nYou shaped a small, soft pot from the clay.\n");
		break;
	}
	case 0x335A9: { // the metal bucket: empty (frame 0) it takes tar from a tar pit or water from water next to the member; with water (frame 2) it is poured onto something
		int a = _world.actorObject(actor);
		if (a < 0)
			break;
		int px = _world.objects[a].x, py = _world.objects[a].y;
		int mask = z == 0 ? 0x3FF : 0xFF;
		if (o.frame == 2) { // sub_33364
			int bucket = idx;
			seAsk([this, bucket, actor](int obj, int tx, int ty) {
				World::Obj &b = _world.objects[bucket];
				auto empty = [&]() {
					for (size_t i = 0; i < _world.objects.size(); ++i)
						if (!_world.objects[i].deleted && _world.objects[i].location() == ObjRecord::IN_CONTAINER && _world.objects[i].parent == bucket && _world.objects[i].type == 0x8C)
							_world.deleteObject((int)i);
					b.frame = 0;
				};
				if (obj == -2) {
					message("nothing!\n");
					return;
				}
				int ax = 0, ay = 0;
				int k = _world.actorObject(actor);
				if (k >= 0) {
					ax = _world.objects[k].x;
					ay = _world.objects[k].y;
				}
				if (obj >= 0 && !carriedObject(_world.objects[obj]) && (std::abs(tx - ax) > 1 || std::abs(ty - ay) > 1)) {
					message("Out of range!\n");
					return;
				}
				if (obj < 0) {
					int tile = _world.map.tile(z, tx & (z == 0 ? 0x3FF : 0xFF), ty & (z == 0 ? 0x3FF : 0xFF));
					message(Describe::article(_world, tile) + Describe::tileName(_world, tile, false) + "\n");
					empty();
					message("\nYou pour the water out.\n");
					return;
				}
				World::Obj &t = _world.objects[obj];
				if (t.type == 0xF2 || t.type == 0xFE) {
					empty();
					message("\nYou put the fire out.\n");
					_world.deleteObject(obj);
				} else if (t.type == 0xCF && t.frame == 2) {
					empty();
					message("\nYou put the fire out.\n");
					t.frame = 0;
				} else {
					empty();
					message("\nYou pour the water out.\n");
				}
			});
			break;
		}
		if (o.frame == 1) { // sub_32D11: tar poured onto cloth strips (0x86) tars as many as the bucket holds (the tar is used up as many as there are strips). APPROX(79): the fires it can start (eggs, nests, webs, the ground: sub_32B5E) are not ported
			int bucket = idx;
			seAsk([this, bucket, actor](int obj, int tx, int ty) {
				if (obj < 0) {
					message("No effect.\n");
					return;
				}
				if (!seReach(_world.objects[obj], tx, ty))
					return;
				if (_world.objects[obj].type != 0x86) {
					message("No effect.\n");
					return;
				}
				int tar = -1;
				for (size_t i = 0; i < _world.objects.size(); ++i)
					if (!_world.objects[i].deleted && _world.objects[i].location() == ObjRecord::IN_CONTAINER && _world.objects[i].parent == bucket && _world.objects[i].type == 0x8B)
						tar = (int)i;
				int have = tar >= 0 ? _world.objects[tar].quantity : 0, strips = _world.objects[obj].quantity;
				int n = std::min(have, strips);
				message("cloth strips\n\nYou tarred some cloth strips.\n");
				bool held = carriedObject(_world.objects[obj]) && _world.objects[obj].parent == actor;
				int sx = _world.objects[obj].x, sy = _world.objects[obj].y;
				if (!held || !seGive(actor, 0xBF, n))
					seMake(0xBF, sx, sy, z, n);
				if (have >= strips)
					_world.deleteObject(obj);
				else
					_world.objects[obj].quantity = (uint8_t)(strips - have);
				if (have <= strips) { // the bucket is empty now
					for (size_t i = 0; i < _world.objects.size(); ++i)
						if (!_world.objects[i].deleted && _world.objects[i].location() == ObjRecord::IN_CONTAINER && _world.objects[i].parent == bucket && _world.objects[i].type == 0x8B)
							_world.deleteObject((int)i);
					_world.objects[bucket].frame = 0;
				} else {
					_world.objects[tar].quantity = (uint8_t)(have - strips);
				}
			});
			break;
		}
		bool got = false;
		for (int dx = 0; dx < 3 && !got; ++dx) {
			for (int dy = 0; dy < 3 && !got; ++dy) {
				if (dx == 1 && dy == 1)
					continue;
				int sx = (px + dx - 1) & mask, sy = (py + dy - 1) & mask;
				std::vector<int> here;
				_world.objectsAt(sx, sy, z, here);
				int top = here.empty() ? -1 : (int)_world.objects[here.front()].type;
				if (top == 0xE5) { // a tar pit
					seFill(idx, 0x8B, 10);
					o.frame = (uint8_t)(o.frame + 1);
					message("\nYou got some tar in the bucket.\n");
					got = true;
				} else {
					// (the original asks the terrain of the square (x, y - dx + dy), a slip of its own)
					int tile = _world.map.tile(z, sx, (py + dy - dx) & mask);
					bool water = (_world.flags.flags1[tile & (TileFlags::TILES - 1)] & 1) || top == 0xA2 || top == 0xC8 || top == 0xA6 || top == 0x96 || top == 0x97;
					if (water) {
						seFill(idx, 0x8C, 10);
						o.frame = (uint8_t)(o.frame + 2);
						message("\nYou got some water in the bucket.\n");
						got = true;
					}
				}
			}
		}
		if (!got)
			message("\nThere is nothing to get in the bucket.\n");
		break;
	}
	case 0x31B48: { // the fishing pole: deep water (terrain 0x0C) on the diagonal through the member's square (the original asks (x + i - 1, y + i - 1) for both loops), a catch one time in five
		int a = _world.actorObject(actor);
		if (a < 0)
			break;
		int px = _world.objects[a].x, py = _world.objects[a].y;
		int mask = z == 0 ? 0x3FF : 0xFF;
		bool found = false;
		for (int i = 0; i < 3 && !found; ++i) {
			for (int j = 0; j < 3 && !found; ++j) {
				if (i == 1 && j == 1)
					continue;
				if (_world.map.tile(z, (px + i - 1) & mask, (py + i - 1) & mask) != 0x0C)
					continue;
				found = true;
				if (_rng.next() % 5 == 0 && seGive(actor, 0xD5, 1))
					message("\nYou caught a fish!\n");
				else
					message("\nYou didn't get a fish.\n");
			}
		}
		if (!found)
			message("\nYou need to stand next to deep water.\n");
		break;
	}
	case 0x33C06: // a cloth strip binds wounds: 1 to 5 hit points back
	case 0x35972: { // a crystal heals completely and cures poison
		bool crystal = sub == 0x35972;
		int strip = idx;
		seAsk([this, strip, crystal](int obj, int tx, int ty) {
			if (obj < 0) {
				message("nothing\n");
				return;
			}
			World::Obj &t = _world.objects[obj];
			if (!seReach(t, tx, ty))
				return;
			if (t.actor < 0 && t.type != 0xF8 && t.type != 0xF9) {
				message("nothing\n");
				return;
			}
			if (t.actor < 0) {
				message("a body.\n\nThat person is beyond help.\n");
				return;
			}
			int who = t.actor;
			std::string name = Describe::object(_world, t);
			message(name + "\n");
			int most = seMaxHp(who);
			int hp = _world.actors.hp[who];
			if (most <= hp) {
				message(crystal ? "\n" + name + " Doesn't need any help.\n" : "\n" + name + " doesn't need any help.\n");
				return;
			}
			if (crystal) {
				_world.actors.hp[who] = (uint8_t)most;
				_world.actors.status[who] &= 0xF7;
				message("\n" + name + " is healed.\n");
			} else {
				int add = seRoll(1, 5);
				_world.actors.hp[who] = (uint8_t)(most > hp + add ? hp + add : most);
				message("\n" + name + "'s wounds are bound.\n");
			}
			World::Obj &s = _world.objects[strip];
			if (s.quantity > 1)
				--s.quantity;
			else
				_world.deleteObject(strip);
		});
		break;
	}
	case 0x31436: { // hammers: on a boulder (0xAC) a piece of rock flakes off, on cold burnt logs (0xCF, frame 0) charcoal is made
		int actorNo = actor;
		seAsk([this, actorNo](int obj, int tx, int ty) {
			if (obj < 0) {
				message("nothing!\n");
				return;
			}
			World::Obj &t = _world.objects[obj];
			if (!seReach(t, tx, ty))
				return;
			int a = _world.actorObject(actorNo);
			int px = a >= 0 ? _world.objects[a].x : 0, py = a >= 0 ? _world.objects[a].y : 0;
			if (t.type == 0xAC) {
				if (seTooWeak(seStrength(actorNo))) {
					message("\nGlanced off!\n");
					return;
				}
				message("\nA piece of rock flakes off.\n");
				if (!seGive(actorNo, 0x3A, 1))
					seMake(0x3A, px, py, z, 1);
			} else if (t.type == 0xCF && t.frame == 0) {
				int count = seRoll(1, seStrength(actorNo));
				int stack = -1;
				for (size_t i = 0; i < _world.objects.size() && stack < 0; ++i) {
					const World::Obj &c = _world.objects[i];
					if (!c.deleted && c.type == 0x81 && carriedObject(c) && c.parent == actorNo && c.quantity + count <= 255)
						stack = (int)i;
				}
				if (stack < 0) {
					message("\nYou made some charcoal.\n");
					if (!seGive(actorNo, 0x81, count))
						seMake(0x81, px, py, z, count);
				} else {
					_world.objects[stack].quantity = (uint8_t)(_world.objects[stack].quantity + count);
				}
			} else {
				message("No effect.\n");
			}
		});
		break;
	}
	case 0x31293: { // the bell: on a tame, still unbelled dinosaur (0x118, frames 0D / 11: the second kind points at its head by the quality) it is hung round its neck
		int bell = idx;
		seAsk([this, bell](int obj, int tx, int ty) {
			if (obj < 0) {
				message("nothing!\n");
				return;
			}
			World::Obj &t = _world.objects[obj];
			if (!seReach(t, tx, ty))
				return;
			if (t.type != 0x118 || (t.frame != 0x0D && t.frame != 0x11)) {
				message("No effect.\n");
				return;
			}
			int who = t.frame == 0x11 ? t.quality : t.actor;
			if (who < 0 || who > 255)
				return;
			if (!(_world.actors.status[who] & 4)) {
				message("No effect.\n");
				return;
			}
			if (_world.actors.talkFlags[who]) {
				message("\nIt already has a bell on its neck.\n");
				return;
			}
			_world.deleteObject(bell);
			_world.actors.talkFlags[who] = 0xFF;
			message("\nYou put the bell on the dinosaur's neck.\n");
			if (who == 0xB5)
				_world.actors.talkFlags[0x14] |= 0x80;
		});
		break;
	}
	case 0x354BC: { // the jug of plachta: on a spear (0x1A) it makes the spear of Shamap (type 1) in the member's pack (else at the member's feet); the jug is used up
		int jug = idx;
		seAsk([this, jug, actor](int obj, int tx, int ty) {
			if (obj < 0) {
				message("nothing!\n");
				return;
			}
			World::Obj &t = _world.objects[obj];
			if (!seReach(t, tx, ty))
				return;
			if (t.type != 0x1A) {
				message("No effect.\n");
				return;
			}
			int a = _world.actorObject(actor);
			int px = a >= 0 ? _world.objects[a].x : 0, py = a >= 0 ? _world.objects[a].y : 0;
			bool held = carriedObject(t) && t.parent == actor;
			if (t.quantity > 1)
				--t.quantity;
			else
				_world.deleteObject(obj);
			if (!held || !seGive(actor, 1, 1))
				seMake(1, px, py, z, 1);
			_world.deleteObject(jug);
		});
		break;
	}
	case 0x34FDE: { // the vine: on a hole (0xA8) a rope (0x14) hangs from the square up and to the left, with the quality of the hole; else "You can't use a rope here!"
		int vine = idx;
		seAsk([this, vine](int obj, int tx, int ty) {
			if (obj < 0) {
				message("nothing!\n");
				return;
			}
			World::Obj &t = _world.objects[obj];
			if (!seReach(t, tx, ty))
				return;
			if (t.type != 0xA8) {
				message("\nYou can't use a rope here!\n");
				return;
			}
			int quality = t.quality, hx = t.x, hy = t.y;
			_world.deleteObject(vine);
			seMake(0x14, hx - 1, hy - 1, z, 0, quality);
		});
		break;
	}
	case 0x343E9: { // the fire extinguisher: puts out a fire (0xF2, 0xFE: gone) or burning logs (0xCF frame 2: frame 0); on lava next to the member it hardens (an object 0x39 whose frame is the place of the tile in the table of lava tiles)
		seAsk([this](int obj, int tx, int ty) {
			static const int LAVA[15] = {0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0xAA, 0xAB};
			if (obj >= 0) {
				World::Obj &t = _world.objects[obj];
				if (!seReach(t, tx, ty))
					return;
				if (t.type == 0xF2 || t.type == 0xFE) {
					sfx(0x1D);
					message("\nYou put the fire out.\n");
					_world.deleteObject(obj);
				} else if (t.type == 0xCF && t.frame == 2) {
					sfx(0x1D);
					message("\nYou put the fire out.\n");
					t.frame = 0;
				} else {
					message("No effect.\n");
				}
				return;
			}
			if (obj != -1)
				return;
			int a = _world.actorObject(actingActor());
			if (a >= 0 && (std::abs(tx - (int)_world.objects[a].x) > 1 || std::abs(ty - (int)_world.objects[a].y) > 1)) {
				message("Out of range!\n");
				return;
			}
			int mask = z == 0 ? 0x3FF : 0xFF;
			int tile = _world.map.tile(z, tx & mask, ty & mask);
			int at = -1;
			for (int i = 0; i < 15; ++i)
				if (LAVA[i] == tile)
					at = i;
			if (at < 0) {
				message("nothing!\n");
				return;
			}
			if (z == 3) {
				message("\nThe lava is too far below you.\n");
				return;
			}
			sfx(0x1D);
			message("Lava\n");
			message("You harden some of the lava.\n");
			int n = seMake(0x39, tx, ty, z, 0);
			_world.objects[n].frame = (uint8_t)at;
		});
		break;
	}
	case 0x32488: { // knives and scissors: cloth (0xB4) becomes ten strips; the dead (0x118, by frame) are butchered; a body part (0xF9) loses its feathers (frame 15) or is made a mess of (frame 11)
		int knife = actor;
		seAsk([this, knife](int obj, int tx, int ty) {
			if (obj < 0) {
				message("nothing!\n");
				return;
			}
			World::Obj &t = _world.objects[obj];
			if (!seReach(t, tx, ty))
				return;
			int a = _world.actorObject(knife);
			int px = a >= 0 ? _world.objects[a].x : 0, py = a >= 0 ? _world.objects[a].y : 0;
			int x = t.x, y = t.y;
			// something lands in the pack, or on the square if it does not fit
			auto give = [&](int type, int quantity, int sx, int sy) {
				if (!seGive(knife, type, quantity))
					seMake(type, sx, sy, z, quantity);
			};
			auto gore = [&](int sx, int sy) {
				int n = seMake(0xF8, sx, sy, z, 0);
				_world.objects[n].frame = (uint8_t)seRoll(0, 2);
			};
			auto meat = [&]() {
				message("\nYou cut up some meat.\n");
				give(0x6E, seRoll(2, 7), x, y);
			};
			if (t.type == 0xB4) {
				bool held = carriedObject(t) && t.parent == knife;
				if (!held || !seGive(knife, 0x86, 10))
					seMake(0x86, px, py, z, 10);
				World::Obj &cloth = _world.objects[obj]; // (the creation above may have moved the vector)
				if (cloth.quantity > 1)
					--cloth.quantity;
				else
					_world.deleteObject(obj);
				message("\nYou made some cloth strips.\n");
			} else if (t.type == 0x118) {
				// what each frame (1 to 47) of the dead is: G ghoul, H tough hide, M meat, C clank, S big cat, X a mess, B meat and a skeleton
				static const char KIND[48] = "G.HH.H.HH.H...H.H...M.CCC.HH.HH.H.H.S.M.M.H.MXB";
				int f = t.frame;
				char k = f >= 1 && f <= 47 ? KIND[f - 1] : '.';
				switch (k) {
				case 'G':
					message("\nYou ghoul!\n");
					break;
				case 'H':
					message("\nThe hide is too tough.\n");
					break;
				case 'C':
					message("\nClank!\n");
					break;
				case 'M': {
					int bones = seMake(0x52, x, y, z, 0);
					_world.objects[bones].frame = 1;
					gore(x, y);
					gore(x - 1, y);
					_world.deleteObject(obj);
					meat();
					break;
				}
				case 'B': {
					seMake(0x50, x, y, z, 0);
					gore(x, y);
					_world.deleteObject(obj);
					meat();
					break;
				}
				case 'S': {
					int bones = seMake(0x52, x, y, z, 0);
					_world.objects[bones].frame = 1;
					gore(x, y);
					gore(x - 1, y);
					_world.deleteObject(obj);
					message("\nYou skinned the big cat.\n");
					give(0x6E, seRoll(2, 7), x, y);
					give(0x5C, 1, x, y);
					break;
				}
				case 'X':
					message("\nYou make a mess.\n");
					seMake(0xF8, x, y, z, 0);
					break;
				default:
					break;
				}
			} else if (t.type == 0xF9) {
				if (t.frame == 0x0F) {
					gore(x, y);
					_world.deleteObject(obj);
					message("\nYou remove the feathers.\n");
					give(0x30, seRoll(5, 20), x, y);
				} else if (t.frame == 0x0B) {
					message("\nYou make a mess.\n");
					seMake(0xF8, x, y, z, 0);
				} else {
					message("\nYou ghoul!\n");
				}
			} else {
				message("No effect.\n");
			}
		});
		break;
	}
	case 0x34F6B: { // a magnesium ribbon: one is used up and it flashes at the member's square (sub_34DD4, as for the camera)
		consumeOne(idx);
		int hk = _world.actorObject(actor);
		if (hk >= 0)
			seFlash(_world.objects[hk].x, _world.objects[hk].y);
		break;
	}
	case 0x35B44: { // the machine: a gem (0x48, 0x49 or 0x4A) from the member's pack goes in and an 0xC5 comes out on the machine's square
		int mx = o.x, my = o.y;
		int gem = -1;
		for (size_t i = 0; i < _world.objects.size() && gem < 0; ++i) {
			const World::Obj &c = _world.objects[i];
			if (!c.deleted && (c.type == 0x48 || c.type == 0x49 || c.type == 0x4A) && carriedObject(c) && c.parent == actor)
				gem = (int)i;
		}
		if (gem < 0) {
			message("\nYou don't have any gems.\n");
			break;
		}
		consumeOne(gem);
		sfx(0x20);
		message("\nYou put a gem in the machine.\n");
		seMake(0xC5, mx, my, z, 1);
		break;
	}
	case 0x35150: { // a sliding door (0xEA): frames 1 and 3 slide it one square east / south (to frame 5 / 7) unless something solid is there, frames 5 and 7 slide it back (to frame 1 / 3)
		int f = o.frame, ox = o.x, oy = o.y;
		int mask = z == 0 ? 0x3FF : 0xFF;
		if (f == 1 || f == 3) {
			int nx = (ox + (f == 1 ? 1 : 0)) & mask, ny = (oy + (f == 3 ? 1 : 0)) & mask;
			std::vector<int> here;
			_world.objectsAt(nx, ny, z, here);
			bool blocked = false;
			for (int i : here) {
				const World::Obj &c = _world.objects[i];
				if (c.deleted || c.type == 0xEA || c.type == 0xEC || c.type == 0xE2)
					continue;
				if (!(_world.flags.flags2[(_world.baseTile.firstTile[c.type & 0x3FF] + c.frame) & (TileFlags::TILES - 1)] & 0x10))
					blocked = true;
			}
			if (blocked) {
				message("\nThe door is blocked.\n");
				break;
			}
			_world.moveObject(idx, nx, ny, z);
			_world.objects[idx].frame = (uint8_t)(f + 4);
		} else if (f == 5) {
			_world.moveObject(idx, (ox - 1) & mask, oy, z);
			_world.objects[idx].frame = 1;
			// the door at 184,235 on the surface lets actor 4, if in view and not yet met, join: APPROX(82): the conversation that follows is left out
			if (ox == 0x184 && oy == 0x235 && z == 0 && !(_world.actors.talkFlags[4] & 0x10) && seInView(4)) {
				_world.actors.status[4] &= 0xFB;
				_world.actors.talkFlags[4] |= 0x10;
				_world.joinParty(4);
			}
		} else if (f == 7) {
			_world.moveObject(idx, ox, (oy - 1) & mask, z);
			_world.objects[idx].frame = 3;
		}
		break;
	}
	case 0x33777: { // tarred cloth: "On -" a log (0xCE) it makes a torch (0xD0); on a fired pot (0x85) with five pinches of gunpowder (0xCC) a grenade (0x89)
		int cloth = idx;
		seAsk([this, cloth, actor](int obj, int tx, int ty) {
			if (obj < 0) {
				message("nothing!\n");
				return;
			}
			if (!seReach(_world.objects[obj], tx, ty))
				return;
			int type = _world.objects[obj].type;
			bool held = carriedObject(_world.objects[obj]) && _world.objects[obj].parent == actor;
			int a = _world.actorObject(actor);
			int px = a >= 0 ? _world.objects[a].x : 0, py = a >= 0 ? _world.objects[a].y : 0;
			auto one = [&](int i) {
				World::Obj &u = _world.objects[i];
				if (u.quantity > 1)
					--u.quantity;
				else
					_world.deleteObject(i);
			};
			auto product = [&](int made) {
				if (held) {
					one(obj);
					if (!seGive(actor, made, 1))
						seMake(made, px, py, z, 1);
				} else {
					seMake(made, tx, ty, z, 1);
					one(obj);
				}
			};
			if (type == 0xCE) {
				message("\nYou make a torch\n");
				one(cloth);
				product(0xD0);
			} else if (type == 0x85) {
				int powder = sePackItem(actor, 0xCC);
				if (powder < 0 || _world.objects[powder].quantity < 5) {
					message("\nYou need five pinches of gunpowder to make a grenade.\n");
					return;
				}
				if (_world.objects[powder].quantity == 5)
					_world.deleteObject(powder);
				else
					_world.objects[powder].quantity = (uint8_t)(_world.objects[powder].quantity - 5);
				one(cloth);
				message("\nYou made a grenade.\n");
				product(0x89);
			} else {
				message("No effect.\n");
			}
		});
		break;
	}
	case 0x33F9E: { // the loom: on flax (0xD2) cloth (0xB4) is woven, in the pack if the flax was carried, else on the flax's square; the flax is used up
		seAsk([this, actor](int obj, int tx, int ty) {
			if (obj < 0) {
				message("nothing!\n");
				return;
			}
			World::Obj &t = _world.objects[obj];
			if (!seReach(t, tx, ty))
				return;
			if (t.type != 0xD2) {
				message("No effect.\n");
				return;
			}
			message("\nYou wove cloth from the flax.\n");
			int a = _world.actorObject(actor);
			int px = a >= 0 ? _world.objects[a].x : 0, py = a >= 0 ? _world.objects[a].y : 0;
			if (carriedObject(t)) {
				if (!seGive(actor, 0xB4, 1))
					seMake(0xB4, px, py, z, 1);
			} else {
				seMake(0xB4, tx, ty, z, 1);
			}
			World::Obj &flax = _world.objects[obj];
			if (flax.quantity > 1)
				--flax.quantity;
			else
				_world.deleteObject(obj);
		});
		break;
	}
	case 0x33211: { // a tar pit: "On -" cloth strips (0x86): all of them become tarred cloth strips (0xBF)
		seAsk([this, actor](int obj, int tx, int ty) {
			if (obj < 0) {
				message("No effect.\n");
				return;
			}
			World::Obj &t = _world.objects[obj];
			if (!seReach(t, tx, ty))
				return;
			if (t.type != 0x86) {
				message("No effect.\n");
				return;
			}
			int n = t.quantity;
			message("cloth strips\n");
			bool held = carriedObject(t) && t.parent == actor;
			if (!held || !seGive(actor, 0xBF, n))
				seMake(0xBF, tx, ty, z, n);
			_world.deleteObject(obj);
		});
		break;
	}
	case 0x35C2E: { // the folded skin: a free 2 x 2 patch up and to the left of it (nothing on the two squares of the left column, the three squares passable) lets it be unfolded into a big skin (type 0xF5, three frames on); not while it is carried
		int mask = z == 0 ? 0x3FF : 0xFF;
		bool blocked = carriedObject(o);
		for (int dy = 0; dy < 2 && !blocked; ++dy) {
			std::vector<int> here;
			_world.objectsAt((o.x - 1) & mask, (o.y - dy) & mask, z, here);
			blocked = !here.empty();
		}
		int sq[3][2] = {{o.x - 1, o.y}, {o.x, o.y - 1}, {o.x - 1, o.y - 1}};
		for (auto &s : sq)
			blocked |= (_world.flags.flags1[_world.map.tile(z, s[0] & mask, s[1] & mask) & (TileFlags::TILES - 1)] & 2) != 0;
		if (blocked) {
			message("\nThere is not enough room to unfold the skin.\n");
			break;
		}
		o.type = 0xF5;
		o.frame = (uint8_t)(o.frame + 3);
		break;
	}
	case 0x34147: // the metal head: next to the member (the eight squares around) the headless statue (0xF7) gets it back
	case 0x3423A: { // the headless statue: the member's pack must hold a metal head (0xBE)
		bool head = sub == 0x34147;
		int a = _world.actorObject(actor);
		if (a < 0)
			break;
		int statue = idx, hd = idx;
		int mx = _world.objects[a].x, my = _world.objects[a].y;
		if (head) {
			statue = -1;
			for (int dx = 0; dx < 3 && statue < 0; ++dx)
				for (int dy = 0; dy < 3 && statue < 0; ++dy)
					if (!(dx == 1 && dy == 1))
						statue = seFind(0xF7, mx + dx - 1, my + dy - 1);
		} else {
			hd = -1;
			for (size_t i = 0; i < _world.objects.size() && hd < 0; ++i) {
				const World::Obj &c = _world.objects[i];
				if (!c.deleted && c.type == 0xBE && carriedObject(c) && c.parent == actor)
					hd = (int)i;
			}
		}
		if (statue < 0 || hd < 0) {
			message("No effect.\n");
			break;
		}
		int sx = head ? mx : _world.objects[statue].x, sy = head ? my : _world.objects[statue].y;
		_world.deleteObject(hd);
		_world.deleteObject(statue);
		int yuna = _world.actorObject(0xB);
		if (yuna >= 0)
			_world.moveObject(yuna, sx, sy, z);
		_world.joinParty(0xB);
		_world.actors.talkFlags[0xB] |= 0x40;
		message("\nYou replace the statue's missing head.\n");
		break;
	}
	case 0x35D54: { // the crystal garden: Rafkin (actor 7) or else Johann (actor 5), in the party, awake, in view, identifies potassium nitrate (0x82): ten of it
		int who = -1;
		for (int c : {7, 5}) {
			uint8_t st = _world.actors.status[c];
			if (who < 0 && (st & 0x80) && !(st & 0x10) && !(st & 4) && seInView(c))
				who = c;
		}
		if (who < 0) {
			message("\nThere is nobody in the party who can identify which crystals are useful.\n");
			break;
		}
		if (!seGive(actor, 0x82, 10)) {
			message("You are carrying too much already.\n");
			break;
		}
		message(who == 7 ? "\nRafkin " : "\nJohann ");
		message("identifies some potassium nitrate, ");
		message("and you scrape a quantity from the crystal.\n");
		break;
	}
	case 0x346A8: // a door (0x113): frame 1 opens to frame 4, frame 3 to frame 4 as well (APPROX(79): the sub_2DBFE call when the party is close is left out)
		if (o.frame == 1)
			o.frame = 4;
		else if (o.frame == 3)
			o.frame = 4;
		break;
	case 0x31671: { // a grenade (0x89) or a device (0xF0), frame 0: it is lit (0x8A) or activated (0xF1) and goes off after three minutes (sub_31838, in advanceTime)
		bool grenade = o.type == 0x89;
		if (o.frame != 0) {
			message("No effect.\n");
			break;
		}
		if (seTimerLeft() != 0) {
			message(grenade ? "\nA grenade has already been lit.\n" : "\nA device has already been activated.\n");
			break;
		}
		int holder = -1, container = -1;
		if (carriedObject(o)) {
			holder = o.parent;
			container = o.parent;
		} else if (o.location() == ObjRecord::IN_CONTAINER) {
			container = o.parent;
			int up = container;
			while (up >= 0 && _world.objects[up].location() == ObjRecord::IN_CONTAINER)
				up = _world.objects[up].parent;
			holder = up >= 0 && carriedObject(_world.objects[up]) ? _world.objects[up].parent : -1;
		}
		int ox = o.x, oy = o.y;
		consumeOne(idx);
		int made;
		int type = grenade ? 0x8A : 0xF1;
		if (container >= 0 && container == holder && seGive(holder, type, 1)) {
			made = sePackItem(holder, type);
		} else if (container >= 0 && container != holder) {
			seFill(container, type, 1);
			made = (int)_world.objects.size() - 1;
		} else {
			int hx = ox, hy = oy;
			int ha = holder >= 0 ? _world.actorObject(holder) : -1;
			if (ha >= 0) {
				hx = _world.objects[ha].x;
				hy = _world.objects[ha].y;
			}
			made = seMake(type, hx, hy, z, 1);
		}
		seSetTimer(3, made);
		message(grenade ? "\nGrenade lit!\n" : "\nDevice activated.\n");
		break;
	}
	case 0x35686: { // a lit torch: "On -" a pteranodon nest (0xAB) or the egg in it (0x6F), a web (0x11) or anything on the burning list sets it alight; gunpowder (0xCC) explodes
		seAsk([this](int obj, int tx, int ty) {
			if (obj < 0) {
				message("nothing!\n");
				return;
			}
			World::Obj &t = _world.objects[obj];
			if (!seReach(t, tx, ty))
				return;
			int type = t.type, ox = t.x, oy = t.y;
			if (type == 0xAB || type == 0x6F) {
				int nest = type == 0xAB ? obj : seFind(0xAB, ox, oy);
				if (nest < 0) {
					message("dinosaur egg\n"); // (the fire the original makes here is out again at once: an egg does not burn)
					return;
				}
				message("pteranodon nest\n");
				seMake(0x4C, ox, oy, z, 0);
				_world.objects[(int)_world.objects.size() - 1].frame = 3;
				seBurn(ox, oy);
			} else if (type == 0x11) {
				message("web\n");
				seBurn(tx, ty);
			} else if (seBurns(type)) {
				message("\nYou incinerated the " + Describe::object(_world, t) + ".\n");
				_world.deleteObject(obj);
			} else if (type == 0xCC) {
				int holder = carriedObject(t) ? t.parent : -1;
				if (holder >= 0)
					_combat.applyDamage(1, holder, seRoll(1, 30));
				_world.deleteObject(obj);
				seExplode(tx, ty, false);
			} else {
				message("No effect.\n");
			}
		});
		break;
	}
	case 0x3646E: // a totem (0x56 Heluzz, 0x57 Motazz, 0x58 Aphazz): only the shaman (actor 9) can invoke one
		if (actor == 9)
			seTotem(idx);
		else
			message("`" + _combat.nameOf(actor) + " can't use that!\n");
		break;
	case 0x31E67: { // the flintlock (frame 1 = loaded): one gunpowder (0xCC) from the pack loads it
		if (o.frame == 1) {
			message("\nFlintlock already loaded.\n");
			break;
		}
		int powder = sePackItem(actor, 0xCC);
		if (powder < 0) {
			message("\nYou don't have any gunpowder.\n");
			break;
		}
		consumeOne(powder);
		_world.objects[idx].frame = 1;
		message("\nFlintlock loaded.\n");
		break;
	}
	case 0x34C0A: { // gunpowder: on a flintlock (0x32) that is not loaded it loads it and one of the powder is used up
		int powder = idx;
		seAsk([this, powder](int obj, int tx, int ty) {
			if (obj < 0) {
				message("nothing!\n");
				return;
			}
			World::Obj &t = _world.objects[obj];
			if (!seReach(t, tx, ty))
				return;
			if (t.type != 0x32) {
				message("No effect.\n");
				return;
			}
			if (t.frame == 1) {
				message("\nFlintlock already loaded.\n");
				return;
			}
			t.frame = 1;
			consumeOne(powder);
			message("\nFlintlock loaded.\n");
		});
		break;
	}
	case 0x310AD: { // the mortar and the grinding stone: corn (0x6C) alone is ground into flour (0x5D); charcoal (0x81), sulphur powder (0x7B) and 0x82 together are mixed into gunpowder (0xCC), as much as the smallest heap
		int slot[3] = {-1, -1, -1}; // 0x81, 0x7B, 0x82
		int corn = -1, least = 0;
		bool any = false;
		for (size_t i = 0; i < _world.objects.size(); ++i) {
			const World::Obj &c = _world.objects[i];
			if (c.deleted || c.location() != ObjRecord::IN_CONTAINER || c.parent != idx)
				continue;
			any = true;
			if (c.type == 0x6C)
				corn = (int)i;
			else if (c.type == 0x81)
				slot[0] = (int)i;
			else if (c.type == 0x7B)
				slot[1] = (int)i;
			else if (c.type == 0x82)
				slot[2] = (int)i;
			else {
				message("\nYou can't grind that!\n");
				goto done;
			}
			if (corn >= 0)
				break;
			least = (c.quantity > least && least != 0) ? least : c.quantity;
		}
		if (!any) {
			message("\nThere's nothing to grind!\n");
			break;
		}
		if (corn >= 0) {
			if (slot[0] < 0 && slot[1] < 0 && slot[2] < 0) {
				message("\nYou grind the corn into corn flour.\n");
				int n = _world.objects[corn].quantity ? _world.objects[corn].quantity : 1;
				_world.deleteObject(corn);
				seFill(idx, 0x5D, n);
			} else {
				message("\nThe mixture in the " + Describe::object(_world, o) + " is wrong.\n");
			}
			break;
		}
		for (int s : slot) {
			if (s < 0) {
				message("\nYou're missing some ingredients.\n");
				goto done;
			}
			if (_world.objects[s].quantity > least)
				_world.objects[s].quantity = (uint8_t)(_world.objects[s].quantity - least);
			else
				_world.deleteObject(s);
		}
		seFill(idx, 0xCC, least);
		message("\nYou mix a batch of gunpowder.\n");
	done:
		break;
	}
	default: // APPROX(79): not ported yet
		break;
	}
}

// The Use of a square without an object (sub_35E46 35F72-35FE8, sub_33DFF, sub_33EF8): beyond one square "Out of range!"; a tree (the tiles of ds:1134) gives a branch (0xCE), the yucca plant (tile
// 0x34) flax (0xD2), lava (the tiles 0xF-0x1B, 0xAA, 0xAB) is a fire (seFire); anything else "Not possible.". Five moves are spent.
void Game::seUseTerrain(int tx, int ty) {
	static const uint16_t TREES[19] = {0x32, 0x33, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0xC0, 0xC1, 0xC2, 0xC3, 0xC6, 0xB3, 0xB4, 0x82};
	int actor = actingActor();
	int a = _world.actorObject(actor);
	if (a >= 0 && (std::abs(tx - (int)_world.objects[a].x) > 1 || std::abs(ty - (int)_world.objects[a].y) > 1)) {
		message("Out of range!\n");
		return;
	}
	int mask = z == 0 ? 0x3FF : 0xFF;
	int tile = _world.map.tile(z, tx & mask, ty & mask);
	bool tree = false;
	for (uint16_t t : TREES)
		tree |= t == tile;
	if (tree) {
		message("Tree\n");
		if (seGive(actor, 0xCE, 1))
			message("\nYou pulled a branch from the tree.\n");
		else
			message("You are carrying too much already.\n");
	} else if (tile == 0x34) {
		message("Yucca plant\n");
		if (seGive(actor, 0xD2, 1))
			message("\nYou got some flax from the yucca plant.\n");
		else
			message("You are carrying too much already.\n");
	} else if ((tile >= 0xF && tile <= 0x1B) || tile == 0xAA || tile == 0xAB) {
		message("Lava\n");
		seFire();
		return; // ("On " waits for its object)
	} else {
		message("Not possible.\n");
		return;
	}
	_world.spendMoves(actor, 5);
}

// sub_3215F, what is done at a fire: "On " a clay pot (0x84) fires it (0x85), corn flour (0x5D) becomes tortillas (0x66), a log (0xCE) charcoal (0x81, ten),
// the things on the table at ds:118A burn up, gunpowder (0xCC) explodes. The explosion is seExplode.
void Game::seFire() {
	int actor = actingActor();
	seAsk([this, actor](int obj, int tx, int ty) {
		if (obj < 0) {
			message("nothing!\n");
			return;
		}
		World::Obj &t = _world.objects[obj];
		if (!seReach(t, tx, ty))
			return;
		int a = _world.actorObject(actor);
		int px = a >= 0 ? _world.objects[a].x : 0, py = a >= 0 ? _world.objects[a].y : 0;
		bool held = carriedObject(t) && t.parent == actor;
		auto one = [&]() {
			World::Obj &u = _world.objects[obj];
			if (u.quantity > 1)
				--u.quantity;
			else
				_world.deleteObject(obj);
		};
		auto give = [&](int type, int quantity, int sx, int sy) {
			if (!seGive(actor, type, quantity))
				seMake(type, sx, sy, z, quantity);
		};
		int type = t.type, quantity = t.quantity;
		bool burns = seBurns(type);
		if (type == 0x84) {
			message("\nYou fired the pot.\n");
			one();
			give(0x85, 1, px, py);
		} else if (type == 0x5D) {
			if (held)
				give(0x66, quantity, tx, ty);
			else
				seMake(0x66, tx, ty, z, quantity);
			_world.deleteObject(obj);
			message("\nYou made corn tortillas.\n");
		} else if (type == 0xCE) {
			message("\nYou made some charcoal.\n");
			one();
			give(0x81, 10, px, py);
		} else if (burns) {
			message("\nYou incinerated the " + Describe::object(_world, t) + ".\n");
			_world.deleteObject(obj);
		} else if (type == 0xCC) {
			int ex = held ? px : tx, ey = held ? py : ty;
			_world.deleteObject(obj);
			seExplode(ex, ey, false);
		} else {
			message("No effect.\n");
		}
	});
}

// the device timer of sub_14746: words at ds:422A2 / 422A4 = globals 0x5A / 0x5C, minutes left and the object that goes off
int Game::seTimerLeft() const {
	const std::vector<uint8_t> &g = _world.actors.globals;
	return g.size() > 0x5B ? g[0x5A] | (g[0x5B] << 8) : 0;
}
void Game::seSetTimer(int minutes, int object) {
	std::vector<uint8_t> &g = _world.actors.globals;
	if (g.size() <= 0x5D)
		return;
	g[0x5A] = (uint8_t)minutes;
	g[0x5B] = (uint8_t)(minutes >> 8);
	g[0x5C] = (uint8_t)object;
	g[0x5D] = (uint8_t)(object >> 8);
}

// sub_14746's first lines: the timer runs down by the minutes that pass; when it is out the lit grenade or device (sub_31838) goes off where it is (where its holder is)
void Game::seTimerTick(int minutes) {
	int left = seTimerLeft();
	if (left == 0)
		return;
	if (left > minutes) {
		seSetTimer(left - minutes, _world.actors.globals[0x5C] | (_world.actors.globals[0x5D] << 8));
		return;
	}
	int obj = (int16_t)(_world.actors.globals[0x5C] | (_world.actors.globals[0x5D] << 8));
	seSetTimer(0, 0xFFFF);
	if (obj < 0 || obj >= (int)_world.objects.size() || _world.objects[obj].deleted)
		return;
	int at = obj;
	while (at >= 0 && !_world.objects[at].onMap() && _world.objects[at].parent >= 0 && _world.objects[at].location() != ObjRecord::ON_MAP) {
		int p = _world.objects[at].parent;
		at = _world.objects[at].location() == ObjRecord::IN_CONTAINER ? p : _world.actorObject(p);
	}
	int ex = at >= 0 ? _world.objects[at].x : _world.objects[obj].x, ey = at >= 0 ? _world.objects[at].y : _world.objects[obj].y;
	bool device = _world.objects[obj].type == 0xF1;
	_world.deleteObject(obj);
	seExplode(ex, ey, device);
}

// sub_29A41 (a grenade) and sub_29AC1 (a device): the 3 x 3 squares around are hit, then sub_318F5 sets off the grenades that lie around and tears the webs down.
// APPROX(81): the flash (sub_37A40), the damage to objects that are not persons (sub_2A99B's other half: doors, walls, the things of ds:...) and the objects 0xA7 / 0x107 of sub_318F5 are not done;
// the persons are hurt by Combat::applyDamage, which credits a kill to the avatar.
void Game::seExplode(int ex, int ey, bool device) {
	int mask = z == 0 ? 0x3FF : 0xFF;
	if (device) {
		sfx(0x12);
		sfx(0x1D);
	} else {
		sfx(6);
	}
	for (int dx = -1; dx <= 1; ++dx) {
		for (int dy = -1; dy <= 1; ++dy) {
			std::vector<int> here;
			_world.objectsAt((ex + dx) & mask, (ey + dy) & mask, z, here);
			for (int i : here) {
				if (_world.objects[i].deleted || _world.objects[i].actor < 0)
					continue;
				int who = _world.objects[i].actor, type = _world.objects[i].type;
				if (device) {
					if (type == 0x143 || type == 0x148)
						_combat.applyDamage(1, who, seRoll(1, 35));
				} else {
					_combat.applyDamage(1, who, seRoll(1, 20));
				}
			}
		}
	}
	// sub_318F5: a grenade in the squares around goes off at once, a web goes
	for (int dx = -1; dx <= 1; ++dx) {
		for (int dy = -1; dy <= 1; ++dy) {
			int sx = (ex + dx) & mask, sy = (ey + dy) & mask;
			int nade = seFind(0x89, sx, sy);
			if (nade >= 0) {
				consumeOne(nade);
				seExplode(sx, sy, false);
			}
			int web = seFind(0x11, sx, sy);
			if (web >= 0)
				_world.deleteObject(web);
		}
	}
}

// sub_19505 for a rope (0x14) and a hole in the ceiling (0x16): the destinations are the table at ds:4D4 (x, y words by quality; the rope reads it from entry 9, where ds:4F8 begins).
// APPROX(83): the ladders 0xA3 / 0xE3 / 0x103 / 0xA0 of the same routine read tables (ds:38C, 3DC) that hold other data in the file; they are not used here (Ultima VI's ladder code serves them).
void Game::seClimb(int idx) {
	static const uint16_t WHERE[19][2] = {{0xFFFF, 0xFFFF}, {396, 68}, {600, 223}, {138, 457}, {139, 509}, {368, 336}, {633, 464}, {666, 689}, {234, 61}, {353, 633},
										  {139, 11}, {218, 27}, {19, 83}, {11, 203}, {98, 83}, {235, 84}, {227, 179}, {243, 204}, {51, 243}};
	const World::Obj o = _world.objects[idx];
	int q = o.quality;
	bool rope = o.type == 0x14;
	int at = rope ? 9 + q : q;
	if (at < 0 || at >= 19)
		return;
	int nx = WHERE[at][0], ny = WHERE[at][1], nz = rope ? 1 : (q == 8 ? 2 : 0);
	if (nx == 0xFFFF)
		return;
	if (rope) { // the hole there hangs its rope (frame 1); another hole is replaced
		int hole = -1;
		std::vector<int> here;
		_world.objectsAt(nx, ny, nz, here);
		for (int i : here)
			if (!_world.objects[i].deleted && _world.objects[i].type == 0x16)
				hole = i;
		if (hole >= 0 && _world.objects[hole].quality == q) {
			_world.objects[hole].frame = 1;
		} else {
			if (hole >= 0)
				_world.deleteObject(hole);
			int n = seMake(0x16, nx, ny, nz, 0, q);
			_world.objects[n].frame = 1;
		}
	}
	message("\n");
	changeLevel(nx, ny, nz);
	advanceTime(0);
}

// sub_22EF7: the shaman invokes a totem and makes an offering (an object, 0x36 / 0x37 / 0x38), which picks one of the totem's three powers and is used up.
// The code of the ritual is the totem's (Heluzz 0, Aphazz 0x10, Motazz 0x20, the last only in combat) with the offering's 1 (0x36), 0 (0x37) or 2 (0x38) added.
void Game::seTotem(int totem) {
	int type = _world.objects[totem].type;
	int base = 0;
	message("\nInvoke: ");
	if (type == 0x56) {
		message("Heluzz!\n");
	} else if (type == 0x58) {
		message("Aphazz!\n");
		base = 0x10;
	} else if (_world.actors.globals.size() > 0x78 && _world.actors.globals[0x78] == 1) {
		message("Motazz!\n");
		base = 0x20;
	} else {
		sfx(0x21);
		message("Motazz aids only those who are in mortal combat.\n");
		return;
	}
	message("offering-- ");
	seAsk([this, base](int obj, int tx, int ty) {
		if (obj < 0) {
			message("nothing!\n");
			return;
		}
		if (!seReach(_world.objects[obj], tx, ty))
			return;
		int t = _world.objects[obj].type, code;
		if (t == 0x36)
			code = base | 1;
		else if (t == 0x37)
			code = base;
		else if (t == 0x38)
			code = base | 2;
		else {
			message("\nOffering refused.\n");
			return;
		}
		World::Obj &offering = _world.objects[obj];
		if (offering.quantity > 1 && _world.isStackable(offering.type))
			--offering.quantity;
		else
			_world.deleteObject(obj);
		seRitual(code);
	});
}

// sub_22D86: what the offering buys. Heluzz: 0 the shaman's spirit looks down at the party (APPROX(85): the picture is not drawn), 1 light for 200 minutes, 2 a sense of the danger around.
// Aphazz: 0x10 heals, 0x11 charms, 0x12 protects. Motazz: 0x20 curses, 0x21 calls a helper (not underground), 0x22 makes the party wild.
void Game::seRitual(int code) {
	message("\n");
	std::vector<uint8_t> &g = _world.actors.globals;
	switch (code) {
	case 0:
		message("\nTriolo leaves his body and looks down at the party!\n");
		break;
	case 1:
		if (g.size() > 0x12)
			g[0x12] = 0xC8;
		sfx(0x10);
		message("\nTriolo creates light.\n");
		advanceTime(0);
		break;
	case 2:
		seDanger();
		break;
	case 0x21:
		if (z != 0) {
			message("\nNo help available.\n");
			sfx(0x21);
		} else {
			seAid(actingActor());
		}
		break;
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x20:
	case 0x22:
		seGroup(code);
		break;
	default:
		break;
	}
}

// sub_227AB: Triolo senses the enemies (actors 0x10 and up on this level, awake, evil or on the attack with worktype 0x9B) within 25 squares by the compass. APPROX-free copy of the original:
// it takes absolute differences, so the directions it names are the original's (north for what is mostly above or below, east for what is mostly beside, southeast for the rest).
void Game::seDanger() {
	static const char *NAMES[8] = {"north", "northeast", "east", "southeast", "south", "southwest", "west", "northwest"};
	int count[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	for (int actor = 0x10; actor < 0x100; ++actor) {
		int k = _world.actorObject(actor);
		if (k < 0 || _world.objects[k].deleted || _world.objects[k].z != z)
			continue;
		if (_world.actors.status[actor] & 0x10)
			continue;
		if (!(_world.actors.status[actor] & 0x20) && _world.actors.worktype[actor] != 0x9B)
			continue;
		int a = std::abs((int)_world.objects[k].x - x), b = std::abs((int)_world.objects[k].y - y);
		if (a > 25 || b > 25)
			continue;
		if (b == 0)
			b = -1;
		if (a == 0)
			a = -1;
		if (std::abs(a) < (std::abs(b) >> 1))
			++count[b > 0 ? 0 : 4];
		else if (std::abs(b) < (std::abs(a) >> 1))
			++count[a > 0 ? 2 : 6];
		else if (a > 0)
			++count[b > 0 ? 3 : 1];
		else
			++count[b > 0 ? 5 : 7];
	}
	int n = 0;
	for (int c : count)
		n += c != 0;
	message("\nTriolo senses ");
	if (n == 0) {
		message("no danger.\n");
		sfx(0x21);
		return;
	}
	if (n == 8) {
		message("danger all around!\n");
		sfx(0x10);
		return;
	}
	message("danger to the ");
	bool first = true;
	for (int left = n; left > 0; --left) {
		for (int i = 0; i < 8; ++i) {
			if (!count[i])
				continue;
			if (!first)
				message(left > 1 ? ", " : " and ");
			first = false;
			message(NAMES[i]);
			count[i] = 0;
			break;
		}
	}
	message(".\n");
	sfx(0x10);
}

// sub_229F8 with sub_226E4: the people in the 11 x 9 squares round the acting member (not those with movement flag 0x10, not the parts of the big creatures) are picked by the power and
// affected; the ones picked are named ("Aphazz protects the party." for 0x12).
void Game::seGroup(int code) {
	ObjList &a = _world.actors;
	int member = actingActor();
	int me = _world.actorObject(member);
	if (me < 0)
		return;
	int mx = _world.objects[me].x, my = _world.objects[me].y;
	int mask = z == 0 ? 0x3FF : 0xFF;
	std::vector<int> around;
	for (int dx = 0; dx < 11 && around.size() < 0x3F; ++dx) {
		for (int dy = 0; dy < 9 && around.size() < 0x3F; ++dy) {
			std::vector<int> here;
			_world.objectsAt((mx + dx - 5) & mask, (my + dy - 4) & mask, z, here);
			for (int i : here) {
				const World::Obj &o = _world.objects[i];
				if (o.deleted || o.actor < 0 || around.size() >= 0x3F)
					continue;
				if (a.movementFlags[o.actor] & 0x10)
					continue;
				around.push_back(o.actor);
			}
		}
	}
	auto part = [](int type) { // sub_1B3BF: the tails of the big creatures
		int i = type - 0x11C;
		static const bool TAIL[23] = {1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1};
		return i >= 0 && i <= 0x16 && TAIL[i];
	};
	auto charmable = [](int type) { // sub_231DD
		return (type >= 0x133 && type <= 0x146) || type >= 0x150 || type == 0x12D || type == 0x12E || type == 0x12B || type == 0x123 || type == 0x11A || type == 0x124;
	};
	auto resists = [&](int target) { // sub_28662 (the intelligence of sub_2862A, less 3 when cursed or wild)
		int type = _world.objects[_world.actorObject(target)].type;
		if (type == 0x143)
			return seRoll(1, 10) == 1;
		if (type == 0x148)
			return false;
		auto intel = [&](int who) {
			int v = a.intelligence[who];
			int st = _world.objects[_world.actorObject(who)].status;
			if ((st & 0x40) || ((st & 0x80) && who != 1))
				v = std::max(1, v - 3);
			return v;
		};
		return intel(target) - intel(member) > seRoll(-12, 12);
	};
	std::vector<int> picked;
	for (int who : around) {
		int k = _world.actorObject(who);
		if (k < 0 || part(_world.objects[k].type))
			continue;
		World::Obj &ob = _world.objects[k];
		switch (code) {
		case 0x22:
			if ((a.status[who] & 0x80) && !(ob.status & 0x80)) {
				ob.status |= 0x80;
				if (who != 1)
					picked.push_back(who);
			}
			break;
		case 0x12:
			if ((a.status[who] & 0x80) && !(a.status[who] & 1))
				a.status[who] |= 1;
			break;
		case 0x20:
			if ((a.status[who] & 0x20) && !(ob.status & 0x40) && !resists(who) && charmable(ob.type)) {
				picked.push_back(who);
				ob.status |= 0x40;
			}
			break;
		case 0x10:
			if ((a.status[who] & 0x80) && !(a.status[who] & 0x10) && (seMaxHp(who) > a.hp[who] || (a.status[who] & 8)))
				picked.push_back(who);
			break;
		case 0x11:
			if ((a.status[who] & 0x20) && !(ob.status & 4) && !resists(who) && charmable(ob.type))
				picked.push_back(who);
			break;
		default:
			break;
		}
	}
	if (code == 0x12) {
		sfx(0xE);
		message("\nAphazz protects the party.\n");
		return;
	}
	const char *what = "";
	if (code == 0x22) {
		sfx(0xF);
		what = "wild!";
	} else if (code == 0x20) {
		sfx(0xC);
		what = "cursed!";
	} else if (code == 0x10) {
		sfx(0xD);
		what = "healed.";
		for (int who : picked) {
			int add = seRoll(1, 10);
			int most = seMaxHp(who);
			a.hp[who] = (uint8_t)(most > a.hp[who] + add ? a.hp[who] + add : most);
			a.status[who] &= 0xF7;
		}
	} else if (code == 0x11) {
		sfx(0xB);
		what = "charmed.";
		for (int who : picked) {
			a.movementFlags[who] = (uint8_t)((a.movementFlags[who] & 0x9F) | (a.status[who] & 0x60));
			_world.objects[_world.actorObject(who)].status |= 4;
			a.status[who] = (uint8_t)((a.status[who] & 0x9F) | (a.status[member] & 0x60));
			a.worktype[who] = 8;
		}
	}
	if (picked.empty()) {
		sfx(0x21);
		message("No effect.\n");
		return;
	}
	message("\n");
	size_t n = picked.size();
	for (size_t i = 0; i + 2 < n; ++i)
		message(_combat.nameOf(picked[i]) + ", ");
	if (n > 1)
		message(_combat.nameOf(picked[n - 2]) + " and " + _combat.nameOf(picked[n - 1]) + " are ");
	else
		message(_combat.nameOf(picked[0]) + " is ");
	message(std::string(what) + "\n");
}

// sub_2259E: Motazz sends a helper (type 0x12C, or 0x14B when the shaman is of type 0x133) to fight beside the party, once at a time: it appears within five squares, takes the member's side
// and goes when the fight is over (advance_time). APPROX(85): the creature's numbers are the engine's default for a type it has no table for, and the square is the engine's idea of a free one.
void Game::seAid(int member) {
	sfx(0xA);
	int k = _world.actorObject(member);
	int type = k >= 0 && _world.objects[k].type == 0x133 ? 0x12C : 0x14B;
	if (_seAid != 0) {
		message("Failed.\n");
		sfx(0x21);
		return;
	}
	for (int tries = 0; tries < 8; ++tries) {
		int dx = seRoll(0, 10), dy = seRoll(0, 10);
		int actor = _creatures.spawn(type, 8, x + dx - 5, y + dy - 5, z, true);
		if (actor < 0)
			continue;
		_world.actors.status[actor] = (uint8_t)((_world.actors.status[actor] & 0x9F) | (_world.actors.status[member] & 0x60));
		_world.actors.worktype[actor] = 8;
		_seAid = actor;
		message("Success!\n");
		sfx(0x1B);
		return;
	}
	message("Failed.\n");
	sfx(0x21);
}

// the table at ds:118A (sub_2E7A2): what burns up
bool Game::seBurns(int type) const {
	static const uint16_t BURNS[16] = {0x04, 0x05, 0x0D, 0x07, 0x09, 0x24, 0x27, 0x2A, 0x11, 0x2D, 0x86, 0xB7, 0xB4, 0xBF, 0xD2, 0xAB};
	for (uint16_t b : BURNS)
		if (b == type)
			return true;
	return false;
}

// sub_32B5E / sub_32A28 / sub_32C39: a fire at a square burns up what burns there (and the things in a 0xE6), a web or a nest passes it on to the webs next to it, until it is out
void Game::seBurn(int bx, int by) {
	int mask = z == 0 ? 0x3FF : 0xFF;
	std::vector<std::pair<int, int>> todo;
	todo.push_back({bx & mask, by & mask});
	for (size_t k = 0; k < todo.size() && k < 400; ++k) {
		int fx = todo[k].first, fy = todo[k].second;
		std::vector<int> here;
		_world.objectsAt(fx, fy, z, here);
		for (int i : here) {
			World::Obj &c = _world.objects[i];
			if (c.deleted || c.actor >= 0)
				continue;
			int type = c.type;
			if (!seBurns(type) && type != 0xE6)
				continue;
			if (type == 0x11 || type == 0xAB) {
				_world.deleteObject(i);
				for (int dx = -1; dx <= 1; ++dx)
					for (int dy = -1; dy <= 1; ++dy) {
						if (dx == 0 && dy == 0)
							continue;
						int nx = (fx + dx) & mask, ny = (fy + dy) & mask;
						if (seFind(0x11, nx, ny) >= 0)
							todo.push_back({nx, ny});
					}
			} else if (type == 0xE6) {
				for (size_t j = 0; j < _world.objects.size(); ++j)
					if (!_world.objects[j].deleted && _world.objects[j].location() == ObjRecord::IN_CONTAINER && _world.objects[j].parent == i)
						_world.deleteObject((int)j);
				_world.deleteObject(i);
			} else {
				_world.deleteObject(i);
			}
		}
	}
}

// sub_1ADDA: the first thing of the type that the actor holds and does not wear or wield (status 0x18 is readied), -1 if there is none
int Game::sePackItem(int actor, int type) const {
	for (size_t i = 0; i < _world.objects.size(); ++i) {
		const World::Obj &c = _world.objects[i];
		if (!c.deleted && c.type == type && carriedObject(c) && c.location() != ObjRecord::READIED && c.parent == actor)
			return (int)i;
	}
	return -1;
}

// the sums of the "On " handlers
int Game::seRoll(int lo, int hi) {
	return lo + (int)(_rng.next() % (unsigned)(hi - lo + 1));
}

// sub_285A0
int Game::seStrength(int actor) const {
	int s = _world.actors.strength[actor];
	int a = _world.actorObject(actor);
	int st = a >= 0 ? _world.objects[a].status : 0;
	if (st & 0x40) {
		s = std::max(1, s - 3);
		if (st & 0x80)
			s = std::min(30, s + 3);
	}
	return s;
}

// sub_2872A (APPROX(80): the other places that use a maximum of 30 hit points per level are Ultima VI's)
int Game::seMaxHp(int actor) const {
	int h = seStrength(actor) * 2 + _world.actors.level[actor] * 4;
	return h == 0 ? 1 : std::min(h, 255);
}

// sub_30E72
bool Game::seTooWeak(int strength) {
	return seRoll(1, 30) < (45 - strength) / 2;
}

// the first part of the "On " handlers after the object is named: a thing that is not carried must be within one square of the member
bool Game::seReach(const World::Obj &target, int tx, int ty) {
	if (carriedObject(target))
		return true;
	int a = _world.actorObject(actingActor());
	if (a >= 0 && (std::abs(tx - (int)_world.objects[a].x) > 1 || std::abs(ty - (int)_world.objects[a].y) > 1)) {
		message("Out of range!\n");
		return false;
	}
	return true;
}

// sub_2EC8A: searching a container or surface next to the member (the types of the table at ds:1122 and 0xF9; a crate must be open): what is inside lands on the square
void Game::seSearch(int idx) {
	static const uint16_t SEARCHABLE[9] = {0x3B, 0x3C, 0x102, 0xB5, 0xBC, 0xB7, 0xB6, 0xB8, 0x61};
	World::Obj &o = _world.objects[idx];
	int a = _world.actorObject(actingActor());
	if (a < 0 || o.actor >= 0 || std::abs((int)_world.objects[a].x - (int)o.x) > 1 || std::abs((int)_world.objects[a].y - (int)o.y) > 1)
		return;
	bool searchable = o.type == 0xF9;
	for (uint16_t t : SEARCHABLE)
		searchable |= t == o.type;
	if (!searchable || (o.type == 0xBC && o.frame != 0) || o.type == 0x61)
		return;
	std::vector<int> contents;
	for (size_t i = 0; i < _world.objects.size(); ++i) {
		const World::Obj &c = _world.objects[i];
		if (!c.deleted && c.location() == ObjRecord::IN_CONTAINER && c.parent == idx)
			contents.push_back((int)i);
	}
	if (contents.empty()) {
		message("\nSearching here, you find nothing.\n");
		return;
	}
	message("\nSearching here, you find ");
	for (size_t n = 0; n < contents.size(); ++n) {
		World::Obj &c = _world.objects[contents[n]];
		if (n > 0)
			message(n + 1 < contents.size() ? ", " : " and ");
		if (_world.isStackable(c.type) && c.quantity == 0)
			c.quantity = 1;
		message(Describe::object(_world, c));
		c.status = (uint8_t)(c.status & ~ObjRecord::LOCATION_MASK);
		c.parent = -1;
		c.x = o.x;
		c.y = o.y;
		c.z = o.z;
		_world.addToMap(contents[n]);
	}
	message(".\n");
}

} // namespace Ultima6
} // namespace Ultima6
