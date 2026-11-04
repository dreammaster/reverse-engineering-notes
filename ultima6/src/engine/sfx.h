#ifndef ULTIMA6_ENGINE_SFX_H
#define ULTIMA6_ENGINE_SFX_H

#include "engine/random.h"

#include <cstdint>
#include <vector>

namespace Ultima6 {

/**
 * The sound effects of GAME.EXE (sub_36E85: 23 effects, each a short run of PC speaker tones, noise bursts and pulse sweeps; the original plays them while the game waits). They are
 * rendered here to 16-bit mono samples at 49716 Hz (the rate of Opl2) so they can be mixed with the music.
 * APPROX(73): the delay loops of the original are timed against the machine (word_40816, counted over one 18.2 Hz tick at start-up); here the unit of sub_3B8AA is one millisecond
 */
struct Sfx {
	enum { SAMPLE_RATE = 49716 };
	/** `effect` is the second argument of sub_36E85 (0 to 22, anything above is a tone of `param` Hz), `param` the first. */
	static std::vector<int16_t> render(int effect, int param, BorlandRandom &rng);
};

} // namespace Ultima6

#endif
