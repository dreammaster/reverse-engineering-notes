// SEGMENTS COMBAT / CINIT / CUTIL / SWINGASW / CASTASPE (DOS p-code segnums 7, 10, 11, 9, 8): a fight.
// Ported from the DOS p-code (tools/pcode_decomp.py COMBAT|CINIT|CUTIL|SWINGASW|CASTASPE all).
//
// The original keeps every fight in COMBAT:1's frame: five 123-word "group" records (group 0 is the
// party, 1..4 the monster groups -- each: [0] identified, [1] units left, [2] units at the start,
// [3] monster number, [4 + 8u + k] unit u's eight words, [76 + k] the monster record) plus the
// message window, the enemy list window and a few flags.  The port keeps that layout so every
// expression can be read against the p-code: unit(g, u, k), rec(g, k).
//
//   unit words:  0 target (group for a party member, party index for a monster),
//                1 action (party: -1 none, 0 parry, ... / monster: spell hash, -1 fight, -2 run,
//                  -3 breath, -4 call for help, -5 dispell), 2 initiative (-1 = no action),
//                3 hit points, 4 armour class modifier, 5 spell-silence / hold counter,
//                6 level, 7 status (0 OK, 1 afraid, 2 asleep, 3 paralysed, 4 stoned, 5 dead, 7 lost)
#pragma once
#include "wiz/maze_dos.h"

#include <array>

namespace wiz {

struct CombatWorld {
    Party &party;
    Roster &roster;
    const Scenario &sc;
    Rng &rng;
    std::string rosterPath, partyPath;
    MazeGlobals &g;
    Win *&g10;
};

struct CombatOutcome {
    int xgoto = 11;                  // 11 REWARDS (the fight was won), 7 back to the maze (fled / friendly),
                                     // 16 the party is dead, 9 / 10 MALOR took them away
    bool windowClosed = false;
};

class Combat {
public:
    Combat(Dos &d, CombatWorld &w) : d_(d), w_(w), g_(w.g) {}
    // COMBAT:1.  The encounter is MazeGlobals::g37 (monster number) and g29 (0 wandering, 1 / 2 a room).
    CombatOutcome run();

    // what REWARDS needs to know afterwards (CINIT:11 leaves it in the I/O cache)
    int groupsAtStart(int g) const { return G_[g][2]; }
    int monsterOf(int g) const { return G_[g][3]; }
    int memberFlag(int i) const { return flag_[i]; }

private:
    struct ExitProc { int proc; };        // EXIT(seg:proc) of a nested procedure
    struct EndCombat {};                  // EXIT(7:1)
    struct EndRound {};                   // EXIT(11:1) / EXIT(8:1) etc.: leave the current phase

    // ---- the p-code frame ----
    int G_[5][123] = {};                  // COMBAT:1 L13
    int disk_[6] = {};                    // L628: CHARDISK as the fight began
    int flag_[6] = {};                    // L634: per member "used a spell / acted"
    u8 pic_[4] = {255, 255, 255, 255};    // L640: the pictures on screen
    Win *list_ = nullptr;                 // L8
    Win *msg_ = nullptr;                  // L7
    int phase_ = 0;                       // L9
    int ambush_ = 0;                      // L10: 1 the monsters were surprised, 2 the party was
    bool done_ = false;                   // L11
    bool msgPending_ = false;             // L12
    int msgRow_ = 0;                      // L6
    int tgt_ = 0, act_ = 0, unitIdx_ = 0, grpIdx_ = 0, time_ = 0;   // L1..L5 of COMBAT:1 (RUNNER's COMBAT:5 loop)
    int xgoto_ = 11;

    int &unit(int g, int u, int k) { return G_[g][4 + 8 * u + k]; }
    int &rec(int g, int k) { return G_[g][76 + k]; }
    int rnd(int n) { return w_.rng.mod(n); }
    Character &chr(int i) { return w_.party.member(i); }
    static u16 rawWord(const Character &c, int word) { return u16(c.raw[word * 2] | (c.raw[word * 2 + 1] << 8)); }
    bool bit(u16 v, int i) const { return (v >> (i & 15)) & 1; }
    int partyCount() const { return w_.party.count(); }

