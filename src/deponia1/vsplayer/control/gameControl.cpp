#include "vsplayer/control/gameControl.h"

#include <algorithm>
#include <cmath>

#include "AppGlobals.h"
#include "Diagnostics.h"
#include "TComposedFileManager.h"
#include "TGAction.h"
#include "TGAnimation.h"
#include "TGInterface.h"
#include "THCharacter.h"
#include "THInterface.h"
#include "THText.h"
#include "TMSavegame.h"
#include "TManagedObject.h"
#include "TTText.h"
#include "TTempFile.h"
#include "baselib/composedfile.h"
#include "baselib/file.h"
#include "datastruct/visionaireobject.h"
#include "graphicslib/graphics.h"
#include "vscommon/scripting/argument.h"
#include "vscommon/scripting/id.h"

namespace {
// Packs a TVisObjRef::GetId() 3-byte id into a 32-bit value the same way
// StartDialog/EndDialog/GetCharacter's hash lookup all do it: byte0 |
// (byte1<<8) | (sign-extended byte2<<16) - the sign extension of the third
// byte is confirmed (an `and 0xFF000000` masking a `sar 0x1F`-derived sign
// mask in the disassembly), its purpose is not.
int PackVisId(const std::uint8_t *id) {
	return id[0] | (id[1] << 8) | (static_cast<int>(static_cast<std::int8_t>(id[2])) << 16);
}

// Confirmed global (not per-instance) engine-event queue guarded by a
// global critical section (TGameControl::PushEngineEvent, Deponia_Linux.asm
// lines 473304-473417: uses the global symbols EngineEvents/
// EngineEventLock, not any `this`-relative field) - a growable buffer of
// (name, arg) string pairs, matching std::vector<std::pair<std::string,
// std::string>>::push_back()'s own inlined codegen exactly.
wxCriticalSection EngineEventLock;
std::vector<std::pair<std::string, std::string>> EngineEvents;
}  // namespace

TGameControl::TGameControl() {
	// TMasterControl's own base subobject is constructed automatically.
	// TGameControl embeds a real TSceneControl and points TMasterControl's
	// (protected) _sceneControl at it - TMasterControl's own constructor
	// never sets that pointer, so this is presumed to be TGameControl's
	// job (not otherwise confirmed - see masterControl.h).
	_sceneControl = &_ownedSceneControl;

	_visionaireGame = new TVisionaireGame();

	// Confirmed: the constructor makes *this* the global game-controller
	// singleton itself, rather than leaving that to the caller.
	g_pGameControl = this;

	// Confirmed: registers a TComposedFile::onError handler (logs the
	// failing archive's exe-filename + the error string) - the handler
	// body itself wasn't traced in detail (see TComposedFile::onError's
	// declaration).
	TComposedFile::onError = [](TComposedFile */*file*/, std::string /*message*/) {};
}

TGameControl::~TGameControl() {
	// Confirmed (asm lines 474222-474270): sets _isClearingAnimations
	// before tearing down, then deletes every owned TGCharacter*. The
	// destructor also calls into ModelContainer::Destroy - not modeled,
	// since that class hasn't been reversed at all yet - and a virtual
	// teardown call through a pointer at a still-unidentified field - left
	// out rather than guessing.
	_isClearingAnimations = true;
	TGAction::ClearActions();
	TGAnimation::ClearAnimations();
	ClearTexts();
	for (TGCharacter *character : _characters)
		delete character;
	_characters.clear();
}

bool TGameControl::Update() {
	// The entire per-frame game-logic dispatcher - almost certainly
	// thousands of lines given the class's overall size. Not reversed.
	return true;
}

bool TGameControl::DisplayDialog() {
	// Confirmed (asm lines 455780-455803): only draws/activates the cursor
	// when a dialog is actually active (_dialog's TVisObjRef target is
	// non-empty).
	if (_dialog.IsEmpty())
		return false;
	GetCursorControl()->SetActive(true);
	_dialog.Draw();
	return true;
}

namespace {
// Shared by DisplayTexts()'s two identical flag computations: true unless
// the text has a speaker whose TVisObjRef field id 0x319 reads as 0 - field
// id meaning unresolved.
bool ShouldPushMatrices(TGText *text) {
	TGCharacter *speaker = text->GetSpeaker();
	return speaker == nullptr || speaker->GetRef().GetInt(0x319) != 0;
}
}  // namespace

bool TGameControl::DisplayTexts() {
	// Confirmed (asm lines 455890-456037). First, drop every active text
	// whose target no longer reads as "displayed" (field id 0x211, matching
	// several other text methods); then draw everything that's left, plus
	// _currentText if it's still displayed too.
	for (auto it = _activeTexts.begin(); it != _activeTexts.end();) {
		if (!(*it)->GetTarget().GetBool(0x211)) {
			(*it)->OnCleared();
			(*it)->Discard();
			it = _activeTexts.erase(it);
		} else {
			++it;
		}
	}

	_ownedSceneControl.GetScene()->SetCurrent();

	for (TGText *text : _activeTexts) {
		bool saved = matricesActive;
		matricesActive = ShouldPushMatrices(text);
		text->Draw(1.0f);
		matricesActive = saved;
	}

	if (_currentText == nullptr || !_currentText->GetTarget().GetBool(0x211))
		return false;

	bool saved = matricesActive;
	matricesActive = ShouldPushMatrices(_currentText);
	_currentText->Draw(1.0f);
	matricesActive = saved;
	return true;
}

bool TGameControl::DisplayConsole() {
	// Confirmed tail-call (asm lines 455750-455756): DisplayConsole() is
	// exactly TConsole::Draw() on the embedded console.
	return _console.Draw();
}

void TGameControl::DisplayInSceneConsole() {
	// Confirmed tail-call (asm lines 455758-455764).
	_console.DrawInScene();
}

void TGameControl::HandleMouseMove(const wxPoint &/*pos*/, bool /*isHolding*/) {
}

void TGameControl::HandleMouseUp(const wxPoint &/*pos*/, TMouseMessageEnum /*msg*/) {
}

void TGameControl::HandleMouseHolding(const wxPoint &/*pos*/) {
	// Confirmed (asm lines 455671-455746): `pos` itself is never read - only
	// whether the cursor is currently active. Field id 0x182 meaning not
	// resolved.
	if (!GetCursorControl()->IsActive())
		return;
	TVisObjRef game = _visionaire->GetGame();
	TVisObjRef link = game.GetLink(0x182);
	if (!link.IsEmpty())
		TGAction::AddRunningAction(link);
}

void TGameControl::UpdateTexts() {
	// Confirmed (asm lines 456183-456282): field id 0x211 (matches
	// IsTextActive/IsNoTextDisplayed's usage) gates whether each text needs
	// recalculating. If _currentText still reads as "displayed" after
	// recalculating, its tail is exactly ClearCurrentText()'s body
	// (matching field id 0x1DD).
	for (TGText *text : _activeTexts) {
		if (text->GetTarget().GetBool(0x211))
			text->CalculateCurrentText();
	}

	if (_currentText != nullptr && _currentText->GetTarget().GetBool(0x211)) {
		_currentText->CalculateCurrentText();
		if (_currentText->GetTarget().GetBool(0x211))
			ClearCurrentText();
	}
}

TSceneControl *TGameControl::GetSceneControl() {
	return &_ownedSceneControl;
}

TGScene *TGameControl::GetScene() {
	return _ownedSceneControl.GetScene();
}

TGCharacter *TGameControl::GetCurrentCharacter() {
	// Confirmed (asm line 456332): plain field access, not always-nullptr.
	return _currentCharacter;
}

TGCharacter *TGameControl::GetCurrentCharacterPointer() const {
	// Confirmed (asm line 456349): same field as GetCurrentCharacter().
	return _currentCharacter;
}

// GetCharacter/GetCharacterPointer/GetCharacterPointerEx (asm lines
// 456362-456578): all three share one pattern - if the TVisObjRef arg
// IsEmpty(), return a fallback (_currentCharacter for the first two,
// nullptr for GetCharacterPointerEx); otherwise pack TVisObjRef::GetId()
// via PackVisId() and look it up in _charactersByHash (see its
// declaration - InitCharacters populates it with the very same formula),
// falling back the same way on a miss.
TGCharacter *TGameControl::GetCharacter(const TVisObjRef &character) {
	if (character.IsEmpty())
		return _currentCharacter;
	auto it = _charactersByHash.find(PackVisId(character.GetId()));
	return it != _charactersByHash.end() ? it->second : _currentCharacter;
}

TGCharacter *TGameControl::GetCharacterPointer(const TVisObjRef &character) const {
	if (character.IsEmpty())
		return _currentCharacter;
	auto it = _charactersByHash.find(PackVisId(character.GetId()));
	return it != _charactersByHash.end() ? it->second : _currentCharacter;
}

TGCharacter *TGameControl::GetCharacterPointerEx(const TVisObjRef &character) const {
	if (character.IsEmpty())
		return nullptr;
	auto it = _charactersByHash.find(PackVisId(character.GetId()));
	return it != _charactersByHash.end() ? it->second : nullptr;
}

std::vector<TGCharacter *> &TGameControl::GetAllCharacters() {
	return _characters;
}

TGInterface *TGameControl::GetInterface(const TVisObjRef &interfaceObj) const {
	// Confirmed (asm lines 456603-456648).
	for (TGInterface *interface : _activeInterfaces) {
		if (interface->GetRef() == interfaceObj)
			return interface;
	}
	return nullptr;
}

void *TGameControl::GetObject(const TVisObjRef &object) const {
	// Confirmed (asm lines 456656-456737): scene lookup first; then, only
	// when the id's 4th byte is zero (meaning unconfirmed - some kind of
	// "is a character" type tag), a character lookup; either way, falls
	// back to searching the active interfaces last.
	if (void *obj = _ownedSceneControl.GetScene()->GetObject(object))
		return obj;

	if (object.GetId()[3] == 0) {
		if (TGCharacter *character = GetCharacterPointerEx(object))
			return character;
	}

	for (TGInterface *interface : _activeInterfaces) {
		if (void *obj = interface->GetObject(object))
			return obj;
	}
	return nullptr;
}

TGObjectManager *TGameControl::GetObjectManager() {
	// Confirmed (asm line 456751): embedded by value in TGameControl itself,
	// not TMasterControl as first guessed.
	return &_objectManager;
}

