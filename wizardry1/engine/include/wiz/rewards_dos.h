// SEGMENT REWARDS (DOS p-code segnum 19): the spoils of a won fight -- the chest (inspect / CALFO / disarm
// / open and its traps), gold and items, the experience points and the level drain refund -- and the three
// camp jobs the mainline sends here as XGOTO 24 (R)EAD the spell books, I)DENTIFY an item, locate a body
// with KANDI).  Ported from the DOS p-code (tools/pcode_decomp.py REWARDS all).
#pragma once
#include "wiz/maze_dos.h"

namespace wiz {

struct RewardsWorld {
    Party &party;
    Roster &roster;
    const Scenario &sc;
    Rng &rng;
    std::string rosterPath, partyPath;
    MazeGlobals &g;
};

struct RewardsResult {
    int xgoto = 7;               // 7 back to the maze, 16 the party is gone, 23 SPECIALS (a fled fight), 14 an item turned out cursed,
                                 // 27 back to CAMP
    bool windowClosed = false;
};

class Rewards {
public:
    Rewards(Dos &d, RewardsWorld &w) : d_(d), w_(w), g_(w.g) {}
    // REWARDS:1.  `xgoto` = 11 (the fight was won), 22 (the party ran away) or 24 (a camp job; MazeGlobals::g36
    // says which: 0 READ, 1 IDENTIFY, 2 KANDI; the character is MazeGlobals::g9)
    RewardsResult run(int xgoto);

private:
    struct EndAll {};            // EXIT(19:1)
    struct EndProc { int proc; };

    // ---- the spoils ----
    void treasure();                                      // REWARDS:6
    void chest();                                         // REWARDS:8
    void chooseTrap();                                    // REWARDS:10
    int  pickMember(int verbStr);                         // REWARDS:2
    void picture(int pic);                                // REWARDS:3
    void loadRewardRecord();                              // REWARDS:4 / 7
    void trapName(std::string &out, int type, int variant) const;   // REWARDS:12
    void showTrap(int type, int variant);                 // REWARDS:13
    void chestDone();                                     // REWARDS:9
    void triggerTrap();                                   // REWARDS:14
    void damage(int member, int count, int sides);        // REWARDS:15
    void blaster(bool mage);                              // REWARDS:16 / 17 / 18
    void groupDamage(int chance, int count, int sides);   // REWARDS:20
    void crossbow();                                      // REWARDS:19
    void revealTrap();                                    // REWARDS:21
    void guessTrap();                                     // REWARDS:22
    void inspectChest();                                  // REWARDS:23
    void calfoChest();                                    // REWARDS:24
    void disarmChest();                                   // REWARDS:25
    void disarmTry();                                     // REWARDS:26
    void openChest();                                     // REWARDS:27
    int  rollDice(int count, int sides, int add);         // REWARDS:29
    void dropOne(const int *tuple);                       // REWARDS:28
    void dropGold(const int *tuple);                      // REWARDS:30
    void dropItem(const int *tuple);                      // REWARDS:31
    void foundItem(int member, int item);                 // REWARDS:5
    void shareGold();                                     // REWARDS:32
    // ---- experience ----
    void experience();                                    // REWARDS:42
    void checkAlive();                                    // REWARDS:43
    void totalExperience();                               // REWARDS:44 / 45
    long long monsterExperience(const int *rec) const;    // REWARDS:46..48
    void refundDrain();                                   // REWARDS:49 / 50
    // ---- camp jobs ----
    void readBooks();                                     // REWARDS:33..35
    void locateBody();                                    // REWARDS:36..39
    void identifyItem();                                  // REWARDS:40

    Dos &d_;
    RewardsWorld &w_;
    MazeGlobals &g_;

    Win *back_ = nullptr;                // BACKGND
    // REWARDS:1's frame
    long long expEach_ = 0;              // L1..L3
    int alive_ = 0;                      // L4
    int kind_ = 1;                       // L5: 1 normal, 2 double (a room fought for the first time)
    int rewardIdx_ = 0;                  // L6
    int variant_ = 0;                    // L7: which of three traps of one kind
    int rec_[84] = {};                   // REWARDS:6's reward record (rec_[0] chest, [1] trap types, [2] count, [3 + 9k] tuples)
    long long gold_ = 0;                 // REWARDS:6's L3..L5
    int trap_ = 0;                       // REWARDS:8's L7: 0 trapless, 1..7
    bool looked_[6] = {};                // REWARDS:8's L1..L6: who inspected
    int member_ = 0;                     // REWARDS:25's U2.1: the member working on the chest
    int fightCount_[4] = {}, fightMonster_[4] = {}, fightFlag_[6] = {};   // what CINIT:11 left behind
    int xgoto_ = 7;
};

} // namespace wiz
