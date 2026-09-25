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
static void LuaDoStringStub(const std::string& /*script*/, const std::string& /*chunkName*/) {
}

TMasterControl::TMasterControl() {
    m_cursorControl = new TCursorControl();
    m_gameController = new TGameController();
    m_loadingControl = new TLoadingControl();
    m_soundManager = new TSoundFFMPEG();
    m_fontManager = new TFontManager();
    m_gameClientSDK = new TGameClientSDK();
    // m_sceneControl is deliberately left null here: there's no confirmed
    // evidence TMasterControl's own constructor sets it (see its
    // declaration in masterControl.h). TGameControl embeds an actual
    // TSceneControl by value and points this at it - a previous version of
    // this constructor also heap-allocated one here as a filler guess,
    // which caused ~TMasterControl() to `delete` TGameControl's non-heap
    // member and corrupt the heap. Only the owning subclass should manage
    // this pointer's lifetime.
    m_visionaire = new TVisionaire();
    m_moviesEnabled = true;
}

TMasterControl::~TMasterControl() {
    delete m_cursorControl;
    delete m_gameController;
    delete m_loadingControl;
    delete m_soundManager;
    delete m_fontManager;
    delete m_gameClientSDK;
    delete m_visionaire;
}

void TMasterControl::QuitGame() {
    m_quitGame = true;
}

bool TMasterControl::GetQuitGame() const {
    return m_quitGame;
}

void TMasterControl::SetClearMessage() {
    m_clearMessage = true;
}

bool TMasterControl::GetClearMessage() {
    bool result = m_clearMessage;
    m_clearMessage = false;
    return result;
}

TPaintControl* TMasterControl::GetMainControl() {
    return m_sceneControl->GetScene();
}

TCursorControl* TMasterControl::GetCursorControl() {
    return m_cursorControl;
}

TGameController* TMasterControl::GetGameController() {
    return m_gameController;
}

TSoundFFMPEG* TMasterControl::GetSoundManager() const {
    return m_soundManager;
}

TFontManager* TMasterControl::GetFontManager() {
    return m_fontManager;
}

TGameClientSDK* TMasterControl::GetGameClientSDK() const {
    return m_gameClientSDK;
}

void TMasterControl::SetLoadingScreen(SLoadingScreen& screen) {
    m_loadingScreen = screen;
}

void TMasterControl::ShowLoadingScreen() {
    if (!m_loadingControl)
        return;
    m_loadingScreenCachedWidth = m_windowWidth;
    m_loadingScreenCachedHeight = m_windowHeight;
    // Real TLoadingControl::Init(SLoadingScreen&, TSoundInterface*) not
    // reversed yet; TSoundInterface appears to be an interface TSoundFFMPEG
    // implements, so passing it directly here is a reasonable placeholder.
}

void TMasterControl::CleanUp() {
    // Empty in the original (a single `retn`).
}

void TMasterControl::EnableMovies(bool enable) {
    m_moviesEnabled = enable;
}

void TMasterControl::DrawInterfaces() {
    // The original gates this on several TVisObjRef::GetBool() field-id
    // checks (ids 0x274, 0x315, 0x1DF, 0x124) whose meaning isn't resolved
    // (see datastruct/visobjref.h) - reproduced structurally: only draw the
    // registered interfaces when the game data says to.
    TVisObjRef game = m_visionaire->GetGame();
    TVisObjRef link = game.GetLink(0x1D5);
    bool shouldDraw = link.IsEmpty() ? link.GetBool(0x124) : !link.GetBool(0x274);
    if (!shouldDraw)
        return;

    for (TGInterface* interface : m_activeInterfaces)
        interface->Draw();
}

