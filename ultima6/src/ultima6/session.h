#ifndef ULTIMA6_ULTIMA6_SESSION_H
#define ULTIMA6_ULTIMA6_SESSION_H

#include "engine/backend.h"
#include <functional>
#include "ultima6/map_renderer.h"
#include "ultima6/movement.h"
#include "ultima6/actors.h"
#include "ultima6/combat.h"
#include "ultima6/creatures.h"
#include "ultima6/magic.h"
#include "ultima6/talk.h"
#include "ultima6/text_window.h"
#include "ultima6/world.h"

#include <memory>
#include <functional>
#include <string>
#include <vector>

namespace Ultima6 {
namespace Ultima6 {

/**
 * The game gameClock as the saved game keeps it (objlist +0x1BF3..): minute, hour, day (1-28), month
 * (1-13) and year; GAME.EXE's time advance (sub_13305) steps it by minutes.
 */
struct GameClock {
	int minute, hour, day, month, year;

	void advance(int minutes);
	/** Day of the week 1-7 as the schedule check computes it: (day - 1) % 7 + 1. */
	int dayOfWeek() const {
		return (day - 1) % 7 + 1;
	}
};

/**
 * The running game: party position, gameClock, message log and the commands. Movement and time follow the
 * original in structure (one step is one minute; directions are the eight-way table at ds:01A2 / 01AA
 * with north first, clockwise); walkability is the port of GAME.EXE's sub_25D7F in Movement.
 */
class Game {
public:
	explicit Game(World &world);

	/** Handles one key; false when the game should end. */
	bool handleKey(int key);
	/** A click at (x, y) of the 320 x 200 screen as the key it stands for (0: none); see the definition. */
	int mouseKey(int x, int y, bool right);
	void render(Framebuffer &screen);
	/**
	 * One redraw of the idle game (the key poll sub_154AE redraws the map, sub_12C6C): the frame counter of the animated tiles (word_3DB88) and the counter of the palette cycling
	 * (byte_3DA22, sub_114D0) both step; neither does while time is stopped (byte_404BA). The host calls it about eight times a second while it waits for a key. APPROX(75)
	 */
	void animate() {
		if (_timeStop <= 0) {
			++_animTick;
			++_palTick;
		}
	}

	/** Moves the party leader one step in a direction (0 = north, clockwise); false if blocked. */
	bool step(int dir);
	/** Whether the party leader can enter a tile. */
	bool canWalk(int x, int y, int z);

	/** Lets game time pass: the gameClock, the hourly schedules and the people walking about. */
	void advanceTime(int minutes);
	/** sub_2AB7A: runs the actors' turns after a command until it is the player's turn again; a game minute passes whenever nobody has moves left. */
	void runTurns();

	/** Writes the saved game (the world and the gameClock) to `saveDir`. */
	bool save();
	std::string saveDir;

	void message(const std::string &text);
	/** Prints the command prompt: the name of the party member in control, a colon, and '>'. */
	void prompt();

	/** Set when the Codex has been taken to the ending (use_vortex_cube): the front end then plays Ending. */
	/** The music that goes with the place (sub_377C8): a track number for the list in MusicTrack names, 0 for none. */
	/** Set by the host: plays sound effect `effect` of sub_36E85 with its first argument `param`. */
	std::function<void(int, int)> sound;
	int music() const {
		return _music;
	}
	bool endingStarted() const {
		return _ending == 2;
	}
	/** Years, months and days of play since the new game's start (4/7/161), borrowing 28 days and 13 months. */
	void timePlayed(int &years, int &months, int &days) const;

