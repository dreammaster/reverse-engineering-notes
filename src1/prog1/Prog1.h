/* Yendor1::Prog1 -- PROG1.EXE, the title/credits program: the SmithWare logo animation, the title picture, and the "About the
 * Authors" book page. SW.EXE starts it with a mode in the parameter block (parm_03) and parm_02 = 1 on the first run.
 * Function names map to the disassembly (prog1.idb): see the comments on each routine. */

#ifndef YENDOR1_PROG1_PROG1_H
#define YENDOR1_PROG1_PROG1_H

#include "data/ExeImage.h"
#include "shared/Params.h"
#include "shared/Runtime.h"
#include "shared/Ui.h"

namespace Yendor1 {
namespace Prog1 {

using Shared::byte;
using Shared::uint16;

class Prog1 {
public:
	Prog1(Shared::Runtime &rt, const Shared::Params &params);

	/** Run the program for the mode in parm_03; returns the DOS exit code (0 normal, 1 the user pressed Esc, FFh bad mode). */
	int run();

private:
	// sub_102E4 .. sub_10C2C, in address order
	void showCopyrightLine();      // sub_102E4
	void drawRuleLines();          // sub_1033A
	void fillBackgroundTiles();    // sub_1037E
	void drawAuthorsPage();        // sub_103D2
	void titleSweeps();            // sub_1046D
	void dissolveTitle();          // sub_1056C
	void drawLowerPanels();        // sub_105A7
	void shrinkingFrame();         // sub_1061D
	void drawBookBackground();     // sub_106C8
	void preloadTiles();           // sub_10B2C
	void closeCurtains();          // sub_10B60
	bool escapePressed();          // sub_10BE6
	void logoAnimation();          // sub_10C2C

	// helpers for the many "set position, draw tile" sequences
	void tile(uint16 x, uint16 y, uint16 index);
	void run(uint16 firstIndex, int count);            // drawTileRun from the current x1 (file 0)
	void setColours(byte fg, byte bg);
	byte colour(int index) const { return _c.colourMap[index]; }
	void delayAfterSequence(bool paletteHelper);

	Shared::Runtime &_rt;
	Vid::Context &_c;
	Shared::BookUi _ui;
	Shared::Params _params;
	byte _tileFile;      // byte_12DE5: the file number the next tileOp uses
	byte _exitCode;      // byte_12DDC
	Data::ExeImage _exe; // PROG1.EXE: the texts are read from the original file (data segment starts 1F50h into its image)
	const char *_authorLines[17];
	const char *_copyright;
};

} // namespace Prog1
} // namespace Yendor1

#endif