bool TMasterControl::Draw(bool showActionText) {
    wxPoint savedScrollPos{};
    if (m_earthquakeActive) {
        TPaintControl* scene = m_sceneControl->GetScene();
        savedScrollPos = scene->GetScrollPos();

        if (m_earthquakeTimer.GetTime() > m_earthquakeJitterInterval) {
            int range = m_earthquakeAmount * 2;
            m_earthquakeOffsetX = range ? (std::rand() % range - m_earthquakeAmount) : 0;
            m_earthquakeOffsetY = range ? (std::rand() % range - m_earthquakeAmount) : 0;
            m_earthquakeTimer.SetTime();
        }

        wxPoint jittered{savedScrollPos.x - m_earthquakeOffsetX, savedScrollPos.y - m_earthquakeOffsetY};
        m_sceneControl->GetScene()->SetScrollPos(jittered);
    }

    if (!showActionText) {
        for (const std::string& script : s_luaDrawBeforeScene)
            LuaDoStringStub(script, script);

        m_sceneControl->Draw();

        for (const std::string& script : s_luaDrawAfterScene)
            LuaDoStringStub(script, script);

        TVisObjRef game = m_visionaire->GetGame();
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
            TPaintControl* scene = m_sceneControl->GetScene();
            if (scene->IsActive() && !DisplayTexts()) {
                SetCurrent();
                if (m_cursorControl->IsActive())
                    m_cursorControl->Draw();
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
                m_fontManager->SetCurrentFont(link);
                wxString text = m_objectManager.GetActionText();
                wxPoint textSize;
                m_fontManager->GetTextDimension(text, textSize);
                wxPoint drawPos = m_cursorControl->GetPositionNextToCursor();
                if (drawPos.x + textSize.x > m_windowWidth)
                    drawPos.x = m_windowWidth - textSize.x;
                if (drawPos.y + textSize.y > m_windowHeight)
                    drawPos.y = m_windowHeight - textSize.y;
                m_fontManager->PrintText(text, TextAlignmentEnum::Left, drawPos, 1.0f);
            }
        }

        for (const std::string& script : s_luaDrawAfterInterfaces)
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
    if (m_quitGame)
        return;

    TVisObjRef game = m_visionaire->GetGame();
    m_easeDirectionFlag = game.GetBool(0x33A);
    m_isScrolling = false;

    TPaintControl* scene = m_sceneControl->GetScene();
    if (!scene->IsScrollable())
        return;

    int worktopWidth = scene->GetWorktopWidth();
    FloatPoint floatScroll = scene->GetFloatScrollPos();
    (void)floatScroll;  // used by the real easing formula; not reproduced here (see below)
    int targetX = game.GetInt(0x2B1);   // field id not resolved - a scroll target x position
    int edgeMargin = game.GetInt(0x2B2);  // field id not resolved - an edge-scroll trigger margin

    constexpr float kEaseFactor = 0.1f;
    constexpr float kStartSpeed = 1.0f;

    if (targetX >= m_mousePos.x || worktopWidth - targetX <= m_mousePos.x) {
        m_isScrolling = true;
    }
    bool nearEdge = m_mousePos.x < edgeMargin || m_mousePos.x > worktopWidth - edgeMargin;

    if (m_isScrolling || nearEdge) {
        if (m_scrollTimer.GetTime() > 500) {
            m_scrollTimer.SetTime();
        }
        float targetSpeed = m_easeDirectionFlag ? -kStartSpeed : kStartSpeed;
        m_xspeed = targetSpeed + (m_xspeed - targetSpeed) * kEaseFactor;
        scene->AdjustWindowHorizontal(m_xspeed * 0.001f);
    }
}

int TMasterControl::PlayAVI(const wxFileName& file, bool /*skippable*/, HandleSoundsEnum /*handleSounds*/) {
    // Gathers ~20 configuration fields from the "Game" object (subtitle
    // text/position, letterbox picture overlay, colors) via TVisObjRef,
    // then presumably calls into TMovie's real playback API and pumps
    // events until the clip finishes or is skipped. TMovie's actual Play()
    // signature isn't known (only Initialize/OneFrame are reversed), so
    // this stops short of a full reconstruction - see NOTES.md.
    if (!m_moviesEnabled)
        return 0;
    if (!file.IsOk())
        return 0;

    m_movie.Initialize(false);
    graphics->ResetMatrix(true, false);
    m_sceneControl->Draw();
    DrawInterfaces();
    SetCurrent();

    while (!m_movie.OneFrame()) {
        // Real loop pumps SDL events for a skip key/click and re-renders;
        // not reconstructed.
    }
    return 1;
}

bool TMasterControl::IsVideoPlaying() const {
    return m_videoPlaying;
}

bool TMasterControl::VideoFrame() {
    if (m_movie.OneFrame())
        return true;

    m_videoPlaying = false;
    // m_movieEventHandler's real type/virtual interface isn't reversed;
    // the original calls its vtable slot 1 here.

    if (/* field +0x4C, likely TPaintControl-internal state - not resolved */ false) {
        m_soundManager->OnVideoFrameFinished();
    }
    return false;
}

void TMasterControl::MovieEvent(void* sdlEvent) {
    // This turned out to double as the SDL controller-hotplug handler (its
    // body calls TGameController::AddGameController/RemoveGameController
    // per the disassembly's CODE XREFs), alongside video-overlay event
    // handling. Full reconstruction needs the real SDL_Event layout and
    // TMovie's event model, neither reversed yet.
    (void)sdlEvent;
}

