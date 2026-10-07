// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 220104-223152, all 17 manifest-listed methods): the
// dialog the player picks the replies of. It is a TTDialog (the dialog record the game is in,
// kGameDialog), and is embedded by value in TGameControl: it is "active" exactly when the
// record is not empty (TGameControl::StartDialog()/EndDialog() set it and clear it, and save
// it through GetTarget()).
//
// SetDialog() takes the parts of the dialog the player can pick now (the ones that are
// available, and whose condition holds), cuts the text of each of them into lines that fit the
// dialog area of the character that is speaking (the area, the two fonts - the one under the
// mouse, the others -, the arrows to scroll and the background are the data of that
// character), and puts the lines one under the other. The lines that do not fit in the area
// are scrolled to by the two arrows (or by the mouse wheel).
//
// Picking a part (HandleMouseClick()) builds the game's dialog action (kGameDialogAction): the
// actions of the part, the line the character says for it (the answer text) and the line the
// other one answers (the text of the part), and then what follows: the next dialog, or - when
// there is none to go on with - the dialog above, or the same one again (the part's "return"
// setting); the action then runs and the dialog ends (the next dialog's own action starts it
// again).
//
// Original layout (the record is the TTDialog base): +0x08 the lines, +0x20 their rectangles,
// +0x38 for each line the index of the part it is of, +0x50 the parts, +0x68 the part under the
// mouse (-1: none), +0x6C..+0x6F can scroll up / can scroll down / the mouse is on the up arrow
// / the mouse is on the down arrow, +0x70 the font of the part under the mouse, +0x78 the font
// of the others, +0x80 the character the dialog is with, +0x88 the timer that stops the wheel
// from scrolling too fast, +0x98 the dialog area, +0xA8 the sprite of the active up arrow,
// +0x190 of the inactive one, +0x278 of the active down arrow, +0x360 of the inactive one,
// +0x448 the background, +0x530 how many lines are scrolled away, +0x534 the space between
// lines (the data of the character: kCharacterActiveDialogFont 0x1C9 .. kCharacterDialogVerticalSpace
// 0x1D0, kCharacterInactiveDialogFont 0x245).
#pragma once

#include <vector>

#include "TTimer.h"
#include "WxStub.h"
#include "datastruct/vlist.h"
#include "datastruct/visobjref.h"
#include "graphicslib/picture.h"
#include "vstables/records.h"

enum class TMouseMessageEnum;

class TGDialog : public TTDialog {
public:
	TGDialog();
	~TGDialog();

	/** Makes the dialog from `dialog` (an empty one: no dialog). */
	void SetDialog(const TVisObjRef &dialog);
	/** Ends the dialog: nothing is shown any more. */
	void Clear();

	/** The dialog record (what the saved game keeps as the current dialog). */
	const TVisObjRef &GetTarget() const {
		return *this;
	}

	void Draw();
	void HandleMouseMove(const wxPoint &pos);
	/** The player clicks: on an arrow it scrolls, on a part it is picked. */
	void HandleMouseClick();
	/** The wheel (12: up, 13: down) scrolls like the arrows do (at most every 10 ms). */
	void HandleMouseWheel(TMouseMessageEnum msg);

	/** The part under the mouse (-1: none). */
	int GetCurrentDialogPart() const {
		return _hovered;
	}
	bool IsActiveDialogPart() const {
		return _hovered != -1;
	}
	/** The character the dialog is of (the dialog record's own character, or the one above). */
	TVisObjRef GetDialogCharacter() const;

	// Nothing is kept of a dialog but the record itself.
	void Load() {
	}
	void Save() {
	}

private:
	/** The arrows are shown when there is more than the area shows. */
	void SetScrollButtons();
	/** Adds a line of the action: `link` says (or does) command `command`; `speaker` is who says it. */
	static void AddActionPart(TVisObjRef &action, const TVisObjRef &link, int command, const TVisObjRef &speaker,
	                          TMoveOrderEnum order);
	/** The available parts of `dialog` as the player sees them: is one there? */
	static bool HasAvailablePart(const TVisObjRef &dialog);

	std::vector<wxString> _lines;      // +0x08
	std::vector<wxRect> _lineRects;    // +0x20
	std::vector<int> _lineParts;       // +0x38
	TVList _parts;                     // +0x50
	int _hovered;                      // +0x68
	bool _canScrollUp;                 // +0x6C
	bool _canScrollDown;               // +0x6D
	bool _hoverUp;                     // +0x6E
	bool _hoverDown;                   // +0x6F
	TVisObjRef _activeFont;            // +0x70
	TVisObjRef _normalFont;            // +0x78
	TVisObjRef _partner;               // +0x80
	TTimer _wheelTimer;                // +0x88
	wxRect _area;                      // +0x98
	TPictureIO _activeScrollUp;        // +0xA8
	TPictureIO _inactiveScrollUp;      // +0x190
	TPictureIO _activeScrollDown;      // +0x278
	TPictureIO _inactiveScrollDown;    // +0x360
	TPictureIO _background;            // +0x448
	int _scroll;                       // +0x530
	int _lineSpacing;                  // +0x534
};
