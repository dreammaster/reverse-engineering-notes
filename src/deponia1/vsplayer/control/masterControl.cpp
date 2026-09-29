#include "vsplayer/control/masterControl.h"

#include <cmath>
#include <cstdlib>

#include "Diagnostics.h"
#include "TSceneControl.h"
#include "graphicslib/graphics.h"
#include "vscommon/scripting/argument.h"
#include "vsplayer/control/gameController.h"

// Draw-hook script lists: plain globals in the original (cs:luaDrawBeforeScene
// etc.), populated by a registration mechanism separate from the generic
// Register*EventHandler methods below (never observed being written to in
// what's been reversed so far, so they stay empty here).
static std::vector<std::string> s_luaDrawBeforeScene;
static std::vector<std::string> s_luaDrawAfterScene;
static std::vector<std::string> s_luaDrawAfterInterfaces;

// LuaDoString(std::string const&, std::string const&) - the scripting
// bridge itself isn't reversed; this is a placeholder so Draw()'s hook
// loops compile and are structurally faithful.
static void LuaDoStringStub(const std::string &/*script*/, const std::string &/*chunkName*/) {
}

TMasterControl::TMasterControl() {
	_cursorControl = new TCursorControl();
	_gameController = new TGameController();
	_loadingControl = new TLoadingControl();
	_soundManager = new TSoundFFMPEG();
	_fontManager = new TFontManager();
	_gameClientSDK = new TGameClientSDK();
	// _sceneControl is deliberately left null here: there's no confirmed
	// evidence TMasterControl's own constructor sets it (see its
	// declaration in masterControl.h). TGameControl embeds an actual
	// TSceneControl by value and points this at it - a previous version of
	// this constructor also heap-allocated one here as a filler guess,
	// which caused ~TMasterControl() to `delete` TGameControl's non-heap
	// member and corrupt the heap. Only the owning subclass should manage
	// this pointer's lifetime.
	_visionaire = new TVisionaire();
	_moviesEnabled = true;
}

TMasterControl::~TMasterControl() {
	delete _cursorControl;
	delete _gameController;
	delete _loadingControl;
	delete _soundManager;
	delete _fontManager;
	delete _gameClientSDK;
	delete _visionaire;
}

void TMasterControl::QuitGame() {
	_quitGame = true;
}

bool TMasterControl::GetQuitGame() const {
	return _quitGame;
}

void TMasterControl::SetClearMessage() {
	_clearMessage = true;
}

bool TMasterControl::GetClearMessage() {
	bool result = _clearMessage;
	_clearMessage = false;
	return result;
}

TPaintControl *TMasterControl::GetMainControl() {
	return _sceneControl->GetScene();
}

TCursorControl *TMasterControl::GetCursorControl() {
	return _cursorControl;
}

TGameController *TMasterControl::GetGameController() {
	return _gameController;
}

TSoundFFMPEG *TMasterControl::GetSoundManager() const {
	return _soundManager;
}

TFontManager *TMasterControl::GetFontManager() {
	return _fontManager;
}

TGameClientSDK *TMasterControl::GetGameClientSDK() const {
	return _gameClientSDK;
}

void TMasterControl::SetLoadingScreen(SLoadingScreen &screen) {
	_loadingScreen = screen;
}

void TMasterControl::ShowLoadingScreen() {
	if (!_loadingControl)
		return;
	_loadingScreenCachedWidth = _windowWidth;
	_loadingScreenCachedHeight = _windowHeight;
	// Real TLoadingControl::Init(SLoadingScreen&, TSoundInterface*) not
	// reversed yet; TSoundInterface appears to be an interface TSoundFFMPEG
	// implements, so passing it directly here is a reasonable placeholder.
}

void TMasterControl::CleanUp() {
	// Empty in the original (a single `retn`).
}

void TMasterControl::EnableMovies(bool enable) {
	_moviesEnabled = enable;
}

void TMasterControl::DrawInterfaces() {
	// The original gates this on several TVisObjRef::GetBool() field-id
	// checks (ids 0x274, 0x315, 0x1DF, 0x124) whose meaning isn't resolved
	// (see datastruct/visobjref.h) - reproduced structurally: only draw the
	// registered interfaces when the game data says to.
	TVisObjRef game = _visionaire->GetGame();
	TVisObjRef link = game.GetLink(0x1D5);
	bool shouldDraw = link.IsEmpty() ? link.GetBool(0x124) : !link.GetBool(0x274);
	if (!shouldDraw)
		return;

	for (TGInterface *interface : _activeInterfaces)
		interface->Draw();
}