void TGameControl::SkipCurrentText() {
	// Confirmed (asm lines 456762-456956): field id 0x1E0 normally blocks
	// skipping outright, unless field id 0x235 overrides that (both
	// unresolved). Otherwise identical to letting the text handle its own
	// skip, then - if it finished as a result - clearing it exactly like
	// ClearCurrentText() does (same field id 0x1DD).
	TVisObjRef game = _visionaire->GetGame();
	if (game.GetBool(0x1E0)) {
		TVisObjRef allowOverride = _visionaire->GetGame();
		if (!allowOverride.GetBool(0x235))
			return;
	}

	if (_currentText == nullptr || !_currentText->GetTarget().GetBool(0x211))
		return;

	_currentText->SkipCurrentText();
	if (_currentText->GetTarget().GetBool(0x211))
		return;

	TVisObjRef game2 = _visionaire->GetGame();
	game2.ClearLink(0x1DD, true);
	_currentText->Discard();
	_currentText = nullptr;
}

void TGameControl::UpdateCurrentObject() {
	// Confirmed (asm lines 456964-456999): re-dispatches the last known
	// mouse position through HandleMouseMove(pos, false) unless it's still
	// at the {-1,-1} "no position yet" sentinel.
	if (_lastMousePos.x != -1 || _lastMousePos.y != -1)
		HandleMouseMove(_lastMousePos, false);
}

void TGameControl::RegisterHookFunctionSceneMousePosition(const wxString &name) {
	// Confirmed (asm line 457015): a tail-call to std::wstring::assign on a
	// single field - not a map as first guessed.
	_sceneMousePositionHookName = name.ToStdWstring();
}

TConsole *TGameControl::GetConsole() {
	return &_console;
}

int TGameControl::ConvertControllerButtonToSymKey(SDL_ControllerButtonEvent button) {
	// Confirmed (asm lines 457044-457058, table CSWTCH_876 at 3147444):
	// buttons 0-14 map to a custom keysym space starting at 1000001
	// (presumably reserved above the Unicode range used for regular
	// keyboard keys); anything else yields -1.
	if (button.button > 0x0E)
		return -1;
	return 1000001 + button.button;
}

wxString TGameControl::ConvertControllerAxisToUnicode(SDL_GameControllerAxis axis) {
	// Confirmed (asm lines 457066-457125): a 6-case switch, each case
	// assigning one literal wide string; default (and any axis outside the
	// 6 known ones) leaves it empty. Cases 0/1/4/5 were read directly from
	// the binary's string data ("LEFTX"/"LEFTY"/"TRIGGERLEFT"/
	// "TRIGGERRIGHT"); 2/3 ("RIGHTX"/"RIGHTY") follow the same naming
	// pattern but weren't individually byte-checked.
	switch (axis) {
	case SDL_CONTROLLER_AXIS_LEFTX:
		return wxString(L"LEFTX");
	case SDL_CONTROLLER_AXIS_LEFTY:
		return wxString(L"LEFTY");
	case SDL_CONTROLLER_AXIS_RIGHTX:
		return wxString(L"RIGHTX");
	case SDL_CONTROLLER_AXIS_RIGHTY:
		return wxString(L"RIGHTY");
	case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
		return wxString(L"TRIGGERLEFT");
	case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
		return wxString(L"TRIGGERRIGHT");
	default:
		return wxString();
	}
}

void TGameControl::StartGameAction(TKeyboardMessageEnum msg, const wxString &/*name*/, int a,
                                   unsigned short /*b*/) {
	// Confirmed (asm lines 457301-457406): `name`/`b` are never read in this
	// function (only used by its caller, HandleKeyEvent, to build up the
	// wxString and passed through for a different purpose). Field id 0x242
	// is unresolved. Stops at the first (a, msg)-matching action, whether
	// or not it actually fires.
	TVisObjRef game = _visionaire->GetGame();
	bool overrideBlock = game.GetBool(0x242);

	for (const SGameAction &action : _gameActions) {
		if (action.a != a || action.msg != static_cast<int>(msg))
			continue;

		if (action.flag) {
			TGAction::AddRunningAction(action.target);
		} else if (_dialog.IsEmpty() || overrideBlock) {
			bool textBlocking = _currentText != nullptr && _currentText->GetTarget().GetBool(0x211);
			if (!textBlocking)
				TGAction::AddRunningAction(action.target);
		}
		return;
	}
}

void TGameControl::UpdateAspectRatio() {
	// Confirmed (asm lines 457414-457458): picks width/height from either
	// renderSize (if g_unlockAspect) or the game data's aspect point (field
	// id 0x7E, meaning unconfirmed), stores them, then reconfigures the
	// inherited TPaintControl surface. TVisObjRef::GetPoint()'s field-id
	// meaning is not resolved - see visobjref.h.
	TVisObjRef game = _visionaire->GetGame();
	const wxPoint *aspectPoint = game.GetPoint(0x7E);
	if (g_unlockAspect) {
		_aspectWidth = renderSize.width;
		_aspectHeight = renderSize.height;
	} else {
		_aspectWidth = aspectPoint->x;
		_aspectHeight = aspectPoint->y;
	}
	InitControl(_aspectWidth, _aspectHeight);
}

void TGameControl::InitAfterLoadingScreen() {
}

void TGameControl::SaveEventHandlers() {
}

void TGameControl::ExecuteStartingAction() {
	// Confirmed (asm lines 458052-458116): field id 0x170, meaning not
	// resolved.
	TVisObjRef game = _visionaire->GetGame();
	TVisObjRef link = game.GetLink(0x170);
	if (!link.IsEmpty()) {
		TGAction::AddRunningAction(link);
		TGAction::ContinueRunningActions(false);
	}
}

void TGameControl::InitInterfaces() {
	// Confirmed (asm lines 458124-458250): field id 0x296 and TypeOrder
	// value 1 are both unresolved.
	TVisObjRef game = _visionaire->GetGame();
	TVList links;
	game.GetLinks(0x296, TypeOrder::kValue1, links);

	for (TVisionaireObject *object : links)
		_allInterfaces.push_back(new THInterface(TVisObjRef(*object)));
}

void TGameControl::SetCharacterInterfaces() {
	// Confirmed (asm lines 458260-458285).
	for (TGCharacter *character : _characters)
		character->SetInterfaces();
}

void TGameControl::InitFonts() {
	// Confirmed (asm lines 458293-458322): field id 3, flag true - meaning
	// of either not resolved.
	TVList fonts;
	_visionaire->GetList(3, fonts, true);
	GetFontManager()->Initialize(fonts);
}

void TGameControl::InitScripts() {
}

TVisionaireGame *TGameControl::GetGameSystem() {
	return _visionaireGame;
}

TVisionaireGame *TGameControl::GetVisionaire() {
	return _visionaireGame;
}

void TGameControl::ScrollToCharacterIfNeeded(const TVisObjRef &character) {
	// Confirmed (asm lines 458931-459278). The character read throughout is
	// _previousCharacter (offset 0x290), not _currentCharacter - see that
	// field's own comment for why they're modeled as separate members
	// despite always holding the same value. Field ids are best-effort names
	// from context, not confirmed beyond their raw ids: 0x263 ("the active
	// character" link - ChangeCharacter/InitCharacters keep it in sync with
	// _currentCharacter/_previousCharacter, same value set at the same time
	// in all three), 0x231 (a "scrolling enabled" toggle), 0x29B/0x29C (an
	// x/y offset added before the visible-area comparisons below), and
	// 0x1D9/0x1DA (horizontal/vertical scroll-direction codes - CenterScene
	// resets both to 0; here, 1=left, 2=right, 3=up, 4=down).
	TVisObjRef game = _visionaire->GetGame();
	TGScene *scene = _ownedSceneControl.GetScene();
	if (scene->IsMenu() || _previousCharacter == nullptr)
		return;

	bool shouldScroll = false;
	if (game.GetLink(0x263) == character && game.GetBool(0x231)) {
		TVisObjRef charSceneLink = _previousCharacter->GetRef().GetLink(0x1F7);
		shouldScroll = (charSceneLink == scene->GetRef());
	}
	if (!shouldScroll)
		return;

	const FloatPoint &scrollPos = scene->GetFloatScrollPos();
	int worktopWidth = scene->GetWorktopWidth();
	int worktopHeight = scene->GetWorktopHeight();
	const wxSize &visibleSize = scene->GetVisibleSize();

	wxPoint charPos = _previousCharacter->GetScreenPosition();
	wxRect charRect = _previousCharacter->GetVisibleRect();
	if (charRect.IsEmpty()) {
		charRect.SetLeft(charPos.x);
		charRect.SetWidth(0);
		charRect.SetTop(charPos.y);
		charRect.SetHeight(0);
	}

	int offsetX = game.GetInt(0x29B);
	int offsetY = game.GetInt(0x29C);

	// Horizontal: right-scroll and left-scroll are checked independently (not
	// mutually exclusive in the disassembly - a right-scroll match doesn't
	// skip the left-scroll check below it).
	if (static_cast<float>(worktopWidth) > visibleSize.width + scrollPos.x) {
		if (static_cast<float>(charRect.GetRight() + offsetX) - scrollPos.x > visibleSize.width)
			game.SetValue(0x1D9, 2, TSendEventEnum::kSendEvent);
	}
	if (scrollPos.x > 0.0f) {
		if (static_cast<float>(charRect.GetLeft() - offsetX) - scrollPos.x < 0.0f)
			game.SetValue(0x1D9, 1, TSendEventEnum::kSendEvent);
	}

	// Vertical: scroll-up returns immediately on a match, so scroll-down is
	// only ever checked when scroll-up didn't fire.
	if (scrollPos.y > 0.0f) {
		if (static_cast<float>(charRect.GetTop() - offsetY) - scrollPos.y < 0.0f) {
			game.SetValue(0x1DA, 3, TSendEventEnum::kSendEvent);
			return;
		}
	}
	if (static_cast<float>(worktopHeight) > visibleSize.height + scrollPos.y) {
		if (static_cast<float>(charRect.GetBottom() + offsetY) - scrollPos.y > visibleSize.height)
			game.SetValue(0x1DA, 4, TSendEventEnum::kSendEvent);
	}
}

