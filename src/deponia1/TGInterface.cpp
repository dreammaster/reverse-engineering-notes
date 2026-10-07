#include "TGInterface.h"

#include <cstring>

#include "AppGlobals.h"
#include "Diagnostics.h"
#include "TGActionArea.h"
#include "TGCharacter.h"
#include "TGCommand.h"
#include "TGObjectManager.h"
#include "THItem.h"
#include "TGPlaceHolder.h"
#include "TGScrollButton.h"
#include "datastruct/visionaire.h"
#include "vscommon/fontManager.h"
#include "vsplayer/animationGame.h"
#include "vsplayer/control/gameControl.h"
#include "vstables/fieldIds.h"
#include "vstables/records.h"
#include "vstables/visionaireGame.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/vsplayer/interfaceGame.cpp";

// TGameControl implements everything used from it here, but g_pGameControl is only
// declared as TMasterControl* (AppGlobals.h) - the same cast the other classes use.
static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

// Confirmed (asm lines 143021-143198): the interface starts opaque and is set up by Init().
TGInterface::TGInterface(const TVisObjRef &ref) : _ref(ref) {
	_alpha = 1.0f;
	_alphaFrom = 0.0f;
	_alphaTarget = 1.0f;
	_alphaDurationMs = 0;
	_scrollBack = nullptr;
	_scrollForward = nullptr;
	_activeCommand = nullptr;
	_activePlaceHolders = 0;

	Init();
}

// Confirmed (asm lines 141155-141555): the buttons and the items are owned by the interface.
TGInterface::~TGInterface() {
	delete _scrollBack;
	_scrollBack = nullptr;
	delete _scrollForward;
	_scrollForward = nullptr;

	for (TGActionArea *area : _actionAreas)
		delete area;
	_actionAreas.clear();

	for (TGCommand *command : _commands)
		delete command;
	_commands.clear();

	for (TGPlaceHolder *placeHolder : _placeHolders)
		delete placeHolder;
	_placeHolders.clear();

	_buttons.clear();
	_buttonIndex.clear();
	_activePlaceHolders = 0;
	_activeCommand = nullptr;

	for (THItem *item : _items)
		delete item;
	_items.clear();
	_itemIndex.clear();
}

// The key of an id: its four bytes (the original hashes the same way, and compares with TId).
std::uint32_t TGInterface::idKey(const TVisObjRef &object) {
	std::uint32_t key;

	std::memcpy(&key, object.GetId(), sizeof(key));
	return key;
}

// Adds a button to the list of all of them and to their index (confirmed, asm lines
// 142272-142523, repeated for each kind of button).
void TGInterface::addButton(TMButton *button) {
	_buttons.push_back(button);
	_buttonIndex[idKey(button->GetRef())] = (int)_buttons.size() - 1;
}

// Confirmed (asm lines 142052-143010)
void TGInterface::Init() {
	UpdateActiveStatus();
	InitActiveCommand();

	_background.Set(_ref.GetSprite(kInterfaceSprite));
	SetVisibleSize(0, 0);
	SetWorktopSize(0, 0);
	_bounds = wxRect();

	std::vector<wxPoint> border;

	_ref.GetPoints(kInterfaceBorder, border);
	CreatePolygonsFromPointList(border, _polygons);

	if (!_polygons.empty())
		_bounds = GetBoundingBox(_polygons);

	// the buttons, each made as the kind its type says
	TVList buttons;

	_ref.GetLinks(kInterfaceButtons, TypeOrder::kValue1, buttons);

	for (TVisionaireObject *item : buttons) {
		TVisObjRef button(item);

		if (button.IsEmpty())
			continue;

		switch (button.GetInt(kButtonType)) {
		case 0: {
			TGPlaceHolder *placeHolder = new TGPlaceHolder(button, this);

			_placeHolders.push_back(placeHolder);
			addButton(placeHolder);
			break;
		}
		case 1:
			_scrollBack = new TGScrollButton(button, this);
			addButton(_scrollBack);
			break;
		case 2:
			_scrollForward = new TGScrollButton(button, this);
			addButton(_scrollForward);
			break;
		case 3:
		case 6: {
			TGCommand *command = new TGCommand(button, this);

			_commands.push_back(command);
			addButton(command);
			break;
		}
		case 4: {
			TGActionArea *area = new TGActionArea(button, this);

			_actionAreas.push_back(area);
			addButton(area);
			break;
		}
		default:
			break;
		}
	}

	SetActiveCommand(_ref.GetLink(kInterfaceActiveCommand), false);
	SetObjectsActive(true);

	// (the scroll position is set again, to have everything that depends on it follow)
	_ref.SetValue(kInterfaceItemsScrollPosition, _ref.GetInt(kInterfaceItemsScrollPosition),
	              TSendEventEnum::kSendEvent);
}