bool TMasterControl::UnregisterEngineEventHandler(const wxString& name) {
    for (size_t i = 0; i < m_engineEventHandlerNames.size(); ++i) {
        wxString converted;
        toUTF(&converted, m_engineEventHandlerNames[i].c_str());
        if (converted.ToStdWstring() == name.ToStdWstring()) {
            m_engineEventHandlerNames.erase(m_engineEventHandlerNames.begin() + static_cast<long>(i));
            return true;
        }
    }
    return false;
}

bool TMasterControl::UnregisterKeyboardEventHandler(const wxString& name) {
    for (size_t i = 0; i < m_keyboardEventHandlers.size(); ++i) {
        if (m_keyboardEventHandlers[i].name.ToStdWstring() == name.ToStdWstring()) {
            m_keyboardEventHandlers.erase(m_keyboardEventHandlers.begin() + static_cast<long>(i));
            return true;
        }
    }
    return false;
}

bool TMasterControl::UnregisterMouseEventHandler(const wxString& name) {
    for (size_t i = 0; i < m_mouseEventHandlers.size(); ++i) {
        if (m_mouseEventHandlers[i].name.ToStdWstring() == name.ToStdWstring()) {
            m_mouseEventHandlers.erase(m_mouseEventHandlers.begin() + static_cast<long>(i));
            return true;
        }
    }
    return false;
}

void TMasterControl::RegisterEngineEventHandler(const wxString& name) {
    for (const std::string& existing : m_engineEventHandlerNames) {
        wxString converted;
        toUTF(&converted, existing.c_str());
        if (converted.ToStdWstring() == name.ToStdWstring())
            return;
    }
    m_engineEventHandlerNames.push_back(std::string(static_cast<const char*>(name.mb_str())));
}

void TMasterControl::RegisterMouseEventHandler(const wxString& name, const std::vector<int>& filter) {
    for (const auto& handler : m_mouseEventHandlers) {
        if (handler.name.ToStdWstring() == name.ToStdWstring())
            return;
    }
    TMouseEventHandler entry;
    entry.name = name;
    entry.mouseButtonFilter.assign(filter.begin(), filter.end());
    m_mouseEventHandlers.push_back(std::move(entry));
}

void TMasterControl::RegisterKeyboardEventHandler(const wxString& name) {
    for (const auto& handler : m_keyboardEventHandlers) {
        if (handler.name.ToStdWstring() == name.ToStdWstring())
            return;
    }
    m_keyboardEventHandlers.push_back(TKeyboardEventHandler{name});
}

void TMasterControl::ProcessMessage(TMouseMessageEnum msg, const wxPoint& pos) {
    for (auto& handler : m_mouseEventHandlers) {
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

void TMasterControl::ProcessMessage(TMouseMessageEnum msg, const wxPoint& pos, float /*a*/, float /*b*/, int /*c*/) {
    // Not traced in detail; presumed to follow the same dispatch pattern as
    // the (msg, pos) overload with extra TArgument::Set() calls for the
    // additional float/int payload (e.g. mouse wheel delta).
    ProcessMessage(msg, pos);
}

void TMasterControl::GetWindowSize(int* width, int* height) const {
    *width = m_windowWidth;
    *height = m_windowHeight;
}

const wxPoint& TMasterControl::GetMousePos() const {
    return m_mousePos;
}

bool TMasterControl::IsScrolling() const {
    return m_isScrolling;
}

void TMasterControl::StartEarthquake(int amount, int jitterInterval) {
    m_earthquakeActive = true;
    m_earthquakeAmount = amount;
    m_earthquakeJitterInterval = jitterInterval <= 0 ? 1 : jitterInterval;
    int range = amount * 2;
    m_earthquakeOffsetX = range ? (std::rand() % range - amount) : 0;
    m_earthquakeOffsetY = range ? (std::rand() % range - amount) : 0;
    m_earthquakeTimer.SetTime();
}

void TMasterControl::StopEarthquake() {
    m_earthquakeActive = false;
}

void TMasterControl::Signal(const TSignalData& signal, TSignalData& result) {
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
        if (m_loadingControl) {
            m_loadingControl->UpdateStatus(signal.loadingCurrent, signal.loadingTotal);
            graphics->ResetMatrix(true, false);
            m_loadingControl->Draw();
            graphics->SetMatrixMode(true, true);
        }
        return;
    default:
        x_assert(false, "false", "/home/simon/Documents/jenkins/branchPillars/src/vsplayer/control/masterControl.cpp",
                 118);
        return;
    }
}