void TGameControl::MoveScene() {
	// Confirmed (asm lines 459288-460538+): a large scroll-to-target easing
	// function, structurally similar to TMasterControl::ScrollUpdate (and
	// sharing its xspeed/yspeed/startspeed globals and _easeDirectionFlag)
	// but driving the scene toward a stored target point (field 0x1D7)
	// instead of the mouse cursor. Approximated the same way ScrollUpdate
	// already is: the gating logic, target point, and the confirmed
	// formulas (exponential ease toward +-1 or a distance-clamped
	// speedDownX/Y, using a delta time derived from _timingValueSeconds)
	// are faithful, but the exact decision tree for which side to approach
	// from per axis - keyed on fields 0x1D9/0x1DA (0=auto, 1/2=forced
	// left/right, 3/4=forced up/down, matching ScrollToCharacterIfNeeded's
	// own codes) and a two-tier "is there room to scroll, and if so which
	// side" structure - is simplified into one merged condition per axis
	// rather than transcribed branch-by-branch, since it's gameplay-feel-
	// specific and can't be verified without running the original. Field
	// ids 0x1D8 ("unconditional movement" override) and 0x257 (a character
	// facing-angle check gating one sub-case, skipped here) are as
	// confirmed but not further pursued.
	static TTimer scrollTimer;

	TVisObjRef game = _visionaire->GetGame();
	bool overrideGate = game.GetBool(0x1D8);

	if (!overrideGate) {
		if (IsScrolling()) {
			scrollTimer.SetTime();
			return;
		}
		if (_currentCharacter == nullptr)
			return;
		TVisObjRef charSceneLink = _currentCharacter->GetRef().GetLink(0x1F7);
		if (!(charSceneLink == _ownedSceneControl.GetScene()->GetRef()))
			return;
	}

	TGScene *scene = _ownedSceneControl.GetScene();
	if (scene->IsMenu())
		return;
	if (_previousCharacter == nullptr || _ownedSceneControl.FadingToNewScene())
		return;

	const FloatPoint &scrollPos = scene->GetFloatScrollPos();
	int worktopWidth = scene->GetWorktopWidth();
	int worktopHeight = scene->GetWorktopHeight();
	const wxSize &visibleSize = scene->GetVisibleSize();
	const wxPoint *target = game.GetPoint(0x1D7);

	wxPoint charPos = _previousCharacter->GetScreenPosition();
	wxRect charRect = _previousCharacter->GetVisibleRect();
	if (charRect.IsEmpty()) {
		charRect.SetLeft(charPos.x);
		charRect.SetWidth(0);
		charRect.SetTop(charPos.y);
		charRect.SetHeight(0);
	}

	double elapsedMs = static_cast<double>(scrollTimer.GetTime());
	float dt = (elapsedMs > 500.0) ? 1.0f : static_cast<float>(elapsedMs) * _timingValueSeconds;

	int horizState = game.GetInt(0x1D9);
	float targetLeft = static_cast<float>(target->x) - static_cast<float>(visibleSize.width) / 2.0f;
	if (horizState != 0 || static_cast<float>(worktopWidth) > scrollPos.x + static_cast<float>(visibleSize.width)) {
		float distance = targetLeft - scrollPos.x;
		if (std::fabs(distance) > 1.0f && dt > 0.0f) {
			float maxSpeed = std::min(1.0f, std::fabs(distance) / dt * 0.025f);
			speedDownX = maxSpeed;
			float targetSpeed = _easeDirectionFlag ? -1.0f : (distance < 0.0f ? -maxSpeed : maxSpeed);
			xspeed = targetSpeed + (xspeed - targetSpeed) * startspeed;
			scene->AdjustWindowHorizontal(xspeed * dt);
		} else {
			xspeed = 0.0f;
			if (horizState != 0 && _previousCharacter->IsWalking())
				game.SetValue(0x1D9, 2, TSendEventEnum::kSendEvent);
		}
	}

	int vertState = game.GetInt(0x1DA);
	float targetTop = static_cast<float>(target->y) - static_cast<float>(visibleSize.height) / 2.0f;
	if (vertState == 3 || vertState == 4 ||
	        static_cast<float>(worktopHeight) > scrollPos.y + static_cast<float>(visibleSize.height)) {
		float distance = targetTop - scrollPos.y;
		if (std::fabs(distance) > 1.0f && dt > 0.0f) {
			float maxSpeed = std::min(1.0f, std::fabs(distance) / dt * 0.025f);
			speedDownY = maxSpeed;
			float targetSpeed = _easeDirectionFlag ? -1.0f : (distance < 0.0f ? -maxSpeed : maxSpeed);
			yspeed = targetSpeed + (yspeed - targetSpeed) * startspeed;
			scene->AdjustWindowVertical(yspeed * dt);
		} else {
			yspeed = 0.0f;
			if ((vertState == 3 || vertState == 4) && _previousCharacter->IsWalking())
				game.SetValue(0x1DA, vertState, TSendEventEnum::kSendEvent);
		}
	}

	game.SetValue(0x1D6, scene->GetScrollPos(), TSendEventEnum::kSendEvent);
	scrollTimer.SetTime();
}

void TGameControl::CenterScene() {
	// Confirmed (asm lines 460533-460715): only proceeds when the current
	// character's scene-link (field id 0x1F7) matches the current scene's
	// own identifying TVisObjRef. Field ids 0x1D9/0x1DA/0x1D6 (all
	// SetValue()d at the end) are unresolved.
	TVisObjRef game = _visionaire->GetGame();
	TGScene *scene = _ownedSceneControl.GetScene();

	TVisObjRef link = _currentCharacter->GetRef().GetLink(0x1F7);
	if (!(link == scene->GetRef()))
		return;

	const wxSize &visibleSize = scene->GetVisibleSize();
	wxPoint charPos = _currentCharacter->GetScreenPosition();
	wxRect charRect = _currentCharacter->GetVisibleRect();

	if (!(charPos == wxPoint{-1, -1})) {
		scene->AdjustWindowHorizontal(static_cast<float>(charPos.x - visibleSize.width / 2));

		int verticalAdjust;
		if (!charRect.IsEmpty() && charRect.GetHeight() > 0)
			verticalAdjust = charRect.GetTop() + charRect.GetHeight() / 2 - visibleSize.height / 2;
		else
			verticalAdjust = charPos.y - visibleSize.height / 2;
		scene->AdjustWindowVertical(static_cast<float>(verticalAdjust));
	}

	game.SetValue(0x1D9, 0, TSendEventEnum::kSendEvent);
	game.SetValue(0x1DA, 0, TSendEventEnum::kSendEvent);
	game.SetValue(0x1D6, scene->GetScrollPos(), TSendEventEnum::kSendEvent);
}

void TGameControl::SetOnScrollDestination() {
	// Confirmed (asm lines 460720-460821): field ids 0x1D7 (a scroll
	// adjustment point), 0x1D6 (write-back of the resulting scroll pos),
	// 0x231 (whether to re-center), and 0x1D8 (cleared afterward) are all
	// unresolved. GetPoint() is called twice (once per axis) in the
	// original - matched here rather than caching, since our TVisObjRef
	// stub always returns the same value anyway.
	TVisObjRef game = _visionaire->GetGame();
	TGScene *scene = _ownedSceneControl.GetScene();

	scene->AdjustWindowHorizontal(static_cast<float>(game.GetPoint(0x1D7)->x));
	scene->AdjustWindowVertical(static_cast<float>(game.GetPoint(0x1D7)->y));

	game.SetValue(0x1D6, scene->GetScrollPos(), TSendEventEnum::kSendEvent);
	if (game.GetBool(0x231))
		CenterScene();

	TVisObjRef game2 = _visionaire->GetGame();
	game2.SetValue(0x1D8, false, TSendEventEnum::kSendEvent);
}

void TGameControl::HandleCharacters() {
	// Confirmed (asm lines 460829-460866): skipped entirely while the scene
	// is a menu.
	if (GetScene()->IsMenu())
		return;
	for (TGCharacter *character : _characters) {
		character->WalkWay();
		character->UpdateCharacter();
	}
}

void TGameControl::SetAllCharactersOnDestination() {
	// Confirmed (asm lines 460874-460902).
	for (TGCharacter *character : _characters)
		character->SetOnDestination();
}

void TGameControl::ResetState() {
	// Confirmed (asm lines 460910-460953): field id 0x1E6 on the game's
	// TVisObjRef, meaning unconfirmed.
	_objectManager.ResetCurrentObject();
	_objectManager.ResetEventInfo();
	TVisObjRef game = _visionaire->GetGame();
	game.ClearLink(0x1E6, false);
	_objectManager.RemoveItem(true);
	_pendingItems.clear();
}

const TGDialog *TGameControl::GetDialog() const {
	return &_dialog;
}

void TGameControl::StartDialog(const TVisObjRef &dialog) {
	// Confirmed (asm lines 460983-461073): surprisingly, only takes effect
	// when a dialog is ALREADY active (_dialog not empty) - a no-op
	// otherwise. Field ids 0x11B (the current character's cursor link) and
	// 0x1DC (the game's active-dialog link) are unresolved.
	if (_dialog.IsEmpty())
		return;

	TVisObjRef cursorLink = _currentCharacter->GetRef().GetLink(0x11B);
	GetCursorControl()->SetCursor(PackVisId(cursorLink.GetId()), true);

	TVisObjRef game = _visionaire->GetGame();
	game.SetLink(0x1DC, dialog, true);

	_dialog.SetDialog(dialog);
}

void TGameControl::EndDialog() {
	// Confirmed (asm lines 461079-461192): field ids 0x1DC (matches
	// StartDialog's) and 0x262, both unresolved.
	if (_dialog.IsEmpty())
		return;

	_dialog.Clear();

	TVisObjRef game = _visionaire->GetGame();
	game.ClearLink(0x1DC, true);

	TVisObjRef link = _visionaire->GetGame().GetLink(0x262);
	if (!link.IsEmpty())
		GetCursorControl()->SetCursor(false, PackVisId(link.GetId()), false);
}

void TGameControl::StartText(const TVisObjRef &text, TGCharacter *character, TextAlignmentEnum alignment,
                             const TVisObjRef &target, const wxPoint &pos) {
	// Confirmed (asm lines 461200-461415): dedupes against an existing
	// active text with the same speaker, drops any current text, creates
	// the new one, and keeps it as _currentText only if its target reads
	// as "displayed" (field id 0x211) - otherwise discards it immediately.
	if (character != nullptr) {
		for (auto it = _activeTexts.begin(); it != _activeTexts.end(); ++it) {
			if ((*it)->GetSpeaker() == character) {
				(*it)->OnCleared();
				(*it)->Discard();
				_activeTexts.erase(it);
				break;
			}
		}
	}

	if (_currentText != nullptr) {
		_currentText->Discard();
		TVisObjRef game = _visionaire->GetGame();
		game.ClearLink(0x1DD, true);
		_currentText = nullptr;
	}

	TVisObjRef activeObject = _visionaire->CreateActiveObject(0x18, text);
	TVisObjRef emptyObject = _visionaire->GetEmptyObject();
	_currentText = new THText(activeObject, text, character, emptyObject, alignment, target, pos, true, false);

	if (_currentText->GetTarget().GetBool(0x211)) {
		TVisObjRef game = _visionaire->GetGame();
		game.SetLink(0x1DD, _currentText->GetTarget(), true);
	} else {
		_currentText->Discard();
		_currentText = nullptr;
	}
}