// Confirmed (asm lines 138996-139130)
void TGInterface::InitActiveCommand() {
	TVisObjRef standard = _ref.GetLink(kInterfaceStandardCommand);

	if (!standard.IsEmpty()) {
		_ref.SetLink(kInterfaceActiveCommand, _ref.GetLink(kInterfaceStandardCommand), false);
		return;
	}

	// (no standard command: the first command button)
	TVList buttons;

	_ref.GetLinks(kInterfaceButtons, TypeOrder::kValue1, buttons);

	for (TVisionaireObject *item : buttons) {
		TVisObjRef button(*item);
		TTButton command(button);

		if (command.IsCommand()) {
			_ref.SetLink(kInterfaceActiveCommand, button, false);
			return;
		}
	}
}

// Confirmed (asm lines 139697-139726)
void TGInterface::SetPolygon(const std::vector<wxPoint> &points) {
	CreatePolygonsFromPointList(points, _polygons);

	if (!_polygons.empty())
		_bounds = GetBoundingBox(_polygons);
}

// Confirmed (asm lines 138536-138950), except the matrix-transformed drawing: the buttons
// are drawn from the last to the first (the place holders with the item that belongs to
// them: the place holders count down from the last, and the item of the slot is the one at
// the scroll position plus the slot's number), then the text of the action under the cursor.
void TGInterface::Draw() {
	if (!IsActive() || _ref.IsEmpty())
		return;

	UpdateAlpha();
	SetCurrent();

	_background.SetMatrixId(_ref.GetInt(kInterfaceMatrixId));
	_background.Draw(_alpha, 0xFFFFFFFF);

	// an interface that is not shown through a matrix is drawn without them
	bool savedMatrices = true;

	if (_ref.GetInt(kInterfaceMatrixId) == 0) {
		savedMatrices = matricesActive;
		matricesActive = false;
	}

	int scroll = _ref.GetInt(kInterfaceItemsScrollPosition);
	int slot = _activePlaceHolders - 1;
	int itemIndex = scroll + _activePlaceHolders - 1;

	if (itemIndex >= (int)_items.size())
		itemIndex = (int)_items.size() - 1;

	for (size_t i = _buttons.size(); i > 0; i--) {
		TMButton *button = _buttons[i - 1];

		if (button->GetRef().GetInt(kButtonType) != 0) {
			button->Draw();
			continue;
		}

		if (!button->IsActive())
			continue;

		button->Draw();

		if (scroll <= itemIndex && itemIndex - scroll == slot) {
			if (itemIndex >= 0 && itemIndex < (int)_items.size()) {
				wxRect bounds = button->GetBoundingRect();
				THItem *item = _items[itemIndex];

				item->SetCenteredPosition(wxPoint{bounds.GetLeft() + bounds.GetWidth() / 2,
				                                  bounds.GetTop() + bounds.GetHeight() / 2});
				item->Draw();
			} else {
				x_assert(false, "false", kSourceFile, 0x270);
			}

			itemIndex--;
		}

		slot--;
	}

	if (_ref.GetBool(kInterfaceDrawActionText)) {
		// the name of what the cursor is on, centred in the interface's text rectangle
		wxRect area = *_ref.GetRect(kInterfaceActionTextRect);
		TFontManager *fonts = g_pGameControl->GetFontManager();

		fonts->SetCurrentFont(_ref.GetLink(kInterfaceActionTextFont));

		wxString text = gameControl()->GetObjectManager()->GetActionText();
		wxPoint size;

		g_pGameControl->GetFontManager()->GetTextDimension(text, size);

		wxPoint position{area.GetLeft() + (area.GetWidth() >> 1) - (size.x >> 1), area.GetTop()};

		g_pGameControl->GetFontManager()->PrintText(text, TextAlignmentEnum::kLeft, position, _alpha);
	}

	if (_ref.GetInt(kInterfaceMatrixId) == 0)
		matricesActive = savedMatrices;
}

