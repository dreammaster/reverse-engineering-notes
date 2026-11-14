/* The screen of the standalone game: a Ui that draws what the original's logic asks for on a 320x200 Surface.
 *
 * It keeps the state the original keeps in its screen routines: the room picture with the overlays (items) that are drawn into it, the compass state, the text
 * window's pen. The routines of the original it implements are named in `other()`; every other one is ignored for now (the headless behaviour of RecordingUi).
 */
#ifndef XANTH_DISPLAY_H
#define XANTH_DISPLAY_H

#include "common/files.h"
#include "legend/rgn.h"
#include "legend/surface.h"
#include "legend/textwindow.h"
#include "xanth/game.h"
#include "xanth/mainscreen.h"

namespace Xanth {

struct OverlayRecord;

/** What the player does: the frontend (or a test) implements this; the game waits for events through it where the original waits for the mouse or keyboard. */
struct InputEvent {
	enum Type { Move, Click, Key, Quit } type;
	int x, y, key;
	InputEvent() : type(Quit), x(0), y(0), key(0) {}
};
class InputSource {
public:
	virtual ~InputSource() {}
	/** Blocks until the next event; the screen may be shown first (a frontend presents it here). */
	virtual InputEvent wait() = 0;
};

class Display : public Ui {
public:
	Display(const Common::GameFiles &files);
	/** Loads the interface pictures and fonts; false if a file is missing. */
	bool init();
	void attach(Game *game) { _game = game; }

	const Legend::Surface &screen() const { return _screen; }
	/** Where the clicks and keys come from while the game waits for them (conversations, "click to go on"); without one the game does not wait. */
	void setInput(InputSource *input) { _input = input; }
	/** The verbs of the list at the top left. */
	void setVerbs(const std::vector<std::string> &verbs) { _verbs = verbs; }
	/** Lets `ms` milliseconds pass for the animations of the room (update_animation: frame deltas are exclusive-ored onto the picture window when their time has come). */
	void advance(unsigned ms);
	/** The mouse pointer position and the name shown at the bottom of the picture window for the object under it ("" = none). */
	void setMouse(int x, int y) { _mouseX = x; _mouseY = y; }
	void setHoverName(const std::string &name) { _hover = name; }
	int mouseX() const { return _mouseX; }
	int mouseY() const { return _mouseY; }
	/** The screen as the player sees it: the game screen plus the hover name and the mouse pointer (neither is part of what the game draws). */
	void render(Legend::Surface &out) const;
	/** The object under a point of the screen (a hotspot of the room picture), 0 if none; with `orRoom` the room itself for a point of the picture that has no hotspot (a click there is a command on the room). */
	int objectAt(int x, int y, bool orRoom = false);
	/** The verb list entry under a point (index into the verbs set with setVerbs), -1 if none. */
	int verbAt(int x, int y) const;
	/** The thing carried (or the person with you) whose icon is under a point of the inventory, 0 if none. */
	int inventoryObjectAt(int x, int y) const;
	/** A click on the scroll arrows of the inventory (up: top half, down: bottom half); true if the list scrolled. */
	bool scrollInventory(int x, int y);
	/** The compass arrow (0..9, bit i of the exit mask) under a point if it is lit, -1 otherwise. */
	int compassAt(int x, int y) const;
	/** A click on one of the four buttons (0 WAIT, 1 menu, 2 MAP, 3 UNDO): the original draws the button pressed and raised again, which leaves one pixel of its frame different. */
	void pressButton(int button);
	/** Marks a verb of the list (yellow) or none (-1). */
	void selectVerb(int index);
	/** The start of a command (execute_command): the text window is emptied (and shrunk again if it had grown). */
	void beginCommand();
	/** RoomItem_Draw(0x10): the whole interface, the room and the text window. */
	void redrawAll();

	virtual int printText(const std::string &text);
	virtual int putChar(int ch);
	virtual int showTextBox(const std::string &text);
	virtual int playSound(const std::string &) { return 0; }
	virtual int waitTicksOrClick(int ticks);
	virtual void conversationBegin(int picNum);
	virtual void conversationLine(const std::string &text, bool wait);
	virtual int conversationMenu(const std::vector<std::string> &options, int initial);
	virtual void conversationEnd();
	virtual bool askYesNo(const std::string &) { return false; }
	virtual int other(const char *name, const intptr_t *args, int count);

private:
	void drawRoom();
	void clearConversationBubble();
	void expandText(int edge);
	void drawInventory();
	void drawSlot(int picNum, int x, int y);
	void collapseText();
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
	int _mouseX, _mouseY;
	std::string _hover;
	bool _expanded;                      // the text window has grown over the inventory
	std::vector<std::string> _turnText;  // what was printed since the command began (printed again when the window grows)
	std::vector<byte> _below;            // what it covers
	InputSource *_input;
	int _convPicW, _convPicH, _convBubbleRight, _convMenuBottom;
	int _selectedVerb;
	struct Cell {
		int x, y, obj;
	};
	std::vector<Cell> _cells;            // the icons drawn in the inventory (24 x 20 each)
	int _inventoryCount;
	int _inventoryScroll;                // first of the carried things shown (word_85024)
	Legend::RgnFile _rgn;
	int _rgnFile;
};

} // namespace Xanth

#endif
