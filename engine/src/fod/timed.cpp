// KEH checkTimedEffects (hourly): member mutation timers and the "mutations acting up again" events.
#include "fod/progress.h"

namespace fod {

void checkTimedEffects(World &w, Host &h) {
	static const char *kMessages[6] = {
	    "feels a strong tingling sensation in",
	    "blinks back tears from a very painful burning of",
	    "sneezes rapidly and notices a purplish mist radiating from",
	    "becomes momentarily dizzy as a trickle of blood seeps from both",
	    "(somewhat unintelligibly), complains about a sudden horrible spasming of",
	    "reels and staggers as a sudden weakness envelops"};
	static const char *kParts[6] = {"hands", "eyes", "nose", "ears", "tongue", "legs"};
	static const char *kPoss[4] = {"its", "his", "her", "its"};
	static const uint8_t kAttrHit[6] = {2, 1, 3, 4, 6, 0};
	static const uint8_t kSkillUp[4] = {0x0C, 0x0D, 0x0E, 0x1F};

	std::vector<int> due;
	for (int i = 0; i < w.partyCount(); ++i) {
		Member &m = w.party(i);
		if (World::conState(m) == 5) continue;
		if (m.b[0x57]) --m.b[0x57];
		bool flag = false;
		if (m.b[0x21]) { --m.b[0x21]; flag = m.b[0x21] == 0; }
		if (!flag) {
			if (!(m.b[0x53] || m.b[0x54])) continue;
			if (m.b[0x55] != w.disk.weekday || m.b[0x56] != w.disk.hours) continue;
		}
		due.push_back(i);
	}
	for (int i : due) {
		Member &m = w.party(i);
		std::string text;
		bool impaired = false;
		bool fresh = false;
		int mut = 0;
		if (m.b[0x53] == 0) impaired = true;
		else {
			mut = m.b[0x53] - 1;
			const uint8_t bit = uint8_t(1 << mut);
			if (m.b[0x54] & bit) impaired = true;
			else {
				fresh = true;
				m.b[0x58] |= 0x80;
				m.b[0x54] |= bit;
				uint8_t &a = m.b[0x18 + kAttrHit[mut]];
				if (a >= 3) a = uint8_t(a - 2);
				if (m.b[0x18 + 5] > 1) --m.b[0x18 + 5];
				text = m.name() + " " + kMessages[mut] + " " + kPoss[m.b[0x50] & 3] + " " + kParts[mut] + ".\r";
				h.showPortrait(0x2D + mut);
			}
		}
		if (impaired || fresh) {
			uint8_t &s = m.b[0x24 + kSkillUp[w.rng.range(0, 3)]];
			if (s < 99) ++s;
		}
		if (impaired) text = m.name() + "'s mutations are acting up again. Healing ability impaired.";
		h.message(text);
		h.hidePortrait();
		m.b[0x55] = uint8_t((w.disk.weekday + 2) % 7);
		m.b[0x21] = 0;
		m.b[0x53] = 0;
	}
}

} // namespace fod
