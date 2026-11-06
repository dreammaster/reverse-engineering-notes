// Running real conversation scripts against a stub game (skipped when the game data is absent).

#include "files/converse.h"
#include "ultima6/conversation.h"

#include <cstdio>
#include <map>

using namespace Ultima6;
using ::Ultima6::Ultima6::Conversation;
using ::Ultima6::Ultima6::ConversationHost;

static int failures = 0;

#define CHECK(cond)                                                              \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
			++failures;                                                          \
		}                                                                        \
	} while (0)

// A game with nothing in it: nobody is in the party and no one owns anything.
class StubHost : public ConversationHost {
public:
	std::string out;
	std::map<int, int> flags;
	int karma = 0;

	void print(const std::string &t) override {
		for (char c : t) {
			if (c == '\b' && !out.empty())
				out.pop_back();
			else
				out += c;
		}
	}
	std::string playerName() override { return "Avatar"; }
	std::string genderTitle() override { return "milord"; }
	std::string timeOfDay() override { return "morning"; }
	void showPortrait(int) override {}
	int roll(int lo, int) override { return lo; }
	std::string scriptName(int npc) override { return "Npc" + std::to_string(npc); }
	std::string partyName(int, bool &in) override { in = false; return ""; }
	int talkFlags(int n) override { return flags[n]; }
	void setTalkFlags(int n, int f) override { flags[n] = f; }
	bool inParty(int) override { return false; }
	int partyMember(int) override { return 0; }
	int joinParty(int) override { return 0; }
	int leaveParty(int) override { return 0; }
	bool npcNearby(int) override { return true; }
	bool wounded(int) override { return false; }
	bool poisoned(int) override { return false; }
	void heal(int) override {}
	void cure(int) override {}
	void resurrect(int) override {}
	void setWorktype(int, int) override {}
	int adjustStat(int, int, int d) override { return d; }
	bool hasObject(int, int, int, bool) override { return false; }
	int countObject(int, int) override { return 0; }
	void newObject(int, int, int, int) override {}
	void deleteObject(int, int, int, int) override {}
	void giveObject(int, int, int, int) override {}
	int whoHasObject(int, int) override { return -1; }
	int maxCarry(int) override { return 100; }
	int objectWeight(int, int q) override { return q; }
	bool horsed(int) override { return false; }
	void mount(int) override {}
	void addKarma(int n) override { karma += n; }
	void subtractKarma(int n) override { karma -= n; }
	void sleep() override {}
};

static bool contains(const std::string &s, const char *part) {
	return s.find(part) != std::string::npos;
}

// Feeds a line (or a key) whatever the script asks for next and returns the state.
static Conversation::State advance(Conversation &c, const char *line) {
	Conversation::State s = c.run();
	while (s == Conversation::NEED_KEY) {
		c.giveKey();
		s = c.run();
	}
	if (s == Conversation::NEED_LINE && line) {
		c.giveLine(line);
		s = c.run();
	}
	while (s == Conversation::NEED_KEY) {
		c.giveKey();
		s = c.run();
	}
	return s;
}

int main() {
	Bytes a, b;
	if (!readFile("u6/game/CONVERSE.A", a) || !readFile("u6/game/CONVERSE.B", b)) {
		std::printf("skip (no game data)\n");
		return 0;
	}

	// Lord British is the script whose name says so.
	int lb = -1;
	for (int actor = 1; actor < 99 && lb < 0; ++actor) {
		ConverseScript s;
		if (s.load(a, actor) && s.name().find("Lord British") == 0)
			lb = actor;
	}
	CHECK(lb > 0);
	ConverseScript script;
	CHECK(script.load(a, lb));

	StubHost host;
	Conversation conv(script.bytes(), lb, host);
	conv.setVar(Conversation::VAR_SEX, 0);
	conv.setVar(Conversation::VAR_KARMA, 75);
	Conversation::State s = advance(conv, nullptr);
	CHECK(s == Conversation::NEED_LINE); // he greets and waits for a line
	CHECK(contains(host.out, "You see the noble ruler of Britannia."));
	CHECK(contains(host.out, "What doth trolls lack?"));
	CHECK(conv.name() == "Lord British");

	// the king tests the Avatar first: a wrong answer is turned down and the question stays
	s = advance(conv, "name");
	CHECK(s == Conversation::NEED_LINE);
	CHECK(contains(host.out, "Nay"));

	s = advance(conv, "bye");
	CHECK(s == Conversation::ENDED);

	// Every script runs to its first question without trouble.
	int ran = 0, asked = 0;
	for (int actor = 0; actor < 99; ++actor) {
		ConverseScript t;
		if (!t.load(a, actor))
			continue;
		StubHost h;
		Conversation c(t.bytes(), actor, h);
		c.setVar(Conversation::VAR_KARMA, 75);
		Conversation::State st = advance(c, nullptr);
		++ran;
		if (st == Conversation::NEED_LINE)
			++asked;
		// and then say goodbye
		advance(c, "bye");
	}
	std::printf("%d scripts ran, %d reached a question\n", ran, asked);
	CHECK(ran > 60);
	CHECK(asked > ran / 2);

	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}