// Confirmed (asm lines 139404-139520)
TManagedObject *TGInterface::GetObject(const TVisObjRef &object) const {
	std::uint32_t key = idKey(object);
	std::unordered_map<std::uint32_t, int>::const_iterator button = _buttonIndex.find(key);

	if (button != _buttonIndex.end())
		return _buttons[button->second];

	std::unordered_map<std::uint32_t, int>::const_iterator item = _itemIndex.find(key);

	if (item != _itemIndex.end())
		return _items[item->second];

	return nullptr;
}

// Confirmed (asm lines 139195-139400). TODO (low priority, see /TODO.md): while the interface
// is drawn through a matrix the original first moves the position back through it.
TManagedObject *TGInterface::GetObject(const wxPoint &pos) const {
	if (!IsActive())
		return nullptr;

	wxPoint position = GetRelativePoint(pos);
	int scroll = _ref.GetInt(kInterfaceItemsScrollPosition);
	long placeHolders = 0;

	for (TMButton *button : _buttons) {
		bool placeHolder = (button->GetRef().GetInt(kButtonType) == 0);

		if (button->IsInside(position)) {
			// (the place holder gives the item that is shown in it)
			if (placeHolder && placeHolders + scroll < (long)_items.size())
				return _items[placeHolders + scroll];

			return button;
		}

		if (placeHolder)
			placeHolders++;
	}

	return nullptr;
}

// Confirmed (asm lines 139532-139680). TODO (low priority, see /TODO.md): the matrix.
bool TGInterface::IsInside(const wxPoint &pos) const {
	wxPoint position = pos - GetOrigin();

	if (!IsActive())
		return false;

	if (!_bounds.Contains(position))
		return false;

	if (_polygons.empty())
		return true;

	return IsPointInsidePolygon(position, _polygons);
}

// Confirmed (asm lines 139738-139798): the place holders that are active show the items from
// the scroll position on, one each.
TGPlaceHolder *TGInterface::GetPlaceHolder(const TManagedObject *object) const {
	size_t index = _ref.GetInt(kInterfaceItemsScrollPosition);

	for (TGPlaceHolder *placeHolder : _placeHolders) {
		if (!placeHolder->IsActive())
			continue;

		if (index < _items.size() && _items[index] == object)
			return placeHolder;

		index++;
	}

	return nullptr;
}

// Confirmed (asm lines 140397-140605): the size of the interface is the size of its
// background; the buttons are shown as their conditions say (the scroll arrows by
// TestActiveObjects()); the commands count the place holders that are active; an active
// command that is not shown any more is replaced by the next one.
void TGInterface::SetObjectsActive(bool update) {
	_background.EnsureSizeValid();

	int width = _background.GetWidth();
	int height = _background.GetHeight();

	SetVisibleSize(width, height);
	SetWorktopSize(width, height);

	_bounds = wxRect{0, 0, width, height};
	_activePlaceHolders = 0;

	for (TMButton *button : _buttons) {
		if (button->GetRef().IsEmpty()) {
			button->SetActive(false);
			continue;
		}

		TTCondition condition(button->GetRef().GetLink(kButtonCondition));

		if (button != _scrollBack && button != _scrollForward)
			button->SetActive(button->GetRef().GetBool(kButtonConditionNegate) != condition.IsTrue());

		if (button->GetRef().GetInt(kButtonType) == 0 && button->IsActive())
			_activePlaceHolders++;
	}

	if (_activeCommand && !_activeCommand->IsActive())
		TTInterface(_ref).SetNextCommand();

	TestActiveObjects();

	if (update)
		gameControl()->UpdateCurrentObject();
}

