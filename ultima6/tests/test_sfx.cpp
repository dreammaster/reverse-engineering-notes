// Sound effects (sfx.*): every effect renders to some sound and ends (no game data needed).

#include "engine/sfx.h"

#include <cstdio>
#include <cstdlib>

using namespace Ultima6;

int main() {
	int failures = 0;
	BorlandRandom rng(3);
	for (int effect = 0; effect <= 23; ++effect) {
		std::vector<int16_t> s = Sfx::render(effect, effect == 5 || effect == 6 || effect == 16 ? 5 : 1000, rng);
		int peak = 0;
		for (int16_t v : s)
			peak = std::max(peak, std::abs((int)v));
		std::printf("effect %2d: %6zu samples, peak %d\n", effect, s.size(), peak);
		if (effect != 0 && (s.empty() || peak == 0)) {
			std::printf("FAIL effect %d is silent\n", effect);
			++failures;
		}
		if (s.size() > (size_t)Sfx::SAMPLE_RATE * 3) {
			std::printf("FAIL effect %d is too long\n", effect);
			++failures;
		}
	}
	if (!failures)
		std::printf("ok\n");
	return failures ? 1 : 0;
}
