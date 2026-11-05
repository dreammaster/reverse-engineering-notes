// Saving the world and loading it back (skipped when the game data is absent).

#include "ultima6/session.h"

#include <cstdio>
#include <cstdlib>
#include <string>

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

static size_t liveObjects(const World &w) {
	size_t n = 0;
	for (const World::Obj &o : w.objects)
		n += !o.deleted && o.actor < 0;
	return n;
}

static void roundTrip(const char *dir, GameId id, const char *save, const char *save2) {
	static World w, r, r2;
	w = World();
	w.game = id;
	if (!w.load(dir)) {
		std::printf("skip %s (no game data)\n", dir);
		return;
	}
	Game g(w);
	g.saveDir = save;
	g.handleKey(KEY_DOWN);
	for (int i = 0; i < 30; ++i)
		g.handleKey(KEY_SPACE);
	g.handleKey(19);
	g.handleKey('y');
	r = World();
	r.game = id;
	CHECK(r.load(dir, nullptr, save));
	CHECK(liveObjects(r) == liveObjects(w));
	Game g2(r);
	CHECK(g2.x == g.x && g2.y == g.y && g2.z == g.z);
	CHECK(g2.clock.hour == g.clock.hour && g2.clock.minute == g.clock.minute);
	r2 = World();
	r2.game = id;
	CHECK(r2.load(dir, nullptr, save));
	CHECK(r2.save(save2));
	Bytes a, b;
	CHECK(readFile(std::string(save) + "/OBJLIST", a) && readFile(std::string(save2) + "/OBJLIST", b) && a == b);
}

int main() {
	World w;
	if (!w.load("u6/game")) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	Game g(w);
	g.saveDir = "build/test_save_dir";
	// walk a little, take something, then save with ctrl-S
	g.handleKey(KEY_DOWN);
	g.handleKey(KEY_RIGHT);
	for (int i = 0; i < 90; ++i)
		g.handleKey(KEY_SPACE);
	g.handleKey(19);
	g.handleKey('y');

	World r;
	CHECK(r.load("u6/game", nullptr, g.saveDir));
	CHECK(liveObjects(r) == liveObjects(w));
	Game g2(r);
	CHECK(g2.x == g.x && g2.y == g.y && g2.z == g.z);
	CHECK(g2.clock.hour == g.clock.hour && g2.clock.minute == g.clock.minute && g2.clock.day == g.clock.day);
	// every actor stands where it stood
	int same = 0, total = 0;
	for (int a = 0; a < 256; ++a) {
		int k1 = w.actorObject(a), k2 = r.actorObject(a);
		if (k1 < 0)
			continue;
		++total;
		CHECK(k2 >= 0);
		if (k2 >= 0 && w.objects[k1].x == r.objects[k2].x && w.objects[k1].y == r.objects[k2].y && w.objects[k1].z == r.objects[k2].z &&
			w.objects[k1].frame == r.objects[k2].frame)
			++same;
	}
	CHECK(same == total);
	CHECK(std::string((const char *)r.actors.partyNames[0]) == std::string((const char *)w.actors.partyNames[0]));

	// a second save of a freshly reloaded world is byte for byte the same
	Bytes a, b;
	World r2;
	CHECK(r2.load("u6/game", nullptr, g.saveDir));
	CHECK(r2.save("build/test_save_dir2"));
	CHECK(readFile("build/test_save_dir/OBJLIST", a) && readFile("build/test_save_dir2/OBJLIST", b) && a == b);

	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}