    // ---- COMBAT.p ----
    void roundActions();                                  // COMBAT:5
    void endFight();                                      // COMBAT:2
    void say(const std::string &s);                       // COMBAT:6 (centred, line by line)
    void begin(const std::string &s);                     // COMBAT:4: a new message page
    void add(const std::string &s);                       // COMBAT:3: a line under the page header
    // ---- CINIT.p ----
    void cinit();                                         // CINIT:1
    void cinitStart();                                    // CINIT:12
    void cinitMonsters();                                 // CINIT:2
    void cinitEnd();                                      // CINIT:11
    void saveLocations();                                 // CINIT:19
    void makeEnemies();                                   // CINIT:13
    void loadGroup(int monster, int group);               // CINIT:14
    int  roll(int dice, int sides, int add);              // CINIT:15
    void setupParty();                                    // CINIT:16
    void friendly();                                      // CINIT:17
    void alignDrift(bool toGood, bool toEvil);            // CINIT:18
    // ---- CUTIL.p ----
    void cutil();                                         // CUTIL:1
    void redraw();                                        // CUTIL:36
    void tidyGroups();                                    // CUTIL:38
    void dropPictures(const u8 *now);                     // CUTIL:39 / 43
    void slidePicture(int which, int total);              // CUTIL:40 / 42 / 41
    void regenerate();                                    // CUTIL:44
    void tickTimers();                                    // CUTIL:45 / 46
    void enemyList();                                     // CUTIL:47
    void sortParty();                                     // CUTIL:48
    void partyStatusTail(int i, bool &active);            // CUTIL:49
    void swapMembers(int a, int b);                       // CUTIL:50
    void wake(int g, int u, int pct);                     // CUTIL:37
    void inputPhase();                                    // CUTIL:2
    void headline(int strNo);                             // CUTIL:3
    void echoOrder(int member, const std::string &text);  // CUTIL:4 / 5
    bool typedAhead(const std::string &keys);             // CUTIL:7
    void pickGroup(int member, int promptNo, int action); // CUTIL:9
    void tryRun(int member);                              // CUTIL:32
    void giveUpRunning();                                 // CUTIL:33
    void itemSideEffects(int member, int slot, int changeTo, int chance);   // CUTIL:15
    void useItem(int member);                             // CUTIL:10 / 13 / 16..18
    void castSpell(int member);                           // CUTIL:19..31
    std::string spellName(int idx) const;                 // CUTIL:27
    int  spellHash(int idx) const;                        // CUTIL:26
    // ---- SWINGASW.p ----
    std::string nameOf(int g, int u, int tmpl);           // SWINGASW:3
    int  dice(int count, int sides, int add);             // SWINGASW:8
    void sayAbout(int strNo, int g, int u, int tmpl, int strNo2);   // SWINGASW:9
    void kill(int g, int u);                              // SWINGASW:2 / CASTASPE:5
    void specialHit(int target, int group);               // SWINGASW:12..14
    void pickVerb(bool partyStyle);                       // SWINGASW:7 / 15..17
    void monsterAttacks();                                // SWINGASW:11
    void memberAttacks();                                 // SWINGASW:18
    void callForHelp();                                   // SWINGASW:19 / 20
    void runAway();                                       // SWINGASW:21
    void dispell();                                       // SWINGASW:22
    void swing();                                         // SWINGASW:1
    // ---- CASTASPE.p ----
    bool noEffect(int g, int u);                          // CASTASPE:4
    void hurt(int g, int u, int amount);                  // CASTASPE:14
    void hurtDice(int g, int u, int count, int sides);    // CASTASPE:19
    void healDice(int g, int u, int count, int sides);    // CASTASPE:18
    void afflict(int g, int u, int chance, int msgNo, int effect);   // CASTASPE:15
    void armour(int g, int delta, int first, int last);   // CASTASPE:17
    void blast(int g, int count, int sides, int element); // CASTASPE:25
    void loktofeit(int level);                            // CASTASPE:26
    void makanito();                                      // CASTASPE:27
    void malor();                                         // CASTASPE:28
    void breathe();                                       // CASTASPE:32
    void cannotCast(int strNo, const std::string &spell); // CASTASPE:31
    void sleepGroup(int g, int level);                    // CASTASPE:24
    void wish(int spell);                                 // CASTASPE:6..13
    void spellEffect(int hash, int u, int g, int level);  // CASTASPE:29 / 30
    void cast();                                          // CASTASPE:1
    int verb_ = 1155;                                     // SWINGASW:1's L1: the verb string of the attack message

    int code_[6] = {};                    // CUTIL:2's L59: 99 nothing yet, 1 / 2 a mage / priest spell, <= 0 an item that changes
    int slot_[6] = {};                    // CUTIL:2's L53: the spell level / item slot that goes with it
    int pendingText_ = -1;                // the string G9 names the order just given by (5000 + spell)

    Dos &d_;
    CombatWorld &w_;
    MazeGlobals &g_;
};

} // namespace wiz
