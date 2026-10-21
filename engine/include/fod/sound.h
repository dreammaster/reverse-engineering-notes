#pragma once
// PC-speaker sound effects (KEH playSound / soundTick), rendered to PCM.
#include <cstdint>
#include <vector>

namespace fod {

// Effect n (0..6) as 16-bit mono samples. The speaker script is stepped at the vertical retrace rate (70 Hz).
std::vector<int16_t> renderSoundEffect(int n, int sampleRate = 22050);

} // namespace fod
