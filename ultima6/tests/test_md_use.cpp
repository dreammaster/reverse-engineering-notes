// Martian Dreams' Use (the recipes of seg059 / seg060): an object without a recipe "can't be used", a door of recipe 44 opens and shuts, a locked one says so (skipped without data).

#include "ultima6/session.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace Ultima6;
using ::Ultima6::Ultima6::Game;
using ::Ultima6::Ultima6::World;

static int failures = 0;

#define CHECK(cond)                                                              \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
			++failures;                                                          \
		}                                                                        \
	} while (0)

static std::string logText(const Game &g) {
	std::string s;
	for (const std::string &l : g.log)
		s += l;
	return s;
}

static int put(World &w, int type, int frame, int x, int y, int z) {
	World::Obj o;
	o.status = 0;
	o.x = (uint16_t)x;
	o.y = (uint16_t)y;
	o.z = (uint8_t)z;
	o.type = (uint16_t)type;
	o.frame = (uint8_t)frame;
	o.quantity = 1;
	o.quality = 0;
	o.actor = -1;
	o.parent = -1;
	w.objects.push_back(o);
	w.addToMap((int)w.objects.size() - 1);
	return (int)w.objects.size() - 1;
}

static void useEast(World &w, Game &g, int px, int py) {
	w.moveObject(w.actorObject(1), px, py, 0);
	g.x = px;
	g.y = py;
	g.z = 0;
	g.handleKey('u');
	g.handleKey(KEY_RIGHT);
}

