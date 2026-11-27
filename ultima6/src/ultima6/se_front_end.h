#ifndef ULTIMA6_ULTIMA6_SE_FRONT_END_H
#define ULTIMA6_ULTIMA6_SE_FRONT_END_H

#include "engine/backend.h"
#include "engine/framebuffer.h"
#include "files/shape.h"
#include "files/tile_data.h"

#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Ultima6 {

struct ObjList;

namespace Ultima6 {

/** What Savage Empire's "Create Character" decides: the name and the three stats (SAVAGE.EXE sub_15A7C writes them into the saved game). */
struct SeNewCharacter {
	std::string name;
	int strength = 0, dexterity = 0, intelligence = 0;

	/**
	 * What sub_15A7C does to the objlist: the party's first name; actor 1: strength, dexterity (also its moves), intelligence, hit points 2 * strength + intelligence; the clock of the game
	 * (8:00, day 1, month 1, year 0, also at globals 2 to 7 where the engine keeps it), the flags the party's members start with (bit 0 of actors 2, 4, 5, 7, 8, 9 and 0x17), and the byte at globals 0x181 that says a character was made.
	 */
	void applyTo(ObjList &actors) const;
};

/**
 * Savage Empire's front end (SAVAGE.EXE): the menu of CREATE.LZC, the Introduction (ten pages of the story, CREATE.LZC item 5), the Acknowledgements (the scroll of item 6), the creation of a
 * character (Intanya's questions), the animated opening of INTRO.LZC and the ending (ENDGAME.LZC). It is SAVAGE.EXE's own code read by hand: the scenes are lists of small operations (pictures
 * drawn into the back screen, shown by fading in, text put into a window of the screen with the original's word wrapping, waits for a key with the blinking arrow) that run on the time tick()
 * is given. The palettes are the three of SAVAGE.PAL, the font is SAVAGE.FNT, the pictures nested shape archives in the items of the *.LZC.
 *
 * APPROX(105): see docs/approximations.md: the music, the sounds of the creation's cards and the machine-speed tests are left out; the random numbers come from the seed given.
 */
class SeFrontEnd {
public:
	enum State { MENU, SCENE, DONE, QUIT };
	enum Scene { OPENING, INTRODUCTION, CREATION, ACKNOWLEDGEMENTS, ENDING };

	explicit SeFrontEnd(const std::string &dir, uint32_t seed = 1);

	bool ok() const {
		return _ok;
	}
	State state() const {
		return _state;
	}
	/** True while a scene plays by itself (the caller feeds it the time with tick()). */
	bool animating() const {
		return _state == SCENE;
	}
	void tick(unsigned ms);
	void render(Framebuffer &screen) const;
	/** Feeds a key; false when the front end is finished (wantsGame() says whether to go on into the game). */
	bool handleKey(int key);
	bool wantsGame() const {
		return _state == DONE;
	}

	/** What the saved game already holds: a character was made (the Journey Onward choice), and a game is in progress (a new character is refused). */
	void setSaveState(bool characterMade, bool inProgress);
	bool hasNewCharacter() const {
		return _hasCharacter;
	}
	const SeNewCharacter &character() const {
		return _character;
	}

	/** Starts at the menu as SAVAGE.EXE does after the opening: `first` is its argument of sub_14A37 (1 straight from the opening, 0 after a scene). */
	void showMenu(bool first);
	/** Plays the opening (Lord British presents..., the savannah) and then the menu. */
	void startOpening();
	/** Plays the ending alone (SAVAGE.EXE's D, B, Y and L arguments: days, months, years elapsed and 'A' Aiela / 'T' Tristia, 0 neither). */
	void startEnding(int days, int months, int years, char who);

	int selected() const {
		return _selected;
	}
	int choices() const {
		return _characterMade ? 4 : 3;
	}
	/** For tests: the lines of the text window as printed (one per row of the window), the page's picture, the scene now. */
	std::vector<std::string> textLines() const;
	int scene() const {
		return _scene;
	}
	bool finishedScene() const {
		return _state != SCENE;
	}
	/** The creation's current stats and the name typed, for tests. */
	const SeNewCharacter &creating() const {
		return _making;
	}
	bool typing() const {
		return _typing;
	}
	/** True when a key does something now: a wait for a key, the name, or a wait that a key ends. */
	bool acceptsKey() const {
		return _state == SCENE && !_ops.empty() && (_ops.front().kind == Op::PROMPT || _ops.front().kind == Op::INPUT || ((_ops.front().kind == Op::WAIT_MS || _ops.front().kind == Op::WAIT_FRAMES) && _ops.front().interruptible));
	}
	bool waitingForKey() const {
		return _state == SCENE && !_ops.empty() && (_ops.front().kind == Op::PROMPT || _ops.front().kind == Op::INPUT);
	}
	int paletteNumber() const {
		return _palIdx;
	}

private:
	struct Handle {
		std::string file;
		int item = 0;
		bool loaded = false;
		std::vector<Shape> shapes;
	};
	struct Op {
		enum Kind { CALL, WAIT_MS, WAIT_FRAMES, FADE_IN, FADE_OUT, DISSOLVE, PROMPT, INPUT } kind = CALL;
		int a = 0;
		std::function<void()> fn;
		std::function<bool(int)> keyFn; // INPUT: the key (true when it is taken and the operation is over)
		bool interruptible = false;     // a key ends the scene (back to the menu) while it waits
	};
	typedef std::vector<Op> Seq;

