#include "ultima6/describe.h"
#include "ultima6/session.h"

#include <algorithm>
#include <cstdlib>

namespace Ultima6 {
namespace Ultima6 {

#include "ultima6/md_use_tables.inc"

namespace {

const int NONE = 0xFFFF, ANYTHING = 0xFFFE;

// sub_35D73: whether a partner of the given type is taken by a list of types
bool listTakes(const uint16_t *list, int type) {
	if (list[0] == ANYTHING)
		return true;
	if (list[0] == NONE && type == NONE)
		return true;
	for (int i = 0; i < 9 && list[i] != NONE; ++i)
		if (list[i] == type)
			return true;
	return false;
}

// sub_35E47: how specific a list is by its first entry
int specificity(int first) {
	return first == ANYTHING ? 0 : first == NONE ? 1 : 2;
}

} // namespace

// the recipes an object takes part in (sub_35C31)
std::vector<int> Game::mdRecipesOf(int type) const {
	std::vector<int> out;
	for (const auto &rec : MD_USE_OBJECTS) {
		if (rec.type != type)
			continue;
		for (int i = 0; i < rec.count && out.size() < 10; ++i)
			out.push_back(rec.recipes[i]);
	}
	std::vector<int> unique; // duplicates go (sub_35BFA)
	for (int r : out)
		if (std::find(unique.begin(), unique.end(), r) == unique.end())
			unique.push_back(r);
	return unique;
}

bool Game::mdUsable(int type) const {
	for (const auto &rec : MD_USE_OBJECTS)
		if (rec.type == type)
			return true;
	return false;
}

// sub_35F0D after the object is chosen: the recipes of the object; if none take a partner the best one is carried out; else "On - " asks for the first partner (and "To -" for a
// second), the recipes that do not take the partner are dropped, and the one that is most particular about its partners is carried out with (item, first, second).
// APPROX(76): only some of the 75 recipe actions are ported (mdAction); the others do nothing. The reach of the objects' Use (a table at ds:16B0 / dword_44966) is taken as one square
void Game::mdUseObject(int idx) {
	_mdItem = idx;
	_mdAsync = false;
	_mdRecipes = mdRecipesOf(_world.objects[idx].type);
	_mdFirst = _mdSecond = -1;
	if (_mdRecipes.empty()) {
		message(Describe::object(_world, _world.objects[idx]) + " can't be used.\n");
		return;
	}
	mdContinue(1, -1);
}

// the stages of Use: `stage` 1 = the first partner is wanted, 2 = the second; `partner` is the object chosen (-2 given up) when called again
void Game::mdContinue(int stage, int partner) {
	auto needs = [&](int which) {
		for (int r : _mdRecipes)
			if ((which == 1 ? MD_USE_RECIPES[r].first[0] : MD_USE_RECIPES[r].second[0]) != NONE)
				return true;
		return false;
	};
	auto filter = [&](int which, int type) {
		std::vector<int> kept;
		for (int r : _mdRecipes)
			if (listTakes(which == 1 ? MD_USE_RECIPES[r].first : MD_USE_RECIPES[r].second, type))
				kept.push_back(r);
		_mdRecipes = kept;
		return !kept.empty();
	};
	const World::Obj &item = _world.objects[_mdItem];
	if (stage == 1 && partner == -1) {
		if (needs(1)) {
			message("On - ");
			_mdStage = 1;
			_pending = 'Z';
			_cursor = true;
			_cx = _cy = 5;
			return;
		}
	} else if (partner == -2) {
		if (_mdAsync)
			prompt();
		return;
	} else if (stage == 1) {
		_mdFirst = partner;
		int type = _world.objects[partner].type;
		if (!filter(1, type)) {
			message("\n" + Describe::object(_world, item) + " has no effect on " + Describe::object(_world, _world.objects[partner]) + ".\n");
			if (_mdAsync)
			if (_mdAsync)
				prompt();
			return;
		}
		if (needs(2)) {
			message("To -\n");
			_mdStage = 2;
			_pending = 'Z';
			_cursor = true;
			_cx = _cy = 5;
			return;
		}
	} else {
		_mdSecond = partner;
		int type = _world.objects[partner].type;
		if (!filter(2, type)) {
			message("\n" + Describe::object(_world, item) + " on " + Describe::object(_world, _world.objects[_mdFirst]) + " to " + Describe::object(_world, _world.objects[partner]) +
					" has no effect.\n");
			if (_mdAsync)
			if (_mdAsync)
				prompt();
			return;
		}
	}
	// sub_35E6D: the recipe with the most particular partners (the last of equals)
	int best = -1, bestScore = -1;
	for (int r : _mdRecipes) {
		int score = specificity(MD_USE_RECIPES[r].first[0]) + specificity(MD_USE_RECIPES[r].second[0]);
		if (score >= bestScore) {
			bestScore = score;
			best = r;
		}
	}
	message("\n");
	if (best >= 0)
		mdAction(best, _mdItem, _mdFirst, _mdSecond);
	_world.spendMoves(actingActor(), 5);
	if (_mdAsync)
		prompt();
}

// The object the cursor chose: the one on top of the square (a person too), -2 when nothing is there.
void Game::mdChosen(int tx, int ty) {
	_mdAsync = true; // the command goes on after a key: this one prints the prompt
	std::vector<int> here;
	_world.objectsAt(tx, ty, z, here);
	int pick = -1;
	for (size_t i = here.size(); i-- > 0;)
		if (!_world.objects[here[i]].invisible()) {
			pick = here[i];
			break;
		}
	if (pick < 0) {
		message("nothing!\n");
		mdContinue(_mdStage, -2);
		return;
	}
	message(Describe::object(_world, _world.objects[pick]) + ".\n");
	mdContinue(_mdStage, pick);
}

// sub_5D62E: a source of fire (arg_4: a match, a lit thing) lights a lamp, candle or torch (arg_2). It says "(using ...)", a lamp without fuel (quality 0) takes what an oil lamp (0x73) or a
// lantern (0x75) can have from a can of lamp oil (0x7C) of the member ("There is no fuel." else) and a fuel number (0xFF for an oil lamp, 0x64 for a 0x71, 0xB4 for the rest); the thing
// turns into its lit kind (0x6D, 0x6F, 0x71, 0x73, 0x75 -> one more), and a match is used up. APPROX(76): the torch stack handling and "Your hands are full." of the carried torch are left out
void Game::mdLight(int lighter, int target) {
	World::Obj &t = _world.objects[target];
	World::Obj &f = _world.objects[lighter];
	message(std::string("(using ") + (f.type == 0x6B ? "a match" : Describe::object(_world, f)) + ")");
	static const int UNLIT[5] = {0x6D, 0x6F, 0x71, 0x73, 0x75};
	bool lightable = false;
	for (int u : UNLIT)
		lightable |= t.type == u;
	if (!lightable)
		return;
	if (t.quality == 0) {
		if (t.type == 0x73 || t.type == 0x75) {
			int can = findInInventory(0x7C);
			if (can < 0) {
				message("\nThere is no fuel.\n");
				return;
			}
			if (_world.quantityOf(_world.objects[can]) > 1)
				_world.setQuantity(_world.objects[can], _world.quantityOf(_world.objects[can]) - 1);
			else
				_world.deleteObject(can);
			t.quality = t.type == 0x73 ? 0xFF : 0xB4;
		} else {
			t.quality = t.type == 0x71 ? 0x64 : 0xB4;
		}
	}
	t.type = (uint16_t)(t.type + 1);
	if (f.type == 0x6B) {
		if (_world.quantityOf(f) > 1)
			_world.setQuantity(f, _world.quantityOf(f) - 1);
		else
			_world.deleteObject(lighter);
	}
}

// The actions of the recipes (the far pointers of seg060 +0x20, resolved through the overlay thunks to sub_598F6 ...), with (item, first partner, second partner).
void Game::mdAction(int recipe, int item, int first, int second) {
	(void)second;
	World::Obj &o = _world.objects[item];
	switch (recipe) {
	case 44: { // sub_5EFC8: a door (or hatch) opens and shuts: locked (quality bit 0x80) it is not; somebody in it blocks it; else frame bit 2 turns over
		if (o.quality & 0x80) {
			message("It is locked.\n");
			break;
		}
		std::vector<int> there;
		_world.objectsAt(o.x, o.y, o.z, there);
		bool blocked = false;
		for (int i : there)
			blocked |= _world.objects[i].actor >= 0;
		if (blocked) {
			message("Blocked!\n");
			break;
		}
		o.frame ^= 2;
		break;
	}
	case 34: { // sub_5EF0C: a container: frame 2 is shut (a wooden crate 0x56 is locked, a 0x11C is nailed shut), frame 1 closed opens and lets its contents spill out
		       // (sub_14D04), frame 0 open closes. APPROX(76): which of the two texts goes with which type was read from a listing without its labels
		if (o.frame == 2) {
			if (o.type == 0x56)
				message("It is locked.\n");
			else if (o.type == 0x11C)
				message("It is nailed shut.\n");
		} else if (o.frame == 1) {
			o.frame = 0;
			searchHere(item);
		} else {
			o.frame = 1;
		}
		break;
	}
	case 28: { // sub_5D8CA: an unlit lamp, candle or torch by itself: lit with a tinder box (0x6C) or else a match (0x6B) of the member
		int lighter = findInInventory(0x6C);
		if (lighter < 0)
			lighter = findInInventory(0x6B);
		if (lighter < 0) {
			message("\nNothing to light the " + Describe::object(_world, o) + " with.\n");
			break;
		}
		mdLight(lighter, item);
		break;
	}
	case 29: // sub_5D62E: a source of fire on a lamp, candle or torch
		if (first >= 0)
			mdLight(item, first);
		break;
	case 17:
	case 22: { // sub_59E49: a tool on a crate or chest (first): a nailed one (frame 2) is pried open (frame 1); a shut one (frame 1) is nailed shut again by a 0x111; else no effect
		if (first < 0)
			break;
		World::Obj &c = _world.objects[first];
		if (c.frame == 2) {
			c.frame = 1;
			message("You pried the nails out of the crate.\n");
		} else if (c.frame == 1 && o.type == 0x111) {
			c.frame = 2;
			message("You nail the crate shut.\n");
		} else {
			message("It has no effect.\n");
		}
		break;
	}
	case 65: { // sub_59FD7: a heart stone in a metal woman (first), once; the stone is used up
		if (first < 0)
			break;
		World::Obj &m = _world.objects[first];
		if (m.quality & 1) {
			message("The robot already has a heartstone.\n");
		} else {
			m.quality = 1;
			_world.deleteObject(item);
			message("The heartstone is installed.\n");
		}
		break;
	}
	case 70: { // sub_5EE03: a sliding gate: locked (quality bit 0x80) it will not move; the frames 0 and 1 go to 3 (1 a square west), 2 and 3 to 1 (a square east), 4 and 5 to 7, 6 and 7 to 5.
		       // APPROX(76): sub_2CFE2 is taken to be a plain move
		if (o.quality & 0x80) {
			message("It is apparently locked.\n");
			break;
		}
		int mask = z == 0 ? 0x3FF : 0xFF;
		switch (o.frame) {
		case 0:
			o.frame = 3;
			break;
		case 1:
			o.frame = 3;
			_world.moveObject(item, (o.x - 1) & mask, o.y, o.z);
			break;
		case 2:
		case 3:
			o.frame = 1;
			_world.moveObject(item, (o.x + 1) & mask, o.y, o.z);
			break;
		case 4:
		case 5:
			o.frame = 7;
			break;
		default:
			o.frame = 5;
			break;
		}
		break;
	}
	case 33: { // sub_5C8D9: the sextant, on the surface: degrees of longitude (x - 512) * 360 / 1024 (W, 0 and 180 blank, E past it) and of latitude (y - 512) * 240 / 1024 (S, blank at 0, N)
		if (z != 0) {
			message("Not while underground.\n");
			break;
		}
		int k = _world.actorObject(actingActor());
		int px = _world.objects[k].x, py = _world.objects[k].y;
		int lon = (px - 512) * 360 / 1024, lat = (py - 512) * 240 / 1024;
		char ns = lat > 0 ? 'S' : lat == 0 ? ' ' : 'N', ew = 'W';
		if (lon == 180 || lon == -180 || lon == 0)
			ew = ' ';
		else if (lon < 0)
			ew = 'E';
		lon = 180 - std::abs(lon);
		lat = std::abs(lat);
		message("You are somewhere near " + std::string(1, 10) + "Lat:" + std::to_string(lat) + " " + std::string(1, ns) + std::string(1, 10) + "Long:" + std::to_string(lon) + " " + std::string(1, ew) + std::string(1, 10));
		break;
	}
	case 66: // sub_5CE18: the spittoon, by what the member can still spit (the status flag 0x40 of the person)
		message((_world.actors.status[actingActor()] & 0x40) ? "Your mouth is too dry.\n" : "You spit into the spittoon.\n");
		break;
	default:
		break; // APPROX(76)
	}
}

} // namespace Ultima6
} // namespace Ultima6
