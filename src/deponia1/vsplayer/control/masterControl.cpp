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

// Confirmed (asm lines 485602-485732): every interface in the list is drawn, from the last to the first (the list is
// walked backwards), if it is shown always, or the game does not hide the interfaces and no movie plays. The whole is
// skipped only in a scene that is a menu when the game hides the interfaces in the menus and no movie plays.
void TMasterControl::DrawInterfaces() {
	TVisObjRef game = _visionaire->GetGame();
	TVisObjRef scene = game.GetLink(kGameCurrentScene);
	// (an empty scene reads as no menu)
	const bool draw = !game.GetBool(kGameAutoHideInterfacesInMenu) || _videoPlaying || !scene.GetBool(kSceneIsMenu);

	if (!draw)
		return;

	for (auto it = _activeInterfaces.rbegin(); it != _activeInterfaces.rend(); ++it) {
		TGInterface *interface = *it;

		if (interface->GetRef().GetBool(kInterfaceShowAlways) || (!game.GetBool(kGameHideInterfaces) && !_videoPlaying))
			interface->Draw();
	}
}

// Confirmed (asm lines 486009-486597): one frame. With `lock` (the main loop) it holds g_loadingScreenLock while it
// draws, brackets the drawing with the backend's BeforeDrawScene()/AfterDrawScene() and shows the frame in the end (the
// loading screen thread calls it without). The scene and what the scripts draw before and after it, the interfaces, the
// texts or the dialog (the part of the game that is on top), the action text near the cursor - or in the rectangle of the
// game, and the cursor. The setting kGameShaderExclude (0-3) says which of the three parts is drawn through the matrices
// of the scripts. An earthquake shakes the scroll position for the time of the frame.
void TMasterControl::Draw(bool lock) {
	if (lock)
		g_loadingScreenLock.Enter();

	wxPoint savedScrollPos;

	if (_earthquakeActive) {
		TPaintControl *scene = _sceneControl->GetScene();

		savedScrollPos = scene->GetScrollPos();

		if (_earthquakeTimer.GetTime() > _earthquakeJitterInterval) {
			_earthquakeOffsetX = (_earthquakeAmount != 0) ? std::rand() % (_earthquakeAmount * 2) - _earthquakeAmount : 0;
			_earthquakeOffsetY = (_earthquakeAmount != 0) ? std::rand() % (_earthquakeAmount * 2) - _earthquakeAmount : 0;
			_earthquakeTimer.SetTime();
		}

		scene->SetScrollPos(wxPoint{savedScrollPos.x - _earthquakeOffsetX, savedScrollPos.y - _earthquakeOffsetY});
	}

	matricesActive = true;

	if (lock)
		graphics->BeforeDrawScene(true, 0);

	for (size_t i = 0; i < luaDrawBeforeScene.size(); i++)
		LuaDoString(luaDrawBeforeScene[i], luaDrawBeforeScene[i]);

	_sceneControl->Draw();

	for (size_t i = 0; i < luaDrawAfterScene.size(); i++)
		LuaDoString(luaDrawAfterScene[i], luaDrawAfterScene[i]);

	TVisObjRef game = _visionaire->GetGame();
	const int shaderExclude = game.GetInt(kGameShaderExclude);

	if (shaderExclude == 1) {
		graphics->AfterDrawScene(false, true);
		graphics->BeforeDrawScene(false, 0);
		matricesActive = false;
		DrawInterfaces();
	} else {
		DrawInterfaces();

		if (shaderExclude == 2) {
			graphics->AfterDrawScene(false, true);
			graphics->BeforeDrawScene(false, 0);
			matricesActive = false;
		}
	}

	bool cursorDrawn = false;

	if (_sceneControl->GetScene()->IsActive() && !DisplayTexts()) {
		SetCurrent();

		const int actionTextMode = game.GetInt(kGameDrawActionText);

		if (!DisplayDialog() && actionTextMode != 0 && _cursorControl->IsActive()) {
			TVisObjRef font = _visionaire->GetGame().GetLink(kGameActionTextFont);

			_fontManager->SetCurrentFont(font);
			_cursorControl->Draw();

			wxString text = _objectManager.GetActionText();
			wxPoint size;
			wxPoint position;

			_fontManager->GetTextDimension(text, size);

			if (actionTextMode == 1) {
				// next to the cursor, but inside the window
				position = _cursorControl->GetPositionNextToCursor();

				if (position.x + size.x > _windowWidth)
					position.x = _windowWidth - size.x;

				if (position.y + size.y > _windowHeight)
					position.y = _windowHeight - size.y;
			} else if (actionTextMode == 2) {
				// in the middle of the rectangle of the game, at its top
				const wxRect &rect = *_visionaire->GetGame().GetRect(kGameActionTextRect);

				position.x = rect.GetLeft() + rect.GetWidth() / 2 - size.x / 2;
				position.y = rect.GetTop();
			}

			_fontManager->PrintText(text, TextAlignmentEnum::kLeft, position, 1.0f, nullptr);
			cursorDrawn = true;
		}
	}

	for (size_t i = 0; i < luaDrawAfterInterfaces.size(); i++)
		LuaDoString(luaDrawAfterInterfaces[i], luaDrawAfterInterfaces[i]);

	if (shaderExclude == 3) {
		graphics->AfterDrawScene(false, true);
		graphics->BeforeDrawScene(false, 0);
		matricesActive = false;
	}

	if (!cursorDrawn) {
		SetCurrent();

		if (_cursorControl->IsActive())
			_cursorControl->Draw();
	}

	matricesActive = false;
	DisplayInSceneConsole();

	if (lock) {
		graphics->AfterDrawScene(false, shaderExclude == 0);
		graphics->SetDirectToScreen();
		DisplayConsole();
		debugger.EndArea(ProfileArea::kValue3, -1);
		graphics->Swap();
	} else {
		graphics->SetDirectToScreen();
		DisplayConsole();
	}

	if (_earthquakeActive)
		_sceneControl->GetScene()->SetScrollPos(savedScrollPos);

	if (lock) {
		lastFrameEnd = SDL_GetTicks();
		g_loadingScreenLock.Leave();
	}
}

