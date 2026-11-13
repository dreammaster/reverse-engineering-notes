#ifndef ULTIMA6_ULTIMA6_MD_DRIVER_H
#define ULTIMA6_ULTIMA6_MD_DRIVER_H

#include "engine/cpu8086.h"
#include "files/data_file.h"

#include <memory>
#include <string>

namespace Ultima6 {
namespace Ultima6 {

/**
 * The drawing driver SUPERTM.DRV of Martian Dreams (the block for the VGA, item 0 of the archive), whose entry at offset 0x15E9 (the pointer dword_2817A of MARTIAN.EXE) draws a shape turned and
 * scaled about its hot spot. The driver is machine code of the game's own file; it is run on Cpu8086 instead of being ported, so its pictures are the original's to the pixel.
 *
 * The arguments of the entry (pushed in this order, C convention): flags, size (in 256ths, 256 is the shape as it is), angle (degrees, -359 to 359), frame, the picture's segment and offset
 * (a handle the game resolves through a callback at offset 0xC of the driver, here a small routine that answers where the archive lies), y, x and the screen (a table of the row offsets
 * and the clipping rectangle). A shape of size 256 and angle 0 is drawn as it is.
 *
 * APPROX(92): the code is run, not ported.
 */
class MdDriver {
public:
	MdDriver();

	/** Reads and unpacks SUPERTM.DRV of the game's directory. */
	bool load(const std::string &dir);
	bool ok() const {
		return _ok;
	}
	/**
	 * Draws a frame of a picture archive (an item of a *.LZC as LibN gives it) into pixels (320 x 200, the screen the clipping rectangle is on); false if the driver stops (it is not in range).
	 */
	bool draw(uint8_t *pixels, const Bytes &archive, int frame, int x, int y, int angle, int size, int flags = 0);

private:
	std::unique_ptr<Cpu8086> _cpu;
	bool _ok = false;
};

} // namespace Ultima6
} // namespace Ultima6

#endif