// Confirmed (asm lines 140606-140670, also the start of Init() and Load()): the interface
// is shown or hidden as its record says; shown it sets its objects up, hidden it hides them.
void TGInterface::UpdateActiveStatus() {
	bool active = IsActive();
	bool visible = _ref.GetBool(kInterfaceVisible);

	if (active == visible)
		return;

	SetActive(visible);

	if (visible) {
		SetObjectsActive(true);
		return;
	}

	for (TMButton *button : _buttons)
		button->SetActive(false);

	for (THItem *item : _items)
		item->SetActive(false);
}

// Confirmed (asm lines 140194-140390): the scroll arrows show when there is something to
// scroll to (back: the scroll position is above 0; forward: there are items beyond the last
// one that is shown), the items from the scroll position on, as many as there are active
// place holders, show.
void TGInterface::TestActiveObjects() {
	int scroll = _ref.GetInt(kInterfaceItemsScrollPosition);
	int last = _activePlaceHolders + scroll - 1;

	if (_scrollBack) {
		TTCondition condition(_scrollBack->GetRef().GetLink(kButtonCondition));
		bool shown = false;

		if (scroll > 0)
			shown = (_scrollBack->GetRef().GetBool(kButtonConditionNegate) != condition.IsTrue());

		_scrollBack->SetActive(shown);
	}

	if (_scrollForward) {
		TTCondition condition(_scrollForward->GetRef().GetLink(kButtonCondition));
		bool shown = false;

		if (last < (int)_items.size() - 1)
			shown = (_scrollForward->GetRef().GetBool(kButtonConditionNegate) != condition.IsTrue());

		_scrollForward->SetActive(shown);
	}

	for (int i = 0; i < (int)_items.size(); i++)
		_items[i]->SetActive(i >= scroll && i <= last);
}

// Confirmed (asm lines 139810-139846)
void TGInterface::StartAnimations() {
	for (TMButton *button : _buttons)
		button->StartAnimation();

	for (THItem *item : _items)
		item->StartAnimation();
}

// Confirmed (asm lines 139858-139881)
void TGInterface::ReattachAnimations() {
	for (TMButton *button : _buttons)
		TGAnimation::ReattachAnimations(*button);
}

// Confirmed (asm lines 140141-140183)
void TGInterface::RemoveSpritesAndAnimations() {
	for (TMButton *button : _buttons) {
		button->SetActive(false);
		button->RemoveAnimations();
	}

	for (THItem *item : _items)
		item->SetActive(false);
}

// Confirmed (asm lines 138951-138984)
void TGInterface::RemoveAllItems() {
	for (THItem *item : _items)
		delete item;

	_items.clear();
}

