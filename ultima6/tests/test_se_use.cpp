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

	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}
