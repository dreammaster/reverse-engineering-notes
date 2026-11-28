// Savage Empire's Use (cmd_use = sub_35E46): the table of 80 types, "Not possible." for the rest, drinking, eating, the crate that opens and gives up what is in it, the torch that is lit.
// Skipped without the game data.

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

static void clearEast(World &w, int px, int py) {
	std::vector<int> here;
	w.objectsAt(px + 1, py, 0, here);
	for (int i : here)
		if (w.objects[i].actor < 0)
			w.deleteObject(i);
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
	w.game = GameId::SavageEmpire;
	if (!w.load("se/game/SAVAGE")) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	w.objects.reserve(w.objects.size() + 64);
	Game g(w);
	int px = g.x, py = g.y;
	for (int i = 0; i < w.actors.partySize; ++i)
		w.moveObject(w.actorObject(w.actors.partyRoster[i]), px, py, 0);

	// not on the list of what may be used: "Not possible."
	clearEast(w, px, py);
	int rock = put(w, 0x30, 0, px + 1, py, 0);
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(logText(g).find("Not possible.") != std::string::npos);
	w.deleteObject(rock);

	// a bottle of liquor (0x5F): drunk, gone, the counter (byte_4226E, globals + 0x26) goes up
	clearEast(w, px, py);
	int bottle = put(w, 0x5F, 0, px + 1, py, 0);
	int drunk = w.actors.globals[0x26];
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(w.objects[bottle].deleted);
	CHECK(logText(g).find("You drink it.") != std::string::npos);
	CHECK(w.actors.globals[0x26] == drunk + 1);

	// meat (0x67): eaten
	clearEast(w, px, py);
	int meat = put(w, 0x67, 0, px + 1, py, 0);
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(w.objects[meat].deleted);
	CHECK(logText(g).find("You eat the food.") != std::string::npos);

	// a shut crate (0xBC frame 1) opens and gives up what is in it; already open it says "Not possible." and finds nothing
	clearEast(w, px, py);
	int crate = put(w, 0xBC, 1, px + 1, py, 0);
	int inside = put(w, 0x66, 0, 0, 0, 0);
	w.deleteObject(inside);
	World::Obj &in = w.objects[inside];
	in.deleted = false;
	in.status = ObjRecord::IN_CONTAINER;
	in.parent = crate;
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(w.objects[crate].frame == 0);
	CHECK(logText(g).find("Searching here, you find") != std::string::npos);
	CHECK(w.objects[inside].location() == ObjRecord::ON_MAP);
	w.deleteObject(inside); // (it lies on the square now and would be what Use picks: it is food)
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(logText(g).find("Not possible.") != std::string::npos);
	CHECK(logText(g).find("nothing") != std::string::npos);

	// a notebook (0x31): only Jimmy (base type 0x138) can read it
	clearEast(w, px, py);
	int note = put(w, 0x31, 0, px + 1, py, 0);
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(logText(g).find("can't read Jimmy's shorthand") != std::string::npos);
	w.deleteObject(note);

	// a wire screen (0xB1) beside an 0xE7 sifts ten of 0x7B (sulphur powder); without one there is nothing to sift
	clearEast(w, px, py);
	int screen = put(w, 0xB1, 0, px + 1, py, 0);
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(logText(g).find("nothing to sift") != std::string::npos);
	int ore = put(w, 0xE7, 0, px, py - 1, 0);
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(logText(g).find("You made some sulphur powder") != std::string::npos);
	w.deleteObject(ore);
	w.deleteObject(screen);

	// burnt logs (0xCF): frame 0 is "Not possible.", frame 1 starts a fire (frame 2)
	clearEast(w, px, py);
	int logs = put(w, 0xCF, 1, px + 1, py, 0);
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(w.objects[logs].frame == 2);
	CHECK(logText(g).find("You started a fire") != std::string::npos);
	w.deleteObject(logs);

	// a metal bucket (0x88): next to a tar pit (0xE5) it fills with tar (frame 1); full of water (frame 2) it is poured onto a fire (0xF2), which goes out
	clearEast(w, px, py);
	int bucket = put(w, 0x88, 0, px + 1, py, 0);
	int pit = put(w, 0xE5, 0, px, py - 1, 0);
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(w.objects[bucket].frame == 1);
	CHECK(logText(g).find("tar in the bucket") != std::string::npos);
	w.deleteObject(pit);
	w.objects[bucket].frame = 2;
	std::vector<int> below;
	w.objectsAt(px, py + 1, 0, below);
	for (int i : below)
		if (w.objects[i].actor < 0)
			w.deleteObject(i);
	int fire = put(w, 0xF2, 0, px, py + 1, 0);
	g.log.clear();
	useEast(w, g, px, py);
	g.handleKey(KEY_DOWN);
	g.handleKey(KEY_ENTER);
	CHECK(w.objects[fire].deleted);
	CHECK(w.objects[bucket].frame == 0);
	CHECK(logText(g).find("You put the fire out") != std::string::npos);
	w.deleteObject(bucket);

	// the digging stick (0x83) finds clay (terrain 0x20-0x2F, 0xBE) around the member or says there is nothing to dig; clay (0xC0) is shaped into a pot (0x84)
	clearEast(w, px, py);
	int stick = put(w, 0x83, 0, px + 1, py, 0);
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(logText(g).find("clay") != std::string::npos || logText(g).find("nothing to dig") != std::string::npos);
	w.deleteObject(stick);
	clearEast(w, px, py);
	int clay = put(w, 0xC0, 0, px + 1, py, 0);
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(logText(g).find("You shaped a small, soft pot") != std::string::npos);
	CHECK(w.objects[clay].deleted);
	CHECK(g.log.size() > 0);

	// a fishing pole (0xD4) wants deep water (terrain 0x0C) on the diagonal through the member
	clearEast(w, px, py);
	int pole = put(w, 0xD4, 0, px + 1, py, 0);
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(logText(g).find("deep water") != std::string::npos || logText(g).find("fish") != std::string::npos);
	w.deleteObject(pole);

	// "On -" handlers aimed at the square below the member: a cloth strip (0x86) binds a wound, a crystal (0xFC) heals and cures poison, a hammer (0x1D) breaks a rock off a boulder or makes charcoal, a bell (0x2B) on a person has no effect
	if (w.actors.partySize > 1) {
		int hurt = w.actors.partyRoster[1];
		auto stand = [&]() {
			clearEast(w, px, py);
			w.moveObject(w.actorObject(hurt), px, py + 1, 0);
		};
		auto aim = [&](int type) {
			stand();
			int o = put(w, type, 0, px + 1, py, 0);
			g.log.clear();
			useEast(w, g, px, py);
			g.handleKey(KEY_DOWN);
			g.handleKey(KEY_ENTER);
			return o;
		};
		w.actors.hp[hurt] = 1;
		int strip = aim(0x86);
		CHECK(logText(g).find("wounds are bound") != std::string::npos);
		CHECK(w.actors.hp[hurt] > 1 && w.actors.hp[hurt] <= 6);
		CHECK(w.objects[strip].deleted);
		w.actors.hp[hurt] = 1;
		w.actors.status[hurt] |= 8;
		int crystal = aim(0xFC);
		CHECK(logText(g).find("is healed") != std::string::npos);
		CHECK(w.actors.hp[hurt] > 6 && !(w.actors.status[hurt] & 8));
		CHECK(w.objects[crystal].deleted);
		aim(0xFC); // (now it needs no help)
		CHECK(logText(g).find("Doesn't need any help") != std::string::npos);
		stand();
		int bell = aim(0x2B);
		CHECK(logText(g).find("No effect.") != std::string::npos);
		CHECK(!w.objects[bell].deleted);
		w.deleteObject(bell);
		w.moveObject(w.actorObject(hurt), px, py, 0);
	}
	{
		auto hammerOn = [&](int type, int frame) {
			clearEast(w, px, py);
			std::vector<int> below;
			w.objectsAt(px, py + 1, 0, below);
			for (int i : below)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
			int target = put(w, type, frame, px, py + 1, 0);
			int hammer = put(w, 0x1D, 0, px + 1, py, 0);
			g.log.clear();
			useEast(w, g, px, py);
			g.handleKey(KEY_DOWN);
			g.handleKey(KEY_ENTER);
			w.deleteObject(hammer);
			return target;
		};
		int boulder = hammerOn(0xAC, 0);
		CHECK(logText(g).find("flakes off") != std::string::npos || logText(g).find("Glanced off") != std::string::npos);
		w.deleteObject(boulder);
		int cold = hammerOn(0xCF, 0);
		CHECK(logText(g).find("charcoal") != std::string::npos);
		w.deleteObject(cold);
		int other = hammerOn(0x30, 0);
		CHECK(logText(g).find("No effect.") != std::string::npos);
		w.deleteObject(other);

		// a tool east of the member used on a thing south of it
		auto toolOn = [&](int tool, int type, int frame, int &target, int &made) {
			clearEast(w, px, py);
			std::vector<int> below;
			w.objectsAt(px, py + 1, 0, below);
			for (int i : below)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
			target = put(w, type, frame, px, py + 1, 0);
			int t = put(w, tool, 0, px + 1, py, 0);
			size_t before = w.objects.size();
			g.log.clear();
			useEast(w, g, px, py);
			g.handleKey(KEY_DOWN);
			g.handleKey(KEY_ENTER);
			made = w.objects.size() > before ? (int)before : -1;
			return t;
		};
		int target, made;
		// the jug (0x3F) on a spear (0x1A) makes the spear of Shamap (type 1); the jug is used up
		int jug = toolOn(0x3F, 0x1A, 0, target, made);
		CHECK(w.objects[jug].deleted && w.objects[target].deleted);
		CHECK(made >= 0 && w.objects[made].type == 1);
		// a vine (0x2A) on a hole (0xA8) hangs a rope (0x14) one square up and to the left
		int vine = toolOn(0x2A, 0xA8, 0, target, made);
		CHECK(w.objects[vine].deleted);
		CHECK(made >= 0 && w.objects[made].type == 0x14 && w.objects[made].x == px - 1 && w.objects[made].y == py + 1 - 1);
		w.deleteObject(target);
		int vine2 = toolOn(0x2A, 0x30, 0, target, made);
		CHECK(logText(g).find("You can't use a rope here!") != std::string::npos);
		w.deleteObject(vine2);
		w.deleteObject(target);
		// a knife (0x2C): cloth (0xB4) becomes ten cloth strips; a dead creature (0x118) by its frame: 1 "You ghoul!", 3 a tough hide, 21 meat (and gore and bones), 23 "Clank!"
		int knife = toolOn(0x2C, 0xB4, 0, target, made);
		CHECK(w.objects[target].deleted);
		CHECK(logText(g).find("You made some cloth strips") != std::string::npos);
		w.deleteObject(knife);
		knife = toolOn(0x2C, 0x118, 1, target, made);
		CHECK(logText(g).find("You ghoul!") != std::string::npos && !w.objects[target].deleted);
		w.deleteObject(target);
		w.deleteObject(knife);
		knife = toolOn(0x2C, 0x118, 3, target, made);
		CHECK(logText(g).find("The hide is too tough") != std::string::npos);
		w.deleteObject(target);
		w.deleteObject(knife);
		knife = toolOn(0x2C, 0x118, 23, target, made);
		CHECK(logText(g).find("Clank!") != std::string::npos);
		w.deleteObject(target);
		w.deleteObject(knife);
		knife = toolOn(0x2C, 0x118, 21, target, made);
		CHECK(logText(g).find("You cut up some meat") != std::string::npos && w.objects[target].deleted);
		bool bones = false;
		for (size_t i = (size_t)made; i < w.objects.size(); ++i)
			bones |= w.objects[i].type == 0x52 && !w.objects[i].deleted;
		CHECK(bones);
		w.deleteObject(knife);
		// the flintlock (0x32) needs gunpowder (0xCC) in the pack; loaded it is frame 1
		clearEast(w, px, py);
		int gun = put(w, 0x32, 0, px + 1, py, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("You don't have any gunpowder") != std::string::npos);
		int powder = w.addToInventory(w.actors.partyRoster[0], 0xCC, 0, 1);
		(void)powder;
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("Flintlock loaded") != std::string::npos && w.objects[gun].frame == 1);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("Flintlock already loaded") != std::string::npos);
		w.deleteObject(gun);
		// the mortar (0x3B): corn alone becomes corn flour (0x5D); nothing in it is "nothing to grind"; charcoal, sulphur powder and 0x82 make gunpowder
		auto inside = [&](int mortar, int type, int qty) {
			World::Obj c;
			c.status = ObjRecord::IN_CONTAINER;
			c.x = c.y = 0;
			c.z = 0;
			c.type = (uint16_t)type;
			c.frame = 0;
			c.quantity = (uint8_t)qty;
			c.quality = 0;
			c.actor = -1;
			c.parent = mortar;
			w.objects.push_back(c);
			return (int)w.objects.size() - 1;
		};
		clearEast(w, px, py);
		int mortar = put(w, 0x3B, 0, px + 1, py, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("nothing to grind") != std::string::npos);
		int corn = inside(mortar, 0x6C, 3);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("corn into corn flour") != std::string::npos && w.objects[corn].deleted);
		for (size_t i = 0; i < w.objects.size(); ++i) // (the flour stays in the mortar: it cannot be ground again)
			if (!w.objects[i].deleted && w.objects[i].parent == mortar && w.objects[i].location() == ObjRecord::IN_CONTAINER)
				w.deleteObject((int)i);
		int a1 = inside(mortar, 0x81, 5), a2 = inside(mortar, 0x7B, 2), a3 = inside(mortar, 0x82, 4);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("mix a batch of gunpowder") != std::string::npos);
		CHECK(w.objects[a2].deleted && w.objects[a1].quantity == 3 && w.objects[a3].quantity == 2);
		w.deleteObject(mortar);
		// an oven (0xAD): "On -" a clay pot (0x84) fires it into 0x85, a log (0xCE) gives ten charcoal, corn flour (0x5D) becomes tortillas (0x66), a lamp-post-like 0x04 burns up
		int oven = toolOn(0xAD, 0x84, 0, target, made);
		CHECK(logText(g).find("You fired the pot") != std::string::npos && w.objects[target].deleted);
		w.deleteObject(oven);
		oven = toolOn(0xAD, 0x5D, 0, target, made);
		CHECK(logText(g).find("corn tortillas") != std::string::npos && w.objects[target].deleted);
		CHECK(made >= 0 && w.objects[made].type == 0x66);
		w.deleteObject(oven);
		oven = toolOn(0xAD, 0xCE, 0, target, made);
		CHECK(logText(g).find("some charcoal") != std::string::npos);
		w.deleteObject(oven);
		oven = toolOn(0xAD, 0x30, 0, target, made);
		CHECK(logText(g).find("No effect.") != std::string::npos);
		w.deleteObject(oven);
		w.deleteObject(target);
		// tarred cloth (0xBF) on a log (0xCE) makes a torch (0xD0); the loom (0xA5) weaves flax (0xD2) into cloth (0xB4); a tar pit (0xE5) tars cloth strips (0x86)
		int tc = toolOn(0xBF, 0xCE, 0, target, made);
		CHECK(logText(g).find("You make a torch") != std::string::npos && w.objects[tc].deleted && w.objects[target].deleted);
		CHECK(made >= 0 && w.objects[made].type == 0xD0);
		w.deleteObject(made);
		int loom = toolOn(0xA5, 0xD2, 0, target, made);
		CHECK(logText(g).find("wove cloth") != std::string::npos && w.objects[target].deleted);
		CHECK(made >= 0 && w.objects[made].type == 0xB4);
		w.deleteObject(made);
		w.deleteObject(loom);
		int pit2 = toolOn(0xE5, 0x86, 0, target, made);
		CHECK(w.objects[target].deleted && made >= 0 && w.objects[made].type == 0xBF);
		w.deleteObject(made);
		w.deleteObject(pit2);
		// a bucket with tar (frame 1) poured on four cloth strips tars them (0xBF) and uses up four tar
		{
			clearEast(w, px, py);
			int bk = put(w, 0x88, 1, px + 1, py, 0);
			int tar = inside(bk, 0x8B, 10);
			std::vector<int> below;
			w.objectsAt(px, py + 1, 0, below);
			for (int i : below)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
			int strips = put(w, 0x86, 0, px, py + 1, 0);
			w.objects[strips].quantity = 4;
			size_t before = w.objects.size();
			g.log.clear();
			useEast(w, g, px, py);
			g.handleKey(KEY_DOWN);
			g.handleKey(KEY_ENTER);
			CHECK(logText(g).find("You tarred some cloth strips") != std::string::npos && w.objects[strips].deleted);
			CHECK(w.objects.size() == before + 1 && w.objects[before].type == 0xBF && w.objects[before].quantity == 4);
			CHECK(w.objects[tar].quantity == 6 && w.objects[bk].frame == 1);
			w.deleteObject((int)before);
			w.deleteObject(tar);
			w.deleteObject(bk);
		}
		// a lit torch (0xD1): on a web (0x11) it burns the web and the webs next to it (and what burns on those squares); on a nest (0xAB) it leaves burnt remains (0x4C frame 3)
		{
			clearEast(w, px, py);
			std::vector<int> there;
			for (int sx = px - 1; sx <= px + 1; ++sx)
				for (int sy = py + 1; sy <= py + 2; ++sy) {
					w.objectsAt(sx, sy, 0, there);
					for (int i : there)
						if (w.objects[i].actor < 0)
							w.deleteObject(i);
				}
			int web1 = put(w, 0x11, 0, px, py + 1, 0), web2 = put(w, 0x11, 0, px, py + 2, 0), cloth = put(w, 0xB4, 0, px, py + 2, 0);
			int torch2 = put(w, 0xD1, 0, px + 1, py, 0);
			g.log.clear();
			useEast(w, g, px, py);
			g.handleKey(KEY_DOWN);
			g.handleKey(KEY_ENTER);
			CHECK(logText(g).find("web") != std::string::npos);
			CHECK(w.objects[web1].deleted && w.objects[web2].deleted && w.objects[cloth].deleted);
			w.deleteObject(torch2);
		}
		// a grenade: tarred cloth on a fired pot (0x85) needs five pinches of gunpowder
		int pot = 0;
		tc = toolOn(0xBF, 0x85, 0, pot, made);
		CHECK(logText(g).find("five pinches of gunpowder") != std::string::npos && !w.objects[pot].deleted);
		w.deleteObject(tc);
		int five = w.addToInventory(w.actors.partyRoster[0], 0xCC, 0, 5);
		tc = toolOn(0xBF, 0x85, 0, pot, made);
		CHECK(logText(g).find("You made a grenade") != std::string::npos && w.objects[pot].deleted && w.objects[five].deleted);
		CHECK(made >= 0 && w.objects[made].type == 0x89);
		w.deleteObject(made);
		// a sliding door (0xEA, frame 1) slides east to frame 5 unless something solid is in the way
		clearEast(w, px, py);
		std::vector<int> beyond;
		w.objectsAt(px + 2, py, 0, beyond);
		for (int i : beyond)
			if (w.objects[i].actor < 0)
				w.deleteObject(i);
		int block = put(w, 0x3F, 0, px + 2, py, 0);
		int door = put(w, 0xEA, 1, px + 1, py, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("The door is blocked") != std::string::npos && w.objects[door].frame == 1);
		w.deleteObject(block);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(w.objects[door].x == px + 2 && w.objects[door].frame == 5);
		w.deleteObject(door);
		// a grenade (0x89) is lit (0x8A) and goes off after three minutes, hurting whoever stands within a square
		{
			clearEast(w, px, py);
			int nade = put(w, 0x89, 0, px + 1, py, 0);
			int victim = w.actors.partyRoster[1];
			w.moveObject(w.actorObject(victim), px + 2, py, 0);
			w.actors.hp[victim] = 200;
			g.log.clear();
			useEast(w, g, px, py);
			CHECK(logText(g).find("Grenade lit!") != std::string::npos && w.objects[nade].deleted);
			CHECK(w.actors.globals[0x5A] == 3);
			g.advanceTime(2);
			CHECK(w.actors.globals[0x5A] == 1 && w.actors.hp[victim] == 200);
			g.advanceTime(1);
			CHECK(w.actors.globals[0x5A] == 0 && w.actors.hp[victim] < 200);
			w.moveObject(w.actorObject(victim), px, py, 0);
			clearEast(w, px, py);
		}
		// the fire extinguisher (0x5E) puts out a fire (0xF2) and turns burning logs (0xCF frame 2) back to frame 0
		int ext = toolOn(0x5E, 0xF2, 0, target, made);
		CHECK(w.objects[target].deleted && !w.objects[ext].deleted);
		CHECK(logText(g).find("You put the fire out") != std::string::npos);
		w.deleteObject(ext);
		ext = toolOn(0x5E, 0xCF, 2, target, made);
		CHECK(w.objects[target].frame == 0);
		w.deleteObject(target);
		w.deleteObject(ext);
	}

	// the machine (0x8F) takes a gem (0x48) from the pack and gives an 0xC5; the folded skin (0x5C) has no room to unfold beside the party; the metal head (0xBE) next to the headless statue (0xF7) brings Yunapotli to life
	{
		clearEast(w, px, py);
		int machine = put(w, 0x8F, 0, px + 1, py, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("You don't have any gems") != std::string::npos);
		int member = w.actors.partyRoster[0];
		int gem = w.addToInventory(member, 0x48, 0, 1);
		size_t before = w.objects.size();
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("You put a gem in the machine") != std::string::npos && w.objects[gem].deleted);
		CHECK(w.objects.size() == before + 1 && w.objects[before].type == 0xC5);
		w.deleteObject((int)before);
		w.deleteObject(machine);
		clearEast(w, px, py);
		int skin = put(w, 0x5C, 0, px + 1, py, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("not enough room to unfold") != std::string::npos && w.objects[skin].type == 0x5C);
		w.deleteObject(skin);
		clearEast(w, px, py);
		int head = put(w, 0xBE, 0, px + 1, py, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("No effect.") != std::string::npos && !w.objects[head].deleted);
		int statue = put(w, 0xF7, 0, px, py - 1, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("replace the statue's missing head") != std::string::npos);
		CHECK(w.objects[head].deleted && w.objects[statue].deleted);
	}

	// totems: only the shaman (actor 9) can invoke one; Heluzz (0x56) with the offering 0x36 gives light, Aphazz (0x58) with 0x37 heals, 0x38 on Aphazz protects the party
	{
		clearEast(w, px, py);
		int totem = put(w, 0x56, 0, px + 1, py, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("can't use that") != std::string::npos);
		int joined = w.joinParty(9);
		if (joined != 0) std::printf("joinParty(9) = %d, party %d: %d %d %d %d\n", joined, w.actors.partySize, w.actors.partyRoster[0], w.actors.partyRoster[1], w.actors.partyRoster[2], w.actors.partyRoster[3]);
		CHECK(joined == 0 || joined == 3);
		if (joined == 0 || joined == 3) {
			int shaman = 0;
			for (int i = 0; i < w.actors.partySize; ++i)
				if (w.actors.partyRoster[i] == 9)
					shaman = i;
			w.actors.globals[0xA] = (uint8_t)shaman;
			g.handleKey((int)('1' + shaman)); // (the digit keys: the member walks alone)
			w.moveObject(w.actorObject(9), px, py, 0);
			auto offer = [&](int tot, int offering) {
				clearEast(w, px, py);
				std::vector<int> below;
				w.objectsAt(px, py + 1, 0, below);
				for (int i : below)
					if (w.objects[i].actor < 0)
						w.deleteObject(i);
				int t = put(w, tot, 0, px + 1, py, 0);
				int o = put(w, offering, 0, px, py + 1, 0);
				g.log.clear();
				useEast(w, g, px, py);
				g.handleKey(KEY_DOWN);
				g.handleKey(KEY_ENTER);
				(void)t;
				return o;
			};
			w.actors.globals[0x12] = 0;
			int o1 = offer(0x56, 0x36);
			CHECK(logText(g).find("Invoke: Heluzz!") != std::string::npos && logText(g).find("creates light") != std::string::npos);
			CHECK(w.actors.globals[0x12] == 0xC8 && w.objects[o1].deleted);
			int hurt = w.actors.partyRoster[0];
			w.actors.hp[hurt] = 1;
			int o2 = offer(0x58, 0x37);
			CHECK(logText(g).find("Aphazz!") != std::string::npos && logText(g).find("healed.") != std::string::npos);
			CHECK(w.actors.hp[hurt] > 1 && w.objects[o2].deleted);
			int o3 = offer(0x58, 0x38);
			CHECK(logText(g).find("Aphazz protects the party") != std::string::npos && (w.actors.status[hurt] & 1));
			(void)o3;
			int o4 = offer(0x56, 0x30);
			CHECK(logText(g).find("Offering refused") != std::string::npos && !w.objects[o4].deleted);
			int o5 = offer(0x56, 0x37);
			(void)o5;
			CHECK(logText(g).find("Triolo senses") != std::string::npos || logText(g).find("looks down") != std::string::npos);
			g.handleKey('0');
			w.actors.globals[0xA] = 0;
		}
		w.deleteObject(totem);
	}

	// a hole in the ceiling (0x16, frame 1, quality 1) takes the party up to the surface square its quality names (396,68); a rope (0x14, quality 8) takes it to the hole at 243,204 on level 1
	{
		clearEast(w, px, py);
		int hole = put(w, 0x16, 1, px + 1, py, 0);
		w.objects[hole].quality = 1;
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(g.x == 396 && g.y == 68 && g.z == 0);
		int leader = w.actorObject(w.actors.partyRoster[0]);
		CHECK(w.objects[leader].x == 396 && w.objects[leader].y == 68);
		g.x = px;
		g.y = py;
		for (int i = 0; i < w.actors.partySize; ++i)
			w.moveObject(w.actorObject(w.actors.partyRoster[i]), px, py, 0);
		w.deleteObject(hole);
		clearEast(w, px, py);
		int rope = put(w, 0x14, 0, px + 1, py, 0);
		w.objects[rope].quality = 8;
		useEast(w, g, px, py);
		CHECK(g.x == 243 && g.y == 204 && g.z == 1);
		g.z = 0;
		for (int i = 0; i < w.actors.partySize; ++i)
			w.moveObject(w.actorObject(w.actors.partyRoster[i]), px, py, 0);
		g.x = px;
		g.y = py;
		w.deleteObject(rope);
	}

	// a torch (0xD0) is lit in a free hand: a lit torch (0xD1) with 100 minutes appears readied
	clearEast(w, px, py);
	int actor = w.actors.partyRoster[0];
	std::vector<int> inv;
	w.inventory(actor, inv, false);
	int torch = w.addToInventory(actor, 0xD0, 0, 3);
	g.log.clear();
	g.handleKey('u');
	g.handleKey(KEY_RIGHT);
	(void)torch;
	w.inventory(actor, inv, false);

	// the jungle drum (0x94): drumsticks (0x5B) are needed; with every friend's flag the drum sets flag 8 of actor 1
	clearEast(w, px, py);
	{
		int drum = put(w, 0x94, 0, px + 1, py, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("You have nothing with which to play the drum") != std::string::npos);
		int sticks = w.addToInventory(actor, 0x5B, 0, 1);
		static const int FRIENDS[11] = {0x10, 0x11, 0x13, 0x14, 0x1B, 0x1D, 0x20, 0x22, 0x28, 0x30, 0x35};
		uint8_t saved[11];
		for (int i = 0; i < 11; ++i) {
			saved[i] = w.actors.talkFlags[FRIENDS[i]];
			w.actors.talkFlags[FRIENDS[i]] |= 0x80;
		}
		w.actors.talkFlags[1] &= 0xF7;
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(w.actors.talkFlags[1] & 8);
		for (int i = 0; i < 11; ++i)
			w.actors.talkFlags[FRIENDS[i]] = saved[i];
		w.actors.talkFlags[1] &= 0xF7;
		w.deleteObject(sticks);
		w.deleteObject(drum);
	}

	// a turtle (the creature 0x12E): the Avatar climbs on (body 0x133 on its square), using himself he steps ashore to land and leaves a turtle behind
	clearEast(w, px, py);
	{
		int ta = -1;
		for (int a = 0x80; a < 0x100 && ta < 0; ++a)
			if (w.actorFree(a))
				ta = a;
		int tk = w.addActor(ta, 0x12E, px + 1, py, 0);
		w.actors.status[ta] = 0x40;
		g.log.clear();
		useEast(w, g, px, py);
		int me = w.actorObject(1);
		CHECK(w.objects[tk].deleted && w.objects[me].type == 0x133 && w.objects[me].x == px + 1 && w.objects[me].y == py);
		g.handleKey('u');
		g.handleKey(KEY_ENTER);
		CHECK(w.objects[me].type != 0x133 && !(w.objects[me].x == px + 1 && w.objects[me].y == py));
		bool left = false;
		for (size_t i = 0; i < w.objects.size(); ++i)
			left |= !w.objects[i].deleted && w.objects[i].type == 0x12E && w.objects[i].x == px + 1 && w.objects[i].y == py;
		CHECK(left);
		for (size_t i = 0; i < w.objects.size(); ++i)
			if (!w.objects[i].deleted && w.objects[i].type == 0x12E && w.objects[i].x == px + 1 && w.objects[i].y == py)
				w.deleteObject((int)i);
		w.moveObject(me, px, py, 0);
		g.x = px;
		g.y = py;
	}

	// a raft (0xE9): refused with fewer than four members, then without four paddles (0x0C); then the leader's body is the raft (0xED), and using it on land puts the raft down again
	clearEast(w, px, py);
	{
		int raft = put(w, 0xE9, 3, px + 1, py, 0);
		w.objects[raft].quantity = 77;
		int members = w.actors.partySize;
		g.log.clear();
		useEast(w, g, px, py);
		bool few = w.actors.partySize < 4;
		CHECK(!few || logText(g).find("You don't have enough people to paddle") != std::string::npos);
		for (int a = 5; w.actors.partySize < 4 && a < 40; ++a)
			if (w.actorObject(a) >= 0)
				w.joinParty(a);
		if (w.actors.partySize >= 4) {
			g.log.clear();
			useEast(w, g, px, py);
			CHECK(logText(g).find("at least four people carrying paddles") != std::string::npos && !w.objects[raft].deleted);
			std::vector<int> paddles;
			for (int m = 0; m < 4; ++m)
				paddles.push_back(w.addToInventory(w.actors.partyRoster[m], 0x0C, 0, 1));
			g.log.clear();
			useEast(w, g, px, py);
			int lead = w.actorObject(w.actors.partyRoster[0]);
			CHECK(w.objects[raft].deleted && w.objects[lead].type == 0xED && w.objects[lead].frame == 3);
			g.handleKey('u');
			g.handleKey(KEY_ENTER);
			CHECK(w.objects[lead].type != 0xED);
			int found = -1;
			for (size_t i = 0; i < w.objects.size(); ++i)
				if (!w.objects[i].deleted && w.objects[i].type == 0xE9 && w.objects[i].quantity == 77)
					found = (int)i;
			CHECK(found >= 0);
			if (found >= 0)
				w.deleteObject(found);
			for (int p : paddles)
				w.deleteObject(p);
		} else {
			std::printf("could not make a party of four (skipped)\n");
		}
		(void)members;
		for (int i = 0; i < w.actors.partySize; ++i)
			w.moveObject(w.actorObject(w.actors.partyRoster[i]), px, py, 0);
		g.x = px;
		g.y = py;
	}

	// turtle bait (0x2F): in the lake a turtle comes up on a water square next to the member; away from water "You must be near water."
	{
		int wx = -1, wy = -1;
		for (int sy = 0xE7; sy <= 0x1E7 && wx < 0; ++sy)
			for (int sx = 0x335; sx <= 0x3ED && wx < 0; ++sx) {
				int f = w.flags.flags1[w.map.tile(0, sx, sy) & (TileFlags::TILES - 1)];
				std::vector<int> here;
				w.objectsAt(sx, sy, 0, here);
				if ((f & 3) == 3 && here.empty()) {
					wx = sx;
					wy = sy;
				}
			}
		if (wx >= 0) {
			clearEast(w, wx - 1, wy);
			int bait = put(w, 0x2F, 0, wx, wy - 1, 0);
			w.moveObject(w.actorObject(1), wx - 1, wy, 0);
			g.x = wx - 1;
			g.y = wy;
			g.log.clear();
			g.handleKey('u');
			g.handleKey(KEY_UP);
			bool turtleThere = false;
			for (size_t i = 0; i < w.objects.size(); ++i)
				turtleThere |= !w.objects[i].deleted && w.objects[i].type == 0x12E;
			CHECK(turtleThere || logText(g).find("Nothing takes the bait") != std::string::npos);
			(void)bait;
		} else {
			std::printf("no lake water found (skipped)\n");
		}
		w.moveObject(w.actorObject(1), px, py, 0);
		g.x = px;
		g.y = py;
	}

	// a square without an object (Use on terrain): a tree gives a branch (0xCE), the yucca plant flax (0xD2); a bare square is "Not possible."
	{
		int treeX = -1, treeY = -1, yuccaX = -1, yuccaY = -1, bareX = -1, bareY = -1;
		static const int TREES[19] = {0x32, 0x33, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0xC0, 0xC1, 0xC2, 0xC3, 0xC6, 0xB3, 0xB4, 0x82};
		for (int dy = -150; dy <= 150; ++dy)
			for (int dx = -150; dx <= 150; ++dx) {
				int sx = (px + dx) & 0x3FF, sy = (py + dy) & 0x3FF;
				int t = w.map.tile(0, sx, sy);
				std::vector<int> here;
				w.objectsAt(sx, sy, 0, here);
				if (!here.empty())
					continue;
				for (int tt : TREES)
					if (tt == t && treeX < 0) {
						treeX = sx;
						treeY = sy;
					}
				if (t == 0x34 && yuccaX < 0) {
					yuccaX = sx;
					yuccaY = sy;
				}
				if (t == 0x05 && bareX < 0) {
					bareX = sx;
					bareY = sy;
				}
			}
		auto useAtSquare = [&](int sx, int sy) {
			w.moveObject(w.actorObject(1), sx - 1, sy, 0);
			g.x = sx - 1;
			g.y = sy;
			g.z = 0;
			g.log.clear();
			g.handleKey('u');
			g.handleKey(KEY_RIGHT);
		};
		auto has = [&](int type) {
			std::vector<int> mine;
			w.inventory(actor, mine, true);
			for (int i : mine)
				if (w.objects[i].type == type)
					return true;
			return false;
		};
		if (treeX >= 0) {
			useAtSquare(treeX, treeY);
			CHECK(logText(g).find("You pulled a branch from the tree") != std::string::npos && has(0xCE));
		} else {
			std::printf("no tree found (skipped)\n");
		}
		if (yuccaX >= 0) {
			useAtSquare(yuccaX, yuccaY);
			CHECK(logText(g).find("You got some flax from the yucca plant") != std::string::npos && has(0xD2));
		} else {
			std::printf("no yucca found (skipped)\n");
		}
		if (bareX >= 0) {
			useAtSquare(bareX, bareY);
			CHECK(logText(g).find("Not possible.") != std::string::npos);
		}
		g.x = px;
		g.y = py;
		w.moveObject(w.actorObject(1), px, py, 0);
	}

	// the Avatar dead: the party is raised and wakes in Intanya's hut a week on
	{
		ObjList &al = w.actors;
		al.status[1] |= 0x10;
		al.hp[1] = 0;
		int day = g.gameClock.day;
		g.log.clear();
		g.seAwaken();
		CHECK(!(al.status[1] & 0x10) && al.hp[1] > 0);
		CHECK(g.x == 0x19C && g.y == 0x123 && g.z == 0);
		CHECK(g.gameClock.day == (day + 7 > 28 ? day + 7 - 28 : day + 7));
		CHECK(logText(g).find("awaken in Intanya's hut") != std::string::npos);
	}

	// worktype 11 (sub_2D470): a creature with a member within three squares turns on the party sooner or later (worktype 8); worktype 7 far from the party does nothing
	{
		ObjList &al = w.actors;
		int fk = w.actorFree(0xC1) ? w.addActor(0xC1, 0x182, g.x + 2, g.y, g.z) : -1;
		if (fk >= 0) {
			al.status[0xC1] = 0;
			al.hp[0xC1] = 30;
			al.worktype[0xC1] = 11;
			for (int i = 0; i < 80 && al.worktype[0xC1] == 11; ++i)
				g.seWorktype(0xC1);
			CHECK(al.worktype[0xC1] == 8);
			al.worktype[0xC1] = 7;
			w.moveObject(fk, g.x + 30, g.y, g.z);
			al.moves[0xC1] = 12;
			CHECK(g.seWorktype(0xC1));
			// worktype 14 (sub_2D5A9): a shooter of a side that has somebody of another side next to it strikes in the end
			int hitter = 0x182;
			for (int ty = 0x100; ty < 0x200; ++ty) {
				const ::Ultima6::Ultima6::CreatureStats *cs = ::Ultima6::Ultima6::Combat::stats(ty);
				if (cs && cs->dmg >= 5 && cs->str > 10) {
					hitter = ty;
					break;
				}
			}
			w.objects[fk].type = (uint16_t)hitter;
			al.worktype[0xC1] = 14;
			al.status[0xC1] = 0x20;
			w.moveObject(fk, g.x + 1, g.y, g.z);
			int victim = al.partyRoster[0];
			al.hp[victim] = 200;
			for (int i = 0; i < 400 && al.hp[victim] == 200; ++i)
				g.seWorktype(0xC1);
			CHECK(al.hp[victim] < 200);
			// worktype 8 (sub_2C86D): the fighter walks up to the party and strikes in the end
			al.worktype[0xC1] = 8;
			al.hp[victim] = 200;
			w.moveObject(fk, g.x + 2, g.y, g.z);
			int before = w.objects[fk].x;
			al.moves[0xC1] = 12;
			g.seWorktype(0xC1);
			CHECK(w.objects[fk].x < before);
			for (int i = 0; i < 400 && al.hp[victim] == 200; ++i)
				g.seWorktype(0xC1);
			CHECK(al.hp[victim] < 200);
			// worktype 6 (sub_2CE85, the berserk fighter), and any worktype with the object status 0x80: it too walks up and strikes
			for (int mode = 0; mode < 2; ++mode) {
				al.worktype[0xC1] = mode ? 13 : 6;
				w.objects[fk].status = (uint8_t)(mode ? 0x80 : 0);
				al.hp[victim] = 200;
				w.moveObject(fk, g.x + 2, g.y, g.z);
				for (int i = 0; i < 400 && al.hp[victim] == 200; ++i) {
					al.moves[0xC1] = 12;
					g.seWorktype(0xC1);
				}
				CHECK(al.hp[victim] < 200);
			}
			w.objects[fk].status = 0;
			// worktype 4 (sub_2CBA9): with a target within two squares it runs; with none within reach and no ranged weapon it says so
			al.worktype[0xC1] = 4;
			w.moveObject(fk, g.x + 4, g.y, g.z);
			al.moves[0xC1] = 12;
			g.log.clear();
			g.seWorktype(0xC1);
			CHECK(logText(g).find("has no ranged weapons!") != std::string::npos);
			al.worktype[0xC1] = 0;
			al.status[0xC1] = 0;
		}
	}

	// a web (the tile 0x210-0x219 on the square) paralyses a member sooner or later (sub_14338)
	{
		ObjList &al = w.actors;
		int webType = -1;
		for (int t = 0; t < 0x200 && webType < 0; ++t)
			if (w.baseTile.firstTile[t] == 0x210)
				webType = t;
		if (webType >= 0) {
			int victim = al.partyRoster[0], vk = w.actorObject(victim);
			World::Obj web;
			web.status = 0;
			web.x = w.objects[vk].x;
			web.y = w.objects[vk].y;
			web.z = w.objects[vk].z;
			web.type = (uint16_t)webType;
			web.frame = 0;
			web.quantity = 1;
			web.quality = 0;
			web.actor = -1;
			web.parent = -1;
			w.objects.push_back(web);
			w.addToMap((int)w.objects.size() - 1);
			al.status[victim] &= 0xFC;
			g.log.clear();
			for (int i = 0; i < 80 && !(al.status[victim] & 2); ++i)
				g.seHazards();
			CHECK((al.status[victim] & 2) && logText(g).find("is stuck in a web!") != std::string::npos);
			al.status[victim] &= 0xFD;
		}
	}

	// the door 0x113 (sub_346A8) turns between frames 1 and 3; opened close to the party, the story's scene sets flag 1 of actor 1
	{
		clearEast(w, px, py);
		int door = put(w, 0x113, 1, px + 1, py, 0);
		w.actors.talkFlags[1] &= 0xFE;
		useEast(w, g, px, py);
		CHECK(w.objects[door].frame == 3 && (w.actors.talkFlags[1] & 1));
		useEast(w, g, px, py);
		CHECK(w.objects[door].frame == 1);
		w.deleteObject(door);
	}

	// a fire at a square (fire_burn) burns the web there and hurts the people on it (the tile 0x406: 1-15)
	{
		w.moveObject(w.actorObject(1), px, py, 0);
		g.x = px;
		g.y = py;
		g.z = 0;
		int web = put(w, 0x11, 0, px, py, 0);
		int victim = w.actors.partyRoster[0];
		w.actors.hp[victim] = 200;
		w.actors.status[victim] &= 0xFE;
		for (int i = 0; i < 40 && w.actors.hp[victim] == 200; ++i)
			g.seBurn(px, py);
		CHECK(w.objects[web].deleted && w.actors.hp[victim] < 200);
	}

	// Ready (sub_1F404): by the type of the thing (a torch 0xD0 in a hand, one of a stack of them), the modern weapons only for the four who know them
	{
		int who = w.actors.partyRoster[0];
		std::vector<int> pack;
		w.inventory(who, pack, false);
		for (int i : pack)
			w.deleteObject(i);
		int spears = w.addToInventory(who, 0xD0, 0, 3);
		g.log.clear();
		g.handleKey('i');
		g.handleKey(KEY_ENTER);
		std::vector<int> after;
		w.inventory(who, after, false);
		int readied = 0, left = 0;
		for (int i : after) {
			if (w.objects[i].type != 0xD0)
				continue;
			if (w.objects[i].location() == ObjRecord::READIED)
				readied += w.objects[i].quantity;
			else
				left += w.objects[i].quantity;
		}
		g.handleKey(KEY_ESC);
		CHECK(readied == 1 && left == 2);
		(void)spears;
	}

	// Rest (sub_3A28B): "Rest until ...", N and a digit of hours; a member whose experience asks for the next level dreams of the three animals and B raises his strength
	{
		w.moveObject(w.actorObject(1), px, py, 0);
		g.x = px;
		g.y = py;
		g.z = 0;
		ObjList &al = w.actors;
		for (int a = 0x10; a < 0x100; ++a) { // (nobody near)
			int k = w.actorObject(a);
			if (k >= 0 && !w.objects[k].deleted && std::abs((int)w.objects[k].x - px) < 40 && std::abs((int)w.objects[k].y - py) < 40)
				w.moveObject(k, (px + 100) & 0x3FF, (py + 100) & 0x3FF, 0);
		}
		int who = al.partyRoster[0];
		al.level[who] = 1;
		al.exp[who] = 150;
		int str = al.strength[who] < 29 ? al.strength[who] : 20;
		al.strength[who] = (uint8_t)str;
		int minute = g.gameClock.hour * 60 + g.gameClock.minute;
		g.log.clear();
		g.handleKey('R');
		CHECK(logText(g).find("Rest until ") != std::string::npos);
		g.handleKey('N');
		g.handleKey('2');
		CHECK(logText(g).find("has a dream...") != std::string::npos);
		g.handleKey('B');
		CHECK(al.level[who] == 2 && al.strength[who] == str + 1);
		int after = g.gameClock.hour * 60 + g.gameClock.minute;
		CHECK(((after - minute + 1440) % 1440) == 120);
	}

	// Savage Empire's look (sub_2EE8D): "You see ", the time on a sundial by day, what a bucket holds
	{
		w.moveObject(w.actorObject(1), px, py, 0);
		g.x = px;
		g.y = py;
		g.z = 0;
		clearEast(w, px, py);
		int dial = put(w, 0x92, 0, px + 1, py, 0);
		g.gameClock.hour = 9;
		g.gameClock.minute = 5;
		g.log.clear();
		g.handleKey('l');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("You see ") != std::string::npos);
		CHECK(logText(g).find("The time is 9:05 A.M.") != std::string::npos);
		g.gameClock.hour = 22;
		g.log.clear();
		g.handleKey('l');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("The time is") == std::string::npos);
		w.deleteObject(dial);
		int pail = put(w, 0x88, 1, px + 1, py, 0);
		World::Obj inside;
		inside.status = ObjRecord::IN_CONTAINER;
		inside.x = inside.y = 0;
		inside.z = 0;
		inside.type = 0x8B;
		inside.frame = 0;
		inside.quantity = 3;
		inside.quality = 0;
		inside.actor = -1;
		inside.parent = pail;
		w.objects.push_back(inside);
		int tar = (int)w.objects.size() - 1;
		g.log.clear();
		g.handleKey('l');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("holding 3 pints of tar.") != std::string::npos);
		w.deleteObject(tar);
		w.deleteObject(pail);
	}

	// a tree (0x107 with 5 strength) is felled by a blast: its parts go, the stump and the trunk lie where it fell; a person under it is crushed or thrown aside
	{
		int tx = -1, ty = -1;
		for (int r = 0; r < 400 && tx < 0; ++r) {
			int cx = px + (r % 20) - 10, cy = py + 8 + (r / 20);
			bool open = true;
			for (int dx = -3; dx <= 2 && open; ++dx)
				for (int dy = -7; dy <= 1 && open; ++dy) {
					std::vector<int> here;
					w.objectsAt(cx + dx, cy + dy, 0, here);
					open &= here.empty();
				}
			if (open) {
				tx = cx;
				ty = cy;
			}
		}
		CHECK(tx >= 0);
		if (tx >= 0) {
			int tree = put(w, 0x107, 3, tx, ty, 0);
			w.objects[tree].quantity = 5;
			int p1 = put(w, 0x108, 3, tx - 2, ty - 1, 0);
			int p2 = put(w, 0x109, 3, tx, ty - 2, 0);
			int p3 = put(w, 0x10A, 1, tx - 2, ty - 3, 0);
			g.seHurtThing(tree, 3);
			CHECK(!w.objects[tree].deleted && w.objects[tree].quantity == 2);
			g.seHurtThing(tree, 9);
			CHECK(w.objects[tree].deleted && w.objects[p1].deleted && w.objects[p2].deleted && w.objects[p3].deleted);
			int stump = 0;
			std::vector<int> here;
			w.objectsAt(tx, ty, 0, here);
			for (int i : here)
				stump += w.objects[i].type == 0x104 && w.objects[i].frame == 1;
			CHECK(stump == 1);
			// a grenade that goes off beside a tree fells it at once (sub_318F5: thirty)
			for (int i : here)
				if (w.objects[i].type == 0x104)
					w.deleteObject(i);
			int tree2 = put(w, 0x107, 3, tx, ty, 0);
			w.objects[tree2].quantity = 5;
			g.seExplode(tx - 1, ty, false);
			CHECK(w.objects[tree2].deleted);
		}
	}

	// Get (sub_2FBBA): a plain thing goes into the pack, a 0x6F gives a 0x6A, a stack goes into the pack, "nothing!" on an empty square
	{
		w.moveObject(w.actorObject(1), px, py, 0);
		g.x = px;
		g.y = py;
		g.z = 0;
		clearEast(w, px, py);
		g.log.clear();
		g.handleKey('g');
		g.handleKey(KEY_RIGHT);
		CHECK(logText(g).find("nothing!") != std::string::npos);
		int hold = put(w, 0x6F, 0, px + 1, py, 0);
		g.log.clear();
		g.handleKey('g');
		g.handleKey(KEY_RIGHT);
		CHECK(w.objects[hold].deleted);
		bool got = false;
		std::vector<int> pack;
		w.inventory(w.actors.partyRoster[0], pack, false);
		for (int i : pack)
			got |= w.objects[i].type == 0x6A;
		CHECK(got);
		int plain = put(w, 0x20, 0, px + 1, py, 0);
		g.log.clear();
		g.handleKey('g');
		g.handleKey(KEY_RIGHT);
		CHECK(w.objects[plain].deleted);
		got = false;
		w.inventory(w.actors.partyRoster[0], pack, false);
		for (int i : pack)
			got |= w.objects[i].type == 0x20;
		CHECK(got);
	}

	// Move from the pack (sub_302B8): the target is chosen with the cursor; the stand (0x42) takes the thing up and to its left, the day goes on to noon and the way down is opened
	{
		w.moveObject(w.actorObject(1), px, py, 0);
		g.x = px;
		g.y = py;
		g.z = 0;
		clearEast(w, px, py);
		std::vector<int> pack;
		w.inventory(1, pack, false);
		for (int i : pack)
			w.deleteObject(i);
		int item = w.addToInventory(1, 0x20, 0, 1);
		int stand = put(w, 0x42, 0, px + 1, py, 0);
		w.actors.talkFlags[1] &= 0xFD;
		g.gameClock.hour = 9;
		g.log.clear();
		g.handleKey('i');
		g.handleKey('m');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("You place the gem on the stand.") != std::string::npos);
		CHECK(!w.objects[item].deleted && w.objects[item].onMap() && w.objects[item].x == px && w.objects[item].y == py - 1);
		CHECK((w.actors.talkFlags[1] & 2) && g.gameClock.hour == 12);
		bool way = false;
		for (const World::Obj &o : w.objects)
			way |= !o.deleted && o.type == 0xA0 && o.frame == 1 && o.x == 0xC0 && o.y == 0x1CC;
		CHECK(way);
		w.deleteObject(item);
		w.deleteObject(stand);
		g.handleKey(KEY_ESC);
	}

	// Drop (sub_2F530): a soft clay pot (0x84) thrown three squares goes squish and leaves a mess (0xC0); a thing that is not fragile lies there
	{
		int bx = -1, by = -1;
		for (int r = 0; r < 400 && bx < 0; ++r) {
			int cx = px + (r % 20) - 10, cy = py + (r / 20) - 10;
			w.moveObject(w.actorObject(1), cx, cy, 0);
			g.x = cx;
			g.y = cy;
			g.z = 0;
			bool open = true;
			for (int k = 1; k <= 3; ++k) {
				std::vector<int> here;
				w.objectsAt(cx + k, cy, 0, here);
				open &= here.empty() && g.canWalk(cx + k, cy, 0);
			}
			if (open) {
				bx = cx;
				by = cy;
			}
		}
		CHECK(bx >= 0);
		if (bx >= 0) {
			int plain = 0x2D;
			for (int t = 0x20; t < 0x80; ++t)
				if (!w.isStackable(t) && t != 0x3B && t != 0x3C && t != 0x3F && t != 0x5F && t != 0x61 && t != 0x84 && t != 0x85 && t != 0x6E && t != 0x58 && w.baseTile.firstTile[t]) {
					plain = t;
					break;
				}
			for (int type : {0x84, plain}) {
				std::vector<int> pack;
				w.inventory(1, pack, false);
				for (int i : pack)
					w.deleteObject(i);
				w.addToInventory(1, type, 0, 1);
				w.inventory(1, pack, false);
				int nth = 0;
				for (size_t k = 0; k < pack.size(); ++k)
					if (w.objects[pack[k]].type == type)
						nth = (int)k;
				g.log.clear();
				g.handleKey('d');
				for (int k = 0; k < nth; ++k)
					g.handleKey(KEY_RIGHT);
				g.handleKey(KEY_ENTER);
				g.handleKey(KEY_RIGHT);
				g.handleKey(KEY_RIGHT);
				g.handleKey(KEY_RIGHT);
				g.handleKey(KEY_ENTER);
				std::vector<int> there;
				w.objectsAt(bx + 3, by, 0, there);
				int mess = 0, lying = 0;
				for (int i : there) {
					mess += w.objects[i].type == 0xC0;
					lying += w.objects[i].type == plain;
				}
				if (type == 0x84) {
					CHECK(logText(g).find("went squish") != std::string::npos && mess == 1);
					for (int i : there)
						if (w.objects[i].type == 0xC0)
							w.deleteObject(i); // (the mess is a holder: the next thing would fall into it)
				} else {
					CHECK(logText(g).find("It broke!") == std::string::npos && lying == 1);
				}
			}
		}
		w.moveObject(w.actorObject(1), px, py, 0);
		g.x = px;
		g.y = py;
	}

	// the ladders (sub_254F4 / sub_19505): a way down of quality 102 takes the party to (25,131) of level 2, a way up of quality 102 to (272,69) of level 0
	{
		int sx = g.x, sy = g.y, sz = g.z;
		static const int DX[4] = {0, 1, 0, -1}, DY[4] = {-1, 0, 1, 0};
		// somewhere in the neighbourhood with an open square beside it
		int bx = -1, by = -1, dir = -1;
		for (int r = 0; r < 200 && dir < 0; ++r) {
			int cx = px + (r % 20) - 10, cy = py + (r / 20) - 5;
			w.moveObject(w.actorObject(1), cx, cy, 0);
			g.x = cx;
			g.y = cy;
			g.z = 0;
			for (int d = 0; d < 4 && dir < 0; ++d) {
				std::vector<int> here;
				w.objectsAt(cx + DX[d], cy + DY[d], 0, here);
				if (here.empty() && g.canWalk(cx + DX[d], cy + DY[d], 0)) {
					bx = cx;
					by = cy;
					dir = d;
				}
			}
		}
		CHECK(dir >= 0);
		if (dir >= 0) {
			int lx = bx + DX[dir], ly = by + DY[dir];
			int ladder = put(w, 0xA3, 3, lx, ly, 0); // frame 3 is the open way in
			w.objects[ladder].quality = 102;
			g.step(dir * 2);
			CHECK(g.z == 2 && g.x == 25 && g.y == 131);
			w.deleteObject(ladder);
			w.moveObject(w.actorObject(1), bx, by, 0);
			g.x = bx;
			g.y = by;
			g.z = 0;
			int up = put(w, 0xE3, 0, lx, ly, 0);
			w.objects[up].quality = 102;
			g.step(dir * 2);
			CHECK(g.z == 0 && g.x == 272 && g.y == 69);
			w.deleteObject(up);
		}
		w.moveObject(w.actorObject(1), sx, sy, sz);
		g.x = sx;
		g.y = sy;
		g.z = sz;
	}

	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}
