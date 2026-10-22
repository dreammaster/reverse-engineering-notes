#pragma once
// Combat engine: ports of KEH.EXE rollAttack / resolveAttack / memberAttack and the combatant bookkeeping.
#include "fod/vm.h"
#include <functional>

namespace fod {

struct Combatant {                // 12 bytes, KEH _combatants
	uint8_t type = 0;             // 0 party member, 1 monster, 2 enemy group slot
	uint8_t index = 0;            // party slot / monster index / group index
	uint8_t sub = 0;              // group slot
	uint8_t tType = 0, tIndex = 0, tSub = 0;   // current target
	uint8_t command = 0;          // 0 move closer, 1 evade, 4 attack, 5 reload, ..., 0Eh none
	uint8_t speed = 0;
	uint8_t distance = 0;
	uint8_t flags = 0;            // bit0: already considered
	uint8_t attacks = 1;          // attacks per round (+0Ah)
	uint8_t b11 = 0;
};

class Combat {
public:
	Combat(World &w, Host &h) : _w(w), _h(h) {}

	// -- state shared by the attack routines (KEH globals 2E43A..2E444) --
	int fireMode = 0;             // 0 single, 1 burst, 2 full auto (byte_2E442)
	int shots = 1;                // byte_2E443
	int divisor = 1;              // word_2E43A
	bool calcOnly = false;        // byte_2E43E: rollAttack returns the hit chance
	uint32_t xpAccum[5] = {};     // per party member xp gained this fight
	int sureHit = 0;              // non-zero: attacks always hit (thrown/blast)
	std::vector<Combatant> list;  // the current combatant table

	// target resolution (KEH selectTargetStats / pickWeakestInGroup): sets _targetHp/_xpReward/_targetNameType
	void selectTarget(const Combatant &c);
	bool pickWeakestInGroup(const Combatant &c);

	// attack primitives
	int distanceToParty(int x, int y) const;
	int rollAttack(const Combatant &c, int &damage, int &wound);   // 0 miss 1 no damage 2 hit 3 useless 4 kill
	int resolveAttack(const Combatant &c);
	void memberAttack(int slot, const Combatant &c);
	void monsterAttack(const Combatant &c, int attacks);
	bool checkWeaponMalfunction(int slot);

	// -- whole fight (KEH runCombat, see combat_fight.cpp) --
	enum class GroupAction { None, GetCloser, Flee };
	// Fills the command of one active party member; may return a whole-party action instead.
	std::function<GroupAction(Combat &, int slot, Combatant &rec)> commands;
	struct Outcome { bool fled = false, wiped = false; };
	Outcome run(bool useMaxRange);              // list must already hold the enemies (buildCombatants)
	void autoCommand(int slot, Combatant &rec); // default policy: shoot the first living enemy
	void setupAttack(const Combatant &c);       // KEH resetAttackState: fire mode, shots, verb for this attacker
	void multiShot(const Combatant &c);         // KEH attackMultiShot: an enemy attacks a party member
	void combatMoveEnemy(Combatant &c);

	// -- enemy side: KEH buildCombatants / chooseEnemyAction / sortCombatants --
	int attackCounts[5] = {};     // per party member: times targeted this round (word array at dseg 0CDAh)
	unsigned maxSpeed = 0;        // word_2EC0A: highest initiative among enemies
	int rangeBand(int weaponItem) const;          // getWeaponRangeBand(item,0,0,0,1)
	int partyMaxRange() const;                    // getMaxWeaponRange
	bool withinRadiusOfParty(int radius, int x, int y) const;
	int buildCombatants(bool useMaxRange);
	void chooseEnemyAction(size_t idx, int weaponItem, int hp, int hpMax, int behavior, int x, int y);
	void sortCombatants();

	// helpers also used by the AI
	bool combatantPos(const Combatant &c, int &x, int &y, int &hp) const;
	std::string attackerName(const Combatant &c) const;
	std::string targetName(const Combatant &c) const;

private:
	int16_t targetHpValue() const;
	void addTargetHp(int delta);
	void useUpAmmo(int slot, int itemSlot);
	int armorClassOf(const Combatant &c) const;
	int trySkillImprove(int slot, int skill);
	void removeDefeated(const Combatant &c);
	bool retarget(Combatant &r);
	bool findNearbyTarget(Combatant &r);

	void executeCombatant(size_t idx, bool &fled);
	bool tryFlee();
	void removeDeadMonsters();
	void postCombatCleanup();
	void awardExperience();
	void rechooseEnemyActions();

	std::string _verb;
	std::string _attackerOverride;
	int _blast = 0;
	World &_w;
	Host &_h;
	// current target (filled by selectTarget)
	uint8_t *_hpPtr = nullptr;     // points at the little-endian int16 hit points
	uint32_t _xpReward = 0;
	int _targetNameType = 0;       // sex bits for "him/her/it"
	int _weakestIdx = 0;
	uint8_t _dummyHp[2] = {0, 0};
};

} // namespace fod
