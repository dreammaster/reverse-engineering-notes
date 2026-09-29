// Reconstructed from Deponia_Linux.asm, TMasterControl methods at asm lines
// 485602-491098+ (address range 0x623ED0-0x627A18+). See NOTES.md.
//
// Original path confirmed via the x_assert() in Signal()'s default case:
// src/vsplayer/control/masterControl.cpp - see manifest/source_layout.tsv.
//
// TMasterControl is the engine's abstract render/game-loop hub: it
// multiply-inherits TPaintControl (confirmed from its vtable dump - a
// secondary vtable section whose slots default to TPaintControl::Prepare/
// Draw) and declares 8 of its own pure virtual methods, all named directly
// from `vtable for TGameControl`'s own dump (see vsplayer/control/
// gameControl.h/.cpp for the concrete overrides): Update, DisplayDialog,
// DisplayTexts, DisplayConsole, DisplayInSceneConsole, HandleMouseMove,
// HandleMouseUp, HandleMouseHolding. It owns essentially every other
// subsystem controller (scene, cursor, game controller, loading, sound,
// font, platform SDK) as members.
//
// This is a partial reconstruction. Depth varies a lot by method:
//   - Fully traced: all simple accessors/mutators, StartEarthquake/
//     StopEarthquake, DrawInterfaces, Draw, VideoFrame/IsVideoPlaying,
//     Register/UnregisterEngineEventHandler, RegisterMouseEventHandler.
//   - Structurally faithful but with named-but-unconfirmed opaque
//     TVisObjRef field ids: Draw, Signal.
//   - Approximated/lighter treatment (real dependency classes - TMovie's
//     Play API, the video/controller event pump - are far too deep to
//     responsibly reconstruct without reversing them first): PlayAVI,
//     MovieEvent, ScrollUpdate's exact easing constants, ProcessMessage's
//     second (float,float,int) overload.
#pragma once

#include <list>
#include <string>
#include <vector>

#include "TCharHolder.h"
#include "TGInterface.h"
#include "TGObjectManager.h"
#include "TGameClientSDK.h"
#include "TMovie.h"
#include "TPaintControl.h"
#include "TSignalData.h"
#include "TSoundFFMPEG.h"
#include "TSprite.h"
#include "TTimer.h"
#include "WxStub.h"
#include "datastruct/visionaire.h"
#include "vscommon/fontManager.h"
#include "vsplayer/control/cursorControl.h"
#include "vsplayer/control/loadingControl.h"

class TSceneControl;
class TGameController;

enum class HandleSoundsEnum { kStop, kPause, kContinue };
enum class TMouseMessageEnum { kMove, kLeftDown, kLeftUp, kRightDown, kRightUp, kWheel };
// Confirmed to have at least 7 distinct values (TGameControl::
// HandleControllerButtonHit/Release pass literal 4/5, and HandleControllerAxis
// passes literal 6, as this same enum's type - Deponia_Linux.asm lines
// 471722-471975) - values 2 and 3 haven't been observed at any call site yet.
enum class TKeyboardMessageEnum {
	kKeyDown = 0,
	kKeyUp = 1,
	kControllerButtonHit = 4,
	kControllerButtonRelease = 5,
	kAxisMove = 6,
};

// Field order/sizes are recovered from TMasterControl::SetLoadingScreen's
// memberwise copy; several fields' real meaning is unconfirmed (see
// NOTES.md) - names are best-effort guesses at "a loading screen needs
// this."
struct SLoadingScreen {
	TCharHolder backgroundImage;
	long long field18 = 0;
	long long field20 = 0;
	int field28 = 0;
	TCharHolder progressBarImage;
	bool field40 = false;
	int field44 = 0;
	float field48 = 0.0f;
	TCharHolder soundOrOverlayImage;
	long long field68 = 0;
	long long field70 = 0;
	int field78 = 0;
	TCharHolder anotherImage;
	bool field90 = false;
	int field94 = 0;
	unsigned short field9C = 0;
	unsigned char fieldA0 = 0;
	std::wstring text;
	long long fieldB0 = 0;
	int fieldB8 = 0;
	int fieldBC = 0;
};

// Confirmed a free function, not a member (TGameControl::PreLoad,
// Deponia_Linux.asm line 464520) - populates an SLoadingScreen from a
// TVisObjRef's own fields (presumably the same ones SetLoadingScreen's own
// confirmed layout above lists); not reversed beyond that call shape.
void FillLoadingScreen(SLoadingScreen &screen, const TVisObjRef &source);