void TGameControl::StartBackgroundText(const TVisObjRef &text, TGCharacter *character, TextAlignmentEnum alignment,
                                       const TVisObjRef &target, const wxPoint &pos) {
	// Confirmed (asm lines 461420-461589): skips creating a duplicate when
	// `character` is already speaking - either as _currentText, or as one
	// of the active texts - matched by TGText::GetSpeaker() pointer
	// equality. Type id 0x18 (matches StartObjectText's CreateActiveObject
	// call) and the two trailing THText constructor bools are unresolved.
	if (character != nullptr) {
		if (_currentText != nullptr && _currentText->GetTarget().GetBool(0x211) &&
		        _currentText->GetSpeaker() == character)
			return;

		for (TGText *activeText : _activeTexts) {
			if (activeText->GetSpeaker() == character)
				return;
		}
	}

	TVisObjRef activeObject = _visionaire->CreateActiveObject(0x18, text);
	TVisObjRef emptyObject = _visionaire->GetEmptyObject();
	THText *newText = new THText(activeObject, text, character, emptyObject, alignment, target, pos, true, true);
	_activeTexts.push_back(newText);
}

void TGameControl::ReattachSceneObjectTexts() {
	// Confirmed (asm lines 461599-461666): field id 0x2AC, and the id-byte-3
	// check ("== 6") meaning are both unresolved.
	for (TGText *text : _sceneTexts) {
		TVisObjRef linked = text->GetTarget().GetLink(0x2AC);
		if (linked.GetId()[3] != 6)
			continue;
		if (TManagedObject *object = _ownedSceneControl.GetScene()->GetObject(linked))
			object->SetText(text);
	}
}

bool TGameControl::IsTextActive(const TVisObjRef &text) const {
	// Confirmed (asm lines 461674-461739).
	if (_currentText == nullptr)
		return false;
	if (!(_currentText->GetDataObject() == text))
		return false;
	return _currentText->GetTarget().GetBool(0x211);
}

bool TGameControl::IsNoTextDisplayed() const {
	// Confirmed (asm lines 461747-461777): a text counts as "displayed"
	// when its target's GetBool(0x211) is set; otherwise fall back to
	// whether a dialog is active.
	if (_currentText != nullptr && _currentText->GetTarget().GetBool(0x211))
		return false;
	return _dialog.IsEmpty();
}

bool TGameControl::IsTalking(const TVisObjRef &character) const {
	// Confirmed (asm lines 461785-461846): a text with no speaker
	// (GetSpeaker() == nullptr) never counts as this character talking,
	// regardless of its target.
	for (TGText *text : _activeTexts) {
		TGCharacter *speaker = text->GetSpeaker();
		if (speaker != nullptr && speaker->GetRef() == character)
			return true;
	}
	return false;
}

void TGameControl::ClearTexts() {
	// Confirmed (asm lines 461872-461993): drains _activeTexts and
	// _sceneTexts (OnCleared() then Discard() on each), then separately
	// discards _currentText (Discard() only, no OnCleared()) and clears
	// its game-data link (field id 0x1DD, matching ClearCurrentText).
	for (TGText *text : _activeTexts) {
		text->OnCleared();
		text->Discard();
	}
	_activeTexts.clear();

	for (TGText *text : _sceneTexts) {
		text->OnCleared();
		text->Discard();
	}
	_sceneTexts.clear();

	if (_currentText != nullptr) {
		_currentText->Discard();
		_currentText = nullptr;
		TVisObjRef game = _visionaire->GetGame();
		game.ClearLink(0x1DD, true);
	}
}

void TGameControl::ClearCurrentText() {
	// Confirmed (asm lines 462001-462040): field id 0x1DD, meaning not
	// resolved.
	if (_currentText != nullptr) {
		_currentText->Discard();
		_currentText = nullptr;
		TVisObjRef game = _visionaire->GetGame();
		game.ClearLink(0x1DD, true);
	}
}

void TGameControl::ClearText(const TVisObjRef &text) {
	// Confirmed (asm lines 462048-462150): removes at most one matching
	// entry from _activeTexts (by GetDataObject() equality), then
	// separately clears _currentText too if it also matches.
	for (auto it = _activeTexts.begin(); it != _activeTexts.end(); ++it) {
		TGText *activeText = *it;
		if (activeText->GetDataObject() == text) {
			activeText->OnCleared();
			activeText->Discard();
			_activeTexts.erase(it);
			break;
		}
	}

	if (_currentText != nullptr && _currentText->GetDataObject() == text)
		ClearCurrentText();
}

void TGameControl::ClearObjectText(const TVisObjRef &object) {
	// Confirmed (asm lines 462158-462228): find the one scene text whose
	// target's GetLink(0x2AC) matches `object`, discard and remove it, then
	// stop (only ever removes at most one entry).
	for (auto it = _sceneTexts.begin(); it != _sceneTexts.end(); ++it) {
		TGText *text = *it;
		if (text->GetTarget().GetLink(0x2AC) == object) {
			text->Discard();
			_sceneTexts.erase(it);
			return;
		}
	}
}

void TGameControl::StartObjectText(const TVisObjRef &object, const TVisObjRef &text, TextAlignmentEnum alignment,
                                   const TVisObjRef &target, const wxPoint &pos) {
	// Confirmed (asm lines 462233-462396): clears any existing scene text
	// targeting `text`, creates a new THText and adds it to _sceneTexts,
	// then attaches it to whichever managed object claims `text` - the
	// scene itself, the current character (only tried when the object id's
	// 4th byte is 0 - meaning unconfirmed), or failing that, whichever
	// active interface recognizes it. Type id 0x18 (passed to
	// CreateActiveObject) and the two trailing THText constructor bools
	// are unresolved.
	ClearObjectText(text);

	TVisObjRef activeObject = _visionaire->CreateActiveObject(0x18, object);
	THText *newText = new THText(activeObject, object, nullptr, text, alignment, target, pos, true, false);
	_sceneTexts.push_back(newText);

	TManagedObject *managed = _ownedSceneControl.GetScene()->GetObject(text);
	if (managed == nullptr && object.GetId()[3] == 0)
		managed = GetCharacterPointerEx(text);
	if (managed == nullptr) {
		for (TGInterface *interface : _activeInterfaces) {
			managed = interface->GetObject(text);
			if (managed != nullptr)
				break;
		}
	}
	if (managed != nullptr)
		managed->SetText(newText);
}

const wxFileName &TGameControl::GetGamePath() const {
	// Confirmed (asm line 462534): returns the member by reference.
	return _gamePath;
}

bool TGameControl::IsClearingAnimations() const {
	// Confirmed (asm line 462551): plain field access, set true by the
	// destructor before it tears anything down (see ~TGameControl below).
	return _isClearingAnimations;
}

bool TGameControl::SavegameExists(int slot) {
	// Confirmed (asm lines 462562-462679): slot has 3 special negative
	// values in addition to real (>=0) slot numbers - -1 asks the current
	// scene which savegame is selected, -2 asks which one is at
	// _savegameClickPos, -3 asks whether any savegame exists at all
	// (a static query, no specific slot). Any other negative value is a
	// logic error (x_assert(false) in the original).
	if (slot == -1) {
		TMSavegame *selected = _ownedSceneControl.GetScene()->GetSelectedSavegame(false);
		return selected != nullptr && selected->Exists();
	}
	if (slot == -2) {
		TMSavegame *found = _ownedSceneControl.GetScene()->GetSavegameAt(_savegameClickPos);
		return found != nullptr && found->Exists();
	}
	if (slot == -3)
		return TMSavegame::SavegameExists();

	TMSavegame save(true, slot, 0, 0, _visionaireGame);
	return save.Exists();
}

bool TGameControl::DeleteSavegame(int slot) {
	// Confirmed (asm lines 462687-462773): slot==-1 deletes the scene's
	// currently-selected savegame; any other slot deletes that numbered
	// save directly.
	if (slot == -1) {
		TGScene *scene = _ownedSceneControl.GetScene();
		TMSavegame *selected = scene->GetSelectedSavegame(false);
		if (selected == nullptr || !selected->Delete())
			return false;
		scene->DeleteSelectedSavegame();
		return true;
	}

	TMSavegame save(true, slot, 0, 0, _visionaireGame);
	return save.Delete();
}

void TGameControl::Save() {
	// Confirmed (asm lines 462781-462971): field ids 0x219 (the save name),
	// 0x1D5/0x1D6 (last playable scene + position), and 0x1DC (matches
	// StartDialog/EndDialog's active-dialog link) are all unresolved.
	TVisObjRef game = _visionaire->GetGame();

	TVisObjRef sceneLink = _currentCharacter->GetRef().GetLink(0x1F7);
	wxString saveName = TMSavegame::MakeSaveGameName(sceneLink);
	game.SetValue(0x219, saveName, TSendEventEnum::kSendEvent);

	TVisObjRef lastScene;
	wxPoint lastPos{};
	_ownedSceneControl.GetLastPlayableSceneParams(lastScene, lastPos);
	game.SetLink(0x1D5, lastScene, false);
	game.SetValue(0x1D6, lastPos, TSendEventEnum::kSendEvent);
	game.SetLink(0x1DC, _dialog.GetTarget(), false);

	if (_currentText != nullptr)
		_currentText->Save();
	for (TGText *text : _activeTexts)
		text->Save();
	for (TGText *text : _sceneTexts)
		text->Save();

	TGAction::SaveActions();
	TGAnimation::SaveAnimations();

	for (TGCharacter *character : _characters)
		character->Save();

	SaveEventHandlers();
	SaveGlobalScriptVariables(*_visionaireGame);
}

void TGameControl::SaveGame(int slot) {
	// Confirmed (asm lines 462981-463264). slot==-1 saves over the scene's
	// currently-selected savegame (bails out if there isn't one); any other
	// slot creates and owns a new numbered TMSavegame, deleted again at the
	// end of this function. Field id 0x1D5 matches Save()'s own "last
	// playable scene" field, overwritten here with the definitive scene
	// reference right before persisting.
	TMSavegame *savegame;
	if (slot != -1) {
		savegame = new TMSavegame(true, slot, 0, 0, _visionaireGame);
	} else {
		savegame = _ownedSceneControl.GetScene()->GetSelectedSavegame(true);
		if (savegame == nullptr)
			return;
	}

	TTimer timer1;
	TTimer timer2;
	timer1.SetTime();
	timer2.SetTime();
	Save();

	// fileName is populated but never read again afterward - kept for
	// fidelity even though its purpose here is unclear (possibly vestigial
	// debug/profiling instrumentation, like the two unused timers above).
	wxFileName fileName;
	int nr = savegame->GetSavegameNr();
	fileName.SetFullName(wxString(L"vtp_saveddata" + std::to_wstring(nr) + L".xml"));

	TXMLStringWriter xmlWriter;
	_visionaire->SaveSaveGame(xmlWriter);
	savegame->SaveGame(xmlWriter);
	TTempFile::DeleteTempFiles();
	_visionaire->ResetActiveData(eVisionaireTable::kValue34);

	if (slot != -1)
		delete savegame;

	TVisObjRef gameRef = _visionaire->GetGame();
	gameRef.SetLink(0x1D5, _ownedSceneControl.GetScene()->GetRef(), false);
}

