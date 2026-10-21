#include "fod/progress.h"
#include <algorithm>
#include <cstdio>

namespace fod {

namespace {
const uint32_t kRankExp[14] = {0, 1500, 1000, 1500, 4500, 6500, 9500, 11500, 23000, 32000, 44000, 57000, 89000, 118000};
uint32_t expForRank(int rank) { return rank > 13 ? 200000u : kRankExp[rank]; }
}

void checkLevelUps(World &w, Host &h) {
	for (int slot = 0; slot < w.partyCount(); ++slot) {
		Member &m = w.party(slot);
		if (m.exp() < m.expNext()) continue;
		const Profession &p = w.data.archetypes.professions().at(m.profession());
		int points = 0, gained = 0;
		while (m.exp() >= m.expNext()) {
			if (m.b[0x4E] < 99) {
				++m.b[0x4E];
				int gain = w.rng.range(p.raw[0x7C], p.raw[0x7D]);
				const int cap = p.raw[0x74] | (p.raw[0x75] << 8);
				if (m.hpMax() + gain > cap) gain = cap - m.hpMax();
				m.setHpMax(m.hpMax() + gain);
				gained += gain;
			}
			points += 2;
			m.setU32(0x5A, m.exp() - m.expNext());
			m.setU32(0x5E, expForRank(m.b[0x4E]));
		}
		char buf[160];
		std::snprintf(buf, sizeof buf, "%s, you have gained %d hit points and you are elevated to the rank of %d", m.name().c_str(), gained, m.b[0x4E]);
		h.message(buf);
		int cancels = 0;
		while (points > 0) {
			std::vector<std::string> names;
			for (int a = 0; a < 7; ++a) {
				const char *n = reinterpret_cast<const char *>(w.data.hdspct.data() + 6 + 22 * size_t(a));
				names.push_back(std::string(n, strnlen(n, 22)) + "  " + std::to_string(m.attr(a)));
			}
			std::snprintf(buf, sizeof buf, "Distribute %d points:", points);
			const int pick = h.pickFromList(buf, names);
			if (pick < 0 && ++cancels < 20) continue;
			if (pick < 0) { for (int a = 0; a < 7 && points > 0; ++a) if (m.attr(a) < p.attrMax[a]) { ++m.attr(a); --points; } break; }
			if (m.attr(pick) < p.attrMax[pick]) { ++m.attr(pick); --points; }
			else h.message("You have attained the highest level for that attribute.");
		}
	}
}

} // namespace fod
