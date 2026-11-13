#ifndef ULTIMA6_ULTIMA6_MD_INTRO_H
#define ULTIMA6_ULTIMA6_MD_INTRO_H

#include "engine/framebuffer.h"
#include "files/shape.h"
#include "files/tile_data.h"

#include <string>
#include <vector>

namespace Ultima6 {
namespace Ultima6 {

/**
 * The introduction of Martian Dreams, played from the script that MARTIAN.EXE's routine sub_19266 makes: it loads the pictures of SCENE1-4B.LZC and MOONGATE.LZC, draws their shapes at places of the
 * 320 x 128 window (y 24-151), puts a line of SCENETXT.LZC under it, shows it, waits, and goes on; the calls it makes were got by running that routine on tools/emulate_intro.py, and
 * md_intro_script.inc holds them (tools/gen_intro_script.py). Any key ends the introduction, as it does in the original.
 *
 * APPROX(90): the drawing driver (TM.DRV) is not disassembled: a shape is put so that its hot spot is at the place given; the pan of the hand with the map (driver entry 0x2817A) and the dots of
 * the thrown orb (0x28172) are guessed; the fades (sub_155DB, sub_155A5) last half a second; the flashes of the palette (sub_20951), the music and the colour of the lines are left out; the
 * lines are centred in gold.
 */
class MdIntro {
public:
	explicit MdIntro(const std::string &dir);

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
		int kind; // 0 shape, 1 pan, 2 dot
		int frame, handle, x, y;
	};
	struct Handle {
		int file = -1, item = 0;
		bool loaded = false;
		std::vector<Shape> shapes;
	};

	void run();
	void compose();
	const std::vector<Shape> &shapes(int handle);
	const std::string &line(int set, int index);
	void shapeAt(int handle, int frame, int x, int y, int dy = 0);

	std::string _dir;
	bool _ok = false, _finished = false;
	size_t _pc = 0;
	int _wait = 0;
	int _fade = 0;     // 1 in, -1 out, 0 none
	int _fadeLeft = 0; // ms
	bool _black = true;
	std::vector<Handle> _handles;
	std::vector<std::vector<std::string>> _texts;
	std::vector<std::string> _lines; // the caption now
	int _captionY = 160;
	std::vector<Draw> _draws;
	std::vector<uint8_t> _front;
	Palette _palette;
	Font _font;
};

} // namespace Ultima6
} // namespace Ultima6

#endif