bool TGameControl::UnregisterEventHandlerMainLoop(const wxString &name) {
	for (size_t i = 0; i < _engineEventHandlerNamesMainLoop.size(); ++i) {
		wxString converted;
		toUTF(&converted, _engineEventHandlerNamesMainLoop[i].c_str());
		if (converted.ToStdWstring() == name.ToStdWstring()) {
			_engineEventHandlerNamesMainLoop.erase(_engineEventHandlerNamesMainLoop.begin() +
			                                       static_cast<long>(i));
			return true;
		}
	}
	return false;
}

void TGameControl::UpdateRandomTimers() {
	// Confirmed (asm lines 463363-463466): iterates a COPY of the scene's
	// character list (matching a std::vector<TGCharacter*> destructor call
	// in the exception-cleanup path), not the live one - presumably so a
	// timer callback can safely add/remove characters mid-iteration.
	std::vector<TGCharacter *> characters = _ownedSceneControl.GetScene()->GetCharacters();
	for (TGCharacter *character : characters)
		character->CheckRandomTimer();
}

void TGameControl::UpdateWalkingSounds() {
}

bool TGameControl::PreLoad(wxString &filePath, wxString &warning, bool isEditor) {
	// Confirmed (asm lines 463692-464965) - see the header declaration's own
	// comment for the parameter-naming notes. Resolves filePath to an
	// absolute path under the resources directory if it wasn't one already,
	// sniffs the file to decide whether it's a password-protected container
	// (extension "vis"/"exe"/"ved", or a "VIS3" magic header) or a plain
	// data file, then loads it either via TComposedFileManager or directly
	// via TVisionaire::Load. The "extension + entry index" temp filename
	// built for the container case approximates a custom (non-printf)
	// wxString::privFormat pattern (data at address 0xD6CC20) that wasn't
	// fully decoded - functionally equivalent, not byte-for-byte the same
	// string. The rare sub-case where field 0x323's string list comes back
	// empty (a 5-pair TComposedFileManager::Init overload, asm lines
	// 464411-464478) is implemented from its confirmed field ids alone,
	// without independently verifying how those 5 (path, int) pairs are
	// actually used downstream.
	_gamePath.Assign(filePath);
	if (!_gamePath.IsAbsolute()) {
		TStandardPaths paths;
		wxString resourcesDir = paths.GetResourcesDir(false);
		_gamePath = wxFileName((resourcesDir + wxString(L"/")).ToStdWstring(), filePath.ToStdWstring());
	}

	if (!_gamePath.IsOk())
		return false;

	TFile file;
	file.OpenRead(_gamePath);
	char magic[5] = "EOF_";
	file.ReadByte(magic[0]);
	file.ReadByte(magic[1]);
	file.ReadByte(magic[2]);
	file.ReadByte(magic[3]);
	magic[4] = 0;
	file.Close();

	warning = wxString();

	wxString ext = _gamePath.GetExt();
	bool isContainer = ext.CmpNoCase(wxString(L"vis")) == 0 || ext.CmpNoCase(wxString(L"exe")) == 0 ||
	                   ext.CmpNoCase(wxString(L"ved")) == 0 ||
	                   (magic[0] == 'V' && magic[1] == 'I' && magic[2] == 'S' && magic[3] == '3');

	if (isContainer) {
		warning = passw;
		if (!TComposedFileManager::InitMainContainer(_gamePath, warning))
			return false;

		TComposedFile *container = TComposedFileManager::GetMainContainer();
		int numEntries = container->GetNumberOfEntries();
		int entryIndex = (numEntries == 0) ? 0 : (numEntries - 1);

		wxFileName altFile(filePath.ToStdWstring());
		wxString newExt = altFile.GetExt() + wxString(std::to_wstring(entryIndex));
		altFile.SetExt(newExt);
		filePath = altFile.GetFullPath();
	} else {
		filePath = _gamePath.GetFullPath();
	}

	if (!_gamePath.FileExists())
		return false;

	_gamePath.SetCwd();
	SLoadingScreen loadingScreen;
	TVisObjRef currentRef;
	wxFileName loadFile(filePath.ToStdWstring());
	int unusedFlag = 0;
	bool loadOk = _visionaire->Load(loadFile, warning, eSaveGame::kValue0, TLoadingTypeEnum::kValue0, &unusedFlag,
	                                nullptr, nullptr);
	if (!loadOk)
		return false;

	currentRef = _visionaire->GetGame();
	if (currentRef.IsEmpty()) {
		if (isEditor)
			SetLoadingScreen(loadingScreen);
	} else {
		TVList list15;
		_visionaire->GetList(0x15, list15, false);
		TVisObjRef pickedRef = list15.empty() ? currentRef : TVisObjRef(list15.front());
		if (isEditor)
			FillLoadingScreen(loadingScreen, pickedRef);
	}

	std::vector<TCharHolder> strings;
	currentRef.GetStrings(0x323, strings);
	if (!strings.empty()) {
		TComposedFileManager::Init(_gamePath, passw, strings);
	} else {
		TComposedFileManager::Init(_gamePath, passw, currentRef.GetInt(0x26D), wxFileName(currentRef.GetPath(0x26E)),
		                           currentRef.GetInt(0x29E), wxFileName(currentRef.GetPath(0x29F)),
		                           currentRef.GetInt(0x26B), wxFileName(currentRef.GetPath(0x26C)),
		                           currentRef.GetInt(0x269), wxFileName(currentRef.GetPath(0x26A)),
		                           currentRef.GetInt(0x2AA), wxFileName(currentRef.GetPath(0x2AB)));
	}

	return true;
}

void TGameControl::AdjustInterfacesOnScreen(bool force, TPaintControl *scene) {
	// Confirmed (asm lines 464975-465524). Refreshes the current character's
	// own interfaces' item lists (field 0x297) when the character has
	// changed since the last call, then - unless a specific `scene` was
	// requested and isn't currently active - repositions every active
	// interface according to its own field 0x13A (position mode, see
	// TInterfacePositionEnum) and field 0x144 (a margin/reserved-space
	// amount for the docking modes), before giving whatever screen space is
	// left over to the current scene. Field id 0x1DF (an "always centered"/
	// manual-positioning-override toggle checked once per call, not per
	// interface) is unresolved beyond its call shape.
	if (_lastInterfaceCharacter != _currentCharacter) {
		_lastInterfaceCharacter = _currentCharacter;
		TVList items;
		_currentCharacter->GetRef().GetLinks(0x297, TypeOrder::kValue0, items);
		for (TGInterface *interface : _currentCharacter->GetInterfaces())
			interface->UpdateItems(items);
	}

	if (scene != nullptr) {
		bool found = false;
		for (TGInterface *interface : _activeInterfaces) {
			if (interface == scene) {
				found = true;
				break;
			}
		}
		if (!found)
			return;
	}

	int windowWidth = 0;
	int windowHeight = 0;
	GetWindowSize(&windowWidth, &windowHeight);
	TGScene *ownedScene = _ownedSceneControl.GetScene();
	bool isMenu = ownedScene->IsMenu();

	bool overrideAll = false;
	if (!isMenu) {
		TVisObjRef game = _visionaire->GetGame();
		overrideAll = game.GetBool(0x1DF);
	}

	int remainingWidth = windowWidth;
	int remainingHeight = windowHeight;
	int accumX = 0;
	int accumY = 0;

	if (!isMenu && !_activeInterfaces.empty()) {
		for (TGInterface *interface : _activeInterfaces) {
			if (interface->GetRef().IsEmpty() || !interface->IsActive() || overrideAll)
				continue;

			int margin = interface->GetRef().GetInt(0x144);
			auto posMode = static_cast<TInterfacePositionEnum>(interface->GetRef().GetInt(0x13A));

			int x = 0;
			int y = 0;
			switch (posMode) {
			case TInterfacePositionEnum::kDockTopStacked:
				if (margin > 0) {
					y = accumY;
					remainingHeight -= margin;
					accumY += margin;
				} else {
					int h = interface->GetWorktopHeight();
					y = accumY;
					remainingHeight -= h;
					accumY += h;
				}
				break;
			case TInterfacePositionEnum::kDockBottomStacked:
				if (margin > 0) {
					y = (accumY + remainingHeight) - interface->GetWorktopHeight();
					remainingHeight -= margin;
				} else {
					int h = interface->GetWorktopHeight();
					remainingHeight -= h;
					y = remainingHeight + accumY;
				}
				break;
			case TInterfacePositionEnum::kDockTopRow:
				if (margin > 0) {
					x = accumX;
					remainingWidth -= margin;
					accumX += margin;
				} else {
					int w = interface->GetWorktopWidth();
					x = accumX;
					remainingWidth -= w;
					accumX += w;
				}
				break;
			case TInterfacePositionEnum::kFixedReserveWidth: {
				remainingWidth -= (margin > 0) ? margin : interface->GetWorktopWidth();
				const wxPoint *pt = interface->GetRef().GetPoint(0x12E);
				x = pt->x;
				y = pt->y;
				break;
			}
			case TInterfacePositionEnum::kFixed: {
				const wxPoint *pt = interface->GetRef().GetPoint(0x12E);
				x = pt->x;
				y = pt->y;
				break;
			}
			case TInterfacePositionEnum::kDraggableClamped: {
				const wxPoint *pt = interface->GetRef().GetPoint(0x12E);
				if (force && interface == scene) {
					const wxPoint &mousePos = GetMousePos();
					x = mousePos.x - pt->x;
					y = mousePos.y - pt->y;
				} else {
					const wxPoint &origin = interface->GetOrigin();
					x = origin.x;
					y = origin.y;
				}
				if (x < 0)
					x = 0;
				else if (windowWidth < interface->GetWorktopWidth() + x)
					x = windowWidth - interface->GetWorktopWidth();
				if (y < 0)
					y = 0;
				else if (windowHeight < interface->GetWorktopHeight() + y)
					y = windowHeight - interface->GetWorktopHeight();
				break;
			}
			}

			wxPoint pos{x, y};
			interface->GetRef().SetValue(0x2B0, pos, TSendEventEnum::kSendEvent);
			interface->SetOrigin(x, y);

			int worktopHeight = interface->GetWorktopHeight();
			int worktopWidth = interface->GetWorktopWidth();
			interface->SetWorktopSize(worktopWidth, worktopHeight);

			int visibleWidth = (windowWidth < worktopWidth + x) ? (windowWidth - x) : worktopWidth;
			int visibleHeight = (windowHeight < worktopHeight + y) ? (windowHeight - y) : worktopHeight;
			interface->SetVisibleSize(visibleWidth, visibleHeight);
		}
	}

	ownedScene->SetOrigin(accumX, accumY);
	ownedScene->SetVisibleSize(remainingWidth, remainingHeight);

	const FloatPoint &scrollPos = ownedScene->GetFloatScrollPos();
	ownedScene->AdjustWindowHorizontal(scrollPos.x);
	ownedScene->AdjustWindowVertical(scrollPos.y);

	TVisObjRef game = _visionaire->GetGame();
	game.SetValue(0x1D6, wxPoint{static_cast<int>(scrollPos.x), static_cast<int>(scrollPos.y)},
	              TSendEventEnum::kSendEvent);
}

