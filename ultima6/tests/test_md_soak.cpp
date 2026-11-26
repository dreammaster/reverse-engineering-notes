// Martian Dreams' own worktypes in the shipped data: the party is put next to each actor that has one and the turns are run for a while (nothing may crash or hang; skipped without data).

#include "ultima6/session.h"

#include <cstdio>

using namespace Ultima6;
using ::Ultima6::Ultima6::Game;
using ::Ultima6::Ultima6::World;

int main() {
	static World w;
	w.game = GameId::MartianDreams;
	if (!w.load("md/game/MARTIAN")) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	Game g(w);
	ObjList &al = w.actors;
	int seen[0xA0] = {};
	int total = 0;
	for (int a = 1; a < 0x100; ++a) {
		int k = w.actorObject(a);
		int wt = al.worktype[a];
		if (k < 0 || w.objects[k].deleted || ((wt < 0x0D || wt > 0x1A) && (wt < 0x99 || wt > 0x9E)) || (al.status[a] & 0x10))
			continue;
		if (seen[wt] >= 3)
			continue;
		++seen[wt];
		++total;
		const World::Obj o = w.objects[k];
		g.z = o.z;
		g.x = (o.x + 3) & 0x3FF;
		g.y = o.y;
		for (int i = 0; i < al.partySize; ++i) {
			int pk = w.actorObject(al.partyRoster[i]);
			if (pk >= 0)
				w.moveObject(pk, g.x, g.y, o.z);
		}
		for (int i = 0; i < 60; ++i) {
			g.runTurns();
			g.advanceTime(1);
		}
	}
	std::printf("%d actors of own worktypes run; ", total);
	for (int wt = 0x0D; wt <= 0x9E; ++wt)
		if (seen[wt])
			std::printf("%02X:%d ", wt, seen[wt]);
	std::printf("\nok\n");
	return 0;
}