struct TMouseEventHandler {
	wxString name;
	std::vector<unsigned int> mouseButtonFilter;  // empty = matches any message
};

struct TKeyboardEventHandler {
	wxString name;
};

class TMasterControl : public TPaintControl {
public:
	TMasterControl();
	virtual ~TMasterControl();

	virtual void Signal(const TSignalData &signal, TSignalData &result);

	// The 8 pure virtuals from the original vtable (offsets 0x18-0x50);
	// TGameControl provides the real overrides (confirmed from
	// vtable-for-TGameControl's own dump, which names every slot directly -
	// no guessing needed here, unlike when this was first written against
	// TMasterControl alone).
	virtual bool Update() = 0;
	virtual bool DisplayDialog() = 0;
	virtual bool DisplayTexts() = 0;
	virtual bool DisplayConsole() = 0;
	virtual void DisplayInSceneConsole() = 0;
	virtual void HandleMouseMove(const wxPoint &pos, bool isHolding) = 0;
	virtual void HandleMouseUp(const wxPoint &pos, TMouseMessageEnum msg) = 0;
	virtual void HandleMouseHolding(const wxPoint &pos) = 0;

	void QuitGame();
	bool GetQuitGame() const;
	void SetClearMessage();
	bool GetClearMessage();

	TPaintControl *GetMainControl();
	TCursorControl *GetCursorControl();
	TGameController *GetGameController();
	TSoundFFMPEG *GetSoundManager() const;
	TFontManager *GetFontManager();
	TGameClientSDK *GetGameClientSDK() const;

	void SetLoadingScreen(SLoadingScreen &screen);
	void ShowLoadingScreen();

	void CleanUp();
	void EnableMovies(bool enable);

	// Hides (doesn't override) TPaintControl::Draw() - the original really
	// does have two distinct Draw entry points (this one and the inherited
	// TPaintControl::Draw() reached via the secondary vtable) - so making
	// that explicit rather than suppressing the compiler's warning.
	using TPaintControl::Draw;
	bool Draw(bool showActionText);
	void DrawInterfaces();
	void ScrollUpdate();

	int PlayAVI(const wxFileName &file, bool skippable, HandleSoundsEnum handleSounds);
	bool IsVideoPlaying() const;
	bool VideoFrame();
	void MovieEvent(void *sdlEvent);

	bool UnregisterEngineEventHandler(const wxString &name);
	bool UnregisterKeyboardEventHandler(const wxString &name);
	bool UnregisterMouseEventHandler(const wxString &name);
	void RegisterEngineEventHandler(const wxString &name);
	void RegisterMouseEventHandler(const wxString &name, const std::vector<int> &filter);
	void RegisterKeyboardEventHandler(const wxString &name);
	// Confirmed called on g_pGameControl (TMasterControl*) after a successful
	// TGameControl::ReplaceGame, Deponia_Linux.asm line 468845 - but IDA
	// resolves the real symbol as `THGameControl::RegisterEventHandler()`,
	// a distinct, not-yet-integrated class only otherwise seen as a caller
	// elsewhere (e.g. `THGameControl::OnEvent`, xref'd from
	// ScrollToCharacterIfNeeded/AdjustInterfacesOnScreen). For a direct,
	// non-virtual call through a TMasterControl* to resolve to a
	// THGameControl method, THGameControl would need to be a base of the
	// concrete TGameControl - unconfirmed and not modeled as such yet;
	// placed here (rather than guessed onto that relationship) purely so
	// ReplaceGame compiles and calls *something* at this point. Revisit once
	// THGameControl itself gets a dedicated pass.
	void RegisterEventHandler();

	void ProcessMessage(TMouseMessageEnum msg, const wxPoint &pos);
	void ProcessMessage(TMouseMessageEnum msg, const wxPoint &pos, float a, float b, int c);

	void GetWindowSize(int *width, int *height) const;
	const wxPoint &GetMousePos() const;
	bool IsScrolling() const;

	void StartEarthquake(int amount, int jitterInterval);
	void StopEarthquake();

protected:
	// TGameControl embeds an actual TSceneControl by value and points this
	// at it (TMasterControl's own constructor never sets it - see
	// gameControl.cpp); protected rather than private for that reason.
	TSceneControl *_sceneControl = nullptr;

