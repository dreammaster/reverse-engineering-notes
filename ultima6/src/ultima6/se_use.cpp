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
		(void)carried;
		if (o.quality > 0) {
			--o.quality;
			message("\nYou take a picture.\n");
		} else {
			message("\nThe camera's out of film, ");
			message("but the flash still works.\n");
		}
		// APPROX(79): the flash itself (sub_34DD4: the picture at the member's square and the whisper of the ghost, actor 0x19, "Closer...", the start of its conversation) is not done
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
	case 0x362F7: // a rope: only in party mode (else "Not in solo mode!"), then the descent (sub_19505, not done)
		if (_solo != -1)
			message("Not in solo mode!\n");
		break;
	case 0x36312: // a hole in the ceiling: frame 1 with the party together is the way down (sub_19505, not done), frame 0 needs a rope from above
		if (_solo == -1 && o.frame == 1) {
			// APPROX(79): the descent into the underground city (sub_19505)
		} else if (o.frame == 0) {
			message("\nYou need a rope dropped from above.\n");
		} else {
			message("Not in solo mode!\n");
		}
		break;
	case 0x36206: // a brazier with frame bit 2: "Not possible."; else the fire routine (sub_3215F, not done)
		if (o.frame & 2)
			message("Not possible.\n");
		break;
	case 0x3623D: // burnt logs: frame 0 "Not possible.", 1 starts a fire (frame 2), 2 the fire routine (sub_3215F, not done)
		if (o.frame == 0) {
			message("Not possible.\n");
		} else if (o.frame == 1) {
			o.frame = 2;
			message("\nYou started a fire.\n");
		}
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
		if (o.frame == 1)
			break; // APPROX(79): pouring tar (sub_32D11: tarred cloth strips, pteranodon nests, webs) is not ported
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
	default: // APPROX(79): not ported yet
		break;
	}
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
