// Hand-to-hand fighting on the real world (skipped when the game data is absent).

#include "ultima6/combat.h"
#include "ultima6/session.h"

#include <cstdio>
#include <string>

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

	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}
