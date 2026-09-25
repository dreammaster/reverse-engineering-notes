// Reconstructed from Deponia_Linux.asm, TGameControl methods at asm lines
// 455671-478479 (address range 0x61A730-0x653C10+, ~22800 lines - by far
// the largest class in this survey; see manifest/proprietary_classes.tsv).
//
// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/vsplayer/control/gameControl.cpp - see manifest/source_layout.tsv.
//
// TGameControl is the concrete class TMasterControl is an abstract
// interface for - this is "the game" itself: it owns the scene, dialog and
// console subsystems, the loaded TVisionaireGame data, character/action/
// tween bookkeeping, and implements every one of TMasterControl's 8 pure
// virtuals (their real names - Update, DisplayDialog, DisplayTexts,
// DisplayConsole, DisplayInSceneConsole, HandleMouseMove, HandleMouseUp,
// HandleMouseHolding - came directly from `vtable for TGameControl`'s own
// dump, which names every slot; no guessing needed there, unlike when
// TMasterControl was first written against these before TGameControl had
// been looked at).
//
// Given the scale (98 methods), this pass is intentionally shallow almost
// everywhere: the constructor and destructor were traced in full (giving an
// accurate member layout and confirming the base-class chain), and a
// handful of trivial-looking accessors are implemented by inference from
// naming/context (GetSceneControl, GetScene, GetConsole, GetDialog,
// GetVisionaire, GetGameSystem, GetObjectManager, GetGamePath, and the two
// flag-setting methods TestCacheFileTime-style). Every other method has a
// correct signature (from manifest/proprietary_functions.tsv) so the class
// compiles completely, but only a stub body - each one (Update alone is
// almost certainly thousands of lines - it's the entire per-frame game
// logic dispatcher) is its own substantial reversing effort for a future
// pass.
#pragma once

#include <string>
#include <vector>

#include "SdlStub.h"
#include "TConsole.h"
#include "TGCharacter.h"
#include "TGDialog.h"
#include "Tween.h"
#include "TSceneControl.h"
#include "vscommon/fontManager.h"
#include "vsplayer/control/masterControl.h"
#include "vstables/visionaireGame.h"

struct SGameAction {
    int id = 0;
};

class TGameControl : public TMasterControl {
public:
    TGameControl();
    ~TGameControl() override;

    // TMasterControl's 8 pure virtuals - real names/signatures confirmed
    // from `vtable for TGameControl`.
    bool Update() override;
    bool DisplayDialog() override;
    bool DisplayTexts() override;
    bool DisplayConsole() override;
    void DisplayInSceneConsole() override;
    void HandleMouseMove(const wxPoint& pos, bool isHolding) override;
    void HandleMouseUp(const wxPoint& pos, TMouseMessageEnum msg) override;
    void HandleMouseHolding(const wxPoint& pos) override;

    void UpdateTexts();
    TSceneControl* GetSceneControl();
    TPaintControl* GetScene();
    TGCharacter* GetCurrentCharacter();
    TGCharacter* GetCurrentCharacterPointer() const;
    TGCharacter* GetCharacter(const TVisObjRef& character);
    TGCharacter* GetCharacterPointer(const TVisObjRef& character) const;
    TGCharacter* GetCharacterPointerEx(const TVisObjRef& character) const;
    // Confirmed (Deponia_Linux.asm line 456592, `lea rax,[rdi+340h]; retn`):
    // this returns the address of the member itself, not a by-value copy -
    // the manifest's inferred by-value signature was wrong.
    std::vector<TGCharacter*>& GetAllCharacters();
    void* GetInterface(const TVisObjRef& interfaceObj) const;
    void* GetObject(const TVisObjRef& object) const;
    TGObjectManager* GetObjectManager();
    void SkipCurrentText();
    void UpdateCurrentObject();
    void RegisterHookFunctionSceneMousePosition(const wxString& name);
    TConsole* GetConsole();