void TGameControl::SetInterfaces() {
	// Confirmed (asm lines 465533-465671): rebuilds _activeInterfaces from
	// the current character's own interface list, calling
	// RemoveSpritesAndAnimations() on any interface that's leaving the
	// active set (not present in the new list) before replacing the list
	// wholesale.
	std::list<TGInterface *> newInterfaces = _currentCharacter->GetInterfaces();

	for (TGInterface *active : _activeInterfaces) {
		bool stillActive = false;
		for (TGInterface *candidate : newInterfaces) {
			if (candidate->GetRef() == active->GetRef()) {
				stillActive = true;
				break;
			}
		}
		if (!stillActive)
			active->RemoveSpritesAndAnimations();
	}

	_activeInterfaces.clear();
	for (TGInterface *interface : newInterfaces) {
		interface->SetObjectsActive(false);
		_activeInterfaces.push_back(interface);
	}
}

void TGameControl::SetCharacterActiveCommand() {
	// Confirmed (asm lines 465679-465841): field ids 0x25F, 0x205, and 0x262
	// are all unresolved. Stops at the first interface with a non-empty
	// 0x25F link, whether or not it matches the character's own 0x205 link.
	if (_currentCharacter == nullptr)
		return;

	for (TGInterface *interface : _currentCharacter->GetInterfaces()) {
		TVisObjRef link = interface->GetRef().GetLink(0x25F);
		if (link.IsEmpty())
			continue;

		TVisObjRef commandLink = _currentCharacter->GetRef().GetLink(0x205);
		if (commandLink == link) {
			TVisObjRef game = _visionaire->GetGame();
			game.SetLink(0x262, link, true);
		} else {
			_currentCharacter->GetRef().SetLink(0x205, link, true);
		}
		return;
	}
}

void TGameControl::ChangeCharacter(const TVisObjRef &character, bool immediate, const TVisObjRef &scene) {
	// Confirmed (asm lines 465849-466072). Field ids 0x1D4/0x263 and the
	// meaning of _previousCharacter are unresolved.
	if (!(_currentCharacter->GetRef() == character)) {
		TGCharacter *newChar = GetCharacter(character);
		for (TGCharacter *candidate : _characters) {
			if (candidate->GetRef() == newChar->GetRef()) {
				_currentCharacter->SetRandomTime();
				_currentCharacter = candidate;
				_previousCharacter = candidate;

				TVisObjRef game = _visionaire->GetGame();
				game.SetLink(0x1D4, candidate->GetRef(), false);
				TVisObjRef game2 = _visionaire->GetGame();
				game2.SetLink(0x263, candidate->GetRef(), false);

				_currentCharacter->SetRandomTime();
				ResetState();
				SetInterfaces();
				SetCharacterActiveCommand();
				break;
			}
		}
	}

	TVisObjRef targetScene = scene.IsEmpty() ? _currentCharacter->GetRef().GetLink(0x1F7) : scene;

	TGScene *currentScene = _ownedSceneControl.GetScene();
	if (targetScene == currentScene->GetRef()) {
		AdjustInterfacesOnScreen(false, nullptr);
		ScrollToCharacterIfNeeded(character);
	} else {
		_ownedSceneControl.ShowScene(targetScene, immediate, false);
	}

	if (_lastMousePos.x != -1 || _lastMousePos.y != -1)
		HandleMouseMove(_lastMousePos, false);
}

std::list<TGInterface *> TGameControl::GetActiveInterfaces() const {
	// Confirmed (asm lines 466080-466133): a plain copy.
	return _activeInterfaces;
}

std::list<TGInterface *> TGameControl::GetAllInterfaces() const {
	// Confirmed (asm lines 466141-466193): a plain copy.
	return _allInterfaces;
}

bool TGameControl::InitCharacters() {
	// Confirmed (asm lines 466201-466735). Field id 0x137 (a character's
	// scene link), 0xDE (walk speed, default 0x10E when unset), 0x153 (a
	// start position), 0x12F (the game's starting-character link), 0x1D4/
	// 0x263/0x205/0x262 (matching ChangeCharacter/SetCharacterActiveCommand's
	// field ids) are all unresolved.
	TVList characterList;
	_visionaire->GetList(0, characterList, false);
	if (characterList.empty()) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"There must be at least one character for a valid game.");
		return false;
	}

	for (TVisionaireObject *object : characterList) {
		TVisObjRef ref(object);
		TVisObjRef parent = ref.GetLink(0x137).GetParent();
		THCharacter *character = new THCharacter(ref, parent);

		wxPoint pos = *ref.GetPoint(0x153);
		int walkSpeed = ref.GetInt(0xDE);
		if (walkSpeed == -1)
			walkSpeed = 0x10E;

		character->Init();
		character->AssignToScene(parent, pos, walkSpeed);

		_characters.push_back(character);
		_charactersByHash[PackVisId(character->GetRef().GetId())] = character;
	}

	TVisObjRef startingLink = _visionaire->GetGame().GetLink(0x12F);
	TGCharacter *starting = nullptr;
	for (TGCharacter *candidate : _characters) {
		if (candidate->GetRef() == startingLink) {
			starting = candidate;
			break;
		}
	}

	if (starting != nullptr) {
		_currentCharacter = starting;
		_previousCharacter = starting;

		_visionaire->GetGame().SetLink(0x1D4, starting->GetRef(), false);
		_visionaire->GetGame().SetLink(0x263, starting->GetRef(), false);
		_visionaire->GetGame().SetLink(0x262, starting->GetRef().GetLink(0x205), false);
	}

	if (_currentCharacter == nullptr) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"An active character must be defined for a valid game.");
		return false;
	}

	_startingCharacter = _currentCharacter;
	UpdateCurrentObject();
	return true;
}

void TGameControl::InitGameActions() {
	// Confirmed (asm lines 466743-467222). Builds _gameActions from the
	// game's list of action-definition objects (field id 0x13E). Field ids
	// 0x9F (an action's bound key code), 0xA2 (a per-action list of
	// "condition" objects), and that list's own 0xB3 (a condition's type)/
	// 0xF2 fields are unresolved beyond the type-id constants 0x6B/0x99 seen
	// compared against 0xB3 below - the real Visionaire schema they come from
	// hasn't been reversed.
	_gameActions.clear();

	TVList links;
	_visionaire->GetGame().GetLinks(0x13E, TypeOrder::kValue0, links);

	// A fixed table of "special" key codes - everything an action can be
	// bound to besides a plain digit, letter, or controller-button pseudo-
	// code (ConvertControllerButtonToSymKey's 1000001-1000015 range).
	// Confirmed byte-for-byte from the binary's own data (Deponia_Linux.asm,
	// address 0xD6D220): a mix of plain ASCII and real SDL2 keycodes,
	// matching this being a wx/SDL-flavored unified "keysym" space.
	static const int kSpecialActionKeyCodes[] = {
		' ', '\r', WXK_ESCAPE, WXK_BACK, ',', '.', '+', '-',
		SDLK_F1, SDLK_F2, SDLK_F3, SDLK_F4, SDLK_F5, SDLK_F6,
		SDLK_F7, SDLK_F8, SDLK_F9, SDLK_F10, SDLK_F11, SDLK_F12,
		SDLK_LEFT, SDLK_RIGHT, SDLK_UP, SDLK_DOWN,
	};
	std::vector<int> validKeyCodes(std::begin(kSpecialActionKeyCodes), std::end(kSpecialActionKeyCodes));
	for (int c = '0'; c <= '9'; c++)
		validKeyCodes.push_back(c);
	for (int c = 'a'; c <= 'z'; c++)
		validKeyCodes.push_back(c);
	for (int c = 1000001; c <= 1000015; c++)
		validKeyCodes.push_back(c);

	for (TVisionaireObject *object : links) {
		TVisObjRef actionRef(object);
		int keyCode = actionRef.GetInt(0x9F);

		// Confirmed arithmetic, not confirmed meaning: classifies keyCode
		// against TKeyboardMessageEnum's raw values (masterControl.h) -
		// kControllerButtonHit(4) if it's a controller-button pseudo-code,
		// else kKeyUp(1); or, if (keyCode - 10000) names a valid key/button
		// instead, kControllerButtonRelease(5)/2 respectively. This is where
		// TKeyboardMessageEnum's otherwise-unobserved values 2 and 3 could
		// come from, though 3 is never actually produced here.
		bool isControllerCode = keyCode >= 1000001 && keyCode <= 1000015;
		int msg = isControllerCode ? static_cast<int>(TKeyboardMessageEnum::kControllerButtonHit)
		          : static_cast<int>(TKeyboardMessageEnum::kKeyUp);
		for (int validCode : validKeyCodes) {
			if (keyCode != validCode + 10000)
				continue;
			bool shiftedControllerCode = validCode >= 1000001 && validCode <= 1000015;
			msg = shiftedControllerCode ? static_cast<int>(TKeyboardMessageEnum::kControllerButtonRelease) : 2;
			break;
		}

		// Confirmed loop/field-access shape, not confirmed meaning: scans the
		// action's condition list (0xA2) for a type-0x6B entry, or a type-
		// 0x99 entry whose own 0xF2 field is 1 - either makes this action
		// fire unconditionally (see StartGameAction's `if (action.flag)`).
		bool flag = false;
		TVList conditions;
		actionRef.GetList(0xA2, conditions);
		for (TVisionaireObject *condition : conditions) {
			int type = condition->GetInt(0xB3);
			if (type == 0x6B) {
				flag = true;
				break;
			}
			if (type == 0x99 && condition->GetInt(0xF2) == 1) {
				flag = true;
				break;
			}
		}

		SGameAction action;
		action.a = keyCode;
		action.flag = flag;
		action.msg = msg;
		action.target = actionRef;
		_gameActions.push_back(action);
	}
}