	int x, y, z;
	GameClock gameClock;
	std::vector<std::string> log;
	TextWindow scroll; // the text scroll of the play screen

private:
	World &_world;
	MapRenderer _renderer;
	Movement _movement;
	ActorManager _actors;
	MoveRules _rules;
	BorlandRandom _rng;
	Combat _combat;
	Creatures _creatures;
	Magic _magic;
	const Magic::SpellInfo *_spell = nullptr; // the spell being cast, waiting for its target
	std::string _invocation;                  // circle digit and syllables typed so far
	void castKey(int key);
	int _stealing = -1; // the person whose things are on show for the Pickpocket spell
	void stealItem(int idx);
	void castSpell(const Magic::SpellInfo *spell, int caster, bool promptAfter = true);
	void useStaff(int staff);
	std::string restCheck();
	bool useHorse(int tx, int ty);
	bool handleKeyInner(int key);
	void minutePassed();
	bool boardOrLeave(int tx, int ty);
	bool shipParts(int part, int &mid, int &bow, int &stern);
	int shipHeading(int dir) const;
	void shipEnds(int mx, int my, int h, int &bx, int &by, int &sx, int &sy) const;
	void moveShipParts(int h);
	int _shipBow = -1, _shipStern = -1, _shipQuality = 0, _shipQuantity = 150; // the parts of the ship being sailed, and the number and hit points of its middle part
	bool _aboard = false;     // the party is on a skiff or raft: the leader's body has the vehicle's type
	int _footType = 0;        // the leader's own type while aboard
	int _footFrame = 0;
	void partyHazards();
	void castAt(int tx, int ty);
	void attackAt(int tx, int ty);
	void attackWeapon(); // announces the weapon of this round of the Attack command and asks for a target
	std::vector<int> _attackWeapons; // sub_2D7B5: the readied weapons (up to 8) the Attack command goes through, one target for each
	size_t _attackIndex = 0;
	std::unique_ptr<Talk> _talk; // a conversation in progress
	int _cx = 5, _cy = 5;        // cursor cell in the map view while targeting with it
	bool _cursor = false;
	void look(int tx, int ty);
	int _pending = 0;            // command letter waiting for its target (a direction), or 0
	bool selectTarget(int key, int &tx, int &ty) const;
	void command(int key);
	void talkAt(int tx, int ty);
	void startTalk(int actor);
	void getAt(int tx, int ty);
	void useAt(int tx, int ty);
	void useObject(int idx);
	bool spellSpecial(const Magic::SpellInfo &spell, int caster, int tx, int ty, int target);
	void updateMoons();
	void placeGates();
	void buryMoonstone(int idx);
	void digAt(int key);
	void telescopeAt(int key);
	void endCharm(int actor);
	World::Obj _vanished; // the thing the Vanish spell took away (word_404BC), for Reappear
	bool _haveVanished = false;
	void operateDrawbridge(int crank);
	bool _eclipse = false; // byte_404BB: the Eclipse spell
	unsigned _animTick = 0;    // word_3DB88
	uint8_t _palTick = 0;      // byte_3DA22
	int _timeStop = 0; // minutes of time stop left (byte_404BA)
	void toggleCombat();
	void helpParty();
	void avatarDied();
	bool _combatMode = false;
	int _music = 0;
	void sfx(int effect, int param = 0) {
		if (sound)
			sound(effect, param);
	} // the track that should be playing: 1 to 12 in the order of the list at ds:2955 (ULTIMA, BOOTUP, INTRO, CREATE, FOREST, HORNPIPE, ENGAGE, STONES, DUNGEON, BRIT, GARGOYLE, END), 0 none
	void chooseMusic();
	void drawGem(Framebuffer &screen) const;
	bool _gem = false; // the overhead map of a gem is on the screen until a key is pressed
	int vehicleDirection(int requested);
	void windChanges();
	bool _spellQuiet = false;  // the spell has said its result itself (or the original says none)
	bool _telekinesis = false; // Use from a distance (the Telekinesis spell)
	void actorStatusDecay();
	void useSilverHorn();
	void readyObject(int idx);
	void unreadyObject(int idx);
	void dropStart(int idx);
	void dropWhere(int count);
	void moveItemStart(int idx);
	void moveWhere(int count);
	void moveGive(int target);
	bool moveBlocked(int idx, int x, int y, int dir);
	bool memberNear(int actor) const;
	bool _moveTarget = false; // "To " waits for the party member to receive the object chosen in the pack
	void dropAt(int tx, int ty);
	bool dropSurface(int tx, int ty) const;
	bool dropRecipient(int target, int item) const;
	void dropPlace(int item, int count, int tx, int ty, int recipient, bool free = true);
	int _instrument = 0;       // play_instrument (329EA): the effect of the instrument being played; the digit keys are its notes until another key is pressed
	int _actorView = -1;       // the person Look is showing (sub_2F969), until a key is pressed
	std::string _actorName;
	int _packScroll = 0;       // the first of the objects the pack view shows (a multiple of four)
	bool _dropPending = false; // "Drop-" is waiting for the object to be chosen in the inventory
	int _dropItem = -1;		// the object chosen, while "Location: " waits for its place
	int _dropCount = 0;
	int _crystalLat = 0;
	static bool degrees(const std::string &s, char plus, char minus, int &out);
	bool _gemSpell = false; // the gem on show is the Peer spell's: "Done" follows it
	bool _eye = false;      // the Wizard Eye is being steered
	int _eyeX = 0, _eyeY = 0, _eyeLeft = 0, _eyeObject = -1;
	void eyeKey(int key);
	bool _peek = false; // the map shows somewhere else (a telescope) until a key is pressed
	int _peekX = 0, _peekY = 0;
	void textKey(int key);
	void shrineChant(const std::string &mantra);
	bool _textActive = false; // a line of text is being typed (sub_1737A): the mantra at a shrine
	std::string _textBuf;
	int _textMax = 0;
	int _textPurpose = 0; // 1 a mantra, 2 a wish
	void wishDone(const std::string &wish);
	void gateTo(int n);
	bool enterMoongate(int px, int py);
	void useOrb(int idx);
	void orbAt(int tx, int ty);
	uint8_t _gateA = 0, _gateB = 0, _phaseA = 0, _phaseB = 0; // byte_40516..40519: the moons' gates and phases
	void moveSelect(int tx, int ty);
	void moveTo(int key);
	void readBook(int idx);
	bool _scrollStudied = false; // word_404F8: after "You study the scroll!" the Gargish translations in books are shown
	void useKeyOnDoor(int idx, bool lock, bool magic, bool unlock);
	void useChest(int idx, bool lock, bool magic, bool unlock);
	void searchHere(int idx);
	void extinguishLight(int idx);
	void useLever(int idx);
	void useLadder(int idx);
	void useKeyOn(int tx, int ty);
	void usePotionOn(int actor);
	void consumeOne(int idx);
	int actingActor() const;
	int findInInventory(int type);
	void fillBucket(bool milk);
	void useFishingPole();
	void explodeKeg();
	void explodeKegObject(int idx);
	void blowUpDoors(int cx, int cy);
	void blastActors(int tx, int ty, std::vector<int> &out);
	bool _fishedToday = false; // one fish a day (byte_404BF, cleared when the day changes)
	int _kegTimer = 0;         // minutes until a lit powder keg explodes (word_404F4)
	int _kegObject = -1;
	bool carriedObject(const World::Obj &o) const;
	int _useItem = -1; // the key, lock pick or potion waiting for its target
	void changeLevel(int nx, int ny, int nz);
	void restKey(int key);
	void restCamp(int guard);
	bool eatFood(int member);
	void restHeal(int member, bool say);
	int _restCooldown = 0; // byte_4049B: hours before another rest mends the party
	void followLeader(int mode = 0);
	bool heldFollower(int actor);
	int formationBusy(int slot, int x, int y);
	int _formation[16] = {};
	bool _confirmSave = false;
	bool _firstPrompt = true;
	bool _inventory = false; // the panel shows the inventory instead of the statistics
	int _selected = 0;
	std::vector<int> _items;
	bool inventoryKey(int key);
	bool _dead = false;
	int _ending = 0; // 1 once the Codex has vanished and a key is awaited, 2 when the ending is to be played
	bool useVortexCube();
	std::string _number;
	int _restHours = 0; // digits typed for the Rest command
	int _member = 0; // party member shown in the panel (the one the player controls, byte_404A4, while no other is looked at in the inventory)
	int _solo = -1;      // byte_40513: the roster place of the member who walks alone (the digit keys), -1 for party mode
	int _soloSaved = 0;  // byte_40514: the worktype the controlling member had before it was made to be controlled
	void soloKey(int key);
	// Martian Dreams' Use (md_use.cpp)
	std::vector<int> mdRecipesOf(int type) const;
	bool mdUsable(int type) const;
	void mdUseObject(int idx);
	// Savage Empire's Use (se_use.cpp)
	void seUseObject(int idx);
	bool seUsable(int type) const;
	void seAction(uint32_t sub, int idx);
	void seSearch(int idx);
	void seEnd();
	bool seInView(int actor) const;
	bool seGive(int actor, int type, int quantity);
	void seFill(int container, int type, int quantity);
	int seMake(int type, int x, int y, int z, int quantity, int quality = 0); // sub_1B486: a new object on the map
	int seFind(int type, int x, int y) const;                                  // sub_1AC47: an object of the type on the square
	bool _seAsync = false;
	std::function<void(int, int, int)> _seThen; // what a handler does with the object chosen by "On " (-2 for none chosen) and its square
	void seAsk(std::function<void(int, int, int)> then);
	int seRoll(int lo, int hi);                                                 // sub_3B58F
	int seStrength(int actor) const;                                            // sub_285A0: the strength, less 3 / plus 3 by the status bits
	int seMaxHp(int actor) const;                                               // sub_2872A: twice the strength and four times the level, 1 to 255
	bool seTooWeak(int strength);                                               // sub_30E72: a roll of 1-30 below (45 - strength) / 2
	bool seReach(const World::Obj &target, int tx, int ty);                     // the "Out of range!" test of the "On " handlers (prints it)
	void seChosen(int tx, int ty);
	void mdContinue(int stage, int partner);
	void mdChosen(int tx, int ty);
	void mdAction(int recipe, int item, int first, int second);
	void mdLight(int lighter, int target);
	std::string mdLookExtra(const World::Obj &o) const;
	bool _mdAsync = false;
	int _mdItem = -1, _mdFirst = -1, _mdSecond = -1, _mdStage = 0;
	std::vector<int> _mdRecipes;
	void setControl(int index); // sub_13AE6
	int _leaderObject; // index of the leader's body in the world's object list
};

} // namespace Ultima6
} // namespace Ultima6

#endif
