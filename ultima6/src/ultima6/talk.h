#ifndef ULTIMA6_ULTIMA6_TALK_H
#define ULTIMA6_ULTIMA6_TALK_H

#include <functional>
#include "engine/random.h"
#include "ultima6/conversation.h"
#include "ultima6/text_window.h"
#include "ultima6/world.h"

#include <memory>
#include <string>

namespace Ultima6 {
namespace Ultima6 {

struct GameClock;

/**
 * A running conversation with one actor: the script's interpreter, the game services it calls
 * (the party, inventories, stats and flags of the world) and the line the player is typing.
 * The Talk command creates one; the game routes keys to it until the conversation ends.
 */
class Talk : public ConversationHost {
public:
	Talk(World &world, TextWindow &scroll, BorlandRandom &rng, const GameClock &clock, int actor);

	/** False if the actor cannot talk (no script, or a creature that is not one of the speaking kinds). */
	bool start();
	/** Feeds a key; false when the conversation has ended. */
	bool key(int key);

	int actor() const {
		return _actor;
	}
	/** Whose portrait is on show (a script may show others) and its caption. */
	int portraitActor() const {
		return _portrait;
	}
	/** Whether the actor can be talked to at all (the script exists). */
	static bool canTalk(const World &world, int actor);
	/** What the game does when a conversation puts the party to sleep (an inn): set by the game. */
	std::function<void()> onSleep;

	// ConversationHost
	void print(const std::string &text) override;
	std::string playerName() override;
	std::string genderTitle() override;
	std::string timeOfDay() override;
	void showPortrait(int npc) override {
		_portrait = npc;
	}
	int random(int lo, int hi) override;
	std::string scriptName(int npc) override;
	std::string partyName(int npc, bool &inParty) override;
	int talkFlags(int npc) override;
	void setTalkFlags(int npc, int flags) override;
	bool inParty(int npc) override;
	int partyMember(int index) override;
	int joinParty(int npc) override;
	int leaveParty(int npc) override;
	bool npcNearby(int npc) override;
	bool wounded(int npc) override;
	bool poisoned(int npc) override;
	void heal(int npc) override;
	void cure(int npc) override;
	void resurrect(int npc) override;
	void setWorktype(int npc, int worktype) override;
	int adjustStat(int npc, int stat, int delta) override;
	bool hasObject(int npc, int obj, int quality, bool checkQuality) override;
	int countObject(int npc, int obj) override;
	void newObject(int npc, int obj, int quality, int quantity) override;
	void deleteObject(int npc, int obj, int quality, int quantity) override;
	void giveObject(int obj, int quality, int from, int to) override;
	int whoHasObject(int obj, int quality) override;
	int maxCarry(int npc) override;
	int objectWeight(int obj, int quantity) override;
	bool horsed(int npc) override;
	void mount(int npc) override;
	void addKarma(int n) override;
	void subtractKarma(int n) override;
	void sleep() override;

private:
	bool load(Bytes &script) const;
	int rosterIndex(int actor) const;
	void advance();

	World &_world;
	TextWindow &_scroll;
	BorlandRandom &_rng;
	const GameClock &_clock;
	int _actor;
	int _portrait;
	std::unique_ptr<Conversation> _conv;
	Conversation::State _state = Conversation::NEED_KEY;
	std::string _typed;
	std::string _nameOfActor(int actor) const;
};

} // namespace Ultima6
} // namespace Ultima6

#endif
