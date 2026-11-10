#include "ultima6/describe.h"
#include "ultima6/session.h"

#include <algorithm>
#include <cstdio>
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
		bool ground = partner == -3; // a square without an object: its type is not any of the recipes' (the original reads a stray word)
		_mdFirst = ground ? -1 : partner;
		int type = ground ? 0 : _world.objects[partner].type;
		if (!filter(1, type)) {
			message("\n" + Describe::object(_world, item) + " has no effect on " + (ground ? mdGroundName(_mdSqX[1], _mdSqY[1]) : Describe::object(_world, _world.objects[partner])) + ".\n");
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
		bool ground = partner == -3;
		_mdSecond = ground ? -1 : partner;
		int type = ground ? 0 : _world.objects[partner].type;
		if (!filter(2, type)) {
			message("\n" + Describe::object(_world, item) + " on " + (_mdFirst >= 0 ? Describe::object(_world, _world.objects[_mdFirst]) : mdGroundName(_mdSqX[1], _mdSqY[1])) + " to " +
					(ground ? mdGroundName(_mdSqX[2], _mdSqY[2]) : Describe::object(_world, _world.objects[partner])) + " has no effect.\n");
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
	if (_keyHook || _pending == 'Z') // (the action waits for a key or a location: mdFinish / mdChosen end the command)
		return;
	_world.spendMoves(actingActor(), 5);
	if (_mdAsync)
		prompt();
}

// a recipe that waits for keys: the hook gets each one; the command ends when it says so
void Game::mdAskKey(std::function<bool(int)> hook) {
	_mdAsync = true;
	_keyHook = hook;
}

void Game::mdFinish() {
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
	_mdSqX[_mdStage] = tx;
	_mdSqY[_mdStage] = ty;
	if (_mdStage == 3) { // the location of a seed
		mdPlant(tx, ty);
		return;
	}
	if (pick < 0) { // the ground (sub_35972 returns -1 and leaves the square): its name, and it must be within reach
		int a = _world.actorObject(actingActor());
		message(mdGroundName(tx, ty) + ".\n");
		if (a >= 0 && (std::abs(tx - (int)_world.objects[a].x) > 1 || std::abs(ty - (int)_world.objects[a].y) > 1)) {
			message("Out of range!\n");
			sfx(5);
			mdContinue(_mdStage, -2);
			return;
		}
		mdContinue(_mdStage, -3);
		return;
	}
	message(Describe::object(_world, _world.objects[pick]) + ".\n");
	mdContinue(_mdStage, pick);
}

// sub_2CFE2: an object is put on a square of the map, from wherever it was (a pack, a container) or from another square
void Game::mdPlace(int idx, int x, int y, int zz) {
	World::Obj &o = _world.objects[idx];
	if (!o.onMap()) {
		o.status = (uint8_t)(o.status & ~ObjRecord::LOCATION_MASK);
		o.parent = -1;
		o.x = (uint16_t)x;
		o.y = (uint16_t)y;
		o.z = (uint8_t)zz;
		_world.addToMap(idx);
	} else {
		_world.moveObject(idx, x, y, zz);
	}
}

// sub_5CC45 after the location: the ground must be of a class that can be dug (sub_3231F), then the seed (one of a stack) lies there
void Game::mdPlant(int tx, int ty) {
	int mask = z == 0 ? 0x3FF : 0xFF;
	int tile = _world.map.tile(z, tx & mask, ty & mask);
	bool ground = (tile >= 0x20 && tile <= 0x24) || tile == 0x0C || tile == 0x0D || (tile >= 0x28 && tile <= 0x3F) || tile == 0x7C || tile == 0x7D || tile == 0x7F;
	if (!ground) {
		message("Won't grow there!\n");
		sfx(5);
		mdFinish();
		return;
	}
	World::Obj &seed = _world.objects[_mdItem];
	if (seed.quantity > 1) {
		--seed.quantity;
		seMake(0x9E, tx, ty, z, 1);
	} else {
		mdPlace(_mdItem, tx, ty, z);
	}
	message("Now it just needs watering.\n");
	mdFinish();
}

// the terrain of a square with its article (sub_2F774 of the tile sub_263B2 finds)
std::string Game::mdGroundName(int tx, int ty) const {
	int mask = z == 0 ? 0x3FF : 0xFF;
	int tile = _world.map.tile(z, tx & mask, ty & mask);
	return Describe::article(_world, tile) + Describe::tileName(_world, tile, false);
}

// sub_5A35B: a hole (0x101) on the square lets go of what is in it
void Game::mdSpill(int x, int y) {
	int hole = seFind(0x101, x, y);
	if (hole >= 0)
		searchHere(hole);
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

// MD's look (sub_14ED4 15119-157F5) after the name and the full stop: a lamp, candle or torch says how much fuel or wick is left, a growing plant how it grows, a ship's part whether it is
// loose or not installed, a robot's heart stone, a clock the time. APPROX(76): the other kinds (the loads of a gun, the controls' purposes, the piles) are not ported
std::string Game::mdLookExtra(const World::Obj &o) const {
	static const int LIT[5] = {0x6E, 0x70, 0x72, 0x74, 0x76}, UNLIT[5] = {0x6D, 0x6F, 0x71, 0x73, 0x75};
	int t = o.type;
	bool lamp = false;
	for (int i = 0; i < 5; ++i)
		lamp |= t == LIT[i] || t == UNLIT[i];
	std::string nl(1, 10);
	if (lamp) {
		if ((t == 0x6D || t == 0x6E) && _world.quantityOf(o) > 1)
			return "";
		bool oil = t == 0x73 || t == 0x75 || t == 0x74 || t == 0x76;
		int q = o.quality;
		std::string out;
		if (q > 0x1E)
			out = nl + "Plenty of ";
		else if (q > 6)
			out = nl + "It has " + std::to_string(q) + " more minutes of ";
		else if (q > 0)
			out = nl + "It's almost out of " + nl;
		else
			out = nl + (oil ? "No " : "Plenty of ");
		return out + (oil ? "fuel." : "wick.") + nl;
	}
	if (t == 0xFB && o.frame <= 3)
		return o.quality == 0x0F ? "It is growing." + nl : o.quality == 0x10 ? "It is ripe." + nl : "It is not growing." + nl;
	if (t == 0x19B)
		return o.quality == 1 ? nl + "It seems to be loose." + nl : o.quality == 2 ? nl + "It is not installed." + nl : "";
	if (t == 0x1CA) { // controls: what they are for, by their quantity (4 to 7), else by the frame
		switch (o.quantity) {
		case 4:
			return "It appears to control a dream machine." + nl;
		case 5:
			return "It appears to control a spray system." + nl;
		case 6:
			return "It appears to control the ruby lens system." + nl;
		case 7:
			return "It appears to control the cistern valves." + nl;
		case 0:
			return o.frame == 0 ? "These appear to be mechanical controls." + nl : o.frame == 1 ? "These appear to be valve controls." + nl : o.frame == 2 ? "These appear to be electrical controls." + nl : "";
		default:
			return "You cannot decipher its purpose." + nl;
		}
	}
	if (t == 0x19A || t == 0x10C) { // a wheelbarrow or a rail car: empty, or so many loads (the quantity) of what the quality names
		if (o.quantity == 0)
			return "It is empty." + nl;
		static const char *const LOAD[4] = {"dirt.", "rock.", "iron ore.", "coal."};
		std::string out = "It has " + std::to_string(o.quantity) + " load" + (o.quantity == 1 ? "" : "s") + " of ";
		if (o.quality >= 1 && o.quality <= 4)
			out += std::string(LOAD[o.quality - 1]) + nl;
		return out;
	}
	if (t == 0x14D) // the cannon, when it is loaded (quality)
		return o.quality ? "The cannon will fire steel cannon balls." + nl : "";
	if (t == 0x13A) { // a panel: loose (quality bit 1, when it is not carried) and broken (bit 2)
		std::string out;
		if ((o.quality & 1) && !carriedObject(o))
			out += "It appears to be loose." + nl;
		if (o.quality & 2)
			out += "It appears to be broken." + nl;
		return out;
	}
	if (t == 0x1C9) { // the remote controls
		static const char *const TEXT[7] = {"a mechanical system", "a valve system", "an electrical system", "this dream machine", "this spray system", "the ruby lens system", "the cistern valves"};
		if (o.quality >= 1 && o.quality <= 7)
			return std::string("It appears to control ") + TEXT[o.quality - 1] + "." + nl;
		return "You cannot decipher what it controls." + nl;
	}
	if (t == 0xB3 || t == 0xE3) { // doors: locked (quality bit 0x80), rusted shut when the bits 0xC0 are set
		if (!(o.quality & 0x80))
			return "";
		return (o.quality & 0x7F) >= 0x40 ? "It is apparently rusted shut." + nl : "It is apparently locked." + nl;
	}
	if (t == 0x11F)
		return o.quality ? "It has a heartstone." + nl : "";
	if (t == 0x5B || t == 0x62) {
		int h = gameClock.hour, m = gameClock.minute;
		char buf[32];
		if (h == 0)
			std::snprintf(buf, sizeof buf, "12:%02d A.M.", m);
		else if (h < 12)
			std::snprintf(buf, sizeof buf, "%d:%02d A.M.", h, m);
		else if (h == 12)
			std::snprintf(buf, sizeof buf, "12:%02d P.M.", m);
		else
			std::snprintf(buf, sizeof buf, "%d:%02d P.M.", h - 12, m);
		return nl + nl + "The time is " + buf + nl;
	}
	return "";
}

// The actions of the recipes (the far pointers of seg060 +0x20, resolved through the overlay thunks to sub_598F6 ...), with (item, first partner, second partner).
void Game::mdAction(int recipe, int item, int first, int second) {
	(void)second;
	World::Obj &o = _world.objects[item];
	switch (recipe) {
	case 3:
	case 57: { // sub_5F166: an instrument (or the hammer-like 0xE7 on one: the instrument is then the first partner) is played
		int inst = o.type == 0xE7 ? first : item;
		if (inst < 0)
			break;
		int t = _world.objects[inst].type;
		switch (t) {
		case 0x11A:
			message("Thump! Thump!\n");
			sfx(0x34);
			sfx(0x34);
			break;
		case 0x118:
			message("Ching!\n");
			sfx(0x36);
			break;
		case 0x119:
			message("Shing!\n");
			sfx(0x35);
			break;
		case 0x11B:
			message("Wheeeeze\n");
			sfx(0x37);
			break;
		case 0x12A:
			message("GONG!!!!\n");
			sfx(0xF);
			break;
		default:
			message("You play the " + Describe::object(_world, _world.objects[inst]) + ".\n");
			break;
		}
		break;
	}
	case 4: { // sub_59964: a switch bar (first): a loose one (quality 1) is fastened if its base (0x19D, up and to the left) is there, else it is made loose
		if (first < 0)
			break;
		World::Obj &sw = _world.objects[first];
		if (sw.quality == 1) {
			int base = seFind(0x19D, sw.x - 1, sw.y - 1);
			if (base < 0) {
				message("This switch cannot be fixed.\n");
				sfx(5);
				break;
			}
			sw.frame = _world.objects[base].frame;
			sw.quality = 0;
			sfx(0x1F);
			message("The switch is fastened.\n");
		} else {
			sw.quality = 1;
			sfx(0x1F);
			message("The switch is loose.\n");
		}
		break;
	}
	case 5: { // sub_59A5E: a drill (first) is installed onto the drill cart (0x1B7) on its square: both go and a drill cart with a drill (0x1B9, frame 1) takes their place
		if (first < 0)
			break;
		int dx = _world.objects[first].x, dy = _world.objects[first].y;
		int cart = seFind(0x1B7, dx, dy);
		if (cart < 0) {
			message("The drill must be installed onto a drill cart.\n");
			break;
		}
		int n = seMake(0x1B9, dx, dy, z, 1);
		_world.objects[n].frame = 1;
		_world.deleteObject(first);
		_world.deleteObject(cart);
		sfx(0x1F);
		message("The drill's power is connected.\n");
		break;
	}
	case 6: { // sub_59B41: a control panel (first), not carried: loose (quality 0 -> 1); installed onto the cabinet (0x1C9) on its square if the cabinet wants this kind; "It must be repaired first." with quality bit 2
		if (first < 0)
			break;
		World::Obj &pn = _world.objects[first];
		if (carriedObject(pn)) {
			message("It has no effect.\n");
			break;
		}
		int q = pn.quality, kind = pn.quantity;
		if (q == 0) {
			pn.quality = 1;
			sfx(0x1F);
			message("The panel is loose.\n");
			break;
		}
		if (q & 2) {
			message("It must be repaired first.\n");
			sfx(5);
			break;
		}
		int cab = seFind(0x1C9, pn.x, pn.y);
		if (cab < 0) {
			message("Panels are only installed onto cabinets.\n");
			sfx(5);
			break;
		}
		int cq = _world.objects[cab].quality;
		bool fits;
		if (cq == 0)
			fits = kind == 0;
		else if (cq <= 3 && kind == 0)
			fits = pn.frame == cq - 1;
		else
			fits = cq == kind;
		if (!fits) {
			message("This cabinet requires a different type of panel.\n");
			sfx(5);
			break;
		}
		pn.quality = (uint8_t)(pn.quality - 1);
		sfx(0x1F);
		message("The panel is fastened.\n");
		if (pn.quality == 4) { // (the original tests the quality it has just left)
			_world.actors.talkFlags[0x74] |= 8;
			if (_world.actors.talkFlags[0x74] & 1)
				_world.actors.talkFlags[0x60] |= 4;
		}
		break;
	}
	case 7: // sub_59CC9: a sledge hammer (item) on a track: the first partner is used up and the second becomes the mended rail (0x19E)
		sfx(0x20);
		message("Which section of rail needs fixing -\n");
		if (first >= 0)
			_world.deleteObject(first);
		if (second >= 0) {
			_world.objects[second].type = 0x19E;
		}
		break;
	case 9:
	case 64: { // sub_5A27E: what the thing says when it is used by itself, by its type
		static const struct {
			int type;
			const char *text;
		} SAYS[] = {{0x41, "You can't dig any deeper.\n"}, {0x42, "You can't dig any deeper.\n"}, {0x43, "You can't dig any deeper.\n"}, {0x4D, "The berries are not edible.\n"}, {0x4E, "The berries are not edible.\n"},
					{0x83, "You will chew it instinctively.\n"}, {0xB8, "The plate in the camera has already been exposed.\n"}, {0xC7, "You need to use pliers to attach the cable.\n"},
					{0x107, "You can't dig any deeper.\n"}, {0x10B, "You can't dig any deeper.\n"}, {0x120, "The control panel operates the dream machine.\n"}, {0x125, "You can't determine how to read the time.\n"},
					{0x143, "You don't know how it works.\n"}, {0x147, "You can't dig any deeper.\n"}, {0x1AB, "You can't dig any deeper.\n"}, {0x1BA, "You must use a shovel to move that.\n"},
					{0x1BB, "You must use a shovel to move that.\n"}, {0x1BC, "You must use a shovel to move that.\n"}};
		const char *text = "It has no effect.\n";
		for (const auto &e : SAYS)
			if (e.type == o.type)
				text = e.text;
		message(text);
		break;
	}
	case 41: { // sub_5BC07: steel cannon balls (item) into the cannon (first), once
		if (first < 0)
			break;
		World::Obj &cannon = _world.objects[first];
		if (cannon.quality != 0) {
			message("It already has steel cannon balls.\n");
		} else {
			cannon.quality = 1;
			message("The cannon will fire steel cannon balls.\n");
			_world.deleteObject(item);
		}
		break;
	}
	case 51: { // sub_5F06C: rubber (0x14C) or dye (0x7B) into the sprayer system (first)
		if (first < 0)
			break;
		World::Obj &sp = _world.objects[first];
		if (o.type == 0x14C) {
			_world.deleteObject(item);
			sp.quality |= 1;
			message("The sprayer system now has rubber.\n");
		} else if (o.type == 0x7B) {
			_world.deleteObject(item);
			sp.quality |= 2;
			message("The sprayer system now has dye.\n");
		}
		break;
	}
	case 56: { // sub_5F0DE: a door that works by itself: rusted shut (quality bits 0xC0), locked (0x80), or without power (flags of actor 0x73: 4 for the 0xE3, 0x10 for the 0xB3)
		message("`It is automatic and can't be opened by hand.\n");
		int q = o.quality, f = _world.actors.talkFlags[0x73];
		if ((q & 0xC0) == 0xC0)
			message("It is apparently rusted shut.\n");
		else if (q & 0x80)
			message("It is apparently locked.\n");
		else if ((o.type == 0xE3 && !(f & 4)) || (o.type == 0xB3 && !(f & 0x10)))
			message("It has no power.\n");
		break;
	}
	case 73: { // sub_5B412: a radium chip (item) into the mechanical man (actor 0x75): one is used up, the man changes (frame + 9) and his flag 8 is set; anyone else is told he has it
		if (first < 0 || _world.objects[first].actor != 0x75) {
			message("This mechanical man already has radium.\n");
			break;
		}
		consumeOne(item);
		_world.objects[first].frame = (uint8_t)(_world.objects[first].frame + 9);
		_world.actors.talkFlags[0x75] |= 8;
		break;
	}
	case 21: { // sub_5A17C: a hammer on a geode (first) that is whole (frame 0): it breaks (frame 1) and 2 to 100 of the stuff inside (0x83) fall out, in the pack of whoever has it or on its square
		if (first < 0)
			break;
		World::Obj &geode = _world.objects[first];
		if (geode.frame != 0) {
			message("It has no effect.\n");
			break;
		}
		sfx(0x20);
		geode.frame = 1;
		int n = seRoll(1, 50) + seRoll(1, 50);
		bool held = carriedObject(geode);
		int holder = geode.parent, gx = geode.x, gy = geode.y, gz = geode.z;
		if (held) {
			if (!seGive(holder, 0x83, n)) {
				int a = _world.actorObject(holder);
				seMake(0x83, a >= 0 ? _world.objects[a].x : gx, a >= 0 ? _world.objects[a].y : gy, z, n);
			}
		} else {
			seMake(0x83, gx, gy, gz, n);
		}
		break;
	}
	case 27: { // sub_5EC84: the battle frenzy elixir: only in combat; the member gets the flag 0x80 (wild) and the elixir is used up
		if (_world.actors.globals.size() <= 0x34 || !_world.actors.globals[0x34]) {
			message("That would be a waste outside of combat.\n");
			sfx(5);
			break;
		}
		int me = _world.actorObject(actingActor());
		if (me >= 0 && !(_world.objects[me].status & 0x80)) {
			message(_combat.nameOf(actingActor()) + " enters a battle frenzy!\n");
			sfx(0x32);
		}
		if (me >= 0)
			_world.objects[me].status |= 0x80;
		consumeOne(item);
		break;
	}
	case 30: { // sub_5D93E: lamp oil (item) into a lamp (first): lamps 0x75 / 0x76 take it up to 0x7E (then 0xB4), 0x73 / 0x74 up to 0xB2 (then 0xFF)
		if (first < 0)
			break;
		World::Obj &lamp = _world.objects[first];
		int t = lamp.type, q = lamp.quality;
		if (((t == 0x75 || t == 0x76) && q >= 0x7E) || ((t == 0x73 || t == 0x74) && q >= 0xB2)) {
			message("\nIt's already full.\n");
			sfx(5);
			break;
		}
		consumeOne(item);
		_world.objects[first].quality = (t == 0x75 || t == 0x76) ? 0xB4 : 0xFF;
		break;
	}
	case 63: { // sub_5D4B0: a lit light used on itself: a torch that is not readied burns out, a lamp or candle is put out (0x70 -> 0x6F, 0x72 -> 0x71, 0x74 -> 0x73, 0x76 -> 0x75); on another thing "It's already lit."
		if (item != first) {
			message("It's already lit.\n");
			break;
		}
		if (o.type == 0x6E) {
			if ((o.status & 0x18) == 0x18) {
				message("No.\n");
			} else {
				_world.deleteObject(item);
				message("A torch burns out.\n");
			}
			break;
		}
		static const int OUT[7] = {0x6F, -1, 0x71, -1, 0x73, -1, 0x75};
		int d = o.type - 0x70;
		if (d >= 0 && d <= 6 && OUT[d] >= 0)
			o.type = (uint16_t)OUT[d];
		message("The " + Describe::object(_world, o) + " is extinguished.\n");
		break;
	}
	case 71: { // sub_5F25E: the contents of a bucket (the frame says what is in it) poured into another that is empty
		int a = item, b = first;
		if (b < 0)
			break;
		if (a == b) {
			message("\n\nAn item can't be placed inside itself!\n");
			sfx(5);
			break;
		}
		int fa = _world.objects[a].frame, fb = _world.objects[b].frame;
		if (fa == 0 && fb > 0) {
			std::swap(a, b);
			std::swap(fa, fb);
		}
		if (fb > 0) {
			message("There is already something in the bucket.\n");
			sfx(5);
			break;
		}
		if (fa == 0) {
			message("There is nothing in the bucket.\n");
			sfx(5);
			break;
		}
		_world.objects[b].frame = (uint8_t)fa;
		_world.objects[a].frame = 0;
		break;
	}
	case 11: { // sub_5A5C5: a hoe, pick or shovel on a square: the terrain decides what is dug (soil: a hole 0x101 and a dirt pile 0x102, ice: a hole in the ice 0xFF and a chunk 0x100), the pile goes on the first free square of the eight round the member
		if (first >= 0) {
			const World::Obj &t = _world.objects[first];
			if (!(_world.flags.flags2[(_world.baseTile.firstTile[t.type & 0x3FF] + t.frame) & (TileFlags::TILES - 1)] & 0x10)) {
				message("The ground is not clear for digging.\n");
				break;
			}
		}
		int mask = z == 0 ? 0x3FF : 0xFF;
		int sx = _mdSqX[1], sy = _mdSqY[1];
		int tile = _world.map.tile(z, sx & mask, sy & mask);
		int cls = ((tile >= 0x20 && tile <= 0x24) || tile == 0x0C || tile == 0x0D || (tile >= 0x28 && tile <= 0x3F)) ? 0x102 : (tile == 0x7C || tile == 0x7D || tile == 0x7F) ? 0x100 : 0; // sub_3231F
		if (cls == 0) {
			message("It has no effect.\n");
			break;
		}
		int hole = seMake(cls == 0x102 ? 0x101 : 0xFF, sx, sy, z, 1);
		int me = _world.actorObject(actingActor());
		static const int DX[8] = {0, 1, 1, 1, 0, -1, -1, -1}, DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
		int at = -1, nx = 0, ny = 0;
		for (int i = 0; i < 8 && at < 0 && me >= 0; ++i) {
			nx = (_world.objects[me].x + DX[i]) & mask;
			ny = (_world.objects[me].y + DY[i]) & mask;
			std::vector<int> here;
			_world.objectsAt(nx, ny, z, here);
			if (!here.empty())
				continue;
			if (_world.flags.flags1[_world.map.tile(z, nx, ny) & (TileFlags::TILES - 1)] & 2)
				continue;
			at = i;
		}
		if (at < 0) {
			message("You can't dig here.\n");
			_world.deleteObject(hole);
			break;
		}
		seMake(cls, nx, ny, z, 1);
		sfx(0x1B);
		break;
	}
	case 12: { // sub_5A9C5: a shovel moves a pile (first: rocks, iron ore, coal) onto an empty, passable square (the second partner's): what was under it is let go
		if (first < 0)
			break;
		int mask = z == 0 ? 0x3FF : 0xFF;
		int tx = _mdSqX[2] & mask, ty = _mdSqY[2] & mask;
		std::vector<int> there;
		_world.objectsAt(tx, ty, z, there);
		if ((_world.flags.flags1[_world.map.tile(z, tx, ty) & (TileFlags::TILES - 1)] & 2) || !there.empty()) {
			message("That is not possible.\n");
			sfx(5);
			_mdFail = -1;
			break;
		}
		sfx(0x1B);
		int ox = _world.objects[first].x, oy = _world.objects[first].y;
		_world.moveObject(first, tx, ty, z);
		_world.objects[first].frame = 0;
		mdSpill(ox, oy);
		break;
	}
	case 62: { // sub_5A93C: a dirt pile (first) shovelled into a hole (second): if the hole holds something that is let go, else both go and the hole under the pile spills
		if (first < 0 || second < 0)
			break;
		int inside = -1;
		for (size_t i = 0; i < _world.objects.size() && inside < 0; ++i)
			if (!_world.objects[i].deleted && _world.objects[i].location() == ObjRecord::IN_CONTAINER && _world.objects[i].parent == second)
				inside = (int)i;
		if (inside >= 0) {
			searchHere(second);
			_mdFail = 1;
			break;
		}
		int px = _world.objects[first].x, py = _world.objects[first].y;
		mdSpill(px, py);
		_world.deleteObject(first);
		_world.deleteObject(second);
		sfx(0x1B);
		sfx(0x1B);
		message("You filled in the hole.\n");
		break;
	}
	case 16: { // sub_5ADA9: a pile (first) shovelled into the furnace (second): the pile goes; coal (0x1BC) into a furnace that is not yet full (frame below 4) sets the belt going (APPROX(76): the animation of sub_4EEB0 is left out)
		if (first < 0 || second < 0)
			break;
		int t = _world.objects[first].type, fr = _world.objects[second].frame;
		sfx(0x1B);
		_world.deleteObject(first);
		if (t != 0x1BC) {
			message("It has no effect.\n");
		} else if (fr < 4) {
			message("The coal begins moving down the belt.\n");
			_world.actors.talkFlags[0x73] |= 4;
			_world.actors.talkFlags[0x71] |= 8;
		}
		break;
	}
	case 13: { // sub_5A7BF: a pile (first: dirt 0x102, rocks, iron ore, coal) shovelled into a wheelbarrow (0x10C: one load) or a rail car (0x19A: up to seven of one kind); the quality says which kind (sub_5987C)
		if (first < 0 || second < 0)
			break;
		auto kind = [](int type) { // sub_5987C
			return type == 0x1BA ? 2 : type == 0x1BB ? 3 : type == 0x1BC ? 4 : 1;
		};
		int k = kind(_world.objects[first].type);
		std::string what = Describe::object(_world, _world.objects[first]);
		World::Obj &box = _world.objects[second];
		if (box.type == 0x10C) {
			if (box.quantity == 1) {
				message("There is no more room.\n");
				sfx(5);
				_mdFail = 1;
				break;
			}
			sfx(0x1B);
			box.quantity = 1;
			box.quality = (uint8_t)k;
			message("You put the " + what + " into the wheelbarrow.\n");
			_world.deleteObject(first);
		} else if (box.type == 0x19A) {
			int n = box.quantity;
			if (n == 7) {
				message("There is no more room.\n");
				sfx(5);
				_mdFail = 1;
				break;
			}
			if (n != 0 && k != box.quality) {
				message("You can't mix different things in the same load!\n");
				sfx(5);
				_mdFail = 1;
				break;
			}
			++n;
			box.quality = (uint8_t)k;
			box.frame = (uint8_t)((box.frame & 1) + (n == 7 ? 4 : 2));
			box.quantity = (uint8_t)n;
			sfx(0x1B);
			message("You put the " + what + " into the rail car.\n");
			_world.deleteObject(first);
		}
		break;
	}
	case 1: { // sub_59641: reading a book, note or sign: quality 0 is the affidavit that vouches for the member (signed by those whose flag 8 is set: actors 5, 0x19, 0x1A), else MDTEXT.LZC item quality - 1. APPROX(76): the text window of the original (a page of its own, "Press any key...") is the scroll here
		int q = o.quality;
		if (q == 0) {
			message("--Affidavit--\n\nThe undersigned do hereby vouch for the trust-\nworthiness of " + _combat.nameOf(actingActor()) + ", who bears this chit, commending same to the members of the Olympus Mons outpost.\n\nSigned:\n");
			const ObjList &a = _world.actors;
			message((a.talkFlags[5] & 8) ? " Richard Sherman\n" : "");
			message((a.talkFlags[0x19] & 8) ? " Capt. Gregory Duprey\n" : "");
			message((a.talkFlags[0x1A] & 8) ? " Doctor David Yellin\n" : "");
			message("\n");
			break;
		}
		std::string text = _world.mdTextItem(q - 1);
		if (text.empty()) {
			message("You can't read it.\n");
			break;
		}
		std::string out;
		for (size_t i = 0; i < text.size(); ++i) {
			char c = text[i];
			if (c == '\r')
				continue;
			if (c == '*') { // (a page break)
				out += "\n";
				continue;
			}
			out += c;
		}
		message(out + "\n");
		break;
	}
	case 14: { // sub_5AE03: a shovel unloads a wheelbarrow or rail car (first): a pile of its kind (sub_598BF: 1 dirt, 2 rocks, 3 iron ore, 4 coal) appears on its square and is put to the second partner as the loading recipes would (the ground: 12, a container: 13, the furnace: 16, a hole: 62); if that does not work the pile goes again, else the load is down by one
		if (first < 0)
			break;
		_mdFail = 0;
		int cx = _world.objects[first].x, cy = _world.objects[first].y, cq = _world.objects[first].quality;
		if (_world.objects[first].quantity == 0) {
			message("There is nothing to unload.\n");
			sfx(5);
			break;
		}
		int pile = seMake(cq == 2 ? 0x1BA : cq == 3 ? 0x1BB : cq == 4 ? 0x1BC : 0x102, cx, cy, _world.objects[first].z, 1);
		int t = second >= 0 ? _world.objects[second].type : -1;
		if (second < 0)
			mdAction(12, item, pile, -1);
		else if (t == 0x10C || t == 0x19A)
			mdAction(13, item, pile, second);
		else if (t == 0xE9)
			mdAction(16, item, pile, second);
		else if (t == 0x101)
			mdAction(62, item, pile, second);
		else
			message("It has no effect.\n"); // APPROX(76): the recipe 15 (sub_5AAA1: a pile onto anything) is not ported
		if (_mdFail != 0 || !(t == -1 || t == 0x10C || t == 0x19A || t == 0xE9 || t == 0x101)) {
			if (!_world.objects[pile].deleted)
				_world.deleteObject(pile);
			break;
		}
		World::Obj &box = _world.objects[first];
		box.quantity = (uint8_t)(box.quantity - 1);
		if (box.type == 0x19A)
			box.frame = (uint8_t)((box.frame & 1) + (box.quantity == 0 ? 0 : 2));
		break;
	}
	case 31: { // sub_5DA12: a switch bar turns over (frame 0 <-> 1) together with its base (0x19D) up and to the left; the rail car (0x19A) on that square turns with it; a loose bar (quality 1) only turns loosely
		int bx = o.x - 1, by = o.y - 1;
		int base = -1, car = -1;
		std::vector<int> there;
		_world.objectsAt(bx, by, z, there);
		for (int i : there) {
			if (_world.objects[i].type == 0x19D)
				base = i;
			else if (_world.objects[i].type == 0x19A)
				car = i;
		}
		if (base < 0) {
			message("It has no effect.\n");
			sfx(5);
			break;
		}
		int next = o.frame == 0 ? 1 : 0;
		o.frame = (uint8_t)next;
		if (o.quality == 1) {
			message("It turns loosely.\n");
			break;
		}
		_world.objects[base].frame = (uint8_t)next;
		if (car >= 0) {
			int f = _world.objects[car].frame;
			_world.objects[car].frame = (uint8_t)((f & 1) ? f - 1 : f + 1);
		}
		break;
	}
	case 38: { // sub_5E7E8: a lens (0xCE: socket frame 0, else frame 1; once each) put into its socket (0xCA of that frame) on or next to the thing chosen; a broken one (quality bit 2) will not go
		if (first < 0)
			break;
		if (o.quality & 2) {
			message("The " + Describe::object(_world, o) + " is broken.\n");
			break;
		}
		int slot = o.type == 0xCE ? 0 : 1;
		uint8_t &done = _world.actors.talkFlags[slot == 0 ? 0x2E : 0x39];
		if (done & 0x80) {
			message("It has no effect.\n");
			break;
		}
		int sock = first;
		if (_world.objects[first].frame != slot) {
			sock = -1;
			int fx = _world.objects[first].x, fy = _world.objects[first].y;
			for (int dx = -1; dx <= 1; ++dx)
				for (int dy = -1; dy <= 1; ++dy) {
					std::vector<int> here;
					_world.objectsAt(fx + dx, fy + dy, z, here);
					for (int i : here)
						if (_world.objects[i].type == 0xCA && _world.objects[i].frame == slot && sock < 0)
							sock = i;
				}
			if (sock < 0) {
				message("You cannot install it there.\n");
				break;
			}
		}
		int sx = _world.objects[sock].x, sy = _world.objects[sock].y;
		mdPlace(item, sx, sy, z);
		_world.objects[item].quality = 0;
		done |= 0x80;
		break;
	}
	case 48: { // sub_5C418: paint (0x77) into an empty spray gun (first: quality 0) gives 20 charges of green paint (quality 1); weed killer (0x80) into one with paint (quality not 0) 10 charges of weed killer (quality 0); else ten more charges up to thirty
		if (first < 0)
			break;
		World::Obj &gun = _world.objects[first];
		int ty = o.type, charges = gun.quantity, kind = gun.quality;
		if (ty == 0x77 && kind == 0) {
			message("The spray gun now contains 20 charges of green paint.\n");
			gun.quantity = 20;
			gun.quality = 1;
		} else if (ty == 0x80 && kind != 0) {
			message("The spray gun now contains 10 charges of weed killer.\n");
			gun.quantity = 10;
			gun.quality = 0;
		} else {
			charges = std::min(charges + 10, 30);
			gun.quantity = (uint8_t)charges;
			message("The spray gun has 10 more charges.\n");
		}
		_world.deleteObject(item);
		break;
	}
	case 50: { // sub_5C9F8: the oxium bin: twenty of the stuff (0x83) into the member's pack ("You are carrying too much already."); the flag 0x10 of actor 0x12 is set. APPROX(76): the line of sight to the bin ("Blocked!") is left out
		int actor = actingActor();
		if (!seGive(actor, 0x83, 20)) {
			message("You are carrying too much already.\n");
			break;
		}
		message("You get two handfuls of oxium from the bin.\n");
		_world.actors.talkFlags[0x12] |= 0x10;
		break;
	}
	case 49: { // sub_5C518: a spray gun (item) at something: empty "There is nothing in the gun."; with green paint (quality not 0) a person of type 0x91 or anything invisible becomes visible; with weed killer (quality 0) a plant (the types of ds:0E86) is killed: 0xCD sets a flag of actor 0x30 and goes, 0x198 becomes 0xA6, the others go. APPROX(76): weed killer on a person (sub_1A6F2) does nothing
		if (first < 0)
			break;
		int charges = o.quantity, kind = o.quality;
		if (charges == 0) {
			message("There is nothing in the gun.\n");
			break;
		}
		static const int PLANTS[8] = {0xCD, 0x198, 0xA8, 0xAA, 0xAB, 0x12C, 0x133, 0x12B};
		World::Obj &t = _world.objects[first];
		int ty = t.type;
		if (kind != 0) {
			o.quantity = (uint8_t)(charges - 1);
			if (t.actor >= 0 && ty == 0x91) {
				t.type = 0x16C;
				message("`" + Describe::object(_world, t) + " becomes visible!\n");
			} else if (t.status & 2) {
				t.status &= 0xFD;
				message("`" + Describe::object(_world, t) + " becomes visible!\n");
			} else {
				message("It has no effect.\n");
			}
			break;
		}
		if (t.actor >= 0)
			break;
		bool plant = false;
		for (int p : PLANTS)
			plant |= p == ty;
		o.quantity = (uint8_t)(charges - 1);
		if (!plant) {
			message("It has no effect.\n");
			break;
		}
		message("You killed the plant.\n");
		if (ty == 0xCD) {
			_world.actors.talkFlags[0x30] |= 0x80;
			_world.deleteObject(first);
		} else if (ty == 0x198) {
			t.type = 0xA6;
		} else {
			_world.deleteObject(first);
		}
		break;
	}
	case 58: { // sub_5CAFA: the seed plant (item): from frame 1 on one seed (0x9E) can be taken, the plant starts again at frame 0 with a growth number of 10 to 15 in its quality
		if (o.frame < 1) {
			message("No seed to harvest yet.\n");
			break;
		}
		if (!seGive(actingActor(), 0x9E, 1)) {
			message("You are carrying too much already.\n");
			break;
		}
		World::Obj &plant = _world.objects[item];
		plant.frame = 0;
		plant.quality = (uint8_t)seRoll(10, 15);
		break;
	}
	case 40: { // sub_5AFA9: a ray gun (0x2D has no charges to count) says its setting and asks for a new one: R rifle (quality 1), S shotgun (2), C combination (0); Enter leaves it
		int q = o.quality;
		message(std::string("It is set to \"") + (q == 0 ? "Combination" : q > 1 ? "Shotgun" : "Rifle") + "\".");
		if (o.type != 0x2D)
			message("  It has " + std::to_string((int)o.quantity) + " charges left.");
		message("  New setting (R/S/C)?\n");
		int gun = item;
		mdAskKey([this, gun](int key) {
			int c = key >= 'a' && key <= 'z' ? key - 32 : key;
			if (c != 'S' && c != 'R' && c != 'C' && c != KEY_ENTER && c != '\n' && c != '\r')
				return false;
			if (c == 'C') {
				message("Combination\n");
				_world.objects[gun].quality = 0;
			} else if (c == 'R') {
				message("Rifle\n");
				_world.objects[gun].quality = 1;
			} else if (c == 'S') {
				message("Shotgun\n");
				_world.objects[gun].quality = 2;
			}
			return true;
		});
		break;
	}
	case 59: { // sub_5CC45: a seed (item): "Location: " asks for a square (the cursor; none is "nowhere."); the seed must go in ground that can be dug (the classes of sub_3231F), "Won't grow there!" else
		message("Location: ");
		_mdStage = 3;
		_pending = 'Z';
		_cursor = true;
		_cx = _cy = 5;
		_mdAsync = true;
		break;
	}
	case 36: { // sub_5B6F7: the pipe organ: organ music (0x199) on the organ (0xA5 next to it) or an organ with the music beside it (up and to the left) plays the Toccata; else the keys 1-0 play notes until another key is pressed. APPROX(76): the Toccata itself (sub_306A5) is only named
		int music = -1;
		int organ = item;
		if (o.type == 0x199) {
			if (carriedObject(o)) {
				message("Organ music must be played on a pipe organ.\n");
				break;
			}
			music = item;
			organ = seFind(0xA5, o.x + 1, o.y);
			if (organ < 0) {
				message("Organ music must be played on a pipe organ.\n");
				break;
			}
		}
		if (music < 0) {
			int ox = _world.objects[organ].x - 1, oy = _world.objects[organ].y;
			std::vector<int> there;
			_world.objectsAt(ox, oy, z, there);
			for (int i : there)
				if (_world.objects[i].type == 0x199 && music < 0)
					music = i;
		}
		if (music >= 0) {
			message("Tocatta and Fugue in D minor (J.S. Bach).\n");
			break;
		}
		message("Press keys 1-0 to play -");
		mdAskKey([this](int key) {
			if (key >= '0' && key <= '9') {
				sfx(key - 10);
				return false;
			}
			return true;
		});
		break;
	}
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
