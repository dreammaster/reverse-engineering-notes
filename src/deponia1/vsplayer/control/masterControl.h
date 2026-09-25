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

enum class HandleSoundsEnum { Stop, Pause, Continue };
enum class TMouseMessageEnum { Move, LeftDown, LeftUp, RightDown, RightUp, Wheel };
enum class TKeyboardMessageEnum { KeyDown, KeyUp };

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

    virtual void Signal(const TSignalData& signal, TSignalData& result);

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
    virtual void HandleMouseMove(const wxPoint& pos, bool isHolding) = 0;
    virtual void HandleMouseUp(const wxPoint& pos, TMouseMessageEnum msg) = 0;
    virtual void HandleMouseHolding(const wxPoint& pos) = 0;

    void QuitGame();
    bool GetQuitGame() const;
    void SetClearMessage();
    bool GetClearMessage();

    TPaintControl* GetMainControl();
    TCursorControl* GetCursorControl();
    TGameController* GetGameController();
    TSoundFFMPEG* GetSoundManager() const;
    TFontManager* GetFontManager();
    TGameClientSDK* GetGameClientSDK() const;

    void SetLoadingScreen(SLoadingScreen& screen);
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

    int PlayAVI(const wxFileName& file, bool skippable, HandleSoundsEnum handleSounds);
    bool IsVideoPlaying() const;
    bool VideoFrame();
    void MovieEvent(void* sdlEvent);

    bool UnregisterEngineEventHandler(const wxString& name);
    bool UnregisterKeyboardEventHandler(const wxString& name);
    bool UnregisterMouseEventHandler(const wxString& name);
    void RegisterEngineEventHandler(const wxString& name);
    void RegisterMouseEventHandler(const wxString& name, const std::vector<int>& filter);
    void RegisterKeyboardEventHandler(const wxString& name);

    void ProcessMessage(TMouseMessageEnum msg, const wxPoint& pos);
    void ProcessMessage(TMouseMessageEnum msg, const wxPoint& pos, float a, float b, int c);

    void GetWindowSize(int* width, int* height) const;
    const wxPoint& GetMousePos() const;
    bool IsScrolling() const;

    void StartEarthquake(int amount, int jitterInterval);
    void StopEarthquake();

protected:
    // TGameControl embeds an actual TSceneControl by value and points this
    // at it (TMasterControl's own constructor never sets it - see
    // gameControl.cpp); protected rather than private for that reason.
    TSceneControl* m_sceneControl = nullptr;

private:
    TMovie m_movie;
    TGObjectManager m_objectManager;
    TSprite m_sprite1;
    TSprite m_sprite2;

    std::vector<TMouseEventHandler> m_mouseEventHandlers;
    std::vector<TKeyboardEventHandler> m_keyboardEventHandlers;
    std::vector<std::string> m_engineEventHandlerNames;

    // Two std::list members confirmed present (self-referential empty-list
    // sentinel init in the constructor); only the second is used by any
    // method reconstructed so far (DrawInterfaces). Element type unclear -
    // the destructor symbol IDA shows for both ("ctdrpc::earlyrole_ip_t",
    // an unrelated networking type) is almost certainly another
    // identical-code-folding artifact like the ones in NOTES.md.
    std::list<void*> m_unknownList;
    std::list<void*> m_interfaces;

    int m_windowWidth = 0;
    int m_windowHeight = 0;
    TVisionaire* m_visionaire = nullptr;
    SLoadingScreen m_loadingScreen;
    TCursorControl* m_cursorControl = nullptr;
    TGameController* m_gameController = nullptr;
    TLoadingControl* m_loadingControl = nullptr;
    TSoundFFMPEG* m_soundManager = nullptr;
    TFontManager* m_fontManager = nullptr;
    TGameClientSDK* m_gameClientSDK = nullptr;

    void* m_movieEventHandler = nullptr;  // +0xC0 in the original; real type/purpose unconfirmed
    bool m_videoPlaying = false;          // +0xB8
    int m_loadingScreenCachedWidth = 0;   // +0x248
    int m_loadingScreenCachedHeight = 0;  // +0x24C
    wxPoint m_mousePos;                   // +0x258
    bool m_isScrolling = false;           // +0x260
    bool m_quitGame = false;              // +0x255
    bool m_clearMessage = false;          // +0x256
    bool m_earthquakeActive = false;      // +0x261
    int m_earthquakeAmount = 0;           // +0x264
    int m_earthquakeJitterInterval = 0;   // +0x268
    int m_earthquakeOffsetX = 0;          // +0x26C
    int m_earthquakeOffsetY = 0;          // +0x270
    TTimer m_earthquakeTimer;             // +0x278
    bool m_moviesEnabled = true;          // +0x288
    // Set from a TVisObjRef::GetBool(0x33A) field at the top of every
    // ScrollUpdate() call, then read back inside its easing formula to pick
    // which of two target speeds to ease toward - real meaning (some kind
    // of "scrolling direction/mode" flag) not resolved. +0x254 in the
    // original; unrelated to m_moviesEnabled (+0x288) despite both being a
    // lone bool set near the start of a method.
    bool m_easeDirectionFlag = false;

    // ScrollUpdate's eased scroll speed; a plain global (`xspeed`) in the
    // original, made an instance member here since nothing needs it shared.
    float m_xspeed = 0.0f;
    // A function-local static TTimer in the original
    // (ScrollUpdate(void)::scrollTimer, magic-statics-initialized on first
    // call) - a plain member here since TMasterControl is effectively a
    // singleton anyway.
    TTimer m_scrollTimer;
};