// Confirmed (asm lines 141576-142010): the items that are there already are kept (also in
// their order of the new list), the new ones made (at the alpha of the interface), the ones
// that are not wanted any more are told to the object manager and deleted; then the scroll
// position is kept inside the items.
void TGInterface::UpdateItems(const TVList &items) {
	std::vector<THItem *> kept;

	for (TVisionaireObject *wanted : items) {
		THItem *existing = nullptr;

		for (THItem *item : _items) {
			if (std::memcmp(item->GetRef().GetId(), wanted->GetId(), 4) == 0) {
				existing = item;
				break;
			}
		}

		if (existing) {
			kept.push_back(existing);
			continue;
		}

		THItem *item = new THItem(TVisObjRef(*wanted), true);

		kept.push_back(item);
		item->SetDestAlpha((int)(100.0f * _alpha), 0);
	}

	for (THItem *item : _items) {
		bool wantedStill = false;

		for (TVisionaireObject *wanted : items) {
			if (std::memcmp(item->GetRef().GetId(), wanted->GetId(), 4) == 0) {
				wantedStill = true;
				break;
			}
		}

		if (!wantedStill) {
			gameControl()->GetObjectManager()->NotifyObjectRemoved(item->GetRef());
			delete item;
		}
	}

	_items = kept;

	_itemIndex.clear();

	for (size_t i = 0; i < _items.size(); i++)
		_itemIndex[idKey(_items[i]->GetRef())] = (int)i;

	// the scroll position stays at an item (the nearest start of a page that has one)
	int scroll = _ref.GetInt(kInterfaceItemsScrollPosition);

	if (_items.empty()) {
		scroll = 0;
	} else {
		int step = _ref.GetInt(kInterfaceScrollStepSize);

		if (step > 0) {
			while (scroll >= (int)_items.size())
				scroll -= step;

			if (scroll < 0)
				scroll = 0;
		}
	}

	_ref.SetValue(kInterfaceItemsScrollPosition, scroll, TSendEventEnum::kNoEvent);
	TestActiveObjects();
}

// Confirmed (asm lines 139893-140010): the fade starts from where it is; without a time it
// is over at once.
void TGInterface::SetDestAlpha(int percent, int milliseconds) {
	_alphaDurationMs = milliseconds;
	_alphaFrom = _alpha;
	_alphaTarget = (float)percent / 100.0f;

	if (milliseconds == 0 && _alpha != _alphaTarget)
		UpdateAlpha();

	_timer.SetTime();
}

// Confirmed (asm lines 140019-140114): the buttons and items fade with the interface.
void TGInterface::UpdateAlpha() {
	if (_alphaTarget == _alpha)
		return;

	long elapsed = _timer.GetTime();

	if (elapsed >= _alphaDurationMs)
		_alpha = _alphaTarget;
	else
		_alpha = _alphaFrom + (_alphaTarget - _alphaFrom) * ((float)elapsed / (float)_alphaDurationMs);

	int percent = (int)(100.0f * _alpha);

	for (TMButton *button : _buttons)
		button->SetDestAlpha(percent, 0);

	for (THItem *item : _items)
		item->SetDestAlpha(percent, 0);
}

// Confirmed (asm lines 140123-140130)
int TGInterface::GetDestAlpha() const {
	return (int)(100.0f * _alphaTarget);
}

// Confirmed (asm lines 140681-140780)
void TGInterface::SetActiveCommand(const TVisObjRef &command, bool update) {
	for (TGCommand *candidate : _commands) {
		if (candidate->GetRef() == command) {
			candidate->SetActiveSprite(true);
			_activeCommand = candidate;
		} else {
			candidate->SetActiveSprite(false);
		}
	}

	if (!update || command.IsEmpty())
		return;

	// the current character and the game take it over
	TGCharacter *character = gameControl()->GetCurrentCharacter();

	if (!character->GetRef().IsEmpty())
		character->GetRef().SetLink(kCharacterActiveCommand, command, false);

	gameControl()->GetGameSystem()->GetGame().SetLink(kGameActiveCommand, command, true);
}

// Confirmed (asm lines 140797-140818)
TVisObjRef TGInterface::GetActiveCommand() {
	if (_activeCommand)
		return _activeCommand->GetRef();

	return TVisObjRef();
}

// Confirmed (asm lines 140829-141150)
void TGInterface::Load() {
	UpdateActiveStatus();

	StartAnimations();
	ReattachAnimations();

	// the fade the record has: first the alpha it is at, then what it goes to
	SetDestAlpha(_ref.GetInt(kInterfaceVisibility), 0);
	SetDestAlpha(_ref.GetInt(kInterfaceDestVisibility), _ref.GetInt(kInterfaceTimeToDestVisibility));

	SetActiveCommand(_ref.GetLink(kInterfaceActiveCommand), false);
}
