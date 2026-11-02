#include "engine/game_id.h"

#include <cstdio>

int main(int argc, char **argv) {
	if (argc != 2) {
		std::fprintf(stderr, "usage: %s <game directory>\n", argv[0]);
		return 2;
	}

	Ultima6::GameId id = Ultima6::detectGame(argv[1]);
	if (id == Ultima6::GameId::Unknown) {
		std::fprintf(stderr, "%s: no Ultima VI / Martian Dreams / Savage Empire found\n", argv[1]);
		return 1;
	}

	std::printf("%s\n", Ultima6::gameInfo(id).title);
	return 0;
}
