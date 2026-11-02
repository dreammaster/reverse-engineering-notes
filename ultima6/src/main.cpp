#include "engine/framebuffer.h"
#include "engine/game_id.h"
#include "ultima6/map_renderer.h"
#include "ultima6/world.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc, char **argv) {
	if (argc < 2) {
		std::fprintf(stderr, "usage: %s <game directory> [--shot file.png [x y z]]\n", argv[0]);
		return 2;
	}

	Ultima6::GameId id = Ultima6::detectGame(argv[1]);
	if (id == Ultima6::GameId::Unknown) {
		std::fprintf(stderr, "%s: no Ultima VI / Martian Dreams / Savage Empire found\n", argv[1]);
		return 1;
	}
	std::printf("%s\n", Ultima6::gameInfo(id).title);
	if (id != Ultima6::GameId::Ultima6)
		return 0; // the spin-offs are not playable yet

	Ultima6::Ultima6::World world;
	if (!world.load(argv[1])) {
		std::fprintf(stderr, "cannot load %s\n", world.error().c_str());
		return 1;
	}

	if (argc >= 4 && !std::strcmp(argv[2], "--shot")) {
		int x, y, z;
		world.avatarPosition(x, y, z);
		if (argc >= 7) {
			x = std::atoi(argv[4]);
			y = std::atoi(argv[5]);
			z = std::atoi(argv[6]);
		}
		Ultima6::Framebuffer screen;
		screen.setPalette(world.palette.rgb);
		Ultima6::Ultima6::MapRenderer renderer(world);
		renderer.draw(screen, x, y, z);
		if (!screen.savePng(argv[3], 3)) {
			std::fprintf(stderr, "cannot write %s\n", argv[3]);
			return 1;
		}
		std::printf("wrote %s (view at %d,%d,%d, %zu objects)\n", argv[3], x, y, z, world.objects.size());
	}
	return 0;
}