	// TGameControl reads this directly (confirmed: SkipCurrentText() and
	// UpdateAspectRatio() both do `_visionaire->GetGame()` on it) - a
	// TMasterControl-only accessor was never called at those sites, so this
	// is protected rather than private for the same reason as
	// _sceneControl above.
	TVisionaire *_visionaire = nullptr;

	// TGameControl reads both directly (GetInterface/GetAllInterfaces/
	// GetActiveInterfaces/GetObject, asm lines 456603-466193) - same
	// reasoning as _sceneControl/_visionaire above. _allInterfaces is
	// every registered interface; _activeInterfaces (formerly named
	// _interfaces, also used by DrawInterfaces above) is the subset
	// currently being drawn.
	std::list<TGInterface *> _allInterfaces;
	std::list<TGInterface *> _activeInterfaces;

	// TGameControl reads this directly (HandleEngineEvent, Deponia_Linux.asm
	// lines 469160-469504: iterates it via raw begin()/end() pointers, not a
	// TMasterControl-only accessor) - same reasoning as _sceneControl/
	// _visionaire above. Only its size is read there, not any element's
	// content - see HandleEngineEvent's own comment.
	std::vector<std::string> _engineEventHandlerNames;

	// TGameControl reads both directly (LoadAndInitGame, Deponia_Linux.asm
	// lines 467746-467747, 468227-468230: InitControl()/EndLoading() called
	// straight on these, not through TMasterControl's own accessors) - same
	// reasoning as _sceneControl/_visionaire above.
	TLoadingControl *_loadingControl = nullptr;
	TSoundFFMPEG *_soundManager = nullptr;

	// TGameControl reads this directly (HandleKeyEvent, Deponia_Linux.asm
	// lines 471062-471068: iterates it via raw begin()/end() pointers, not a
	// TMasterControl-only accessor) - same reasoning as
	// _engineEventHandlerNames above. Only its size is read there, not any
	// element's content.
	std::vector<TKeyboardEventHandler> _keyboardEventHandlers;

	// Set from a TVisObjRef::GetBool(0x33A) field at the top of every
	// ScrollUpdate() call, then read back inside its easing formula to pick
	// which of two target speeds to ease toward - real meaning (some kind
	// of "scrolling direction/mode" flag) not resolved. +0x254 in the
	// original; unrelated to _moviesEnabled (+0x288) despite both being a
	// lone bool set near the start of a method. TGameControl also reads it
	// directly at the same offset (MoveScene, Deponia_Linux.asm line 459500
	// and others) as a gate between its own eased xspeed/yspeed value and a
	// fixed snap-to value - same field, same role, different caller, so
	// protected rather than private for the same reason as the other
	// TGameControl-reads-directly fields above.
	bool _easeDirectionFlag = false;

private:
	TMovie _movie;
	TGObjectManager _objectManager;
	TSprite _sprite1;
	TSprite _sprite2;

	std::vector<TMouseEventHandler> _mouseEventHandlers;

	int _windowWidth = 0;
	int _windowHeight = 0;
	SLoadingScreen _loadingScreen;
	TCursorControl *_cursorControl = nullptr;
	TGameController *_gameController = nullptr;
	TFontManager *_fontManager = nullptr;
	TGameClientSDK *_gameClientSDK = nullptr;

	void *_movieEventHandler = nullptr;  // +0xC0 in the original; real type/purpose unconfirmed
	bool _videoPlaying = false;          // +0xB8
	int _loadingScreenCachedWidth = 0;   // +0x248
	int _loadingScreenCachedHeight = 0;  // +0x24C
	wxPoint _mousePos;                   // +0x258
	bool _isScrolling = false;           // +0x260
	bool _quitGame = false;              // +0x255
	bool _clearMessage = false;          // +0x256
	bool _earthquakeActive = false;      // +0x261
	int _earthquakeAmount = 0;           // +0x264
	int _earthquakeJitterInterval = 0;   // +0x268
	int _earthquakeOffsetX = 0;          // +0x26C
	int _earthquakeOffsetY = 0;          // +0x270
	TTimer _earthquakeTimer;             // +0x278
	bool _moviesEnabled = true;          // +0x288
	// A function-local static TTimer in the original
	// (ScrollUpdate(void)::scrollTimer, magic-statics-initialized on first
	// call) - a plain member here since TMasterControl is effectively a
	// singleton anyway.
	TTimer _scrollTimer;
};
