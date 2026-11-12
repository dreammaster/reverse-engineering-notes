/* The main game screen of Companions of Xanth: the 320x200 layout that RoomItem_Draw(0x10) builds, and the palette that goes with it.
 *
 *   marble background (XANTH_00 picture 31)                 verb list at the top left (x 0..49)
 *   picture window (51,4)-(314,125)  264x122                 compass (4,129)  WAIT / save / MAP / UNDO buttons at (4,168) (25,168) (4,182) (25,182)
 *   text window (51,129)-(314,148)                           inventory panel (51,152) and (88,152), scroll arrows (293,152)
 *
 * The palette is the original's game_palette: a room picture's palette replaces all 256 entries, and Screen_RestoreSubPalettes then puts back the interface
 * colours (0..15 a grey ramp, 16..31 from the marble picture, 32..79 from XANTH_01 picture 0) and sets entries 16, 17, 254 and 255.
 */
#ifndef XANTH_MAINSCREEN_H
#define XANTH_MAINSCREEN_H

#include "common/files.h"
#include "legend/font.h"
#include "legend/pic.h"
#include "legend/surface.h"

namespace Xanth {

class MainScreen {
public:
	/** Loads the pictures of the interface from the game files (XANTH_00.PIC, XANTH_01.PIC). */
	bool load(const Common::GameFiles &files);

	/** Draws the whole frame (RoomItem_Draw 0x10 without the text and the room picture) on `s` and sets its palette. */
	void drawFrame(Legend::Surface &s) const;
	/** Draws a room picture (picNum = file number << 8 | entry) in the picture window and takes over its palette. `overlays` are further picture numbers
	 *  (setup_pic_regions: the items of the room that are visible) whose pixels are exclusive-ored onto the picture at their own positions. */
	bool drawRoomPicture(Legend::Surface &s, const Common::GameFiles &files, int picNum, const std::vector<int> &overlays = std::vector<int>()) const;

	/** The verb list: each name twice, black one pixel down and to the right and white on top (sub_18CBE, font 1). */
	void drawVerbs(Legend::Surface &s, const std::vector<std::string> &verbs) const;
	/** The text window (window 2, font 10, colours 15 on 23): prints a message like Font_WriteLines does. */
	void printMessage(Legend::Surface &s, const std::string &text) const;

	/** The compass: arrow i (bit i of `exits`) is drawn exclusive-or onto the compass picture (sub_2B236 draws only the arrows whose state changed). */
	void drawCompassArrows(Legend::Surface &s, unsigned exits) const;

	/** The interface palette entries applied over a picture's palette (Screen_RestoreSubPalettes). */
	void restoreSubPalettes(byte *palette768) const;

private:
	Legend::Font _font1, _font10;
	Legend::PicImage _marble, _compass, _slotPanel, _listPanel, _scrollPanel, _buttons[5], _arrows[10], _interfacePalette;
};

} // namespace Xanth

#endif