bool TMasterControl::Draw(bool showActionText) {
	wxPoint savedScrollPos{};
	if (_earthquakeActive) {
		TPaintControl *scene = _sceneControl->GetScene();
		savedScrollPos = scene->GetScrollPos();

		if (_earthquakeTimer.GetTime() > _earthquakeJitterInterval) {
			int range = _earthquakeAmount * 2;
			_earthquakeOffsetX = range ? (std::rand() % range - _earthquakeAmount) : 0;
			_earthquakeOffsetY = range ? (std::rand() % range - _earthquakeAmount) : 0;
			_earthquakeTimer.SetTime();
		}

		wxPoint jittered{savedScrollPos.x - _earthquakeOffsetX, savedScrollPos.y - _earthquakeOffsetY};
		_sceneControl->GetScene()->SetScrollPos(jittered);
	}

	if (!showActionText) {
		for (const std::string &script : s_luaDrawBeforeScene)
			LuaDoStringStub(script, script);

		_sceneControl->Draw();

		for (const std::string &script : s_luaDrawAfterScene)
			LuaDoStringStub(script, script);

		TVisObjRef game = _visionaire->GetGame();
		// Field id 0x313, meaning not resolved: picks one of three render
		// paths below (interfaces-only / scene-without-action-text / normal
		// with action text). The original tracks this in a local it also
		// reuses for the action-text positioning mode further down (its
		// `r13d`) - split into two clearly-named locals here instead.
		int drawMode = game.GetInt(0x313);

		if (drawMode == 1) {
			graphics->SetMatrixMode(false, true);
			graphics->ResetMatrix(false, false);
			DrawInterfaces();
		} else {
			if (drawMode == 2) {
				graphics->SetMatrixMode(true, false);
				graphics->ResetMatrix(false, false);
			}
			TPaintControl *scene = _sceneControl->GetScene();
			if (scene->IsActive() && !DisplayTexts()) {
				SetCurrent();
				if (_cursorControl->IsActive())
					_cursorControl->Draw();
			}
			if (drawMode != 2 && !DisplayDialog()) {
				// Action-text overlay: fetch the currently-hovered object's
				// display text and draw it near the cursor, clamped to the
				// window bounds. The original also supports a second,
				// rect-relative positioning mode (selected by a value this
				// reconstruction doesn't determine) - not reproduced here.
				// Field id 0x24D recovered from the disassembly; its real
				// meaning needs the data-schema system to name properly.
				TVisObjRef link = game.GetLink(0x24D);
				_fontManager->SetCurrentFont(link);
				wxString text = _objectManager.GetActionText();
				wxPoint textSize;
				_fontManager->GetTextDimension(text, textSize);
				wxPoint drawPos = _cursorControl->GetPositionNextToCursor();
				if (drawPos.x + textSize.x > _windowWidth)
					drawPos.x = _windowWidth - textSize.x;
				if (drawPos.y + textSize.y > _windowHeight)
					drawPos.y = _windowHeight - textSize.y;
				_fontManager->PrintText(text, TextAlignmentEnum::kLeft, drawPos, 1.0f);
			}
		}

		for (const std::string &script : s_luaDrawAfterInterfaces)
			LuaDoStringStub(script, script);

		if (drawMode != 0) {
			graphics->SetMatrixMode(false, false);
			graphics->ResetMatrix(false, false);
		}
		DisplayInSceneConsole();
	}

	if (showActionText) {
		graphics->Flip();
	}
	return true;
}