    int ConvertControllerButtonToSymKey(SDL_ControllerButtonEvent button);
    int ConvertControllerAxisToUnicode(SDL_GameControllerAxis axis);
    void StartGameAction(TKeyboardMessageEnum msg, const wxString& name, int a, unsigned short b);
    void UpdateAspectRatio();
    void InitAfterLoadingScreen();
    void SaveEventHandlers();
    void ExecuteStartingAction();
    void InitInterfaces();
    void SetCharacterInterfaces();
    void InitFonts();
    void InitScripts();
    TVisionaireGame* GetGameSystem();
    TVisionaireGame* GetVisionaire();
    void ScrollToCharacterIfNeeded(const TVisObjRef& character);
    void MoveScene();
    void CenterScene();
    void SetOnScrollDestination();
    void HandleCharacters();
    void SetAllCharactersOnDestination();
    void ResetState();
    const TGDialog* GetDialog() const;
    void StartDialog(const TVisObjRef& dialog);
    void EndDialog();
    void StartText(const TVisObjRef& text, TGCharacter* character, TextAlignmentEnum alignment,
                   const TVisObjRef& target, const wxPoint& pos);
    void StartBackgroundText(const TVisObjRef& text, TGCharacter* character, TextAlignmentEnum alignment,
                              const TVisObjRef& target, const wxPoint& pos);
    void ReattachSceneObjectTexts();
    bool IsTextActive(const TVisObjRef& text) const;
    bool IsNoTextDisplayed() const;
    bool IsTalking(const TVisObjRef& character) const;
    void ClearTexts();
    void ClearCurrentText();
    void ClearText(const TVisObjRef& text);
    void ClearObjectText(const TVisObjRef& object);
    void StartObjectText(const TVisObjRef& object, const TVisObjRef& text, TextAlignmentEnum alignment,
                          const TVisObjRef& target, const wxPoint& pos);
    // Confirmed (asm line 462534, `lea rax,[rdi+0A80h]; retn`): returns the
    // member itself, not a by-value copy - the manifest's inferred by-value
    // signature was wrong (same pattern as GetAllCharacters above).
    const wxString& GetGamePath() const;
    bool IsClearingAnimations() const;
    bool SavegameExists(int slot);
    bool DeleteSavegame(int slot);
    bool Save();
    bool SaveGame(int slot);
    bool UnregisterEventHandlerMainLoop(const wxString& name);
    void UpdateRandomTimers();
    void UpdateWalkingSounds();
    bool PreLoad(wxString& error, wxString& warning, bool isEditor);
    void AdjustInterfacesOnScreen(bool force, TPaintControl* scene);
    void SetInterfaces();
    void SetCharacterActiveCommand();
    void ChangeCharacter(const TVisObjRef& character, bool immediate, const TVisObjRef& scene);
    std::vector<void*> GetActiveInterfaces() const;
    std::vector<void*> GetAllInterfaces() const;
    void InitCharacters();
    void InitGameActions();
    bool Init();
    bool LoadAndInitGame(wxString& error, const wxString& file, wxString warning, bool isEditor);
    bool ReplaceGame(wxFileName file, bool isEditor);
    void HandleEngineEvent(const std::string& name, const std::string& arg);
    void HandleKeyEvent(TKeyboardMessageEnum msg, const wxString& key, int a, unsigned short b);
    void HandleControllerAxis(SDL_GameControllerAxis axis, int value, int index);
    void HandleControllerButtonRelease(SDL_ControllerButtonEvent button, int index);
    void HandleControllerButtonHit(SDL_ControllerButtonEvent button, int index);
    void PushEngineEvent(const std::string& name, const std::string& arg);
    void SetDelay(double seconds, const std::string& name);
    void SetDelay(double seconds, int id);
    void RegisterEventHandlerMainLoop(const wxString& name);
    void GetWalkingSounds(std::vector<wxFileName>& outSounds);
    void StartTween(const TVisObjTween& tween);
    void LoadEventHandlers();
    bool Load();
    bool LoadGame(void* savegame);
    bool LoadGame(int slot);
    void StartTween(const Tween& tween, const std::string& name);

private:
    TSceneControl m_ownedSceneControl;
    // Several vectors confirmed present in the constructor whose element
    // types could be inferred from the ICF-vulnerable destructor symbols
    // (see NOTES.md) - taken at face value here since nothing contradicts
    // them: pending tweens, walking-sound filenames, per-character
    // scroll-timing pairs, and registered scene-mouse-position hooks.
    std::vector<std::pair<Tween, std::string>> m_pendingTweens;
    std::vector<std::string> m_walkingSoundEventHandlers;
    std::vector<std::pair<double, std::string>> m_delaysByName;
    std::vector<std::pair<double, int>> m_delaysById;
    // Owning: ~TGameControl deletes every element (confirmed, asm lines
    // 474252-474270).
    std::vector<TGCharacter*> m_characters;
    // Confirmed (asm line 457015): RegisterHookFunctionSceneMousePosition
    // just assigns into a single wstring field via std::wstring::assign - a
    // plain "last registered hook name," not a map as first guessed.
    std::wstring m_sceneMousePositionHookName;
    TGDialog m_dialog;
    TConsole m_console;
    TVisionaireGame* m_visionaireGame = nullptr;
    std::vector<SGameAction> m_gameActions;
    std::vector<std::string> m_engineEventHandlerNamesMainLoop;
    // Confirmed embedded by value (asm line 456751, GetObjectManager():
    // `lea rax,[rdi+0D0h]; retn`) - not owned by TMasterControl as first
    // guessed.
    TGObjectManager m_objectManager;
    // Confirmed field accesses, not always-nullptr/false stubs (asm lines
    // 456332/456551/462534/462551): GetCurrentCharacter[Pointer] just reads
    // this back, GetGamePath returns m_gamePath by reference, and the
    // destructor sets m_isClearingAnimations = true before tearing down.
    TGCharacter* m_currentCharacter = nullptr;
    wxString m_gamePath;
    bool m_isClearingAnimations = false;
};
