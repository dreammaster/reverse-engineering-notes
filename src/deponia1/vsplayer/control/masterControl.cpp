#include "vsplayer/control/masterControl.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "AppGlobals.h"
#include "Diagnostics.h"
#include "TSceneControl.h"
#include "graphicslib/graphics.h"
#include "vscommon/scripting/argument.h"
#include "vscommon/scripting/lua.h"
#include "graphicslib/picture.h"
#include "vscommon/objAccess.h"
#include "vsplayer/control/gameControl.h"
#include "vsplayer/control/gameController.h"
#include "vstables/fieldIds.h"

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
	// _visionaire is set by TGameControl (to its TVisionaireGame): the original
	// constructor doesn't create one.
	_moviesEnabled = true;
}

TMasterControl::~TMasterControl() {
	delete _cursorControl;
	delete _gameController;
	delete _loadingControl;
	delete _soundManager;
	delete _fontManager;
	delete _gameClientSDK;
	// (The game data are not deleted here: the owner of the game control, CleanUp(), deletes them after the game control
	// and TVisionaire::CleanUp(); asm 488513-488946 has no such delete.)
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

void FillLoadingScreen(SLoadingScreen &/*screen*/, const TVisObjRef &/*source*/) {
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
	TVisObjRef link = game.GetLink(kGameCurrentScene);
	bool shouldDraw = link.IsEmpty() ? link.GetBool(kSceneIsMenu) : !link.GetBool(kGameAutoHideInterfacesInMenu);
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
		for (const std::string &script : luaDrawBeforeScene)
			LuaDoString(script, script);

		_sceneControl->Draw();

		for (const std::string &script : luaDrawAfterScene)
			LuaDoString(script, script);

		TVisObjRef game = _visionaire->GetGame();
		// Field id 0x313, meaning not resolved: picks one of three render
		// paths below (interfaces-only / scene-without-action-text / normal
		// with action text). The original tracks this in a local it also
		// reuses for the action-text positioning mode further down (its
		// `r13d`) - split into two clearly-named locals here instead.
		int drawMode = game.GetInt(kGameShaderExclude);

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
				TVisObjRef link = game.GetLink(kGameActionTextFont);
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

		for (const std::string &script : luaDrawAfterInterfaces)
			LuaDoString(script, script);

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
	_easeDirectionFlag = game.GetBool(kGameSmoothScrolling);
	_isScrolling = false;

	TPaintControl *scene = _sceneControl->GetScene();
	if (!scene->IsScrollable())
		return;

	int worktopWidth = scene->GetWorktopWidth();
	FloatPoint floatScroll = scene->GetFloatScrollPos();
	(void)floatScroll;  // used by the real easing formula; not reproduced here (see below)
	int targetX = game.GetInt(kGameCursorHorizontalScrollDistance);   // field id not resolved - a scroll target x position
	int edgeMargin = game.GetInt(kGameCursorVerticalScrollDistance);  // field id not resolved - an edge-scroll trigger margin

	constexpr float kMaxSpeed = 1.0f;

	if (targetX >= _mousePos.x || worktopWidth - targetX <= _mousePos.x) {
		_isScrolling = true;
	}
	bool nearEdge = _mousePos.x < edgeMargin || _mousePos.x > worktopWidth - edgeMargin;

	if (_isScrolling || nearEdge) {
		if (_scrollTimer.GetTime() > 500) {
			_scrollTimer.SetTime();
		}
		// xspeed is a real shared global (not a TMasterControl-only value) -
		// TGameControl::MoveScene eases the exact same variable for its own,
		// unrelated destination-scroll movement (see its own comment).
		float targetSpeed = _easeDirectionFlag ? -kMaxSpeed : kMaxSpeed;
		xspeed = targetSpeed + (xspeed - targetSpeed) * startspeed;
		scene->AdjustWindowHorizontal(xspeed * 0.001f);
	}
}

// Confirmed (asm lines 487099-487890): starts a movie. It is played by the main loop (VideoFrame(), once for each frame while
// IsVideoPlaying()). The sounds are paused (0, 1), left alone (2) or stopped (3) meanwhile; the settings of the player are
// the subtitle font (the video subtitle font of the game, else the font of the action texts), position and language, the
// language of the sound, whether the file is encrypted, the picture shown when the movie is paused, and whether the
// screen goes black afterwards. The answer is that of TMovie::PlayCutScene() (false: it cannot be played - the next
// VideoFrame() ends it); the movie is "playing" in any case.
int TMasterControl::PlayAVI(const wxFileName &file, bool skippable, HandleSoundsEnum handleSounds) {
	if (!_moviesEnabled)
		return 0;

	if (!file.IsOk())
		return 0;

	_movie.Initialize(false);
	graphics->SetMatrixMode(true, false);
	_sceneControl->Draw();
	DrawInterfaces();
	SetCurrent();

	TVisObjRef game = _visionaire->GetGame();
	TVisObjRef subtitleFont = game.GetLink(kGameVideoSubtitleFont);

	if (subtitleFont.IsEmpty())
		subtitleFont = _visionaire->GetGame().GetLink(kGameActionTextFont);

	TMovieSettings settings;

	settings.subtitlePosition = *game.GetPoint(kGameVideoSubtitlePosition);
	settings.subtitleLanguage = game.GetStr(kGameVideoSubtitleLanguage);
	settings.audioLanguage = game.GetStr(kGameVideoAudioLanguage);

	switch (static_cast<int>(handleSounds)) {
	case 2:
		break;

	case 3:
		_soundManager->CleanUp();
		break;

	default:
		if (static_cast<unsigned int>(handleSounds) <= 1)
			_soundManager->PauseAll();

		break;
	}

	bool encrypted = game.GetBool(kGameVideosEncrypted);

	settings.videoResolution = *game.GetPoint(kGameWindowResolution);

	// the picture of the pause screen (the file is the game's own; a game without one has no name for it)
	wxFileName pauseFile = game.GetPath(kGameVideoPauseScreen);

	if (!pauseFile.GetName().IsEmpty()) {
		_pauseScreen = new TPictureIO();

		pauseFile.NormalizePath();
		_pauseScreen->LoadPicture(pauseFile, TPictureIO::eLoadSetting::Normal);
	}

	bool showBlackScreenAfter = game.GetBool(kGameShowBlackScreenAfterVideo);

	settings.skippable = skippable;
	settings.pauseScreen = _pauseScreen;
	settings.fontManager = _fontManager;
	settings.fontId = PackVisId(subtitleFont.GetId());
	settings.soundManager = _soundManager;

	bool started = _movie.PlayCutScene(file, settings, encrypted, showBlackScreenAfter);

	_movie._drawFunction = [this] { DrawInterfaces(); };
	_movie._eventFunction = [this](void *event) { MovieEvent(event); };
	_videoPlaying = true;
	_handleSounds = static_cast<int>(handleSounds);
	return started ? 1 : 0;
}

bool TMasterControl::IsVideoPlaying() const {
	return _videoPlaying;
}

// Confirmed (asm lines 487920-487970): one frame of the movie. When it is over the pause screen is let go, and the sounds
// that were paused go on (only when they were paused, 1).
// (The original does not forget the pause screen after deleting it, and a later movie without one would delete it again.)
bool TMasterControl::VideoFrame() {
	if (_movie.OneFrame())
		return true;

	_videoPlaying = false;
	delete _pauseScreen;
	_pauseScreen = nullptr;

	if (_handleSounds == 1)
		_soundManager->ContinueAll();

	return false;
}

// Confirmed (asm lines 490064-490700): an event of the mouse, a touch screen or a controller while a movie plays. A
// controller that is added, removed or remapped is told to the game controller and the key handler (7, 8, 9); everything
// else is given to the script function movieEvent(name, ...) if there is one: "mouseDown" / "mouseUp" with the place
// as a point, "touchDown" / "touchUp" with the place as a point (of the position as the integer part of the
// 0 to 1 numbers the screen gives), "controllerDown" / "controllerUp" with the key of the button (as
// TGameControl::ConvertControllerButtonToSymKey() has it) and the number of the controller. (Another kind of event is
// given with the name "".)
void TMasterControl::MovieEvent(void *sdlEvent) {
	SDL_Event *event = static_cast<SDL_Event *>(sdlEvent);
	TGameControl *control = static_cast<TGameControl *>(g_pGameControl);

	switch (event->type) {
	case SDL_CONTROLLERDEVICEREMOVED: {
		unsigned short id = static_cast<unsigned short>(event->cdevice.which);

		control->HandleKeyEvent(TKeyboardMessageEnum::kControllerRemoved, wxString(), 0, id);
		control->GetGameController()->RemoveGameController(event->cdevice.which);
		return;
	}

	case SDL_CONTROLLERDEVICEREMAPPED:
		control->HandleKeyEvent(TKeyboardMessageEnum::kControllerRemapped, wxString(), 0,
		                        static_cast<unsigned short>(event->cdevice.which));
		return;

	case SDL_CONTROLLERDEVICEADDED: {
		int index = control->GetGameController()->AddGameController(event->cdevice.which);

		control->HandleKeyEvent(TKeyboardMessageEnum::kControllerAdded, wxString(), 0, static_cast<unsigned short>(index));
		return;
	}

	default:
		break;
	}

	lua_getfield(L, LUA_GLOBALSINDEX, "movieEvent");

	bool exists = lua_type(L, -1) == LUA_TFUNCTION;

	lua_settop(L, -2);

	if (!exists)
		return;

	const char *name = "";

	switch (event->type) {
	case SDL_CONTROLLERBUTTONDOWN:
		name = "controllerDown";
		break;

	case SDL_CONTROLLERBUTTONUP:
		name = "controllerUp";
		break;

	case SDL_MOUSEBUTTONDOWN:
		name = "mouseDown";
		break;

	case SDL_MOUSEBUTTONUP:
		name = "mouseUp";
		break;

	case SDL_FINGERDOWN:
		name = "touchDown";
		break;

	case SDL_FINGERUP:
		name = "touchUp";
		break;

	default:
		break;
	}

	TArgument nameArgument;
	TArgument secondArgument;
	TArgument thirdArgument;

	nameArgument.Set(wxString(name));

	if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP) {
		wxPoint position;

		position.x = event->button.x;
		position.y = event->button.y;
		secondArgument.Set(position);
	} else if (event->type == SDL_FINGERDOWN || event->type == SDL_FINGERUP) {
		wxPoint position;

		position.x = static_cast<int>(event->tfinger.x);
		position.y = static_cast<int>(event->tfinger.y);
		secondArgument.Set(position);
	} else {
		secondArgument.Set(static_cast<TGameControl *>(this)->ConvertControllerButtonToSymKey(event->cbutton));
	}

	std::vector<TArgument *> arguments;
	std::vector<TArgument *> results;

	arguments.push_back(&nameArgument);
	arguments.push_back(&secondArgument);

	if (event->type == SDL_CONTROLLERBUTTONDOWN || event->type == SDL_CONTROLLERBUTTONUP) {
		thirdArgument.Set(static_cast<int>(event->cbutton.which));
		arguments.push_back(&thirdArgument);
	}

	LuaExecuteFunction(std::string("movieEvent"), arguments, results);
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

// Confirmed (asm lines 489262-489599): the functions registered for the mouse whose list of messages has this one
// (a handler without a list is never called) are called with the message and the position ("MouseMoveHandler").
void TMasterControl::ProcessMessage(TMouseMessageEnum msg, const wxPoint &pos) {
	for (size_t i = 0; i < _mouseEventHandlers.size(); i++) {
		const TMouseEventHandler &handler = _mouseEventHandlers[i];

		if (std::find(handler.mouseButtonFilter.begin(), handler.mouseButtonFilter.end(), static_cast<unsigned int>(msg)) ==
		        handler.mouseButtonFilter.end())
			continue;

		TArgument msgArg;
		msgArg.Set(static_cast<int>(msg));
		TArgument posArg;
		posArg.Set(pos);

		std::vector<TArgument *> arguments = {&msgArg, &posArg};
		std::vector<TArgument *> results;

		LuaDebugName("MouseMoveHandler");
		LuaExecuteFunction(std::string(handler.name.mb_str()), arguments, results);
	}
}

// Confirmed (asm lines 489607-490054): the same with the two numbers and the integer of the event (the wheel and
// the like) after the position ("MouseEventHandler").
void TMasterControl::ProcessMessage(TMouseMessageEnum msg, const wxPoint &pos, float a, float b, int c) {
	for (size_t i = 0; i < _mouseEventHandlers.size(); i++) {
		const TMouseEventHandler &handler = _mouseEventHandlers[i];

		if (std::find(handler.mouseButtonFilter.begin(), handler.mouseButtonFilter.end(), static_cast<unsigned int>(msg)) ==
		        handler.mouseButtonFilter.end())
			continue;

		TArgument msgArg;
		msgArg.Set(static_cast<int>(msg));
		TArgument posArg;
		posArg.Set(pos);
		TArgument aArg;
		aArg.Set(static_cast<double>(a));
		TArgument bArg;
		bArg.Set(static_cast<double>(b));
		TArgument cArg;
		cArg.Set(c);

		std::vector<TArgument *> arguments = {&msgArg, &posArg, &aArg, &bArg, &cArg};
		std::vector<TArgument *> results;

		LuaDebugName("MouseEventHandler");
		LuaExecuteFunction(std::string(handler.name.mb_str()), arguments, results);
	}
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