void TMasterControl::ScrollUpdate() {
	// Camera edge-scroll easing: accelerate/decelerate the horizontal scroll
	// toward the mouse cursor when it's near a window edge. Structure
	// (worktop bounds check, exponential ease toward a target speed,
	// AdjustWindowHorizontal) is confirmed from the disassembly; the exact
	// tuning constants (start speed, easing factor) are gameplay-feel
	// values that can't be verified without running the original, so
	// they're approximated here rather than transcribed byte-for-byte.
	if (_quitGame)
		return;

	TVisObjRef game = _visionaire->GetGame();
	_easeDirectionFlag = game.GetBool(0x33A);
	_isScrolling = false;

	TPaintControl *scene = _sceneControl->GetScene();
	if (!scene->IsScrollable())
		return;

	int worktopWidth = scene->GetWorktopWidth();
	FloatPoint floatScroll = scene->GetFloatScrollPos();
	(void)floatScroll;  // used by the real easing formula; not reproduced here (see below)
	int targetX = game.GetInt(0x2B1);   // field id not resolved - a scroll target x position
	int edgeMargin = game.GetInt(0x2B2);  // field id not resolved - an edge-scroll trigger margin

	constexpr float kEaseFactor = 0.1f;
	constexpr float kStartSpeed = 1.0f;

	if (targetX >= _mousePos.x || worktopWidth - targetX <= _mousePos.x) {
		_isScrolling = true;
	}
	bool nearEdge = _mousePos.x < edgeMargin || _mousePos.x > worktopWidth - edgeMargin;

	if (_isScrolling || nearEdge) {
		if (_scrollTimer.GetTime() > 500) {
			_scrollTimer.SetTime();
		}
		float targetSpeed = _easeDirectionFlag ? -kStartSpeed : kStartSpeed;
		_xspeed = targetSpeed + (_xspeed - targetSpeed) * kEaseFactor;
		scene->AdjustWindowHorizontal(_xspeed * 0.001f);
	}
}

int TMasterControl::PlayAVI(const wxFileName &file, bool /*skippable*/, HandleSoundsEnum /*handleSounds*/) {
	// Gathers ~20 configuration fields from the "Game" object (subtitle
	// text/position, letterbox picture overlay, colors) via TVisObjRef,
	// then presumably calls into TMovie's real playback API and pumps
	// events until the clip finishes or is skipped. TMovie's actual Play()
	// signature isn't known (only Initialize/OneFrame are reversed), so
	// this stops short of a full reconstruction - see NOTES.md.
	if (!_moviesEnabled)
		return 0;
	if (!file.IsOk())
		return 0;

	_movie.Initialize(false);
	graphics->ResetMatrix(true, false);
	_sceneControl->Draw();
	DrawInterfaces();
	SetCurrent();

	while (!_movie.OneFrame()) {
		// Real loop pumps SDL events for a skip key/click and re-renders;
		// not reconstructed.
	}
	return 1;
}

bool TMasterControl::IsVideoPlaying() const {
	return _videoPlaying;
}

bool TMasterControl::VideoFrame() {
	if (_movie.OneFrame())
		return true;

	_videoPlaying = false;
	// _movieEventHandler's real type/virtual interface isn't reversed;
	// the original calls its vtable slot 1 here.

	if (/* field +0x4C, likely TPaintControl-internal state - not resolved */ false) {
		_soundManager->OnVideoFrameFinished();
	}
	return false;
}

void TMasterControl::MovieEvent(void *sdlEvent) {
	// This turned out to double as the SDL controller-hotplug handler (its
	// body calls TGameController::AddGameController/RemoveGameController
	// per the disassembly's CODE XREFs), alongside video-overlay event
	// handling. Full reconstruction needs the real SDL_Event layout and
	// TMovie's event model, neither reversed yet.
	(void)sdlEvent;
}

bool TMasterControl::UnregisterEngineEventHandler(const wxString &name) {
	for (size_t i = 0; i < _engineEventHandlerNames.size(); ++i) {
		wxString converted;
		toUTF(&converted, _engineEventHandlerNames[i].c_str());
		if (converted.ToStdWstring() == name.ToStdWstring()) {
			_engineEventHandlerNames.erase(_engineEventHandlerNames.begin() + static_cast<long>(i));
			return true;
		}
	}
	return false;
}

bool TMasterControl::UnregisterKeyboardEventHandler(const wxString &name) {
	for (size_t i = 0; i < _keyboardEventHandlers.size(); ++i) {
		if (_keyboardEventHandlers[i].name.ToStdWstring() == name.ToStdWstring()) {
			_keyboardEventHandlers.erase(_keyboardEventHandlers.begin() + static_cast<long>(i));
			return true;
		}
	}
	return false;
}

