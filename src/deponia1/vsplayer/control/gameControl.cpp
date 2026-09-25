#include "vsplayer/control/gameControl.h"

#include "AppGlobals.h"
#include "baselib/composedfile.h"

TGameControl::TGameControl() {
    // TMasterControl's own base subobject is constructed automatically.
    // TGameControl embeds a real TSceneControl and points TMasterControl's
    // (protected) m_sceneControl at it - TMasterControl's own constructor
    // never sets that pointer, so this is presumed to be TGameControl's
    // job (not otherwise confirmed - see masterControl.h).
    m_sceneControl = &m_ownedSceneControl;

    m_visionaireGame = new TVisionaireGame();

    // Confirmed: the constructor makes *this* the global game-controller
    // singleton itself, rather than leaving that to the caller.
    g_pGameControl = this;

    // Confirmed: registers a TComposedFile::onError handler (logs the
    // failing archive's exe-filename + the error string) - the handler
    // body itself wasn't traced in detail (see TComposedFile::onError's
    // declaration).
    TComposedFile::onError = [](TComposedFile* /*file*/, std::string /*message*/) {};
}

TGameControl::~TGameControl() {
    // Confirmed (asm lines 474222-474270): sets m_isClearingAnimations
    // before tearing down, then deletes every owned TGCharacter*. The
    // destructor also calls into several global subsystems not modeled
    // here at all (TGAnimation::ClearAnimations, ModelContainer::Destroy,
    // TGAction::ClearActions) and a virtual teardown call through a
    // pointer at a still-unidentified field - left out rather than
    // guessing at systems that haven't been reversed yet.
    m_isClearingAnimations = true;
    ClearTexts();
    for (TGCharacter* character : m_characters)
        delete character;
    m_characters.clear();
}

bool TGameControl::Update() {
    // The entire per-frame game-logic dispatcher - almost certainly
    // thousands of lines given the class's overall size. Not reversed.
    return true;
}

bool TGameControl::DisplayDialog() {
    // Confirmed (asm lines 455780-455803): only draws/activates the cursor
    // when a dialog is actually active (m_dialog's TVisObjRef target is
    // non-empty).
    if (m_dialog.IsEmpty())
        return false;
    GetCursorControl()->SetActive(true);
    m_dialog.Draw();
    return true;
}

bool TGameControl::DisplayTexts() {
    return false;
}

bool TGameControl::DisplayConsole() {
    // Confirmed tail-call (asm lines 455750-455756): DisplayConsole() is
    // exactly TConsole::Draw() on the embedded console.
    return m_console.Draw();
}

void TGameControl::DisplayInSceneConsole() {
    // Confirmed tail-call (asm lines 455758-455764).
    m_console.DrawInScene();
}

void TGameControl::HandleMouseMove(const wxPoint& /*pos*/, bool /*isHolding*/) {
}

void TGameControl::HandleMouseUp(const wxPoint& /*pos*/, TMouseMessageEnum /*msg*/) {
}

void TGameControl::HandleMouseHolding(const wxPoint& /*pos*/) {
}

void TGameControl::UpdateTexts() {
}

TSceneControl* TGameControl::GetSceneControl() {
    return &m_ownedSceneControl;
}

TPaintControl* TGameControl::GetScene() {
    return m_ownedSceneControl.GetScene();
}

TGCharacter* TGameControl::GetCurrentCharacter() {
    // Confirmed (asm line 456332): plain field access, not always-nullptr.
    return m_currentCharacter;
}

TGCharacter* TGameControl::GetCurrentCharacterPointer() const {
    // Confirmed (asm line 456349): same field as GetCurrentCharacter().
    return m_currentCharacter;
}

// GetCharacter/GetCharacterPointer/GetCharacterPointerEx (asm lines
// 456362-456578): all three share one pattern - if the TVisObjRef arg
// IsEmpty(), return m_currentCharacter; otherwise pack TVisObjRef::GetId()'s
// first 3 bytes into a 32-bit hash, look it up in a custom open-hashing
// table (buckets, bucket count, and a parallel index into m_characters -
// not yet reversed fields), and return m_characters[index] on a match (or
// m_currentCharacter/nullptr on a miss, matching each method's slightly
// different fallback). Left as stubs rather than guessing at TVisObjRef's
// real id encoding or the hash table's field layout.
TGCharacter* TGameControl::GetCharacter(const TVisObjRef& character) {
    return character.IsEmpty() ? m_currentCharacter : nullptr;
}

