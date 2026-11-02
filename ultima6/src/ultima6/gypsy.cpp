#include "ultima6/gypsy.h"

#include <cstring>

namespace Ultima6 {
namespace Ultima6 {

// U.EXE data segment, ds:06B7. Row = first virtue, column = second; the diagonal is unused.
static const int16_t QUESTIONS[64] = {
	-1, 105, 106, 107, 108, 109, 110, 111,
	105, -1, 112, 113, 114, 115, 116, 117,
	106, 112, -1, 118, 119, 120, 121, 122,
	107, 113, 118, -1, 123, 124, 125, 126,
	108, 114, 119, 123, -1, 127, 128, 129,
	109, 115, 120, 124, 127, -1, 130, 131,
	110, 116, 121, 125, 128, 130, -1, 132,
	111, 117, 122, 126, 129, 131, 132, -1
};

// ds:0737 and ds:076F, indexed by question - 105.
static const uint8_t ANSWER_A[28] = {
	0, 0, 0, 0, 0, 0, 0, 1,
	1, 1, 1, 1, 1, 2, 2, 2,
	2, 2, 3, 3, 3, 3, 4, 4,
	4, 5, 5, 6
};
static const uint8_t ANSWER_B[28] = {
	1, 2, 3, 4, 5, 6, 7, 2,
	3, 4, 5, 6, 7, 3, 4, 5,
	6, 7, 4, 5, 6, 7, 5, 6,
	7, 6, 7, 7
};

// ds:07A7, 07B7, 07C7, indexed by virtue.
static const uint8_t STRENGTH_BONUS[8] = {0, 0, 2, 0, 1, 1, 1, 0};
static const uint8_t DEXTERITY_BONUS[8] = {0, 2, 0, 1, 1, 0, 1, 0};
static const uint8_t INTELLIGENCE_BONUS[8] = {2, 0, 0, 1, 0, 1, 1, 0};

GypsyQuiz::GypsyQuiz(::Ultima6::BorlandRandom &rng) : _rng(rng), _round(0) {
	for (int i = 0; i < VIRTUES; ++i)
		_table[i] = i;
	_stats.strength = _stats.dexterity = _stats.intelligence = START_STAT;
	_stats.karma = START_KARMA;
	shuffle(VIRTUES);
}

// U.EXE sub_14DD3: n*n + n at most swaps of two random slots, the count itself random.
void GypsyQuiz::shuffle(int count) {
	int swaps = _rng.next() % (count * count) + count;
	while (swaps-- > 0) {
		int i = _rng.next() % count;
		int j = _rng.next() % count;
		int t = _table[i];
		_table[i] = _table[j];
		_table[j] = t;
	}
}

// The pair of virtues for the current round: neighbours in the table, or the last two survivors.
void GypsyQuiz::pair(int &v, int &w) const {
	int slot = _round < 4 ? _round * 2 : _round == 4 ? 0 : _round == 5 ? 2 : 0;
	v = _table[slot];
	w = _table[slot + 1];
}

int GypsyQuiz::question() const {
	if (done())
		return -1;
	int v, w;
	pair(v, w);
	return questionFor(v, w);
}

void GypsyQuiz::choices(int &a, int &b) const {
	int q = question();
	a = virtueFor(q, false);
	b = virtueFor(q, true);
}

void GypsyQuiz::answer(bool second) {
	if (done())
		return;
	int virtue = virtueFor(question(), second);
	int s, d, i;
	vialBonus(virtue, s, d, i);
	_stats.strength += s;
	_stats.dexterity += d;
	_stats.intelligence += i;
	if (virtue == HUMILITY)
		_stats.karma += 3;

	// The winner of rounds 0-3 goes to slot 0-3; of rounds 4 and 5 to slot 0 and 1.
	int slot = _round < 4 ? _round : _round - 4;
	_table[slot] = virtue;
	++_round;
	if (_round == 4)
		shuffle(4);
}

int GypsyQuiz::questionFor(int v, int w) {
	return QUESTIONS[v * VIRTUES + w];
}

int GypsyQuiz::virtueFor(int question, bool second) {
	return (second ? ANSWER_B : ANSWER_A)[question - FIRST_QUESTION];
}

void GypsyQuiz::vialBonus(int virtue, int &str, int &dex, int &intel) {
	str = STRENGTH_BONUS[virtue];
	dex = DEXTERITY_BONUS[virtue];
	intel = INTELLIGENCE_BONUS[virtue];
}

bool NewCharacter::applyTo(Bytes &objlist) const {
	enum {
		AVATAR = 1,
		OFF_STRENGTH = 0x900,
		OFF_DEXTERITY = 0xA00,
		OFF_INTELLIGENCE = 0xB00,
		OFF_EXP = 0xC00,
		OFF_HP = 0xE00,
		OFF_NAMES = 0xF00,
		OFF_LEVEL = 0xFF1,
		OFF_MAGIC = 0x13F1,
		OFF_KARMA = 0x1BF9,
		OFF_GENDER = 0x1C71,
		OFF_PORTRAIT = 0x1C72
	};
	if (objlist.size() < TEMPLATE_SIZE || objlist[OFF_PORTRAIT] != 0)
		return false;

	uint8_t *o = &objlist[0];
	o[OFF_STRENGTH + AVATAR] = (uint8_t)stats.strength;
	o[OFF_DEXTERITY + AVATAR] = (uint8_t)stats.dexterity;
	o[OFF_INTELLIGENCE + AVATAR] = (uint8_t)stats.intelligence;
	o[OFF_MAGIC + AVATAR] = (uint8_t)(stats.intelligence * 2);
	o[OFF_KARMA] = (uint8_t)stats.karma;
	o[OFF_GENDER] = female ? 1 : 0;
	std::memset(o + OFF_NAMES, 0, MAX_NAME + 1);
	std::memcpy(o + OFF_NAMES, name.data(), name.size() < (size_t)MAX_NAME ? name.size() : (size_t)MAX_NAME);
	o[OFF_EXP + AVATAR * 2] = EXPERIENCE & 0xFF;
	o[OFF_EXP + AVATAR * 2 + 1] = EXPERIENCE >> 8;
	o[OFF_HP + AVATAR] = HIT_POINTS;
	o[OFF_LEVEL + AVATAR] = LEVEL;
	o[OFF_PORTRAIT] = (uint8_t)(portrait + 1);
	return true;
}

} // namespace Ultima6
} // namespace Ultima6