bool TMasterControl::UnregisterMouseEventHandler(const wxString &name) {
	for (size_t i = 0; i < _mouseEventHandlers.size(); ++i) {
		if (_mouseEventHandlers[i].name.ToStdWstring() == name.ToStdWstring()) {
			_mouseEventHandlers.erase(_mouseEventHandlers.begin() + static_cast<long>(i));
			return true;
		}
	}
	return false;
}

void TMasterControl::RegisterEngineEventHandler(const wxString &name) {
	for (const std::string &existing : _engineEventHandlerNames) {
		wxString converted;
		toUTF(&converted, existing.c_str());
		if (converted.ToStdWstring() == name.ToStdWstring())
			return;
	}
	_engineEventHandlerNames.push_back(std::string(static_cast<const char *>(name.mb_str())));
}

void TMasterControl::RegisterMouseEventHandler(const wxString &name, const std::vector<int> &filter) {
	for (const auto &handler : _mouseEventHandlers) {
		if (handler.name.ToStdWstring() == name.ToStdWstring())
			return;
	}
	TMouseEventHandler entry;
	entry.name = name;
	entry.mouseButtonFilter.assign(filter.begin(), filter.end());
	_mouseEventHandlers.push_back(std::move(entry));
}

void TMasterControl::RegisterKeyboardEventHandler(const wxString &name) {
	for (const auto &handler : _keyboardEventHandlers) {
		if (handler.name.ToStdWstring() == name.ToStdWstring())
			return;
	}
	_keyboardEventHandlers.push_back(TKeyboardEventHandler{name});
}

void TMasterControl::ProcessMessage(TMouseMessageEnum msg, const wxPoint &pos) {
	for (auto &handler : _mouseEventHandlers) {
		bool matches = handler.mouseButtonFilter.empty();
		if (!matches) {
			for (unsigned int filterVal : handler.mouseButtonFilter) {
				if (static_cast<int>(filterVal) == static_cast<int>(msg)) {
					matches = true;
					break;
				}
			}
		}
		if (!matches)
			continue;

		TArgument msgArg;
		msgArg.Set(static_cast<int>(msg));
		TArgument posArg;
		posArg.Set(pos);
		// Real dispatch calls LuaExecuteFunction(handler.name, {&msgArg,
		// &posArg}, results) - LuaExecuteFunction/TArgument's full contract
		// isn't reversed yet.
	}
}

void TMasterControl::ProcessMessage(TMouseMessageEnum msg, const wxPoint &pos, float /*a*/, float /*b*/, int /*c*/) {
	// Not traced in detail; presumed to follow the same dispatch pattern as
	// the (msg, pos) overload with extra TArgument::Set() calls for the
	// additional float/int payload (e.g. mouse wheel delta).
	ProcessMessage(msg, pos);
}

void TMasterControl::GetWindowSize(int *width, int *height) const {
	*width = _windowWidth;
	*height = _windowHeight;
}

const wxPoint &TMasterControl::GetMousePos() const {
	return _mousePos;
}

bool TMasterControl::IsScrolling() const {
	return _isScrolling;
}

void TMasterControl::StartEarthquake(int amount, int jitterInterval) {
	_earthquakeActive = true;
	_earthquakeAmount = amount;
	_earthquakeJitterInterval = jitterInterval <= 0 ? 1 : jitterInterval;
	int range = amount * 2;
	_earthquakeOffsetX = range ? (std::rand() % range - amount) : 0;
	_earthquakeOffsetY = range ? (std::rand() % range - amount) : 0;
	_earthquakeTimer.SetTime();
}

void TMasterControl::StopEarthquake() {
	_earthquakeActive = false;
}

void TMasterControl::Signal(const TSignalData &signal, TSignalData &result) {
	result = TSignalData{};
	switch (signal.type) {
	case kSignalGameEvent:
		Update();
		return;
	case kSignalDrawInterfaces:
		DrawInterfaces();
		return;
	case kSignalDraw:
		Draw(false);
		return;
	case kSignalLoadingProgress:
		if (_loadingControl) {
			_loadingControl->UpdateStatus(signal.loadingCurrent, signal.loadingTotal);
			graphics->ResetMatrix(true, false);
			_loadingControl->Draw();
			graphics->SetMatrixMode(true, true);
		}
		return;
	default:
		x_assert(false, "false", "/home/simon/Documents/jenkins/branchPillars/src/vsplayer/control/masterControl.cpp",
		         118);
		return;
	}
}
