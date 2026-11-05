// Hand-to-hand fighting on the real world (skipped when the game data is absent).

#include "ultima6/combat.h"
#include "ultima6/session.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace Ultima6;
using ::Ultima6::Ultima6::Combat;
using ::Ultima6::Ultima6::CreatureStats;
using ::Ultima6::Ultima6::World;

static int failures = 0;

#define CHECK(cond)                                                              \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
			++failures;                                                          \
		}                                                                        \
	} while (0)

int main() {
	// The tables
	CHECK(Combat::stats(410) && Combat::stats(410)->hp == 30);        // the Avatar
	CHECK(Combat::weaponDamageOf(43) == 15 && Combat::weaponDamageOf(46) == 20); // sword, two-handed sword
	CHECK(Combat::weaponDamageOf(1) == -1);
	CHECK(Combat::armourOf(22) == 7); // plate mail

	World w;
	if (!w.load("u6/game")) {
		std::printf("skip (no game data)\n");
		return failures ? 1 : 0;
	}
	BorlandRandom rng(7);
	::Ultima6::Ultima6::TextWindow scroll(22, 14, 17, 10);
	Combat c(w, rng, scroll);

	// The Avatar's friends fight with what they wear.
	CHECK(c.weaponType(2) > 0);
	CHECK(!c.nameOf(2).empty());
	CHECK(c.nameOf(1) == (const char *)w.actors.partyNames[0]);

	// An unarmed, unarmoured peasant (actor 9, Sherry the mouse in the real game is a creature; use any plain actor) dies after enough blows.
	int victim = -1;
	for (int a = 40; a < 0xE0 && victim < 0; ++a) {
		int k = w.actorObject(a);
		if (k >= 0 && w.actors.hp[a] > 0 && !(w.objects[k].status & 0x80))
			victim = a;
	}
	CHECK(victim > 0);
	{ // blows are struck from next to the victim (anything farther is a missile)
		int ka = w.actorObject(1), kv = w.actorObject(victim);
		w.objects[kv].x = w.objects[ka].x;
		w.objects[kv].y = (uint16_t)(w.objects[ka].y + 1);
		w.objects[kv].z = w.objects[ka].z;
	}
	int exp0 = w.actors.exp[1];
	int blows = 0;
	bool died = false;
	size_t before = w.objects.size();
	while (!died && blows < 500) {
		died = c.attack(1, victim);
		++blows;
	}
	CHECK(died);
	CHECK(w.actors.hp[victim] == 0);
	CHECK(w.actorObject(victim) >= 0 && w.objects[w.actorObject(victim)].deleted);
	CHECK(w.objects.size() == before + 1); // the dead body
	CHECK(w.objects.back().type == 339);
	(void)exp0;
	std::printf("killed actor %d (%s) in %d blows\n", victim, c.nameOf(victim).c_str(), blows);

	// A glass sword shatters on the first blow that hits (perform_attack 2D73C); the 0x5B does no harm
	{
		World w3;
		w3.load("u6/game");
		BorlandRandom rng3(5);
		::Ultima6::Ultima6::TextWindow scroll3(22, 14, 17, 10);
		Combat c3(w3, rng3, scroll3);
		int target = -1;
		for (int a = 40; a < 0xE0 && target < 0; ++a) {
			int k = w3.actorObject(a);
			if (k >= 0 && w3.actors.hp[a] > 0 && !(w3.objects[k].status & 0x80))
				target = a;
		}
		{
			int ka = w3.actorObject(2), kt = w3.actorObject(target);
			w3.objects[kt].x = w3.objects[ka].x;
			w3.objects[kt].y = (uint16_t)(w3.objects[ka].y + 1);
			w3.objects[kt].z = w3.objects[ka].z;
		}
		w3.addToInventory(2, 0x30, 0, 1);
		for (World::Obj &o : w3.objects)
			if (!o.deleted && o.type == 0x30 && o.location() == ObjRecord::IN_INVENTORY && o.parent == 2)
				o.status = (uint8_t)((o.status & ~ObjRecord::LOCATION_MASK) | ObjRecord::READIED);
		auto glass = [&]() {
			int n = 0;
			for (const World::Obj &o : w3.objects)
				n += !o.deleted && o.type == 0x30 && o.parent == 2;
			return n;
		};
		CHECK(glass() == 1);
		int hp = w3.actors.hp[target];
		c3.attackWith(2, target, 0x5B);
		CHECK(w3.actors.hp[target] == hp); // no harm (the roll to hit may fail, either way)
		for (int i = 0; i < 100 && glass(); ++i)
			c3.attackWith(2, target, 0x30);
		CHECK(glass() == 0);
	}

	// A bow shot from three squares costs an arrow; a flask of oil costs a flask and sets a fire where it lands; a dagger thrown from afar leaves the pack
	{
		World w5;
		w5.load("u6/game");
		BorlandRandom rng5(9);
		::Ultima6::Ultima6::TextWindow scroll5(22, 14, 17, 10);
		Combat c5(w5, rng5, scroll5);
		int target = -1;
		for (int a = 40; a < 0xE0 && target < 0; ++a) {
			int k = w5.actorObject(a);
			if (k >= 0 && w5.actors.hp[a] > 0 && !(w5.objects[k].status & 0x80))
				target = a;
		}
		int ka = w5.actorObject(1), kt = w5.actorObject(target);
		w5.objects[kt].x = (uint16_t)(w5.objects[ka].x + 3);
		w5.objects[kt].y = w5.objects[ka].y;
		w5.objects[kt].z = w5.objects[ka].z;
		std::vector<std::pair<int, int>> path;
		bool open = w5.boltPath(w5.objects[ka].z, w5.objects[ka].x, w5.objects[ka].y, w5.objects[kt].x, w5.objects[kt].y, 6, path);
		if (open) {
			auto count = [&](int type) {
				int n = 0;
				for (const World::Obj &o : w5.objects)
					if (!o.deleted && o.type == type && o.location() != ObjRecord::ON_MAP)
						n += w5.quantityOf(o);
				return n;
			};
			w5.addToInventory(1, 0x37, 0, 5);
			w5.addToInventory(1, 0x53, 0, 2);
			w5.addToInventory(1, 0x26, 0, 1);
			int arrows = count(0x37);
			c5.attackWith(1, target, 0x29, true);
			CHECK(count(0x37) == arrows - 1);
			int flasks = count(0x53);
			size_t before = w5.objects.size();
			w5.actors.hp[target] = 255;
			c5.attackWith(1, target, 0x53, true);
			CHECK(count(0x53) == flasks - 1);
			bool fire = false;
			for (size_t i = before; i < w5.objects.size(); ++i)
				fire |= w5.objects[i].type == 0x13D;
			CHECK(fire); // grass takes the fire
			int daggers = count(0x26);
			c5.attackWith(1, target, 0x26, true);
			CHECK(count(0x26) == daggers - 1); // the dagger is in the target or on the ground
		}
	}

	// Interference (sub_2BA3E): a member who remembers the one that struck it, who stands next to it and is awake, cannot shoot
	{
		World w4;
		w4.load("u6/game");
		BorlandRandom rng4(5);
		::Ultima6::Ultima6::TextWindow scroll4(22, 14, 17, 10);
		Combat c4(w4, rng4, scroll4);
		int foe = 3; // any other party member's body will do as the one next to member 2
		w4.actors.worktype[2] = 2;
		w4.actors.unknown18f1[2] = (uint8_t)foe;
		int k2 = w4.actorObject(2), kf = w4.actorObject(foe);
		w4.objects[kf].x = w4.objects[k2].x + 1;
		w4.objects[kf].y = w4.objects[k2].y;
		w4.actors.status[foe] = 0;
		w4.actors.movementFlags[foe] = 0;
		CHECK(c4.interferer(2, false) == foe);
		w4.actors.status[foe] = 0x04; // asleep: no
		CHECK(c4.interferer(2, false) == -1);
		w4.actors.status[foe] = 0;
		w4.objects[kf].x = w4.objects[k2].x + 2; // two squares away: no
		CHECK(c4.interferer(2, false) == -1);
		w4.objects[kf].x = w4.objects[k2].x + 1;
		CHECK(c4.interferer(2, true) == -1); // time stopped and the foe is not of the party
		w4.actors.status[foe] = 0x80;
		CHECK(c4.interferer(2, true) == foe);
		CHECK(c4.isMissile(2, foe, 0x29) && !c4.isMissile(2, foe, 0x2B) && !c4.isMissile(2, foe, 0x28));
	}

	// Held (dragged under, sub_26D19): a roll below the strength frees, else the hold hurts and the turn is lost
	{
		World w2;
		w2.load("u6/game");
		BorlandRandom rng2(11);
		::Ultima6::Ultima6::TextWindow scroll2(22, 14, 17, 10);
		Combat c2(w2, rng2, scroll2);
		std::vector<int> heard;
		c2.onSfx = [&](int e, int) { heard.push_back(e); };
		CHECK(!c2.dragStruggle(2)); // not held
		int freed = 0, held = 0;
		for (int i = 0; i < 40 && w2.actors.hp[2] > 0; ++i) {
			w2.actors.movementFlags[2] |= 0x10;
			int hp = w2.actors.hp[2];
			if (c2.dragStruggle(2)) {
				++held;
				CHECK(w2.actors.hp[2] < hp && (w2.actors.movementFlags[2] & 0x10));
			} else {
				++freed;
				CHECK(!(w2.actors.movementFlags[2] & 0x10) && w2.actors.hp[2] == hp);
			}
		}
		CHECK(held > 0 && freed > 0);
		CHECK((int)heard.size() == freed && !heard.empty() && heard[0] == 0xA);
	}

	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}
