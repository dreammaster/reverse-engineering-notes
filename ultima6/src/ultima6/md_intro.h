#ifndef ULTIMA6_ULTIMA6_MD_INTRO_H
#define ULTIMA6_ULTIMA6_MD_INTRO_H

#include "engine/framebuffer.h"
#include "files/shape.h"
#include "files/tile_data.h"
#include "ultima6/md_driver.h"

#include <string>
#include <vector>

namespace Ultima6 {
namespace Ultima6 {

/**
 * The introduction of Martian Dreams, played from the script that MARTIAN.EXE's routine sub_19266 makes: it loads the pictures of SCENE1-4B.LZC and MOONGATE.LZC, draws their shapes at places of the
 * 320 x 128 window (y 24-151), puts a line of SCENETXT.LZC under it, shows it, waits, and goes on; the calls it makes were got by running that routine on tools/emulate_intro.py, and
 * md_intro_script.inc holds them (tools/gen_intro_script.py). Any key ends the introduction, as it does in the original.
 *
 * The same player plays the ending (sub_15C84, the files ENDGAME.LZC and MOONGATE.LZC) and the credits (sub_1675E: LOGOS, FAIR, CREDITS, LOWELL, FUSE, FLASH, FLIGHT, CABIN and MARS .LZC), whose
 * calls were got the same way (MD_SCRIPT_START holds where each script starts).
 *
 * The shapes that turn and grow (the driver entry 0x2817A of SUPERTM.DRV) are drawn by the driver's own code on Cpu8086 (MdDriver); a plain shape is put with its hot spot at the place given, as the
 * driver does, and the dots of the thrown orb (0x28172, a filled circle of radius 1: five pixels) are drawn by hand; everything is clipped to the whole screen, as the screen the game gives them.
 *
 * APPROX(90): the other drawing routines of TM.DRV are not run; the fades (sub_155DB, sub_155A5) last half a second; the flashes of the palette (sub_20951), the music and the colour of the
 * lines are left out; the lines are centred in gold.
 */
class MdIntro {
public:
	enum Script { INTRODUCTION = 0, ENDING = 1, CREDITS = 2 };
	explicit MdIntro(const std::string &dir, Script script = INTRODUCTION);

	bool ok() const {
		return _ok;
	}
	/** Goes on for so many milliseconds. */
	void tick(unsigned ms);
	bool finished() const {
		return _finished;
	}
	void stop() {
		_finished = true;
	}
	void render(Framebuffer &screen) const;
	/** How far the script has got (the number of its calls done). */
	size_t position() const {
		return _pc;
	}
	/** Runs the script to the end at once, for a test. */
	void runToEnd();
	/** The lines on the screen now. */
	const std::vector<std::string> &lines() const {
		return _lines;
	}

private:
	struct Draw {
		int kind; // 0 shape, 2 dot, 3 shape of the driver's entry 0x2817A
		int frame, handle, x, y, size, angle;
	};
	struct Handle {
		int file = -1, item = 0;
		bool loaded = false;
		Bytes raw; // the item of the archive
		std::vector<Shape> shapes;
	};

	void run();
	void startFade();
	void compose();
	const std::vector<Shape> &shapes(int handle);
	const Bytes &archive(int handle);
	const std::string &line(int set, int index);
	void shapeAt(int handle, int frame, int x, int y);

	std::string _dir;
	bool _ok = false, _finished = false;
	size_t _pc = 0, _end = 0;
	int _wait = 0;
	int _fade = 0;     // 1 in, -1 out, 0 none
	int _fadeLeft = 0; // ms
	int _fadeTotal = 0, _fadeSteps = 0;
	bool _black = true;
	std::vector<Handle> _handles;
	std::vector<std::vector<std::string>> _texts;
	std::vector<std::string> _lines; // the caption now
	int _captionY = 160;
	std::vector<Draw> _draws;
	std::vector<uint8_t> _front;
	Palette _palette;
	Font _font;
	MdDriver _driver; // SUPERTM.DRV: the draws that turn and scale a shape
};

} // namespace Ultima6
} // namespace Ultima6

#endif
