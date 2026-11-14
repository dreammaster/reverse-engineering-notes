#ifndef ULTIMA6_ULTIMA6_MD_CREATE_H
#define ULTIMA6_ULTIMA6_MD_CREATE_H

#include "engine/backend.h"
#include "engine/framebuffer.h"
#include "files/shape.h"
#include "files/tile_data.h"
#include "ultima6/ui.h"

#include <deque>
#include <string>
#include <vector>

namespace Ultima6 {

struct ObjList;

namespace Ultima6 {

/** What Martian Dreams' creation of a character decides: the name, the sex and the three stats. */
struct MdNewCharacter {
	std::string name;
	bool female = false;
	int strength = 0, dexterity = 0, intelligence = 0;

	/**
	 * What MARTIAN.EXE's sub_18557 does to the objlist when the questions are over: the party's first name; actor 1 (the Avatar): the three stats, level 4, experience 600, hit points
	 * 24 * level + 2 * strength, 20 moves, and the sex in the type of the body (0x157 a man, 0x158 a woman), which the base type of actors 0 and 1 copies.
	 */
	void applyTo(ObjList &actors) const;
};

/**
 * The creation of a character of Martian Dreams (MARTIAN.EXE sub_18557 and sub_17E7A): Dr. Freud talks to the Avatar on the way to Mars. A scene is a run of lines of SCENETXT.LZC item 5 over
 * the pictures of CREATE.LZC (the study with the faces of Freud, a small figure that drifts about, and a foreground); each line waits as its entry of the table says (seconds, any key, A or B,
 * A to C, or the name that is typed), and the questions' program (sub_18557 itself, turned into small operations by tools/gen_md_create.py) chooses the next scene by the answers and gives the
 * three stats the random numbers 24 to 26, 22 to 24 and 18 to 22 (the stat the answers say, else the last). It runs on the time that tick() is given (the original's 18.2 a second), a key is
 * sent by key().
 *
 * APPROX(93): the colours of the lines (the original passes 4 and 6) are the gold of the introduction (#90); the original's random generator starts from the clock, this one from the seed given;
 * a scene looks at the keyboard and the clock about a thousand times a second here, as fast as the original's loop must run; the name is drawn over the scene as it is, without the
 * background the original leaves; Escape leaves the creation (the original has no way out); the music is left out.
 */
class MdCreation {
public:
	MdCreation(const std::string &dir, uint32_t seed);

	bool ok() const {
		return _ok;
	}
	bool finished() const {
		return _state == DONE;
	}
	bool cancelled() const {
		return _state == CANCELLED;
	}
	bool animating() const {
		return _state == RUN || _state == FADE;
	}
	void tick(unsigned ms);
	void key(int k);
	void render(Framebuffer &screen) const;

	const MdNewCharacter &result() const {
		return _result;
	}
	/** The scene being shown (its first entry in the table) and the line now on the screen, for tests. */
	int scene() const {
		return _sceneStart;
	}
	const std::string &line() const {
		return _text;
	}
	bool typing() const {
		return _typing;
	}
	const std::string &typed() const {
		return _name;
	}

private:
	enum State { RUN, FADE, DONE, CANCELLED };

	void pass();
	void startScene(int index);
	void endScene(int value);
	void runProgram();
	void draw();
	int random(int a, int b);
	uint32_t now() const;

	std::string _dir;
	bool _ok = false;
	State _state = RUN;
	double _ms = 0;
	uint32_t _rng;
	std::deque<int> _keys;

	// the pictures
	std::vector<Shape> _face, _foreground, _figure;
	Palette _palette;
	Font _font;
	std::vector<std::string> _lines; // SCENETXT item 5
	std::vector<uint8_t> _front;
	int _fadeLeft = 0;

	// the program
	size_t _pc = 0;
	int _acc = 0, _reg[6] = {0, 0, 0, 0, 0, 0}; // accumulator, di, si, the three answers
	int _stat[3] = {0, 0, 0}, _random[3] = {0, 0, 0};
	bool _final = false, _cmp = false;

	// the scene (sub_17E7A)
	bool _inScene = false;
	int _sceneStart = 0, _idx = 0;
	bool _redraw = true; // var_20 = 0
	int _value = -1;     // var_6: the key
	uint32_t _t0 = 0, _tMouth = 0, _tBlink = 0, _tFigure = 0;
	int _mouth = 0, _blink = 0, _dx = 0, _dy = 0, _frame = 0;
	bool _typing = false; // the name is asked
	std::string _name;
	uint32_t _cursor = 0xFFFFFFFFu;
	std::string _text;
	int _textColour = 0;

	MdNewCharacter _result;
};

} // namespace Ultima6
} // namespace Ultima6

#endif
