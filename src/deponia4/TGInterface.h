// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 138535-143198, all 25 manifest-listed methods): an
// interface of the game - a panel with buttons that is shown on the screen (the verbs, the
// inventory, the scroll arrows...). It derives from TPaintControl (it is drawn and clipped
// like a surface of its own) and is made from an interface record (kInterface*): its
// background sprite, its border polygon, its buttons and the items of the character.
//
// The buttons come in kinds (the button's kButtonType): 0 a place holder (a slot the items are
// shown in), 1 and 2 the scroll arrows (back and forward), 3 and 6 commands, 4 action areas;
// the type 5 is not made into anything. They are kept in a list of their own (the place
// holders, the commands, the action areas) and all together in the list of buttons (in the
// order of the record), with an index by id. The items (THItem) are made from the items the
// current character has (UpdateItems()), kept in the list of items with their own index by
// id; the place holders show the ones from the scroll position on (kInterfaceItemsScrollPosition),
// one item for each place holder that is active. The active command is the command button that
// the player has chosen (kInterfaceActiveCommand): its "active" sprite is shown.
//
// Fading: the interface fades in and out (kInterfaceVisibility/kInterfaceDestVisibility/
// kInterfaceTimeToDestVisibility); its alpha goes to all its buttons and items.
//
// GetObject()/IsInside() move the position of the cursor back through invMatrix1 while the
// interface is drawn through a matrix (kInterfaceMatrixId).
//
// Original layout: +0x48 the record, +0x50 the alpha, +0x54 where the fade started, +0x58 where
// it goes to, +0x5C its time, +0x60 its timer, +0x70 the bounds, +0x80 the border polygon,
// +0x98 the background picture (0xE8 bytes), +0x180 action areas, +0x198 commands, +0x1B0
// items, +0x1C8/+0x1D0 the scroll buttons, +0x1D8 place holders, +0x1F0 buttons, +0x208 index
// of the buttons, +0x228 index of the items, +0x248 the active command, +0x250 the number of
// place holders that are active.
#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "TPaintControl.h"
#include "TPolygonList.h"
#include "TTimer.h"
#include "datastruct/vlist.h"
#include "datastruct/visobjref.h"
#include "graphicslib/picture.h"

class TGActionArea;
class TGCommand;
class TGPlaceHolder;
class TGScrollButton;
class THItem;
class TManagedObject;
class TMButton;

// Confirmed 6 values, 0-5, from a jump table keyed on an interface's field
// id 0x13A (TGameControl::AdjustInterfacesOnScreen, Deponia_Linux.asm lines
// 465157-465494) - names are a best-effort read of each case's own
// behavior (see AdjustInterfacesOnScreen's comment), not recovered
// identifiers.
enum class TInterfacePositionEnum {
	kDockTopStacked = 0,    // y = accumulated top offset, x = 0; reserves height
	kDockBottomStacked = 1, // y flush against the current bottom edge; reserves height
	kDockTopRow = 2,        // x = accumulated left offset, y = 0; reserves width
	kFixedReserveWidth = 3, // a stored point, but also reserves width like case 2
	kFixed = 4,             // a stored point, no space reserved
	kDraggableClamped = 5,  // follows the mouse while being dragged; always clamped on-screen
};

class TGInterface : public TPaintControl {
public:
	explicit TGInterface(const TVisObjRef &ref);
	~TGInterface() override;

	/** Draws the background, the buttons (the place holders with their items) and the text of
	 *  the action that is under the cursor. */
	void Draw() override;

	const TVisObjRef &GetRef() const {
		return _ref;
	}
	// Confirmed mutated directly (TGameControl::AdjustInterfacesOnScreen
	// calls TVisObjRef::SetValue() on this field in place, Deponia_Linux.asm
	// line 465175) - same "TVisObjRef at a known offset, no accessor in the
	// original" pattern as TGCharacter's own dual GetRef() overloads.
	TVisObjRef &GetRef() {
		return _ref;
	}