bool TGameControl::Init() {
	// Confirmed (asm lines 467226-467624). Field ids 0x79 (starting scene
	// link) and 0x231/0x1D8/0x1D6/0x1D9/0x1DA (matching CenterScene/
	// SetOnScrollDestination's fields) are all unresolved.
	TVisObjRef startScene = _visionaire->GetGame().GetLink(0x79);
	_ownedSceneControl.Set(startScene);

	_visionaire->GetGame().SetLink(0x1D5, startScene, false);
	_visionaire->GetGame().SetValue(0x231, true, TSendEventEnum::kSendEvent);
	_visionaire->GetGame().SetValue(0x1D8, false, TSendEventEnum::kSendEvent);
	_visionaire->GetGame().SetValue(0x1D6, wxPoint{}, TSendEventEnum::kSendEvent);
	_visionaire->GetGame().SetValue(0x1D9, 0, TSendEventEnum::kSendEvent);
	_visionaire->GetGame().SetValue(0x1DA, 0, TSendEventEnum::kSendEvent);

	_sceneControl = &_ownedSceneControl;
	_currentCharacter = nullptr;
	_savegameClickPos = wxPoint{-1, -1};
	_lastMousePos = wxPoint{-1, -1};

	TGScene::InitActionAreas();

	TTimer timer;
	timer.SetTime();

	if (!InitCharacters())
		return false;

	InitInterfaces();
	for (TGCharacter *character : _characters)
		character->SetInterfaces();

	SetInterfaces();
	SetCharacterActiveCommand();

	if (wxLog::loglevel > 1)
		wxLog::logexpanded(L"Interfaces loaded. Needed time: %ld ms", timer.GetTime());

	timer.SetTime();
	TVList fontList;
	_visionaire->GetList(3, fontList, true);
	GetFontManager()->Initialize(fontList);

	InitGameActions();
	InitScripts();

	if (wxLog::loglevel > 1)
		wxLog::logexpanded(L"Scripts loaded. Needed time: %ld ms", timer.GetTime());

	timer.SetTime();
	_console.Init();

	return true;
}

bool TGameControl::LoadAndInitGame(wxString &filePath, const wxString &extra, wxString gameName, bool isEditor) {
	// Confirmed (asm lines 467635-468496) - see the header declaration's own
	// comment for the parameter-naming corrections this pass made. Field ids
	// 0x224/0x225 (graphics filter modes), 0x2E7/0x29D (texture cache
	// sizes), 0xE5 (a button's linked cursor object), and 0xF6 (feeds
	// _timingValueSeconds) are unresolved. `TSignalSlot`, the not-yet-
	// integrated `TVisionaire::LoadDataGame`, and the fact that `this` gets
	// passed as a TSignalSlot* when isEditor is true are all flagged in
	// their own declarations' comments rather than guessed at further here.
	TVisObjRef game = _visionaire->GetGame();
	graphics->SetFilters(static_cast<TInterpolationEnum>(game.GetInt(0x224)),
	                     static_cast<TInterpolationEnum>(game.GetInt(0x225)));
	graphics->PreallocateTextures(game.GetInt(0x2E7));

	const wxPoint *aspectPoint = game.GetPoint(0x7E);
	if (g_unlockAspect) {
		_aspectWidth = renderSize.width;
		_aspectHeight = renderSize.height;
	} else {
		_aspectWidth = aspectPoint->x;
		_aspectHeight = aspectPoint->y;
	}
	InitControl(_aspectWidth, _aspectHeight);
	_loadingControl->InitControl(_aspectWidth, _aspectHeight);
	if (isEditor)
		ShowLoadingScreen();

	TDiagnostic::BeginFixedRegion(wxString(L"Struktur"));
	wxFileName resolvedFile(filePath.ToStdWstring());
	resolvedFile.NormalizePath();
	TSignalSlot *slot = isEditor ? reinterpret_cast<TSignalSlot *>(this) : nullptr;
	bool loaded = _visionaire->LoadDataGame(resolvedFile, extra, TLoadingTypeEnum::kValue1, true, slot, nullptr);

	if (!loaded) {
		if (wxLog::loglevel >= 0) {
			wxString fmt;
			toUTF(&fmt, "Error loading game data from file '%s'");
			wxString gameDirPath = _gamePath.GetFullPath();
			wxLog::logexpanded(fmt.wc_str(), gameDirPath.wc_str());
		}
		return false;
	}

	TDiagnostic::EndFixedRegion();
	graphics->SetCacheSize(game.GetInt(0x29D));

	if (gameName.ToStdWstring().empty()) {
		TVisObjRef link = game.GetLink(0x132);
		gameName = wxString(link.GetName());
	}

	TVList languageList;
	_visionaire->GetList(0x12, languageList, false);
	if (!languageList.empty()) {
		TVisionaireObject *match = nullptr;
		for (TVisionaireObject *obj : languageList) {
			if (obj->GetName() == gameName) {
				match = obj;
				break;
			}
		}
		TVisObjRef languageRef(match != nullptr ? match : languageList.front());
		TTText::SetLanguage(languageRef);
	}

	TVList cursorDefs;
	_visionaire->GetList(0xF, cursorDefs, false);
	for (TVisionaireObject *obj : cursorDefs) {
		TVisObjRef ref(obj);
		if (!ref.IsEmpty())
			GetCursorControl()->LoadCursor(ref);
	}

	TVList buttonList;
	_visionaire->GetList(2, buttonList, false);
	for (TVisionaireObject *obj : buttonList) {
		TVisObjRef linkedRef(obj->GetLink(0xE5));
		if (!linkedRef.IsEmpty())
			GetCursorControl()->LinkButtonCursor(PackVisId(linkedRef.GetId()), PackVisId(obj->GetId()));
	}

	TVisObjRef game2 = _visionaire->GetGame();
	_timingValueSeconds = static_cast<float>(game2.GetInt(0xF6)) / 1000.0f;

	if (!Init()) {
		if (wxLog::loglevel >= 0) {
			wxString fmt;
			toUTF(&fmt, "Failed to initialize game.");
			wxLog::logexpanded(fmt.wc_str());
		}
		return false;
	}

	_sceneControl = &_ownedSceneControl;
	if (isEditor)
		_loadingControl->EndLoading(_soundManager);
	return true;
}

bool TGameControl::ReplaceGame(wxFileName file, bool isEditor) {
	// Confirmed (asm lines 468504-468965). The "prepend the game's own
	// directory, then normalize" string-concatenation step here is a
	// COW-string capacity-check optimization in the disassembly (choosing
	// between append-in-place and insert-at-front based on which operand
	// has spare capacity) - not meaningful control flow, so reproduced as a
	// plain concatenation (same reasoning as StartTween's erase()+
	// push_back(), batch 18). Field id 0x132 (linked from the game, whose
	// GetName() feeds LoadAndInitGame's third argument) and 0x170 (an action
	// run only in editor mode) are unresolved. Calls LoadAndInitGame/
	// RegisterEventHandler/GetCursorControl via the g_pGameControl global
	// rather than `this` - distinct from the `this->_visionaire` used for
	// the editor-mode branch below - reproduced as observed rather than
	// simplified to `this->`. The member read via wxFileName::GetPath() at
	// asm line 468573 is _gamePath (confirmed independently by PreLoad,
	// batch 27 - see that member's own comment). Calls PreLoad (asm line
	// 468791, added in batch 27 - this call was missed in the original pass
	// here since PreLoad was still a stub at the time) before
	// LoadAndInitGame, threading its by-ref outputs through unchanged.
	TStandardPaths standardPaths;

	std::wstring prefix = _gamePath.GetPath() + L"/";
	wxFileName resolved(prefix + file.GetFullPath().ToStdWstring());
	resolved.NormalizePath();
	file = resolved;

	if (!file.Exists()) {
		if (wxLog::loglevel >= 0) {
			wxString fmt;
			toUTF(&fmt, "game file does not exists %s");
			wxString fullPath = file.GetFullPath();
			wxLog::logexpanded(fmt.wc_str(), fullPath.wc_str());
		}
		return false;
	}

	wxString fullPath = file.GetFullPath();
	TVisObjRef game = _visionaire->GetGame();
	TVisObjRef link = game.GetLink(0x132);
	wxString warning = wxString(link.GetName());
	wxString emptyFile;

	auto *gameControl = static_cast<TGameControl *>(g_pGameControl);
	gameControl->GetCursorControl()->Clear();
	TId id(-1, -1);
	UnrefLuaFieldsCache(id, -1);

	if (!gameControl->PreLoad(fullPath, emptyFile, false))
		return false;

	if (!gameControl->LoadAndInitGame(fullPath, emptyFile, warning, false))
		return false;

	gameControl->RegisterEventHandler();
	if (!isEditor)
		return true;

	gameControl->InitAfterLoadingScreen();
	TVisObjRef game2 = _visionaire->GetGame();
	TVisObjRef target = game2.GetLink(0x170);
	TGAction::AddRunningAction(target);
	return true;
}

void TGameControl::HandleEngineEvent(const std::string &name, const std::string &arg) {
	// Confirmed (asm lines 469160-469504): fires once per registered engine-
	// event handler (TMasterControl::_engineEventHandlerNames, populated by
	// RegisterEngineEventHandler), but only that vector's *size* is read -
	// never any element's own name - and every firing dispatches to the same
	// fixed Lua function name, "EngineEventHandler", not to each handler's
	// own registered name. That registered name is apparently just a de-dup
	// key (matching RegisterEngineEventHandler's own dedup-by-name check),
	// not a per-handler Lua entry point.
	for (size_t i = 0; i < _engineEventHandlerNames.size(); i++) {
		wxString convertedName;
		toUTF(&convertedName, name.c_str());
		TArgument nameArg;
		nameArg.Set(convertedName);

		wxString convertedArg;
		toUTF(&convertedArg, arg.c_str());
		TArgument argArg;
		argArg.Set(convertedArg);

		// Real dispatch calls LuaExecuteFunction("EngineEventHandler",
		// {&nameArg, &argArg}, results) - LuaExecuteFunction/TArgument's full
		// contract isn't reversed yet (same gap noted in
		// TMasterControl::ProcessMessage).
	}
}

