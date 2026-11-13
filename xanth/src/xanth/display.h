/* The screen of the standalone game: a Ui that draws what the original's logic asks for on a 320x200 Surface.
 *
 * It keeps the state the original keeps in its screen routines: the room picture with the overlays (items) that are drawn into it, the compass state, the text
 * window's pen. The routines of the original it implements are named in `other()`; every other one is ignored for now (the headless behaviour of RecordingUi).
 */
#ifndef XANTH_DISPLAY_H
#define XANTH_DISPLAY_H

#include "common/files.h"
#include "legend/surface.h"
#include "legend/textwindow.h"
#include "xanth/game.h"
#include "xanth/mainscreen.h"

namespace Xanth {

struct OverlayRecord;

class Display : public Ui {
public:
	Display(const Common::GameFiles &files);
	/** Loads the interface pictures and fonts; false if a file is missing. */
	bool init();
	void attach(Game *game) { _game = game; }

	const Legend::Surface &screen() const { return _screen; }
	/** The verbs of the list at the top left. */
	void setVerbs(const std::vector<std::string> &verbs) { _verbs = verbs; }
	/** Lets `ms` milliseconds pass for the animations of the room (update_animation: frame deltas are exclusive-ored onto the picture window when their time has come). */
	void advance(unsigned ms);
	/** RoomItem_Draw(0x10): the whole interface, the room and the text window. */
	void redrawAll();

	virtual int printText(const std::string &text);
	virtual int putChar(int ch);
	virtual int showTextBox(const std::string &text) { return printText(text); }
	virtual int playSound(const std::string &) { return 0; }
	virtual int waitTicksOrClick(int) { return 0; }
	virtual bool askYesNo(const std::string &) { return false; }
	virtual int other(const char *name, const intptr_t *args, int count);

private:
	void drawRoom();
	void popupWindowEvent();
	int setupPicRegions(unsigned address, unsigned count);
	void updateCompass();
	bool overlayVisible(const OverlayRecord &r);
	bool loadRoom(int picNum);

	/** An animation of the room (sub_2A207): a row of exclusive-or pictures played one after the other. */
	struct Animation {
		std::vector<Legend::PicImage> frames;
		int step;                    // 0 = forward once per round, otherwise the direction (-1: backwards)
		int cur;                     // the next frame, -1 = finished
		unsigned next;               // when it is due (ms)
		unsigned start;              // delay before the first frame (the `delay` argument)
	};
	int addAnimation(int picNum, int count, int step, unsigned start);
	void startAnimations();
	void drawFrame(const Legend::PicImage &f);

	std::vector<Animation> _anims;
	unsigned _now;
	const Common::GameFiles &_files;
	Game *_game;
	MainScreen _ms;
	Legend::Surface _screen;
	Legend::TextWindow _text;
	std::vector<std::string> _verbs;

	int _picNum;                         // the room picture loaded (-1 = none)
	Legend::PicImage _room;              // its picture and palette
	std::vector<byte> _roomPixels;       // the pixels with the overlays that are drawn
	bool _drawn[64];                     // per record of the current overlay table: is its picture XORed into the pixels
	unsigned _compass;                   // the arrows that are drawn
	bool _frameDrawn;
};

} // namespace Xanth

#endif
