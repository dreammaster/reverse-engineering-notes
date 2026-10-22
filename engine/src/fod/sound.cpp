#include "fod/sound.h"
#include "fod/sounddata.h"

namespace fod {

std::vector<int16_t> renderSoundEffect(int n, int rate) {
	std::vector<int16_t> out;
	if (n < 0 || n > 6) return out;
	const int samplesPerTick = rate / 70;
	size_t pc = kSoundOffsets[n];
	int ticksLeft = 0, repeat = 0, noteTicks = 0;
	uint16_t divisor0 = 0, divisor = 0, delta = 0, deltaDelta = 0;
	double phase = 0;
	for (int guard = 0; guard < 70 * 20; ++guard) {   // at most 20 seconds
		// KEH soundTick
		if (ticksLeft) {
			divisor = uint16_t(divisor + delta);
			delta = uint16_t(delta + deltaDelta);
			--ticksLeft;
		} else if (repeat) {
			--repeat;
			divisor = divisor0;
			ticksLeft = noteTicks;
		} else {
			bool stop = false;
			for (;;) {
				const uint8_t op = kSoundScripts[pc++];
				if (op == 0x84) { stop = true; break; }
				if (op == 0x83) {
					ticksLeft = noteTicks = kSoundScripts[pc];
					divisor0 = divisor = uint16_t(kSoundScripts[pc + 1] | (kSoundScripts[pc + 2] << 8));
					delta = uint16_t(kSoundScripts[pc + 3] | (kSoundScripts[pc + 4] << 8));
					deltaDelta = uint16_t(kSoundScripts[pc + 5] | (kSoundScripts[pc + 6] << 8));
					pc += 7;
					break;
				}
				if (op == 0x81) repeat = kSoundScripts[pc++];
			}
			if (stop) break;
		}
		const double freq = divisor ? 1193182.0 / divisor : 0;
		for (int i = 0; i < samplesPerTick; ++i) {
			phase += freq / rate;
			phase -= int(phase);
			out.push_back(freq > 0 ? (phase < 0.5 ? 6000 : -6000) : 0);
		}
	}
	return out;
}

} // namespace fod