int main() {
	static World w;
	w.game = GameId::MartianDreams;
	if (!w.load("md/game/MARTIAN")) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	w.objects.reserve(w.objects.size() + 32);
	Game g(w);
	int px = g.x, py = g.y;

	for (int i = 0; i < w.actors.partySize; ++i)
		w.moveObject(w.actorObject(w.actors.partyRoster[i]), px, py, 0);
	std::vector<int> there;
	w.objectsAt(px + 1, py, 0, there);
	for (int i : there)
		if (w.objects[i].actor < 0)
			w.deleteObject(i);
	// a door (type 0xDE takes recipe 44): frame bit 2 turns over
	int door = put(w, 0xDE, 0, px + 1, py, 0);
	useEast(w, g, px, py);
	CHECK(w.objects[door].frame == 2);
	useEast(w, g, px, py);
	CHECK(w.objects[door].frame == 0);
	// locked: quality bit 0x80
	w.objects[door].quality = 0x80;
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(w.objects[door].frame == 0);
	CHECK(logText(g).find("locked") != std::string::npos);
	w.deleteObject(door);

	// an unlit oil lamp (0x73) with a match: no fuel; with a can of lamp oil (0x7C) it takes fuel and lights (type 0x74), the match is used up
	{
		w.deleteObject(door);
		std::vector<int> here;
		w.objectsAt(px + 1, py, 0, here);
		for (int i : here)
			if (w.objects[i].actor < 0)
				w.deleteObject(i);
		int lamp = put(w, 0x73, 0, px + 1, py, 0);
		w.addToInventory(w.actors.partyRoster[0], 0x6B, 0, 1);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("There is no fuel") != std::string::npos);
		CHECK(w.objects[lamp].type == 0x73);
		w.addToInventory(w.actors.partyRoster[0], 0x7C, 0, 1);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(w.objects[lamp].type == 0x74 && w.objects[lamp].quality == 0xFF);
		w.deleteObject(lamp);
	}

	// a sliding gate (0xB5, recipe 70): frame 0 goes to 3; locked it will not
	{
		std::vector<int> here;
		w.objectsAt(px + 1, py, 0, here);
		for (int i : here)
			if (w.objects[i].actor < 0)
				w.deleteObject(i);
		int gate = put(w, 0xB5, 0, px + 1, py, 0);
		useEast(w, g, px, py);
		CHECK(w.objects[gate].frame == 3);
		w.objects[gate].frame = 0;
		w.objects[gate].quality = 0x80;
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(w.objects[gate].frame == 0 && logText(g).find("locked") != std::string::npos);
		w.deleteObject(gate);
	}

	// an instrument (0x11B, recipe 57) is played ("Wheeeeze"); a thing that has only a message (0x143, recipe 64) says it
	{
		for (int type : {0x11B, 0x143}) {
			std::vector<int> here;
			w.objectsAt(px + 1, py, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
			int thing = put(w, type, 0, px + 1, py, 0);
			g.log.clear();
			useEast(w, g, px, py);
			if (type == 0x11B)
				CHECK(logText(g).find("Wheeeeze") != std::string::npos);
			else
				CHECK(logText(g).find("You don't know how it works") != std::string::npos);
			w.deleteObject(thing);
		}
	}

	// digging (recipe 11): a shovel (0x42) used on a square of soil (the terrain classes of sub_3231F) makes a hole (0x101) there and a dirt pile (0x102) on a free square round the member; a square of the ground that
	// is not soil has no effect. Found by looking over the surface round the start for a place with soil to the south and room round it.
	{
		auto digClass = [&](int tile) { return ((tile >= 0x20 && tile <= 0x24) || tile == 0x0C || tile == 0x0D || (tile >= 0x28 && tile <= 0x3F)) ? 0x102 : (tile == 0x7C || tile == 0x7D || tile == 0x7F) ? 0x100 : 0; };
		auto clear = [&](int x, int y) {
			std::vector<int> here;
			w.objectsAt(x, y, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0 && !w.objects[i].deleted)
					return false;
			return true;
		};
		int fx = -1, fy = -1;
		for (int dy = -30; dy <= 30 && fx < 0; ++dy)
			for (int dx = -30; dx <= 30 && fx < 0; ++dx) {
				int cx = px + dx, cy = py + dy;
				int below = w.map.tile(0, cx, cy + 1);
				if (digClass(below) != 0x102 || !clear(cx, cy + 1) || !clear(cx + 1, cy) || !clear(cx, cy - 1) || !clear(cx - 1, cy))
					continue;
				if (w.flags.flags1[w.map.tile(0, cx, cy - 1)] & 2)
					continue;
				fx = cx;
				fy = cy;
			}
		if (fx < 0) {
			std::printf("no soil found to dig in (skipped)\n");
		} else {
			int shovel = put(w, 0x42, 0, fx + 1, fy, 0);
			w.moveObject(w.actorObject(1), fx, fy, 0);
			for (int i = 1; i < w.actors.partySize; ++i)
				w.moveObject(w.actorObject(w.actors.partyRoster[i]), fx, fy, 0);
			g.x = fx;
			g.y = fy;
			g.z = 0;
			size_t before = w.objects.size();
			g.log.clear();
			g.handleKey('u');
			g.handleKey(KEY_RIGHT);
			g.handleKey(KEY_DOWN);
			g.handleKey(KEY_ENTER);
			bool hole = false, pile = false;
			for (size_t i = before; i < w.objects.size(); ++i) {
				hole |= w.objects[i].type == 0x101 && w.objects[i].x == fx && w.objects[i].y == fy + 1;
				pile |= w.objects[i].type == 0x102;
			}
			CHECK(hole && pile);
			w.deleteObject(shovel);
			for (size_t i = before; i < w.objects.size(); ++i)
				if (w.objects[i].type == 0x101 || w.objects[i].type == 0x102)
					w.deleteObject((int)i);
			for (int i = 0; i < w.actors.partySize; ++i)
				w.moveObject(w.actorObject(w.actors.partyRoster[i]), px, py, 0);
			g.x = px;
			g.y = py;
		}
	}

	// a shovel (0x42): a pile of rocks (0x1BA) to the south goes into the wheelbarrow (0x10C) to the north (recipe 13), then a second one is refused: "There is no more room."
	{
		std::vector<int> here;
		for (auto sq : {std::make_pair(px + 1, py), std::make_pair(px, py + 1), std::make_pair(px, py - 1)}) {
			here.clear();
			w.objectsAt(sq.first, sq.second, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
		}
		int shovel = put(w, 0x42, 0, px + 1, py, 0);
		int rocks = put(w, 0x1BA, 0, px, py + 1, 0);
		int barrow = put(w, 0x10C, 0, px, py - 1, 0);
		w.objects[barrow].quantity = 0; // (empty)
		g.log.clear();
		g.handleKey('u');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_DOWN);
		g.handleKey(KEY_ENTER);
		g.handleKey(KEY_UP);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("You put the") != std::string::npos && logText(g).find("into the wheelbarrow") != std::string::npos);
		CHECK(w.objects[rocks].deleted && w.objects[barrow].quantity == 1 && w.objects[barrow].quality == 2);
		int rocks2 = put(w, 0x1BA, 0, px, py + 1, 0);
		g.log.clear();
		g.handleKey('u');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_DOWN);
		g.handleKey(KEY_ENTER);
		g.handleKey(KEY_UP);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("There is no more room") != std::string::npos && !w.objects[rocks2].deleted);
		w.deleteObject(rocks2);
		w.deleteObject(barrow);
		w.deleteObject(shovel);
	}

	// a book (0x94, recipe 1): its quality names the item of MDTEXT.LZC (quality 2: the power generation system); a quality that has no text can't be read
	{
		std::vector<int> here;
		w.objectsAt(px + 1, py, 0, here);
		for (int i : here)
			if (w.objects[i].actor < 0)
				w.deleteObject(i);
		int book = put(w, 0x94, 0, px + 1, py, 0);
		w.objects[book].quality = 2;
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("Operation of the Power Generation System") != std::string::npos);
		w.objects[book].quality = 120;
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("You can't read it") != std::string::npos);
		w.deleteObject(book);
	}

	// a shovel on a loaded wheelbarrow (rocks) to the north and then the ground to the south unloads it: a pile of rocks lands on the ground square and the barrow is empty
	{
		std::vector<int> here;
		for (auto sq : {std::make_pair(px + 1, py), std::make_pair(px, py + 1), std::make_pair(px, py - 1)}) {
			here.clear();
			w.objectsAt(sq.first, sq.second, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
		}
		int shovel = put(w, 0x42, 0, px + 1, py, 0);
		int barrow = put(w, 0x10C, 0, px, py - 1, 0);
		w.objects[barrow].quantity = 1;
		w.objects[barrow].quality = 2;
		size_t before = w.objects.size();
		g.log.clear();
		g.handleKey('u');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_UP);
		g.handleKey(KEY_ENTER);
		g.handleKey(KEY_DOWN);
		g.handleKey(KEY_ENTER);
		bool pile = false;
		for (size_t i = before; i < w.objects.size(); ++i)
			pile |= w.objects[i].type == 0x1BA && !w.objects[i].deleted && w.objects[i].x == px && w.objects[i].y == py + 1;
		CHECK(pile && w.objects[barrow].quantity == 0);
		g.log.clear();
		g.handleKey('u');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_UP);
		g.handleKey(KEY_ENTER);
		g.handleKey(KEY_DOWN);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("There is nothing to unload") != std::string::npos);
		for (size_t i = before; i < w.objects.size(); ++i)
			if (w.objects[i].type == 0x1BA)
				w.deleteObject((int)i);
		w.deleteObject(barrow);
		w.deleteObject(shovel);
	}

	// a switch bar (0x19B, recipe 31) with its base (0x19D) up and to the left and a rail car (0x19A) on the base's square turns over together; a loose one (quality 1) only turns loosely; without a base it has no effect
	{
		std::vector<int> here;
		for (auto sq : {std::make_pair(px + 1, py), std::make_pair(px, py - 1)}) {
			here.clear();
			w.objectsAt(sq.first, sq.second, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
		}
		int bar = put(w, 0x19B, 0, px + 1, py, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("It has no effect") != std::string::npos && w.objects[bar].frame == 0);
		int base = put(w, 0x19D, 0, px, py - 1, 0);
		int car = put(w, 0x19A, 0, px, py - 1, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(w.objects[bar].frame == 1 && w.objects[base].frame == 1 && w.objects[car].frame == 1);
		w.objects[bar].quality = 1;
		useEast(w, g, px, py);
		CHECK(w.objects[bar].frame == 0 && w.objects[base].frame == 1 && logText(g).find("It turns loosely") != std::string::npos);
		w.deleteObject(bar);
		w.deleteObject(base);
		w.deleteObject(car);
	}

	// a lens (0xCE, recipe 38) is put into its socket (0xCA, frame 0) to the south: it lands on the socket's square and the flag of that lens is set; a second one "has no effect"; a broken one says so
	{
		std::vector<int> here;
		for (auto sq : {std::make_pair(px + 1, py), std::make_pair(px, py + 1)}) {
			here.clear();
			w.objectsAt(sq.first, sq.second, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
		}
		w.actors.talkFlags[0x2E] &= 0x7F;
		int lens = put(w, 0xCE, 0, px + 1, py, 0);
		int socket = put(w, 0xCA, 0, px, py + 1, 0);
		w.objects[lens].quality = 2;
		g.log.clear();
		g.handleKey('u');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_DOWN);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("is broken") != std::string::npos);
		w.objects[lens].quality = 0;
		g.log.clear();
		g.handleKey('u');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_DOWN);
		g.handleKey(KEY_ENTER);
		CHECK(w.objects[lens].x == px && w.objects[lens].y == py + 1 && (w.actors.talkFlags[0x2E] & 0x80));
		w.moveObject(lens, px + 1, py, 0);
		g.log.clear();
		g.handleKey('u');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_DOWN);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("It has no effect") != std::string::npos);
		w.actors.talkFlags[0x2E] &= 0x7F;
		w.deleteObject(lens);
		w.deleteObject(socket);
	}

	// paint (0x77) into an empty spray gun (0x81) gives 20 charges of green paint; the oxium bin (0xD4, recipe 50) puts oxium in the pack
	{
		std::vector<int> here;
		for (auto sq : {std::make_pair(px + 1, py), std::make_pair(px, py + 1)}) {
			here.clear();
			w.objectsAt(sq.first, sq.second, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
		}
		int paint = put(w, 0x77, 0, px + 1, py, 0);
		int gun = put(w, 0x81, 0, px, py + 1, 0);
		w.objects[gun].quantity = 0;
		w.objects[gun].quality = 0;
		g.log.clear();
		g.handleKey('u');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_DOWN);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("20 charges of green paint") != std::string::npos && w.objects[gun].quantity == 20 && w.objects[gun].quality == 1 && w.objects[paint].deleted);
		w.deleteObject(gun);
		int bin = put(w, 0xD4, 0, px + 1, py, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("You get two handfuls of oxium") != std::string::npos || logText(g).find("carrying too much") != std::string::npos);
		w.deleteObject(bin);
	}

	// a seed plant (0x9D, recipe 58): "No seed to harvest yet." at frame 0, from frame 1 one seed (0x9E) goes into the pack and the plant starts over; a spray gun (0x105, recipe 49) with paint makes something invisible visible, with weed killer kills a plant (0xA8)
	{
		std::vector<int> here;
		for (auto sq : {std::make_pair(px + 1, py), std::make_pair(px, py + 1)}) {
			here.clear();
			w.objectsAt(sq.first, sq.second, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
		}
		int plant = put(w, 0x9D, 0, px + 1, py, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("No seed to harvest yet") != std::string::npos);
		w.objects[plant].frame = 1;
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(w.objects[plant].frame == 0 && w.objects[plant].quality >= 10 && w.objects[plant].quality <= 15);
		w.deleteObject(plant);
		int gun = put(w, 0x105, 0, px + 1, py, 0);
		w.objects[gun].quantity = 5;
		w.objects[gun].quality = 1;
		int ghost = put(w, 0x30, 0, px, py + 1, 0);
		w.objects[ghost].status |= 2;
		g.log.clear();
		g.handleKey('u');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_DOWN);
		g.handleKey(KEY_ENTER);
		w.deleteObject(ghost);
		w.deleteObject(gun);
	}

	// recipes that wait for keys: a ray gun (0x2D, recipe 40) says its setting and takes R (rifle) / S / C; a seed (0x9E, recipe 59) asks for a location (a bare spot of rock: "Won't grow there!" or planted); the pipe organ (0xA5, recipe 36) plays the keys 1-0 until another key
	{
		std::vector<int> here;
		for (auto sq : {std::make_pair(px + 1, py), std::make_pair(px, py + 1)}) {
			here.clear();
			w.objectsAt(sq.first, sq.second, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
		}
		int gun = put(w, 0x2D, 0, px + 1, py, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("It is set to \"Combination\"") != std::string::npos && logText(g).find("New setting") != std::string::npos);
		g.handleKey('x'); // (not a setting: still waiting)
		g.handleKey('r');
		CHECK(w.objects[gun].quality == 1 && logText(g).find("Rifle") != std::string::npos);
		w.deleteObject(gun);
		int seed = put(w, 0x9E, 0, px + 1, py, 0);
		g.log.clear();
		g.handleKey('u');
		g.handleKey(KEY_RIGHT);
		CHECK(logText(g).find("Location:") != std::string::npos);
		g.handleKey(KEY_DOWN);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("Won't grow there") != std::string::npos || logText(g).find("Now it just needs watering") != std::string::npos);
		w.deleteObject(seed);
		int organ = put(w, 0xA5, 0, px + 1, py, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("Press keys 1-0 to play") != std::string::npos);
		g.handleKey('5');
		g.handleKey('7');
		g.handleKey(KEY_ENTER); // (any other key ends it)
		w.deleteObject(organ);
	}

	// the cannon (0x14D, recipe 25) facing east shoots six squares: a door (0xAD) there is shaken by steel balls (frame 0 -> 2, then "Kaboom!"), "Splat!" without them; a cannon facing west hits the party's own square
	{
		std::vector<int> here;
		for (auto sq : {std::make_pair(px + 1, py), std::make_pair(px + 8, py)}) {
			here.clear();
			w.objectsAt(sq.first, sq.second, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
		}
		int cannon = put(w, 0x14D, 1, px + 1, py, 0);
		int door = put(w, 0xAD, 0, px + 8, py, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("Fire!") != std::string::npos && logText(g).find("Splat!") != std::string::npos && w.objects[door].frame == 0);
		w.objects[cannon].quality = 1;
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(w.objects[door].frame == 2 && logText(g).find("Kaboom") == std::string::npos);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(w.objects[door].frame == 4 && logText(g).find("You blew the door down") != std::string::npos);
		w.deleteObject(door);
		w.objects[cannon].frame = 3;
		int victim = w.actors.partyRoster[1];
		w.moveObject(w.actorObject(victim), px, py, 0);
		w.actors.hp[victim] = 200;
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(w.actors.hp[victim] < 200);
		w.deleteObject(cannon);
	}

	// lamp oil (0x7C, recipe 60) on a door (0xDE): one that is rusted shut (quality 0xC0) is unstuck (0x40), another is "not rusted"
	{
		std::vector<int> here;
		for (auto sq : {std::make_pair(px + 1, py), std::make_pair(px, py + 1)}) {
			here.clear();
			w.objectsAt(sq.first, sq.second, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
		}
		int oil = put(w, 0x7C, 0, px + 1, py, 0);
		int door = put(w, 0xDE, 0, px, py + 1, 0);
		w.objects[door].quality = 0x40;
		g.log.clear();
		g.handleKey('u');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_DOWN);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("This door is not rusted") != std::string::npos && !w.objects[oil].deleted);
		w.objects[door].quality = 0xC0;
		g.log.clear();
		g.handleKey('u');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_DOWN);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("The door is unstuck") != std::string::npos && w.objects[door].quality == 0x40 && w.objects[oil].deleted);
		w.deleteObject(door);
	}

	// Martian berries (recipe 26): 0x49, 0x4A and 0x4B raise the psychic numbers 0, 1 and 2 of the acting member (the first "a strange", later "an increase in the strange"), the berry is used up; the
	// fourth kind (0x4C) hurts and makes the member dizzy; the dreamstuff hoard (0x1C7, recipe 54) needs power 1
	{
		std::vector<int> here;
		w.objectsAt(px + 1, py, 0, here);
		for (int i : here)
			if (w.objects[i].actor < 0)
				w.deleteObject(i);
		int me = g.actingActor();
		w.actors.level[me] = 3;
		for (int kind = 0; kind < 3; ++kind) {
			int berry = put(w, 0x49 + kind, 0, px + 1, py, 0);
			CHECK(g.mdPsychic(me, kind) == 0);
			g.log.clear();
			useEast(w, g, px, py);
			CHECK(logText(g).find("You eat a Martian berry. You feel a strange ") != std::string::npos);
			CHECK(g.mdPsychic(me, kind) == 1 && w.objects[berry].deleted);
			for (int other = 0; other < 3; ++other)
				CHECK(other == kind || g.mdPsychic(me, other) == (other < kind ? 1 : 0));
		}
		int berry = put(w, 0x4A, 0, px + 1, py, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("an increase in the strange sensitivity") != std::string::npos && g.mdPsychic(me, 1) == 2 && w.objects[berry].deleted);
		// the hoard
		int hoard = put(w, 0x1C7, 0, px + 1, py, 0);
		w.objects[hoard].quality = 1;
		g.mdSetPsychic(me, 1, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("You lack the mental power") != std::string::npos && w.objects[hoard].quality == 1);
		g.mdSetPsychic(me, 1, 1);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("You pull some of the dreamstuff from the hoard") != std::string::npos && w.objects[hoard].quality == 0);
		int vial = -1, special = 0;
		std::vector<int> inv;
		w.inventory(me, inv, true);
		for (int i : inv)
			if (w.objects[i].type == 0x14B)
				vial = i;
		CHECK(vial >= 0);
		for (size_t i = 0; i < w.objects.size(); ++i)
			if (!w.objects[i].deleted && w.objects[i].parent == vial && w.objects[i].type == 0x139)
				special = 1;
		CHECK(special);
		w.deleteObject(hoard);
		// the sick berry
		int bad = put(w, 0x4C, 0, px + 1, py, 0);
		int hp = w.actors.hp[me];
		w.actors.hp[me] = 255;
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("sudden flush, dizziness, and nausea") != std::string::npos && w.actors.hp[me] < 255 && w.actors.hp[me] >= 255 - 40 && w.objects[bad].deleted);
		CHECK(w.actors.globals[0x16] >= 1 && w.actors.globals[0x16] <= 2);
		w.actors.hp[me] = (uint8_t)hp;
		w.actors.globals[0x16] = 0;
		w.actors.globals[0x40 + 2 * me] = 0;
		w.actors.globals[0x41 + 2 * me] = 0;
	}

	// the reach of Use (sub_35972): one square, three more with the first psychic power and level 3; the elixir (0x79, recipe 55) on a barrel is no use but says so
	{
		std::vector<int> here;
		for (auto sq : {std::make_pair(px + 1, py), std::make_pair(px + 3, py)}) {
			here.clear();
			w.objectsAt(sq.first, sq.second, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
		}
		int me = g.actingActor();
		CHECK(g.mdDistance(10, 10, 13, 10) == 3 && g.mdDistance(10, 10, 12, 12) == 2 && g.mdDistance(0, 0, 8, 0) == 9 && g.mdDistance(1020, 5, 2, 5) == 6);
		int elixir = put(w, 0x79, 0, px + 1, py, 0);
		int barrel = put(w, 0x68, 0, px + 3, py, 0);
		w.actors.level[me] = 3;
		g.mdSetPsychic(me, 0, 0);
		g.log.clear();
		g.handleKey('u');
		g.handleKey(KEY_RIGHT);
		for (int i = 0; i < 3; ++i)
			g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("Out of range!") != std::string::npos && logText(g).find("It has no effect") == std::string::npos);
		g.mdSetPsychic(me, 0, 1);
		g.log.clear();
		g.handleKey('u');
		g.handleKey(KEY_RIGHT);
		for (int i = 0; i < 3; ++i)
			g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("Out of range!") == std::string::npos && logText(g).find("It has no effect") != std::string::npos && !w.objects[elixir].deleted);
		g.mdSetPsychic(me, 0, 0);
		w.deleteObject(barrel);
		w.deleteObject(elixir);
	}

	// a ritual pod knife (0xF2, recipe 20) on a covered seed (0xFB): not yet ripe, ripe (frame 3: an unconscious Martian lies by it, flag 8 of actor 0x61), then open
	{
		std::vector<int> here;
		for (auto sq : {std::make_pair(px + 1, py), std::make_pair(px, py + 1)}) {
			here.clear();
			w.objectsAt(sq.first, sq.second, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
		}
		int knife = put(w, 0xF2, 0, px + 1, py, 0);
		int pod = put(w, 0xFB, 1, px, py + 1, 0);
		auto cut = [&]() {
			g.log.clear();
			g.handleKey('u');
			g.handleKey(KEY_RIGHT);
			g.handleKey(KEY_DOWN);
			g.handleKey(KEY_ENTER);
		};
		cut();
		CHECK(logText(g).find("It is not fully grown") != std::string::npos);
		w.objects[pod].frame = 3;
		size_t before = w.objects.size();
		cut();
		CHECK(logText(g).find("An unconscious Martian falls out of the pod") != std::string::npos && w.objects[pod].frame == 4 && (w.objects[pod].status & 0x20));
		CHECK(w.objects.size() == before + 1 && w.objects.back().type == 0x123 && w.objects.back().x == px && w.objects.back().y == py + 1);
		CHECK(w.actors.talkFlags[0x61] & 8);
		w.deleteObject(w.objects.size() - 1);
		cut();
		CHECK(logText(g).find("The pod is already open") != std::string::npos);
		w.deleteObject(pod);
		w.deleteObject(knife);
	}

	// pliers (0x35, recipe 8) on a cable spool (0xC7) to a power cable (0xD7): the story must be at the point (flags 0x73 bit 4), the Avatar needs rubber gloves (0x26) worn or is shocked
	{
		std::vector<int> here;
		for (auto sq : {std::make_pair(px + 1, py), std::make_pair(px, py + 1), std::make_pair(px - 1, py)}) {
			here.clear();
			w.objectsAt(sq.first, sq.second, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
		}
		int pliers = put(w, 0x35, 0, px + 1, py, 0);
		int spool = put(w, 0xC7, 0, px, py + 1, 0);
		int cable = put(w, 0xD7, 0, px - 1, py, 0);
		auto fix = [&]() {
			g.log.clear();
			g.handleKey('u');
			g.handleKey(KEY_RIGHT);
			g.handleKey(KEY_DOWN);
			g.handleKey(KEY_ENTER);
			g.handleKey(KEY_LEFT);
			g.handleKey(KEY_ENTER);
		};
		w.actors.talkFlags[0x73] = 0;
		fix();
		CHECK(logText(g).find("The cable does not need replacement") != std::string::npos && !w.objects[spool].deleted);
		w.actors.talkFlags[0x73] = 4;
		w.actors.talkFlags[0x71] = 0;
		int hp = w.actors.hp[1];
		w.actors.hp[1] = 200;
		fix(); // no gloves: a shock
		for (int n = 0; g.talking() && n < 40; ++n) { // (somebody who saw it talks)
			for (char c : std::string("bye"))
				g.handleKey(c);
			g.handleKey(KEY_ENTER);
		}
		CHECK(!g.talking());
		CHECK(w.actors.hp[1] < 200 && !w.objects[spool].deleted && !(w.actors.talkFlags[0x73] & 0x10));
		int gloves = w.addToInventory(1, 0x26, 0, 1);
		w.objects[gloves].status |= 0x18;
		w.actors.hp[1] = 200;
		fix();
		CHECK(w.actors.hp[1] == 200 && w.objects[spool].deleted && (w.actors.talkFlags[0x73] & 0x10));
		w.actors.hp[1] = (uint8_t)hp;
		w.actors.talkFlags[0x73] = 0;
		w.deleteObject(gloves);
		w.deleteObject(cable);
		w.deleteObject(pliers);
	}

	// a tool (0x66, recipe 18) on the hatch (0x1AB): the first time the people talk (flag 4 of actor 0x10), once flag 0x10 is set the hatch breaks (0x1AC), moves one square down and right, and a minute passes
	{
		std::vector<int> here;
		for (auto sq : {std::make_pair(px + 1, py), std::make_pair(px, py + 1), std::make_pair(px + 1, py + 1)}) {
			here.clear();
			w.objectsAt(sq.first, sq.second, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
		}
		int tool = put(w, 0x66, 0, px + 1, py, 0);
		int hatch = put(w, 0x1AB, 0, px, py + 1, 0);
		auto pry = [&]() {
			g.log.clear();
			g.handleKey('u');
			g.handleKey(KEY_RIGHT);
			g.handleKey(KEY_DOWN);
			g.handleKey(KEY_ENTER);
			for (int n = 0; g.talking() && n < 40; ++n) {
				for (char c : std::string("bye"))
					g.handleKey(c);
				g.handleKey(KEY_ENTER);
			}
		};
		w.actors.talkFlags[0x10] = 0;
		pry();
		CHECK(w.objects[hatch].type == 0x1AB);
		w.actors.talkFlags[0x10] |= 0x10;
		pry();
		CHECK(w.objects[hatch].type == 0x1AC && w.objects[hatch].x == px + 1 && w.objects[hatch].y == py + 2 && (w.actors.talkFlags[0x10] & 0x20));
		w.actors.talkFlags[0x10] = 0;
		w.actors.talkFlags[0x12] = 0;
		w.deleteObject(hatch);
		w.deleteObject(tool);
	}

	// the tent (0x6A, recipe 0): the camp comes up, "Rest until sunrise?" and any key but N sleeps until then; a member whose experience has come to the number of the level dreams and picks what grows
	{
		std::vector<int> here;
		for (int sx = px - 1; sx <= px + 3; ++sx)
			for (int sy = py - 3; sy <= py + 1; ++sy) {
				here.clear();
				w.objectsAt(sx, sy, 0, here);
				for (int i : here)
					if (w.objects[i].actor < 0)
						w.deleteObject(i);
			}
		std::vector<std::pair<int, std::pair<int, int>>> moved; // (people who stand near would be in the way: the test sends them away, and again after the sleep, when they have walked back)
		auto sendAway = [&]() {
			for (int a = 4; a < 0x100; ++a) {
				int k = w.actorObject(a);
				if (k >= 0 && !w.objects[k].deleted && std::abs((int)w.objects[k].x - px) <= 45 && std::abs((int)w.objects[k].y - py) <= 45) {
					moved.push_back({k, {w.objects[k].x, w.objects[k].y}});
					w.moveObject(k, px + 60, py + 60, 0);
				}
			}
		};
		sendAway();
		int tent = put(w, 0x6A, 0, px + 1, py, 0);
		int me = g.actingActor();
		w.actors.level[me] = 1;
		w.actors.exp[me] = 150;
		int str = w.actors.strength[me], dex = w.actors.dexterity[me];
		g.gameClock.hour = 22;
		g.log.clear();
		useEast(w, g, px, py);
		std::string text = logText(g);
		if (text.find("Rest until sunrise?") == std::string::npos)
			std::printf("tent log: [%s]\n", text.c_str());
		CHECK(text.find("Rest until sunrise?") != std::string::npos);
		int pieces = 0;
		for (size_t i = 0; i < w.objects.size(); ++i)
			if (!w.objects[i].deleted && w.objects[i].type == 0x86)
				++pieces;
		CHECK(pieces == 5);
		g.log.clear();
		g.handleKey('y'); // the sleep: the Avatar has a dream
		text = logText(g);
		CHECK(text.find("has a dream") != std::string::npos && text.find("Which (B H S)") != std::string::npos);
		CHECK(w.actors.level[me] == 2);
		g.handleKey('x'); // (not a choice)
		CHECK(w.actors.strength[me] == str);
		g.handleKey('S');
		CHECK(w.actors.strength[me] == str + 1 && w.actors.dexterity[me] == dex);
		pieces = 0;
		for (size_t i = 0; i < w.objects.size(); ++i)
			if (!w.objects[i].deleted && w.objects[i].type == 0x86)
				++pieces;
		CHECK(pieces == 0 && !w.objects[tent].deleted);
		CHECK(g.gameClock.hour == 7 || g.gameClock.hour == 8);
		// poisoned: asks; "n" ends without a camp. With the third psychic power nobody can sleep.
		sendAway();
		w.actors.status[me] |= 8;
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("is poisoned!") != std::string::npos && logText(g).find("Do you really want to sleep anyway?") != std::string::npos);
		g.handleKey('n');
		pieces = 0;
		for (size_t i = 0; i < w.objects.size(); ++i)
			if (!w.objects[i].deleted && w.objects[i].type == 0x86)
				++pieces;
		CHECK(pieces == 0);
		w.actors.status[me] &= 0xF7;
		g.mdSetPsychic(me, 2, 3);
		sendAway();
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("complains of too much light.") != std::string::npos && logText(g).find("Nobody can sleep.") != std::string::npos);
		g.mdSetPsychic(me, 2, 0);
		w.deleteObject(tent);
		w.actors.strength[me] = (uint8_t)str;
		for (size_t i = moved.size(); i-- > 0;)
			w.moveObject(moved[i].first, moved[i].second.first, moved[i].second.second, 0);
	}

	// more recipes: air (74), headgear (52), belt rollers (46), fire (19), the mirror (69)
	{
		auto clearArea = [&]() {
			std::vector<int> here;
			for (int sx = px - 2; sx <= px + 5; ++sx)
				for (int sy = py - 1; sy <= py + 2; ++sy) {
					here.clear();
					w.objectsAt(sx, sy, 0, here);
					for (int i : here)
						if (w.objects[i].actor < 0)
							w.deleteObject(i);
				}
		};
		auto useOnSouth = [&]() { // the item east of the party, the partner south of it
			g.log.clear();
			g.handleKey('u');
			g.handleKey(KEY_RIGHT);
			g.handleKey(KEY_DOWN);
			g.handleKey(KEY_ENTER);
		};
		clearArea();
		int me = g.actingActor();
		// air on a thing that does not breathe, on the body of nobody who can be saved, on the body of a member
		int air = put(w, 0x144, 0, px + 1, py, 0);
		w.objects[air].quantity = 3;
		int barrel = put(w, 0x68, 0, px, py + 1, 0);
		useOnSouth();
		CHECK(logText(g).find("doesn't need oxygenated air") != std::string::npos && w.objects[air].quantity == 3);
		w.deleteObject(barrel);
		int body = put(w, 0x155, 0, px, py + 1, 0);
		w.objects[body].quality = 0x30;
		useOnSouth();
		CHECK(logText(g).find("is beyond help") != std::string::npos && w.objects[body].deleted && w.objects[air].quantity == 2);
		body = put(w, 0x155, 0, px, py + 1, 0);
		w.objects[body].quality = 3;
		int mate = w.actors.partyRoster[2];
		CHECK(mate == 3);
		w.actors.status[mate] |= 0x10;
		w.actors.hp[mate] = 0;
		useOnSouth();
		CHECK(logText(g).find("is revived!") != std::string::npos && !(w.actors.status[mate] & 0x10) && w.actors.hp[mate] > 0 && w.objects[body].deleted && w.objects[air].quantity == 1);
		w.deleteObject(air);
		clearArea();
		// headgear on a chair
		int head = put(w, 0x128, 0, px + 1, py, 0);
		int chair = put(w, 0x120, 0, px, py + 1, 0);
		w.actors.talkFlags[0x74] = 1;
		w.actors.talkFlags[0x60] = 0;
		size_t before = w.objects.size();
		useOnSouth();
		CHECK(logText(g).find("The headgear is installed") != std::string::npos && w.objects[head].deleted && w.objects.size() == before + 1 && w.objects.back().type == 0x64 && w.objects.back().frame == 1);
		CHECK((w.actors.talkFlags[0x74] & 0x10) && (w.actors.talkFlags[0x60] & 4));
		w.deleteObject(w.objects.size() - 1);
		w.deleteObject(chair);
		clearArea();
		// rollers: a row of six on the south row, the first with frame 0: all turn into 0xBC and the fifth one gets a 0xBD
		int part = put(w, 0x90, 0, px + 1, py, 0);
		int rollers[6];
		for (int i = 0; i < 6; ++i)
			rollers[i] = put(w, 0xC0, i == 0 ? 0 : 1, px - 1 + i, py + 1, 0);
		before = w.objects.size();
		useOnSouth();
		bool all = true;
		for (int i = 0; i < 6; ++i)
			all &= w.objects[rollers[i]].type == 0xBC;
		CHECK(all && w.objects[part].deleted && w.objects.size() == before + 1 && w.objects.back().type == 0xBD && w.objects.back().x == px + 3 && (w.actors.talkFlags[0x72] & 4));
		for (int i = 0; i < 6; ++i)
			w.deleteObject(rollers[i]);
		w.deleteObject(w.objects.size() - 1);
		clearArea();
		// rollers that are all not whole
		part = put(w, 0x90, 0, px + 1, py, 0);
		int r1 = put(w, 0xC0, 1, px, py + 1, 0);
		useOnSouth();
		CHECK(logText(g).find("These rollers can never be fixed") != std::string::npos && w.objects[r1].type == 0xC0 && !w.objects[part].deleted);
		w.deleteObject(r1);
		w.deleteObject(part);
		clearArea();
		// fire on a book (0x94) and on a thing that does not burn
		int torch = put(w, 0x72, 0, px + 1, py, 0);
		int book = put(w, 0x94, 0, px, py + 1, 0);
		useOnSouth();
		CHECK(logText(g).find("You incinerated the") != std::string::npos && w.objects[book].deleted);
		int rock = put(w, 0x1BA, 0, px, py + 1, 0);
		useOnSouth();
		CHECK(logText(g).find("It has no effect") != std::string::npos && !w.objects[rock].deleted);
		w.deleteObject(rock);
		w.deleteObject(torch);
		// the potash puzzle: a bottle (0xEB, frame 0: potash) on a mound (0xFB) over a seed (0xFC); each compound once; acid destroys the seed
		int seed = put(w, 0xFC, 0, px, py + 1, 0);
		int mound = put(w, 0xFB, 0, px, py + 1, 0);
		auto pourBottle = [&](int frame) {
			int bottle = put(w, 0xEB, frame, px + 1, py, 0);
			useOnSouth();
			return bottle;
		};
		int bottle = pourBottle(0);
		CHECK(logText(g).find("You pour the potash mixture onto the mound") != std::string::npos && (w.objects[mound].quality & 4) && w.objects[bottle].deleted);
		bottle = pourBottle(0);
		CHECK(logText(g).find("It has no effect") != std::string::npos && w.objects[mound].quality == 4 && w.objects[bottle].deleted);
		bottle = pourBottle(1);
		bottle = pourBottle(3);
		CHECK(w.objects[mound].quality == 7 && !w.objects[seed].deleted);
		bottle = pourBottle(2);
		CHECK(logText(g).find("The seed is destroyed by acid") != std::string::npos && w.objects[seed].deleted && w.objects[mound].type == 0x102 && w.objects[mound].quality == 0);
		w.deleteObject(mound);
		clearArea();
		// a shovel (0x42, recipe 10) on a pod plant (0xFB): a pile goes beside the member; with a hole under the plant it is uprooted
		int shovel = put(w, 0x42, 0, px + 1, py, 0);
		int plant = put(w, 0xFB, 2, px, py + 1, 0);
		size_t objects = w.objects.size();
		useOnSouth();
		CHECK(logText(g).find("It has no effect") != std::string::npos && !w.objects[plant].deleted && w.objects.size() == objects + 1 && w.objects.back().type == 0x102);
		w.deleteObject(w.objects.size() - 1);
		int hole = put(w, 0x101, 0, px, py + 1, 0);
		plant = put(w, 0xFB, 2, px, py + 1, 0);
		useOnSouth();
		CHECK(logText(g).find("You uprooted the pod plant") != std::string::npos && w.objects[plant].deleted);
		w.deleteObject(hole);
		w.deleteObject(shovel);
		clearArea();
		// the mirror away from its place
		int mirror = put(w, 0x93, 0, px + 1, py, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("You see yourself") != std::string::npos);
		w.deleteObject(mirror);
		(void)me;
	}

	// the sextant (0x60, recipe 33) on the surface: (y - 512) * 240 / 1024 = 27 south, (x - 512) * 360 / 1024 gives east
	{
		std::vector<int> here;
		w.objectsAt(px + 1, py, 0, here);
		for (int i : here)
			if (w.objects[i].actor < 0)
				w.deleteObject(i);
		int sextant = put(w, 0x60, 0, px + 1, py, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("You are somewhere near") != std::string::npos);
		CHECK(logText(g).find("S") != std::string::npos);
		w.deleteObject(sextant);
	}

	// Look at a lit torch with 12 minutes left, and at a pocket watch
	{
		std::vector<int> here;
		w.objectsAt(px + 1, py, 0, here);
		for (int i : here)
			if (w.objects[i].actor < 0)
				w.deleteObject(i);
		int torch = put(w, 0x6E, 0, px + 1, py, 0);
		w.objects[torch].quality = 12;
		g.log.clear();
		g.handleKey('l');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("It has 12 more minutes of ") != std::string::npos);
		CHECK(logText(g).find("wick.") != std::string::npos);
		w.deleteObject(torch);
		int watch = put(w, 0x5B, 0, px + 1, py, 0);
		g.log.clear();
		g.handleKey('l');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("The time is ") != std::string::npos);
		w.deleteObject(watch);
	}

	// a thing with no recipe (type 0x1FE has none)
	int thing = put(w, 0x1FE, 0, px + 1, py, 0);
	(void)thing;
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(logText(g).find("can't be used") != std::string::npos || logText(g).find("nothing") != std::string::npos);

	std::printf(failures ? "FAILED\n" : "ok\n");
	return failures ? 1 : 0;
}
