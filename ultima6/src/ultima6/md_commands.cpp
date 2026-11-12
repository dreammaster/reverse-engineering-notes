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

// sub_55F77 (Talk, mode 1) after the cursor has chosen a square: the top thing there. A thing (not a person) is read by psychometry: that needs the second psychic power (the high half of the first
// number, `mdPsychic(actor, 1)`), the thing's status bit 1, and a type of the table at cs:0871: some tell a history, some call a voice (the ghosts 0x72, 0x73, 0x74) that is then talked to.
// A person who is only for the telepaths (status bit 1) is "nothing!" to the others; to oneself "You aren't yet that insane."; a child (type 0x187) is scolded; anybody else is named and the
// conversation begins.
// APPROX(76): what the original does when the conversation ends (word_41F2A, the ten codes of cs:0849: the scenes that move the party, make things at fixed places, kill actor 0x68 ..) is not done
void Game::mdTalk(int tx, int ty) {
	std::vector<int> here;
	_world.objectsAt(tx, ty, z, here);
	int pick = -1;
	for (size_t i = here.size(); i-- > 0 && pick < 0;)
		if (!_world.objects[here[i]].deleted && !_world.objects[here[i]].invisible())
			pick = here[i];
	auto nothing = [&]() {
		message("nothing!\n");
		prompt();
	};
	if (pick < 0) {
		nothing();
		return;
	}
	int me = actingActor();
	int who = _world.objects[pick].actor;
	if (who < 0) { // a thing: psychometry
		const World::Obj &o = _world.objects[pick];
		if (mdPsychic(me, 1) == 0) {
			nothing();
			return;
		}
		if (!(o.status & 1)) {
			message("You receive no psychic impressions.\n");
			prompt();
			return;
		}
		static const struct {
			uint16_t type;
			int ghost;
			const char *text;
		} TABLE[19] = {
			{0xBC, 0x72, nullptr}, {0xBF, 0x72, nullptr}, {0xC0, 0x72, nullptr}, {0xC8, 0x73, nullptr}, {0xC9, 0x73, nullptr}, {0xD6, 0x73, nullptr}, {0xD7, 0x73, nullptr},
			{0x64, 0x74, nullptr}, {0x120, 0x74, nullptr}, {0x121, 0x74, nullptr}, {0x122, 0x74, nullptr},
			{0xF7, 0, "This saw was used by a Martian doctor, or \"Arborist\", for surgery.\n"},
			{0xF9, 0, "This device was used by a Martian doctor, or \"Arborist\", on patients.\n"},
			{0x5C, 0, "This was worn last by a dying man.\n"},
			{0x0C, 0, "You receive a momentary impression of a house falling on you.\n"},
			{0xE7, 0, "This was used by a Martian whose entire life focussed on measuring time.\n"},
			{0xEA, 0, "This jewelry was worn by a distinguished Martian, a mayor or \"Agrarian\".\n"},
			{0xF6, 0, "These footbags were worn by an elderly, distinguished Martian whose job, \"Gatherer\", has no human analog - a combination of undertaker, dustman, and treasurer.\n"},
			{0x113, 0, "This sculpture had great ritual or religious significance to a city, or \"Grove\", of Martians.  The image is of \"very proud soil\".\n"}};
		for (const auto &row : TABLE) {
			if (row.type != o.type)
				continue;
			if (row.text) {
				message(row.text);
				prompt();
				return;
			}
			who = row.ghost;
			break;
		}
		if (who < 0) {
			nothing();
			return;
		}
	}
	int wk = _world.actorObject(who);
	if (wk < 0 || ((_world.objects[wk].status & 1) && mdPsychic(me, 1) == 0)) {
		nothing();
		return;
	}
	if (who <= 1) { // oneself
		message("yourself\n");
		message(_solo >= 0 ? "Not while in solo mode!\n" : "You aren't yet that insane.\n");
		prompt();
		return;
	}
	std::string name = Describe::object(_world, _world.objects[wk]);
	if (_world.objects[wk].type == 0x187) { // a child
		message(name + "\n");
		message(std::string("No backtalk from you, young ") + (_world.actors.globals.size() > 0x38 && _world.actors.globals[0x38] ? "wo" : "") + "man!\n");
		prompt();
		return;
	}
	message(name + "\n");
	startTalk(who);
}

} // namespace Ultima6
} // namespace Ultima6
