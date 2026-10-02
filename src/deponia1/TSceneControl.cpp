#include "TSceneControl.h"

#include "AppGlobals.h"
#include "TGCharacter.h"
#include "vsplayer/control/gameControl.h"

// TGameControl implements every accessor used below, but g_pGameControl is
// only declared as TMasterControl* (AppGlobals.h) - same cast already
// established at TManagedObject::ClickedWithoutReach's own call site.
static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

TSceneControl::TSceneControl() {
	_currentScene = new TGScene();
	_oldScene = new TGScene();
}

TSceneControl::~TSceneControl() {
	delete _currentScene;
	delete _oldScene;
}

TGScene *TSceneControl::GetScene() const {
	return _drawingOldScene ? _oldScene : _currentScene;
}

TGScene *TSceneControl::GetScene() {
	return _currentScene;
}

void TSceneControl::Draw() {
	// Confirmed calls into an unmodeled "graphics" global (vtable slot
	// 0x30, args (bool, int) - not reversed) bracket each of the draws
	// below; skipped, same as this project's other not-yet-integrated
	// external dependencies.
	if (_hasOldScene) {
		_drawingOldScene = true;
		_oldScene->Draw();
		_drawingOldScene = false;
	}
	_currentScene->Draw();
}

void TSceneControl::Set(const TVisObjRef &scene) {
	_currentScene->SetRef(scene);
}

void TSceneControl::ShowScene(TVisObjRef &scene, bool immediate, bool flag) {
	if (scene.IsEmpty())
		return;
	TVisObjRef empty;
	ToScene(immediate, scene, empty, wxPoint(), -1, flag);
	TVisObjRef game = gameControl()->GetVisionaire()->GetGame();
	TVisObjRef currentCharacter = game.GetLink(0x263);
	gameControl()->ScrollToCharacterIfNeeded(currentCharacter);
}

void TSceneControl::ChangeScene(const TVisObjRef &character, const TVisObjRef &target, bool immediate,
                                int direction) {
	TVisObjRef sceneTarget = target.GetParent();
	if (sceneTarget.IsEmpty())
		return;
	wxPoint pos = *target.GetPoint(0x153);
	if (direction == -1)
		direction = TGCharacter::GetOppositeDirection(target.GetInt(0xDE));
	TVisObjRef characterRef = character;
	if (characterRef.IsEmpty()) {
		TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
		characterRef = game.GetLink(0x1D4);
	}
	ToScene(immediate, sceneTarget, characterRef, pos, direction, false);
	TVisObjRef game2 = gameControl()->GetVisionaire()->GetGame();
	TVisObjRef currentCharacter = game2.GetLink(0x263);
	gameControl()->ScrollToCharacterIfNeeded(currentCharacter);
}

void TSceneControl::ChangeScene(const TVisObjRef &character, TVisObjRef &scene, bool immediate,
                                const wxPoint &pos, int direction) {
	if (scene.IsEmpty())
		return;
	TVisObjRef characterRef = character;
	if (characterRef.IsEmpty()) {
		TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
		characterRef = game.GetLink(0x1D4);
	}
	ToScene(immediate, scene, characterRef, pos, direction, false);
	TVisObjRef game2 = gameControl()->GetVisionaire()->GetGame();
	TVisObjRef currentCharacter = game2.GetLink(0x263);
	gameControl()->ScrollToCharacterIfNeeded(currentCharacter);
}

void TSceneControl::GetLastPlayableSceneParams(TVisObjRef &outScene, wxPoint &outPos) const {
	outScene = _ref;
	TGScene *scene = _drawingOldScene ? _oldScene : _currentScene;
	if (scene->IsMenu())
		outPos = _lastPlayableScrollPos;
	else
		outPos = scene->GetScrollPos();
}

bool TSceneControl::FadingToNewScene() const {
	return _fadingToNewScene;
}

bool TSceneControl::OldSceneIsMenu() const {
	return _oldScene->IsMenu();
}

void TSceneControl::SetNextStartScrollPos(const wxPoint &pos) {
	_nextStartScrollPos = pos;
}

void TSceneControl::ToScene(bool /*immediate*/, TVisObjRef &/*scene*/, const TVisObjRef &/*character*/,
                            const wxPoint &/*pos*/, int /*direction*/, bool /*flag*/) {
}
