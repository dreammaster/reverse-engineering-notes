// Martian Dreams' own Get (sub_163AE; the cursor chooses the thing, as for Use) and the pieces it shares with Look and Drop. The reach is the psychic one of Use (`mdReach`), the weights are
// the game's (`World::weightOfType` for Martian Dreams), and the strange things the game lets you carry (a body searched, a lit lamp into a hand, radium that burns, a panel that may not budge)
// are done as the original does them.

#include "ultima6/describe.h"
#include "ultima6/equipment.h"
#include "ultima6/session.h"

#include <cstdlib>

namespace Ultima6 {
namespace Ultima6 {

// sub_14C7A: the thing that is meant on a square: from the top down the first that is not a person and is not to be ignored (flags3 bit 0x10 of its tile)
int Game::mdThingAt(int tx, int ty) const {
	std::vector<int> here;
	_world.objectsAt(tx, ty, z, here);
	for (size_t i = here.size(); i-- > 0;) {
		const World::Obj &o = _world.objects[here[i]];
		if (o.deleted || o.actor >= 0 || o.invisible())
			continue;
		int tile = _world.baseTile.firstTile[o.type] + o.frame;
		if (_world.flags.flags3[tile & (TileFlags::TILES - 1)] & 0x10)
			continue;
		return here[i];
	}
	return -1;
}

// An object goes into the pack of an actor (or into its hand, `location` ObjRecord::READIED): the thing itself, with its frame, quality and quantity, as sub_2D42C moves it
int Game::mdTake(int idx, int actor, int location) {
	World::Obj copy = _world.objects[idx];
	_world.deleteObject(idx);
	copy.deleted = false;
	copy.status = (uint8_t)((copy.status & ~ObjRecord::LOCATION_MASK) | location);
	copy.parent = actor;
	copy.x = copy.y = 0;
	copy.z = 0;
	_world.objects.push_back(copy);
	return (int)_world.objects.size() - 1;
}

// sub_163AE after the square has been chosen (the thing on it, the range, the weight, then what the kind of thing does)
void Game::mdGet(int tx, int ty) {
	int actor = actingActor();
	int pick = mdThingAt(tx, ty);
	auto end = [&]() { prompt(); };
	if (pick < 0 || carriedObject(_world.objects[pick])) {
		message("nothing!\n");
		end();
		return;
	}
	World::Obj &o = _world.objects[pick];
	if (_world.isStackable(o.type) && o.quantity == 0)
		o.quantity = 1;
	message(Describe::object(_world, o) + "\n");
	int dist = 1;
	int a = _world.actorObject(actor);
	if (a >= 0 && (std::abs(tx - (int)_world.objects[a].x) > 1 || std::abs(ty - (int)_world.objects[a].y) > 1)) {
		if (!mdPsychic(actor, 0) || mdReach() < mdDistance(_world.objects[a].x, _world.objects[a].y, tx, ty)) {
			message("Out of range!\n");
			sfx(5);
			end();
			return;
		}
		// APPROX(76): the "Blocked!" test of the line of sight (sub_18A0C) and the thrown picture of the thing are left out
		dist = mdDistance(_world.objects[a].x, _world.objects[a].y, tx, ty);
	}
	int type = o.type;
	int single = _world.weightOf(pick);
	int total = type == 0x155 ? single : single + _world.contentsWeight(pick);
	static const uint16_t TENTH[18] = {0x39, 0x3A, 0x3B, 0x3C, 0x3F, 0x40, 0x5B, 0x5D, 0x5C, 0x6B, 0x83, 0x49, 0x4A, 0x4B, 0x4D, 0x14B, 0x18, 0x84};
	bool tenth = false;
	for (uint16_t t : TENTH)
		tenth |= t == type;
	if ((total == 0 && !tenth) || single >= 10000) {
		message("That is not possible.\n");
		sfx(5);
		end();
		return;
	}
	if (_world.actors.strength[actor] * 20 < total + _world.carriedWeight(actor)) {
		message("\nThe total is too heavy.\n");
		sfx(5);
		end();
		return;
	}
	if (dist > 1)
		sfx(0x32);
	_world.spendMoves(actor, 3);
	// radium (0x1C0, 0x1C1) is hot: without telekinesis, tongs (0x88) in a hand, or being the mechanical man it poisons
	if (type == 0x1C0 || type == 0x1C1) {
		Equipment::Slots slots = Equipment::of(_world, actor);
		bool tongs = false;
		for (int h : {2, 5})
			tongs |= slots.object[h] >= 0 && _world.objects[slots.object[h]].type == 0x88;
		if (!mdPsychic(actor, 0) && actor != 6 && !tongs) {
			_world.actors.status[actor] |= 8;
			message("OUCH. It is very hot.\n");
		}
	}
	if (_world.isStackable(type)) { // a stack joins what the member has of it
		int frame = o.frame;
		int got = _world.addToInventory(actor, type, o.quality, _world.quantityOf(o));
		_world.objects[got].frame = (uint8_t)frame;
		_world.deleteObject(pick);
	} else if (type == 0x155) { // a body: what it holds is laid on its square first
		int bx = o.x, by = o.y, bz = o.z;
		std::string found;
		int count = 0;
		std::vector<int> inside;
		for (size_t i = 0; i < _world.objects.size(); ++i)
			if (!_world.objects[i].deleted && _world.objects[i].location() == ObjRecord::IN_CONTAINER && _world.objects[i].parent == pick)
				inside.push_back((int)i);
		message("\nSearching the body, you find ");
		for (size_t k = 0; k < inside.size(); ++k) {
			int i = inside[k];
			if (_world.isStackable(_world.objects[i].type) && _world.objects[i].quantity == 0)
				_world.objects[i].quantity = 1;
			if (count)
				message(k + 1 == inside.size() ? " and " : ", ");
			mdPlace(i, bx, by, bz);
			message(Describe::object(_world, _world.objects[i]));
			++count;
		}
		message(count ? ".\n" : "nothing!\n");
		mdTake(pick, actor, ObjRecord::IN_INVENTORY);
	} else if (type == 0x19B || type == 0x1CA || type == 0x13A || type == 0xCE) { // a thing that is loose (an odd quality) can be taken
		if (o.quality & 1) {
			mdTake(pick, actor, ObjRecord::IN_INVENTORY);
		} else {
			message("Won't budge !\n");
			sfx(0);
		}
	} else if (type == 0x6E || type == 0x70 || type == 0x72 || type == 0x74 || type == 0x76) { // a lit lamp goes into a hand
		Equipment::Slots slots = Equipment::of(_world, actor);
		if (slots.object[2] >= 0 && slots.object[5] >= 0) {
			message("\nYour hands are full.\n");
			end();
			return;
		}
		int got = mdTake(pick, actor, ObjRecord::READIED);
		if (type == 0x6E)
			_world.objects[got].quality = 0xB4;
	} else {
		mdTake(pick, actor, ObjRecord::IN_INVENTORY);
		if (type == 0x100)
			message("The ice is melting.\n");
	}
	end();
}

} // namespace Ultima6
} // namespace Ultima6
