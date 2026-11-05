// Animated tiles and the cycling palette (skipped when the game data is absent): the water moves from one frame counter to the next, and the palette entries 0xE0-0xE7 turn.

#include "ultima6/map_renderer.h"
#include "ultima6/session.h"

#include <cstdio>
#include <cstring>

using namespace Ultima6;
using ::Ultima6::Ultima6::Game;
using ::Ultima6::Ultima6::MapRenderer;
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
	static World w;
	if (!w.load("u6/game")) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	CHECK(w.anim.count == 0x1D && w.anim.tileToAnimate[0] == 8);

	// a square of the surface whose terrain is one of the animated tiles
	int wx = -1, wy = -1;
	for (int y = 0; y < 1024 && wx < 0; ++y)
		for (int x = 0; x < 1024 && wx < 0; ++x)
			if (w.map.tile(0, x, y) == w.anim.tileToAnimate[0]) {
				wx = x;
				wy = y;
			}
	CHECK(wx >= 0);

	static Framebuffer a, b;
	MapRenderer r(w);
	r.setFrame(0);
	r.draw(a, wx, wy, 0);
	r.setFrame(2);
	r.draw(b, wx, wy, 0);
	bool differ = false;
	for (int i = 0; i < 160 * 160 && !differ; ++i)
		differ = a.pixels()[(8 + i / 160) * 320 + 8 + i % 160] != b.pixels()[(8 + i / 160) * 320 + 8 + i % 160];
	CHECK(differ);

	// the game's own frame counter and palette
	Game g(w);
	static Framebuffer f0, f1;
	g.render(f0);
	for (int i = 0; i < 3; ++i)
		g.animate();
	g.render(f1);
	bool turned = false;
	for (int c = 0xE0; c < 0x100; ++c) {
		int r0, g0, b0, r1, g1, b1;
		f0.colour(c, r0, g0, b0);
		f1.colour(c, r1, g1, b1);
		turned |= r0 != r1 || g0 != g1 || b0 != b1;
	}
	CHECK(turned);
	std::printf(failures ? "FAILED\n" : "ok\n");
	return failures ? 1 : 0;
}
