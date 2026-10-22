/* Yendor1::Shared::Ui: the "open book" screen furniture every Chapter 1 program shares -- frame corners, the bottom decorations,
 * the icon bar with its click handling, and the pixel dissolve. Each routine is named for the shared function it ports (by
 * normalised fingerprint the copies in all programs are identical; names follow docs1/program-graph.md). */

#ifndef YENDOR1_SHARED_UI_H
#define YENDOR1_SHARED_UI_H

#include "shared/Runtime.h"

namespace Yendor1 {
namespace Shared {

/** The game's random generator (shared_9c11a6): a 16-bit LCG seeded from the clock on first use. */
class Rng {
public:
	Rng() : _seed(0) {}
	/** A value in 0..max. `clockSeed` is only used the first time (the original reads the DOS time). */
	uint16 next(uint16 max, uint16 clockSeed);
	void setSeed(uint16 s) { _seed = s; }
	uint16 seed() const { return _seed; }

private:
	uint16 _seed;
};

/** One entry of the icon bar table (16 bytes in the original: DS:10EFh in PROG1). */
struct IconEntry {
	byte flags;          ///< which bar masks show this icon
	byte key;            ///< the key it stands for
	uint16 x;            ///< left edge where its tile is drawn
	uint16 hitXMin, hitXMax;
	uint16 hitYMin, hitYMax;   ///< relative to the bar's y
	uint16 tileUp, tileDown;   ///< FILE05 tile numbers: normal and pressed
};

/** Parameters of the dissolve (the original's word_12DCB..12DD8). */
struct DissolveParams {
	byte srcPage, dstPage;
	uint16 blockCols, blockRows;      ///< number of 32x32 blocks (20 x 6 in the intro)
	uint16 srcX, srcY, dstX, dstY;    ///< origin of the area on each page
	byte skipColour;                  ///< source pixels of this colour are not copied
};

class BookUi {
public:
	explicit BookUi(Runtime &rt);

	// shared_9a625d / shared_89fc75: the four corner ornaments of the book page (FILE00)
	void drawTopCorners();
	void drawBottomCorners();
	/** shared_019ed0: the two rows of bottom border tiles (EGA: FILE00 tiles from 212h). */
	void drawBottomDecorations();
	/** shared_23db76: clear the bar and draw the icons whose flag bits intersect `mask` (FILE05). */
	void drawIconBar(byte mask);
	/** shared_67284e: if the pointer is over an icon enabled for `mask`, animate its press and report it (sets rt.last / ctx.textChar). */
	bool iconClick(byte mask);
	/** shared_21818f with a non-zero mask: wait for a key, or a click that hits an icon enabled for `mask`. */
	void waitForInput(byte mask);
	/** shared_21818f in full: `keyMask` (AH) selects which icon keys end the wait (0 = any key), `clickMask` (AL) which icons a click may hit,
	 *  `flags` is BL: 80h = do not block, report 2 when nothing happened; 20h = a click that misses every icon ends the wait too (3 = left,
	 *  4 = right). Returns the original's result byte (byte_11E90): 0 key, 1 icon hit (rt.last.code / ctx.textChar = its key), 2 nothing
	 *  happened, 3/4 click, FFh when the platform asked to quit. */
	byte input(byte keyMask, byte clickMask, byte flags);
	/** shared_21818f with AX=0: redraw the bottom decorations and show page 0. */
	void refreshAndShow();
	/** shared_47fa83 and helpers: copy the non-skip pixels from one page to the other in random order. */
	void dissolve(const DissolveParams &p);

	Rng rng;
	/** shared_9c11a6 as the programs call it: a value in 0..max; when the generator is unseeded it reads the DOS clock first. */
	uint16 random(uint16 max);

private:
	void drawIconTile(uint16 tile);

	Runtime &_rt;
};

} // namespace Shared
} // namespace Yendor1

#endif