void TGameControl::HandleKeyEvent(TKeyboardMessageEnum msg, const wxString &key, int a, unsigned short b) {
	// Confirmed (asm lines 470963-471714). Field ids 0x279 ("an override
	// action link" - if non-empty, dispatches straight to StartGameAction
	// subject to a 0x1E0 override-block flag, skipping the ESC/cutscene and
	// _gameActions checks below entirely) and 0x235 (a "game actions
	// enabled" toggle, same role as ScrollToCharacterIfNeeded's 0x231) are
	// unresolved. The a==27(ESC)/msg==2 cutscene-skip is a nice
	// cross-confirmation of batch 17's guess that InitGameActions'
	// "shifted key code" category 2 is a real, used TKeyboardMessageEnum
	// value, not just an artifact.
	if (_ownedSceneControl.FadingToNewScene())
		return;
	if (_console.HandleKeyEvent(msg, key, a, b))
		return;

	// Real dispatch calls LuaExecuteFunction("KeyEventHandler", {&msgArg,
	// &nameArg, &aArg, &bArg}, results) once per registered keyboard
	// handler, returning immediately if a handler's result reports it
	// consumed the event - LuaExecuteFunction/TArgument's full contract
	// isn't reversed yet (same gap noted in TMasterControl::ProcessMessage/
	// TGameControl::HandleEngineEvent). `nameArg` comes from
	// SDL_GetKeyName(a) instead of `key` for axis-move/msg==3 messages (the
	// latter another of TKeyboardMessageEnum's unconfirmed values).
	bool useSdlKeyName = msg == TKeyboardMessageEnum::kAxisMove || static_cast<int>(msg) == 3;
	for (std::size_t i = 0; i < _keyboardEventHandlers.size(); i++) {
		TArgument msgArg;
		msgArg.Set(static_cast<int>(msg));

		TArgument nameArg;
		if (useSdlKeyName) {
			wxString converted;
			toUTF(&converted, SDL_GetKeyName(a));
			nameArg.Set(converted);
		} else {
			nameArg.Set(key);
		}

		TArgument aArg;
		aArg.Set(a);
		TArgument bArg;
		bArg.Set(static_cast<int>(b));
	}

	TVisObjRef game = _visionaire->GetGame();
	if (!game.GetLink(0x279).IsEmpty()) {
		TVisObjRef game2 = _visionaire->GetGame();
		if (!game2.GetBool(0x1E0))
			StartGameAction(msg, key, a, b);
		return;
	}

	if (a == 0x1B && static_cast<int>(msg) == 2) {
		TGAction::SkipCutscene();
		return;
	}

	TVisObjRef game3 = _visionaire->GetGame();
	if (!game3.GetBool(0x235))
		return;

	for (const SGameAction &action : _gameActions) {
		if (action.a != a || action.msg != static_cast<int>(msg))
			continue;
		if (!action.flag)
			return;
		if (_currentText != nullptr && _currentText->GetTarget().GetBool(0x211))
			return;
		TGAction::AddRunningAction(action.target);
		return;
	}
}

void TGameControl::HandleControllerAxis(SDL_GameControllerAxis axis, int value, int index) {
	// Confirmed (asm lines 471722-471817): the two branches (value>0 vs
	// <=0) use different-looking constant-division codegen, but both
	// compute the same value*100/32768 (checked via the magic-multiplier
	// math for the >0 branch) - almost certainly rescaling SDL's +-32768
	// raw axis range to a +-100 one. Skipped entirely for an unrecognized
	// axis (empty name).
	wxString axisName = ConvertControllerAxisToUnicode(axis);
	if (!axisName.IsEmpty()) {
		int scaled = value * 100 / 32768;
		HandleKeyEvent(TKeyboardMessageEnum::kAxisMove, axisName, scaled, static_cast<unsigned short>(index));
	}
}

void TGameControl::HandleControllerButtonRelease(SDL_ControllerButtonEvent button, int index) {
	// Confirmed (asm lines 471825-471896): re-dispatches through
	// HandleKeyEvent with an empty key name, the same symkey lookup as
	// ConvertControllerButtonToSymKey, and msg=ControllerButtonRelease.
	HandleKeyEvent(TKeyboardMessageEnum::kControllerButtonRelease, wxString(),
	               ConvertControllerButtonToSymKey(button), static_cast<unsigned short>(index));
}

void TGameControl::HandleControllerButtonHit(SDL_ControllerButtonEvent button, int index) {
	// Confirmed (asm lines 471904-471975): same as ...Release() above but
	// msg=ControllerButtonHit.
	HandleKeyEvent(TKeyboardMessageEnum::kControllerButtonHit, wxString(),
	               ConvertControllerButtonToSymKey(button), static_cast<unsigned short>(index));
}

void TGameControl::PushEngineEvent(const std::string &name, const std::string &arg) {
	wxCriticalSectionLocker locker(EngineEventLock);
	EngineEvents.push_back({name, arg});
}

void TGameControl::SetDelay(double seconds, const std::string &name) {
	_delaysByName.push_back({seconds, name});
}

void TGameControl::SetDelay(double seconds, int id) {
	_delaysById.push_back({seconds, id});
}

void TGameControl::RegisterEventHandlerMainLoop(const wxString &name) {
	for (const std::string &existing : _engineEventHandlerNamesMainLoop) {
		wxString converted;
		toUTF(&converted, existing.c_str());
		if (converted.ToStdWstring() == name.ToStdWstring())
			return;
	}
	_engineEventHandlerNamesMainLoop.push_back(std::string(static_cast<const char *>(name.mb_str())));
}

void TGameControl::GetWalkingSounds(std::vector<wxFileName> &outSounds) {
	// Confirmed (asm lines 474770-474985): field ids 0x1F7 (scene link,
	// matches CenterScene's usage) and 0x110 (a filesystem path) are both
	// unresolved.
	TGScene *scene = _ownedSceneControl.GetScene();
	for (TGCharacter *character : _characters) {
		if (!(character->GetRef().GetLink(0x1F7) == scene->GetRef()))
			continue;

		wxFileName fileName(character->GetRef().GetPath(0x110));
		if (!fileName.IsOk())
			continue;

		fileName.NormalizePath();
		outSounds.push_back(fileName);
	}
}

void TGameControl::StartTween(const TVisObjTween &tween) {
	// Confirmed (asm lines 474993-475136): removes any existing tween
	// targeting the same object, then appends the new one. The disassembly's
	// hand-rolled shift/placement-new/growth-path branches are just -O2's
	// inlined std::vector<TVisObjTween>::erase()/push_back() - writing it
	// that way directly reproduces the same behavior without transliterating
	// the vector internals by hand.
	for (auto it = _visObjTweens.begin(); it != _visObjTweens.end();) {
		if (it->target == tween.target)
			it = _visObjTweens.erase(it);
		else
			++it;
	}
	_visObjTweens.push_back(tween);
}

void TGameControl::LoadEventHandlers() {
	// Confirmed (asm lines 475143-476661+, TGameControl's largest method):
	// parses a ';'-separated "type:names" specification string (field
	// 0x2F7) - each entry's comma-separated names get registered via the
	// matching confirmed Register* method, by the entry's type prefix:
	// "mainLoop" -> RegisterEventHandlerMainLoop, "mouseEvent" ->
	// RegisterMouseEventHandler, "keyEvent" -> RegisterKeyboardEventHandler,
	// "engineEvent" -> RegisterEngineEventHandler (all four strings
	// recovered byte-for-byte from the binary's own data). An entry whose
	// ':'-split token count isn't exactly 2, or whose type matches none of
	// the four, is skipped. The mouse case's real per-name filter-list
	// syntax (TMouseEventHandler needs one) wasn't fully traced - registered
	// here with an empty filter (matches every mouse message), which is a
	// strict superset of whatever the real filter would have restricted it
	// to, not a made-up specific one. Likewise, a handful of comma-separated
	// names get compared against four further fixed strings
	// ("animationStarted"/"animationStopped"/"textStarted"/"textEnded" -
	// data at addresses 0xD686F0-0xD687B0) for a special case not traced
	// here - every name is registered identically instead.
	TVisObjRef game = _visionaire->GetGame();
	wxString handlersStr = game.GetStr(0x2F7);

	wxStringTokenizer entries(handlersStr, L';');
	while (entries.HasMoreTokens()) {
		wxString entry = entries.GetNextToken();
		wxStringTokenizer parts(entry, L':');
		if (parts.CountTokens() != 2)
			continue;

		wxString type = parts.GetNextToken();
		wxString namesStr = parts.GetNextToken();
		wxStringTokenizer names(namesStr, L',');

		if (type.ToStdWstring() == L"mainLoop") {
			while (names.HasMoreTokens())
				RegisterEventHandlerMainLoop(names.GetNextToken());
		} else if (type.ToStdWstring() == L"mouseEvent") {
			std::vector<int> filter;
			while (names.HasMoreTokens())
				RegisterMouseEventHandler(names.GetNextToken(), filter);
		} else if (type.ToStdWstring() == L"keyEvent") {
			while (names.HasMoreTokens())
				RegisterKeyboardEventHandler(names.GetNextToken());
		} else if (type.ToStdWstring() == L"engineEvent") {
			while (names.HasMoreTokens())
				RegisterEngineEventHandler(names.GetNextToken());
		}
	}
}

bool TGameControl::Load() {
	return false;
}

bool TGameControl::LoadGame(TMSavegame */*savegame*/) {
	// The real load logic (asm lines 477404-478377, ~970 lines) is not
	// reversed - left as a stub.
	return false;
}

bool TGameControl::LoadGame(int slot) {
	// Confirmed (asm lines 478385-478469): slot==-1 loads the scene's
	// currently-selected savegame; any other slot constructs a numbered
	// TMSavegame first. _isClearingAnimations is toggled around the
	// actual load in both cases (same field used elsewhere - see
	// ~TGameControl/IsClearingAnimations).
	if (slot == -1) {
		TMSavegame *selected = _ownedSceneControl.GetScene()->GetSelectedSavegame(false);
		if (selected == nullptr)
			return false;
		_isClearingAnimations = true;
		bool result = LoadGame(selected);
		_isClearingAnimations = false;
		return result;
	}

	TMSavegame save(true, slot, 0, 0, _visionaireGame);
	save.CheckVisPaths();
	_isClearingAnimations = true;
	bool result = LoadGame(&save);
	_isClearingAnimations = false;
	return result;
}

void TGameControl::StartTween(const Tween &/*tween*/, const std::string &/*name*/) {
	// Was a guessed `_pendingTweens.push_back({tween, name})` - checking
	// the real asm (lines 478477-478598+) shows this operates on an
	// 88-byte-element vector at a DIFFERENT offset than StartTween(const
	// TVisObjTween&)'s 176-byte-element one, keyed by a string comparison
	// against `name` with a non-trivial erase/replace on a match - not a
	// plain append. Reverted to a stub rather than keep a confidently wrong
	// implementation; see NOTES.md.
}
