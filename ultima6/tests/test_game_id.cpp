#include "engine/game_id.h"

#include <cstdio>
#include <cstdlib>
#include <string>

using namespace Ultima6;

static int failures = 0;

#define CHECK(cond)                                                              \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
			++failures;                                                          \
		}                                                                        \
	} while (0)

int main() {
	CHECK(gameInfo(GameId::Ultima6).endExe != nullptr);
	CHECK(gameInfo(GameId::MartianDreams).endExe == nullptr);
	CHECK(gameInfo(GameId::Unknown).title != nullptr);
	CHECK(detectGame("this/directory/does/not/exist") == GameId::Unknown);

	// The game data is git-ignored, so only check it where it is present.
	struct { const char *dir; GameId id; } games[] = {
		{"u6/game", GameId::Ultima6},
		{"md/game/MARTIAN", GameId::MartianDreams},
		{"se/game/SAVAGE", GameId::SavageEmpire},
	};
	for (const auto &g : games) {
		if (detectGame(g.dir) == GameId::Unknown) {
			std::printf("skip %s (no game data)\n", g.dir);
			continue;
		}
		CHECK(detectGame(g.dir) == g.id);
	}

	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}