	// the machine
	int load(const char *file, int item);
	const std::vector<Shape> &shapes(int handle);
	void draw(std::vector<uint8_t> &buf, int handle, int frame, int x, int y);
	void shadow(std::vector<uint8_t> &buf, int handle, int frame, int x, int y, int colour);
	void setPalette(int n);
	void clearBuffer(std::vector<uint8_t> &buf, int colour);
	void window(int l, int t, int r, int b);
	void clearWindow(int colour);
	void putChar(unsigned char c);
	void newline();
	void scroll();
	void printBuffer(const std::string &buf);
	void say(const std::string &format, const std::string &arg = std::string());
	void printDirect(const std::string &text);
	void drawGlyph(unsigned char c, int x, int y, int fg, int bg);
	int textWidth(const std::string &t) const;

	// the program
	void start(Seq seq, int scene);
	void pushBack(Op op);
	void pushFront(Seq seq);
	void abort();
	void run();
	void startFade(bool in);
	void startDissolve();
	void stepDissolve(int ms);
	void quitToMenu(bool first);
	Op call(std::function<void()> fn) const;
	Op wait(int ms, bool interruptible = true) const;
	Op frames(int n, bool interruptible = true) const;
	Op fade(bool in) const;
	Op dissolve() const;
	Op prompt() const;
	Op input(std::function<bool(int)> fn) const;

	// the scenes
	void buildIntroduction(Seq &s);
	void buildAcknowledgements(Seq &s);
	void buildCreation(Seq &s);
	void buildOpening(Seq &s);
	void buildEnding(Seq &s);
	void menuDraw(std::vector<uint8_t> &buf, int selected, bool pressed);
	void menuChoose(int choice);
	void askQuestion(int index, int number);
	void creationEnd();
	void beginName();
	void finishCreation();
	void leaveScene(int selection);
	void cardAnimation(Seq &s, int stat);
	int random(int lo, int hi);

	std::string _dir;
	uint32_t _seed;
	bool _ok = false;
	State _state = MENU;
	int _scene = -1;

	// the screens: `_front` is the one that is shown, `_back` the one pictures are drawn on first
	std::vector<uint8_t> _front, _back;
	uint8_t _pal[3][256][3];
	int _palIdx = 1;
	Font _font;
	std::vector<Handle> _handles;
	// the text object of the original (ds:2B72): the window, the cursor, the colours
	int _wl = 0, _wt = 0, _wr = 319, _wb = 199;
	int _tx = 0, _ty = 0, _fg = 0, _bg = 0x48;
	std::string _buffer;
	std::map<int, std::string> _rows; // what the window shows, by the y of the row (for tests)

	// the fade and the waits
	int _fade = 0, _fadeLeft = 0, _fadeTotal = 0, _fadeSteps = 0;
	std::vector<uint32_t> _dissolveOrder; // the pixels of the picture in the order the driver's dissolve (TM.DRV 0x4BF6) copies them
	int _dissolveDone = 0, _dissolveLeft = 0;
	bool _black = false;
	double _wait = 0;
	std::deque<Op> _ops;
	bool _blink = false;
	double _blinkClock = 0;
	int _lastKey = 0;

	// the menu
	int _selected = 0;
	bool _characterMade = false, _inProgress = false;
	bool _hasCharacter = false;
	SeNewCharacter _character, _making;
	bool _typing = false;
	std::string _name;
	int _nameX = 0, _nameY = 0, _questionStage = 0;
	int _leaveSelection = 0;
	int _menuHandle = -1, _h1 = -1, _h2 = -1, _h3 = -1, _h4 = -1;
	uint32_t _rng;
	int _answers[3] = {0, 0, 0};
	int _question = 0, _nextQuestion = 0;
	bool _afterOpening = false;
	// the ending
	int _endDays = 0, _endMonths = 0, _endYears = 0;
	char _endWho = 0;
};

} // namespace Ultima6
} // namespace Ultima6

#endif
