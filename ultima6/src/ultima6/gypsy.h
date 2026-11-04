#ifndef ULTIMA6_ULTIMA6_GYPSY_H
#define ULTIMA6_ULTIMA6_GYPSY_H

#include "engine/random.h"
#include "files/data_file.h"

#include <string>

namespace Ultima6 {
namespace Ultima6 {

/**
 * Ultima VI character creation, as U.EXE does it (sub_152C9 and friends): the
 * gypsy asks seven questions, each a choice between two of the eight virtues, and
 * every answer adds to the Avatar's strength, dexterity and intelligence.
 *
 * The questions are played as a tournament: the virtues 0-7 are shuffled and paired
 * off (four questions); the four winners are shuffled and paired off (two more); the
 * last two meet in the final question. Which question a pair gets, which virtue each
 * answer picks and what a virtue adds are all fixed tables in U.EXE's data segment,
 * reproduced here and checked against the executable by the tests.
 *
 * Note: this namespace has the same name as the engine namespace that encloses it;
 * engine types are written ::Ultima6::Name.
 */
class GypsyQuiz {
public:
	enum {
		VIRTUES = 8,
		ROUNDS = 7,
		FIRST_QUESTION = 105, // question numbers 105..132, one per pair of virtues
		QUESTION_COUNT = 28,
		START_STAT = 15,
		START_KARMA = 75,
		HUMILITY = 7 // choosing it adds 3 karma
	};

	struct Stats {
		int strength, dexterity, intelligence, karma;
	};

	explicit GypsyQuiz(::Ultima6::BorlandRandom &rng);

	bool done() const {
		return _round >= ROUNDS;
	}
	/** Number (105..132) of the question to ask now; -1 when finished. */
	int question() const;
	/** The two virtues the current question chooses between. */
	void choices(int &a, int &b) const;
	/** Answer the current question: false picks the first virtue (A), true the second (B). */
	void answer(bool second);
	const Stats &stats() const {
		return _stats;
	}

	/** Question number for a pair of different virtues (table at ds:06B7). */
	static int questionFor(int v, int w);
	/** Virtue picked by answer A or B of a question (tables at ds:0737 and ds:076F). */
	static int virtueFor(int question, bool second);
	/** What a virtue's vial adds (tables at ds:07A7, 07B7, 07C7). */
	static void vialBonus(int virtue, int &str, int &dex, int &intel);

private:
	void shuffle(int count);
	void pair(int &v, int &w) const;

	::Ultima6::BorlandRandom &_rng;
	int _table[VIRTUES];
	int _round;
	Stats _stats;
};

/** The Avatar as written into a new game's savegame\objlist (U.EXE sub_14066). */
struct NewCharacter {
	enum { MAX_NAME = 13, LEVEL = 3, EXPERIENCE = 370, HIT_POINTS = 90, TEMPLATE_SIZE = 0x1C73 };

	std::string name;
	bool female;
	int portrait; // 0-5
	GypsyQuiz::Stats stats;
	int level = LEVEL, experience = EXPERIENCE, hitPoints = HIT_POINTS; // a transferred character has its own

	/**
	 * Patches the new-game objlist template (0x1C73 bytes, the tail of LZDNGBLK): actor 1's
	 * strength, dexterity, intelligence, magic points (2 * intelligence), experience, hit
	 * points and level, the party name, karma, gender (1 = female) and portrait + 1.
	 * Fails if the template already holds a character (U.EXE refuses in that case too).
	 */
	bool applyTo(Bytes &objlist) const;
};

} // namespace Ultima6
} // namespace Ultima6

#endif