	/** Makes the interface as the record says: shown or not, the active command, the
	 *  background and border, and all the buttons. */
	void Init();
	/** Takes the standard command of the interface as the active one (or else the first
	 *  command button). */
	void InitActiveCommand();
	/** The border polygon is made from these points (and the bounds from it). */
	void SetPolygon(const std::vector<wxPoint> &points);

	// Confirmed TManagedObject* (TGameControl::StartObjectText calls
	// TManagedObject::SetText() directly on the result, same as
	// TGScene::GetObject() - asm lines 462233-462396). The button or item with that id.
	TManagedObject *GetObject(const TVisObjRef &object) const;
	/** The button, or the item that is in the place holder, at a screen position. */
	TManagedObject *GetObject(const wxPoint &pos) const;
	/** Whether the position is in the interface (its bounds and border). */
	bool IsInside(const wxPoint &pos) const;
	/** The place holder that shows the item (null when none does). */
	TGPlaceHolder *GetPlaceHolder(const TManagedObject *object) const;

	/** Makes the buttons and items show or hide, as their conditions say and the size of the
	 *  background gives the interface's size; with `update` the object under the cursor is
	 *  looked for again. */
	void SetObjectsActive(bool update);
	/** Shows or hides the interface as its record says. */
	void UpdateActiveStatus();
	/** Shows the scroll arrows and the items that are to be shown at the scroll position. */
	void TestActiveObjects();
	void StartAnimations();
	void ReattachAnimations();
	/** Lets go of the sprites and animations of all buttons and items (an interface that
	 *  leaves the screen). */
	void RemoveSpritesAndAnimations();

	/** The items of the interface are made to be those: the ones it has already are kept, the
	 *  new ones made, the ones that are not there any more removed. */
	void UpdateItems(const TVList &items);
	/** Deletes all the items. */
	void RemoveAllItems();
	/** Takes over what a loaded saved game has for the interface. */
	void Load();

	/** The interface fades to `percent` in `milliseconds`. */
	void SetDestAlpha(int percent, int milliseconds);
	/** The step of the fade: the alpha of the interface, given to all buttons and items. */
	void UpdateAlpha();
	/** The percent the interface fades to. */
	int GetDestAlpha() const;

	/** Makes that command button the active one (its "active" sprite is shown, the others
	 *  show the other one); with `update` the current character and the game are told. */
	void SetActiveCommand(const TVisObjRef &command, bool update);
	/** The record of the active command button (an empty reference when none). */
	TVisObjRef GetActiveCommand();

protected:
	// Confirmed protected-by-need (THInterface::OnEvent() reads the items, asm lines
	// 246931-246955): the data members are moved up from private.
	TVisObjRef _ref;                                // +0x48
	float _alpha;                                   // +0x50
	float _alphaFrom;                               // +0x54
	float _alphaTarget;                             // +0x58
	int _alphaDurationMs;                           // +0x5C
	TTimer _timer;                                  // +0x60
	wxRect _bounds;                                 // +0x70
	TPolygonList _polygons;                         // +0x80
	TPictureIO _background;                         // +0x98
	std::vector<TGActionArea *> _actionAreas;       // +0x180
	std::vector<TGCommand *> _commands;             // +0x198
	std::vector<THItem *> _items;                   // +0x1B0
	TGScrollButton *_scrollBack;                    // +0x1C8
	TGScrollButton *_scrollForward;                 // +0x1D0
	std::vector<TGPlaceHolder *> _placeHolders;     // +0x1D8
	std::vector<TMButton *> _buttons;               // +0x1F0
	std::unordered_map<std::uint32_t, int> _buttonIndex;  // +0x208, the original's HashMap<TId, int>
	std::unordered_map<std::uint32_t, int> _itemIndex;    // +0x228
	TGCommand *_activeCommand;                      // +0x248
	int _activePlaceHolders;                        // +0x250

private:
	/** The key of an id in the indexes above. */
	static std::uint32_t idKey(const TVisObjRef &object);
	/** Adds a button to the list of all of them and to their index. */
	void addButton(TMButton *button);
};
