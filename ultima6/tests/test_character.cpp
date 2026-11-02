// Character creation: the Borland generator, the gypsy's quiz and the new-game objlist patch.

#include "engine/random.h"
#include "files/lzw.h"
#include "files/world_data.h"
#include "ultima6/gypsy.h"

#include <cstdio>
#include <set>

using namespace Ultima6;
using ::Ultima6::Ultima6::GypsyQuiz;
using ::Ultima6::Ultima6::NewCharacter;

static int failures = 0;

#define CHECK(cond)                                                              \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
			++failures;                                                          \
		}                                                                        \
	} while (0)

int main() {
	// Turbo C's well known sequence after srand(1).
	BorlandRandom rng(1);
	const int expected[] = {346, 130, 10982, 1090, 11656};
	for (int e : expected)
		CHECK(rng.next() == e);

	// Every pair of different virtues has a question whose two answers are exactly that pair.
	std::set<int> seen;
	for (int v = 0; v < GypsyQuiz::VIRTUES; ++v) {
		for (int w = 0; w < GypsyQuiz::VIRTUES; ++w) {
			if (v == w)
				continue;
			int q = GypsyQuiz::questionFor(v, w);
			CHECK(q >= GypsyQuiz::FIRST_QUESTION && q < GypsyQuiz::FIRST_QUESTION + GypsyQuiz::QUESTION_COUNT);
			CHECK(GypsyQuiz::questionFor(w, v) == q);
			int a = GypsyQuiz::virtueFor(q, false), b = GypsyQuiz::virtueFor(q, true);
			CHECK((a == v && b == w) || (a == w && b == v));
			seen.insert(q);
		}
	}
	CHECK(seen.size() == GypsyQuiz::QUESTION_COUNT);

	// Seven questions whatever is answered; the stats follow the vials; no question is asked twice.
	for (int seed = 0; seed < 200; ++seed) {
		BorlandRandom r(seed);
		for (int mask = 0; mask < 128; mask += 5) {
			BorlandRandom rr(seed);
			GypsyQuiz quiz(rr);
			int str = 15, dex = 15, intel = 15, karma = 75, rounds = 0;
			std::set<int> asked;
			while (!quiz.done()) {
				int q = quiz.question();
				CHECK(q >= GypsyQuiz::FIRST_QUESTION);
				CHECK(asked.insert(q).second);
				bool second = (mask >> rounds) & 1;
				int s, d, i;
				int virtue = GypsyQuiz::virtueFor(q, second);
				GypsyQuiz::vialBonus(virtue, s, d, i);
				str += s;
				dex += d;
				intel += i;
				if (virtue == GypsyQuiz::HUMILITY)
					karma += 3;
				quiz.answer(second);
				++rounds;
			}
			CHECK(rounds == GypsyQuiz::ROUNDS);
			CHECK(quiz.question() == -1);
			CHECK(quiz.stats().strength == str && quiz.stats().dexterity == dex && quiz.stats().intelligence == intel);
			CHECK(quiz.stats().karma == karma);
			CHECK(str + dex + intel >= 45 + 7 * 0 && str + dex + intel <= 45 + 7 * 3);
		}
	}

	// The tables are those of the shipped U.EXE (ds:06B7, 0737, 076F, 07A7, 07B7, 07C7 = file offsets
	// 16A47, 16AC7, 16AFF, 16B37, 16B47, 16B57).
	Bytes exe;
	if (readFile("u6/game/U.EXE", exe) && exe.size() == 109988) {
		for (int v = 0; v < 8; ++v) {
			for (int w = 0; w < 8; ++w)
				if (v != w)
					CHECK((int)(int16_t)get16(&exe[0x16A47 + (v * 8 + w) * 2]) == GypsyQuiz::questionFor(v, w));
			int s, d, i;
			GypsyQuiz::vialBonus(v, s, d, i);
			CHECK(get16(&exe[0x16B37 + v * 2]) == s && get16(&exe[0x16B47 + v * 2]) == d && get16(&exe[0x16B57 + v * 2]) == i);
		}
		for (int q = 0; q < GypsyQuiz::QUESTION_COUNT; ++q) {
			CHECK(get16(&exe[0x16AC7 + q * 2]) == GypsyQuiz::virtueFor(GypsyQuiz::FIRST_QUESTION + q, false));
			CHECK(get16(&exe[0x16AFF + q * 2]) == GypsyQuiz::virtueFor(GypsyQuiz::FIRST_QUESTION + q, true));
		}
	} else {
		std::printf("skip U.EXE table check\n");
	}

	// New game: patch the template that ships inside LZDNGBLK (5 dungeon blocks, then the objlist).
	Bytes lz, plain;
	if (readFile("u6/game/LZDNGBLK", lz) && lzwDecompress(lz, 0, plain)) {
		size_t pos = 0;
		for (int i = 0; i < 5; ++i)
			pos += 2 + (size_t)get16(&plain[pos]) * 8;
		Bytes tmpl(plain.begin() + pos, plain.end());
		CHECK(tmpl.size() == NewCharacter::TEMPLATE_SIZE);

		BorlandRandom r(1234);
		GypsyQuiz quiz(r);
		while (!quiz.done())
			quiz.answer(false);
		NewCharacter c;
		c.name = "Sir Testalot";
		c.female = true;
		c.portrait = 4;
		c.stats = quiz.stats();

		Bytes copy = tmpl;
		CHECK(c.applyTo(copy));
		ObjList obj;
		CHECK(obj.load(copy));
		CHECK(obj.strength[1] == c.stats.strength && obj.dexterity[1] == c.stats.dexterity);
		CHECK(obj.intelligence[1] == c.stats.intelligence && obj.unknown13f1[1] == c.stats.intelligence * 2);
		CHECK(obj.exp[1] == 370 && obj.hp[1] == 90 && obj.level[1] == 3);
		CHECK(std::string((const char *)obj.partyNames[0]) == c.name);
		CHECK(obj.globals[0x1C71 - 0x1BF1] == 1 && obj.globals[0x1C72 - 0x1BF1] == 5);
		CHECK(!c.applyTo(copy)); // already has a character

		std::printf("quiz result: STR %d DEX %d INT %d karma %d\n", c.stats.strength, c.stats.dexterity, c.stats.intelligence,
					c.stats.karma);
	} else {
		std::printf("skip new-game template check\n");
	}

	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}
