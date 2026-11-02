// The ending: the time played in words, and the pages (skipped when the game data is absent).

#include "ultima6/ending.h"

#include <cstdio>
#include <string>

using namespace Ultima6;
using ::Ultima6::Ultima6::Ending;

static int failures = 0;

#define CHECK(cond)                                                              \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
			++failures;                                                          \
		}                                                                        \
	} while (0)

int main() {
	CHECK(Ending::numberToWords(0) == "zero");
	CHECK(Ending::numberToWords(13) == "thirteen");
	CHECK(Ending::numberToWords(20) == "twenty");
	CHECK(Ending::numberToWords(42) == "forty-two");
	CHECK(Ending::numberToWords(99) == "ninety-nine");
	CHECK(Ending::timePlayed(1, 2, 3) == "in one year, two months,\nand three days.");
	CHECK(Ending::timePlayed(0, 1, 1) == "in zero years, one month,\nand one day.");

	Ending e("u6/game", 1, 2, 3);
	if (!e.ok()) {
		std::printf("skip (no game data)\n");
		return failures ? 1 : 0;
	}
	Framebuffer screen;
	int keys = 0;
	do {
		e.render(screen); // every page draws
		++keys;
	} while (e.handleKey('a') && keys < 100);
	CHECK(keys > 11 && keys < 100); // the story's pages (some split) and the credits

	std::printf("%s\n", failures ? "FAILED" : "ok");
	return failures ? 1 : 0;
}