TGCharacter* TGameControl::GetCharacterPointer(const TVisObjRef& character) const {
    return character.IsEmpty() ? m_currentCharacter : nullptr;
}

TGCharacter* TGameControl::GetCharacterPointerEx(const TVisObjRef& /*character*/) const {
    return nullptr;
}

std::vector<TGCharacter*>& TGameControl::GetAllCharacters() {
    return m_characters;
}

void* TGameControl::GetInterface(const TVisObjRef& /*interfaceObj*/) const {
    return nullptr;
}

void* TGameControl::GetObject(const TVisObjRef& /*object*/) const {
    return nullptr;
}

TGObjectManager* TGameControl::GetObjectManager() {
    // Confirmed (asm line 456751): embedded by value in TGameControl itself,
    // not TMasterControl as first guessed.
    return &m_objectManager;
}

void TGameControl::SkipCurrentText() {
}

void TGameControl::UpdateCurrentObject() {
}

void TGameControl::RegisterHookFunctionSceneMousePosition(const wxString& name) {
    // Confirmed (asm line 457015): a tail-call to std::wstring::assign on a
    // single field - not a map as first guessed.
    m_sceneMousePositionHookName = name.ToStdWstring();
}

TConsole* TGameControl::GetConsole() {
    return &m_console;
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

int TGameControl::ConvertControllerAxisToUnicode(SDL_GameControllerAxis /*axis*/) {
    return 0;
}

void TGameControl::StartGameAction(TKeyboardMessageEnum /*msg*/, const wxString& /*name*/, int /*a*/,
                                    unsigned short /*b*/) {
}

void TGameControl::UpdateAspectRatio() {
}

void TGameControl::InitAfterLoadingScreen() {
}

void TGameControl::SaveEventHandlers() {
}

void TGameControl::ExecuteStartingAction() {
}

void TGameControl::InitInterfaces() {
}

void TGameControl::SetCharacterInterfaces() {
}

void TGameControl::InitFonts() {
}

void TGameControl::InitScripts() {
}

TVisionaireGame* TGameControl::GetGameSystem() {
    return m_visionaireGame;
}

TVisionaireGame* TGameControl::GetVisionaire() {
    return m_visionaireGame;
}

void TGameControl::ScrollToCharacterIfNeeded(const TVisObjRef& /*character*/) {
}

void TGameControl::MoveScene() {
}

void TGameControl::CenterScene() {
}

void TGameControl::SetOnScrollDestination() {
}

void TGameControl::HandleCharacters() {
}

void TGameControl::SetAllCharactersOnDestination() {
}

void TGameControl::ResetState() {
}

const TGDialog* TGameControl::GetDialog() const {
    return &m_dialog;
}

void TGameControl::StartDialog(const TVisObjRef& /*dialog*/) {
}

void TGameControl::EndDialog() {
}

void TGameControl::StartText(const TVisObjRef& /*text*/, TGCharacter* /*character*/, TextAlignmentEnum /*alignment*/,
                              const TVisObjRef& /*target*/, const wxPoint& /*pos*/) {
}

void TGameControl::StartBackgroundText(const TVisObjRef& /*text*/, TGCharacter* /*character*/,
                                        TextAlignmentEnum /*alignment*/, const TVisObjRef& /*target*/,
                                        const wxPoint& /*pos*/) {
}

void TGameControl::ReattachSceneObjectTexts() {
}

bool TGameControl::IsTextActive(const TVisObjRef& /*text*/) const {
    return false;
}

bool TGameControl::IsNoTextDisplayed() const {
    return true;
}

bool TGameControl::IsTalking(const TVisObjRef& /*character*/) const {
    return false;
}

void TGameControl::ClearTexts() {
}

void TGameControl::ClearCurrentText() {
}

void TGameControl::ClearText(const TVisObjRef& /*text*/) {
}

void TGameControl::ClearObjectText(const TVisObjRef& /*object*/) {
}

void TGameControl::StartObjectText(const TVisObjRef& /*object*/, const TVisObjRef& /*text*/,
                                    TextAlignmentEnum /*alignment*/, const TVisObjRef& /*target*/,
                                    const wxPoint& /*pos*/) {
}

const wxString& TGameControl::GetGamePath() const {
    // Confirmed (asm line 462534): returns the member by reference.
    return m_gamePath;
}

bool TGameControl::IsClearingAnimations() const {
    // Confirmed (asm line 462551): plain field access, set true by the
    // destructor before it tears anything down (see ~TGameControl below).
    return m_isClearingAnimations;
}

bool TGameControl::SavegameExists(int /*slot*/) {
    return false;
}

bool TGameControl::DeleteSavegame(int /*slot*/) {
    return false;
}

bool TGameControl::Save() {
    return false;
}

bool TGameControl::SaveGame(int /*slot*/) {
    return false;
}

bool TGameControl::UnregisterEventHandlerMainLoop(const wxString& name) {
    for (size_t i = 0; i < m_engineEventHandlerNamesMainLoop.size(); ++i) {
        wxString converted;
        toUTF(&converted, m_engineEventHandlerNamesMainLoop[i].c_str());
        if (converted.ToStdWstring() == name.ToStdWstring()) {
            m_engineEventHandlerNamesMainLoop.erase(m_engineEventHandlerNamesMainLoop.begin() +
                                                     static_cast<long>(i));
            return true;
        }
    }
    return false;
}

void TGameControl::UpdateRandomTimers() {
}

void TGameControl::UpdateWalkingSounds() {
}

bool TGameControl::PreLoad(wxString& /*error*/, wxString& /*warning*/, bool /*isEditor*/) {
    return false;
}

void TGameControl::AdjustInterfacesOnScreen(bool /*force*/, TPaintControl* /*scene*/) {
}

void TGameControl::SetInterfaces() {
}

void TGameControl::SetCharacterActiveCommand() {
}

void TGameControl::ChangeCharacter(const TVisObjRef& /*character*/, bool /*immediate*/, const TVisObjRef& /*scene*/) {
}

std::vector<void*> TGameControl::GetActiveInterfaces() const {
    return {};
}

std::vector<void*> TGameControl::GetAllInterfaces() const {
    return {};
}

void TGameControl::InitCharacters() {
}

void TGameControl::InitGameActions() {
}

bool TGameControl::Init() {
    return true;
}

bool TGameControl::LoadAndInitGame(wxString& /*error*/, const wxString& /*file*/, wxString /*warning*/,
                                    bool /*isEditor*/) {
    return false;
}

bool TGameControl::ReplaceGame(wxFileName /*file*/, bool /*isEditor*/) {
    return false;
}

void TGameControl::HandleEngineEvent(const std::string& /*name*/, const std::string& /*arg*/) {
}

void TGameControl::HandleKeyEvent(TKeyboardMessageEnum /*msg*/, const wxString& /*key*/, int /*a*/,
                                   unsigned short /*b*/) {
}

void TGameControl::HandleControllerAxis(SDL_GameControllerAxis /*axis*/, int /*value*/, int /*index*/) {
}

void TGameControl::HandleControllerButtonRelease(SDL_ControllerButtonEvent /*button*/, int /*index*/) {
}

void TGameControl::HandleControllerButtonHit(SDL_ControllerButtonEvent /*button*/, int /*index*/) {
}

void TGameControl::PushEngineEvent(const std::string& /*name*/, const std::string& /*arg*/) {
}

void TGameControl::SetDelay(double seconds, const std::string& name) {
    m_delaysByName.push_back({seconds, name});
}

void TGameControl::SetDelay(double seconds, int id) {
    m_delaysById.push_back({seconds, id});
}

void TGameControl::RegisterEventHandlerMainLoop(const wxString& name) {
    for (const std::string& existing : m_engineEventHandlerNamesMainLoop) {
        wxString converted;
        toUTF(&converted, existing.c_str());
        if (converted.ToStdWstring() == name.ToStdWstring())
            return;
    }
    m_engineEventHandlerNamesMainLoop.push_back(std::string(static_cast<const char*>(name.mb_str())));
}

void TGameControl::GetWalkingSounds(std::vector<wxFileName>& /*outSounds*/) {
}

void TGameControl::StartTween(const TVisObjTween& /*tween*/) {
}

void TGameControl::LoadEventHandlers() {
}

bool TGameControl::Load() {
    return false;
}

bool TGameControl::LoadGame(void* /*savegame*/) {
    return false;
}

bool TGameControl::LoadGame(int /*slot*/) {
    return false;
}

void TGameControl::StartTween(const Tween& tween, const std::string& name) {
    m_pendingTweens.push_back({tween, name});
}
