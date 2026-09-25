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

TGameControl::~TGameControl() = default;

bool TGameControl::Update() {
    // The entire per-frame game-logic dispatcher - almost certainly
    // thousands of lines given the class's overall size. Not reversed.
    return true;
}

bool TGameControl::DisplayDialog() {
    return false;
}

bool TGameControl::DisplayTexts() {
    return false;
}

bool TGameControl::DisplayConsole() {
    return false;
}

void TGameControl::DisplayInSceneConsole() {
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
    return nullptr;
}

TGCharacter* TGameControl::GetCurrentCharacterPointer() const {
    return nullptr;
}

TGCharacter* TGameControl::GetCharacter(const TVisObjRef& /*character*/) {
    return nullptr;
}

TGCharacter* TGameControl::GetCharacterPointer(const TVisObjRef& /*character*/) const {
    return nullptr;
}

TGCharacter* TGameControl::GetCharacterPointerEx(const TVisObjRef& /*character*/) const {
    return nullptr;
}

std::vector<TGCharacter*> TGameControl::GetAllCharacters() {
    return m_characters;
}

void* TGameControl::GetInterface(const TVisObjRef& /*interfaceObj*/) const {
    return nullptr;
}

void* TGameControl::GetObject(const TVisObjRef& /*object*/) const {
    return nullptr;
}

TGObjectManager* TGameControl::GetObjectManager() {
    return nullptr;  // TMasterControl's m_objectManager is private; not exposed yet.
}

void TGameControl::SkipCurrentText() {
}

void TGameControl::UpdateCurrentObject() {
}

void TGameControl::RegisterHookFunctionSceneMousePosition(const wxString& /*name*/) {
}

TConsole* TGameControl::GetConsole() {
    return &m_console;
}

int TGameControl::ConvertControllerButtonToSymKey(SDL_ControllerButtonEvent /*button*/) {
    return 0;
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

wxString TGameControl::GetGamePath() const {
    return wxString();
}

bool TGameControl::IsClearingAnimations() const {
    return false;
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