// Confirmed (asm lines 486695-487099): the scroll of the scene towards the edge of the window that the mouse is near
// (kGameCursorHorizontalScrollDistance/VerticalScrollDistance pixels from it). The speed (xspeed, yspeed) is eased in
// with startspeed when the game has smooth scrolling, else it is 1 or -1 at once; in the middle of the window it eases
// out to 0. The time since the last call (500 ms at most, else 1) is multiplied by the game's scroll speed.
// (In the original the vertical speed that eases out does not move the scene, and the horizontal one only while it is
// positive.)
void TMasterControl::ScrollUpdate() {
	if (_quitGame)
		return;

	TVisObjRef game = _visionaire->GetGame();

	_smoothScrolling = game.GetBool(kGameSmoothScrolling);
	_isScrolling = false;

	TPaintControl *scene = _sceneControl->GetScene();

	if (scene->IsScrollable()) {
		const int worktopWidth = scene->GetWorktopWidth();
		const int worktopHeight = scene->GetWorktopHeight();
		const FloatPoint scroll = scene->GetFloatScrollPos();
		const int distanceX = game.GetInt(kGameCursorHorizontalScrollDistance);
		const int distanceY = game.GetInt(kGameCursorVerticalScrollDistance);
		const bool smooth = _smoothScrolling;

		if (distanceX >= _mousePos.x || _mousePos.x >= _windowWidth - distanceX || distanceY >= _mousePos.y ||
		    _mousePos.y >= _windowHeight - distanceY)
			_isScrolling = true;

		float step = 1.0f;

		if (_scrollTimer.GetTime() <= 500)
			step = (float)_scrollTimer.GetTime() * _timingValueSeconds;

		const double start = startspeed;
		const double rest = 1.0 - start;

		if (_mousePos.x >= _windowWidth - distanceX && (float)worktopWidth > (float)_windowWidth + scroll.x) {
			xspeed = (float)(rest * xspeed + start);

			if (!smooth)
				xspeed = 1.0f;

			scene->AdjustWindowHorizontal(xspeed * step + scroll.x);
		} else if (distanceX >= _mousePos.x && scroll.x > 0.0f) {
			xspeed = (float)(rest * xspeed - start);

			if (!smooth)
				xspeed = -1.0f;

			scene->AdjustWindowHorizontal(xspeed * step + scroll.x);
		} else {
			xspeed = (float)(0.0 * start + (double)xspeed * rest);

			if (!smooth)
				xspeed = 0.0f;
			else if (xspeed > 0.0f)
				scene->AdjustWindowHorizontal(xspeed * step + scroll.x);
		}

		if (_mousePos.y >= _windowHeight - distanceY && (float)worktopHeight > (float)_windowHeight + scroll.y) {
			if (smooth)
				yspeed = (float)((double)yspeed * rest + start);
			else
				yspeed = 1.0f;

			scene->AdjustWindowVertical(yspeed * step + scroll.y);
		} else if (distanceY >= _mousePos.y && scroll.y > 0.0f) {
			if (smooth)
				yspeed = (float)((double)yspeed * rest - start);
			else
				yspeed = -1.0f;

			scene->AdjustWindowVertical(scroll.y + yspeed * step);
		} else {
			const double decay = start * 0.0;

			yspeed = (float)((double)yspeed * rest + decay);

			if (!smooth)
				xspeed = 0.0f;

			if (yspeed > 0.0f)
				yspeed = (float)((double)yspeed * rest + decay);
		}
	}

	_scrollTimer.SetTime();
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
	graphics->AfterDrawScene(true, false);
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

	// then the game itself: the position is the mouse's, and the message is a move (1), a hold (6) or one of the
	// clicks and wheel turns (2, 4, 5, 9, 11, 12, 13); the others (the button going down ...) are for the scripts only
	_mousePos = pos;

	const unsigned int number = static_cast<unsigned int>(msg);

	if (number <= 0xD) {
		const unsigned int mask = 1u << number;

		if (mask & 0x3A34)
			HandleMouseUp(pos, msg);
		else if (mask & 0x40)
			HandleMouseHolding(pos);
		else if (mask & 0x2)
			HandleMouseMove(pos, false);
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
			graphics->BeforeDrawScene(true, 0);
			_loadingControl->Draw();
			graphics->AfterDrawScene(true, true);
		}
		return;
	default:
		x_assert(false, "false", "/home/simon/Documents/jenkins/branchPillars/src/vsplayer/control/masterControl.cpp",
		         118);
		return;
	}
}
