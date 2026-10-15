#include "TSceneControl.h"

#include "AppGlobals.h"
#include "TGCharacter.h"
#include <algorithm>

#include "THScene.h"
#include "TSoundFFMPEG.h"
#include "graphicslib/graphics.h"
#include "vsplayer/control/masterControl.h"
#include "vsplayer/control/gameControl.h"
#include "vstables/fieldIds.h"

// TGameControl implements every accessor used below, but g_pGameControl is
// only declared as TMasterControl* (AppGlobals.h) - same cast already
// established at TManagedObject::ClickedWithoutReach's own call site.
static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

TSceneControl::TSceneControl() {
	_currentScene = new THScene();
	_oldScene = new THScene();
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

// Confirmed (asm lines 455548-455606): while the scene is faded over the old one, the old scene is drawn first (into
// the part of the frame that the backend keeps for it: BeforeDrawScene(false, 1), and (false, 2) when it is done) and
// then the new one; BeforeDrawScene(false, 0) puts the frame back.
void TSceneControl::Draw() {
	if (_hasOldScene) {
		graphics->BeforeDrawScene(false, 1);
		_drawingOldScene = true;
		_oldScene->Draw();
		_drawingOldScene = false;
		graphics->BeforeDrawScene(false, 2);
	}

	_currentScene->Draw();

	if (_hasOldScene)
		graphics->BeforeDrawScene(false, 0);
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
	TVisObjRef currentCharacter = game.GetLink(kGameScrollCharacter);
	gameControl()->ScrollToCharacterIfNeeded(currentCharacter);
}

void TSceneControl::ChangeScene(const TVisObjRef &character, const TVisObjRef &target, bool immediate,
                                int direction) {
	TVisObjRef sceneTarget = target.GetParent();
	if (sceneTarget.IsEmpty())
		return;
	wxPoint pos = *target.GetPoint(kObjectPosition);
	if (direction == -1)
		direction = TGCharacter::GetOppositeDirection(target.GetInt(kObjectDirection));
	TVisObjRef characterRef = character;
	if (characterRef.IsEmpty()) {
		TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
		characterRef = game.GetLink(kGameCurrentCharacter);
	}
	ToScene(immediate, sceneTarget, characterRef, pos, direction, false);
	TVisObjRef game2 = gameControl()->GetVisionaire()->GetGame();
	TVisObjRef currentCharacter = game2.GetLink(kGameScrollCharacter);
	gameControl()->ScrollToCharacterIfNeeded(currentCharacter);
}

void TSceneControl::ChangeScene(const TVisObjRef &character, TVisObjRef &scene, bool immediate,
                                const wxPoint &pos, int direction) {
	if (scene.IsEmpty())
		return;
	TVisObjRef characterRef = character;
	if (characterRef.IsEmpty()) {
		TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
		characterRef = game.GetLink(kGameCurrentCharacter);
	}
	ToScene(immediate, scene, characterRef, pos, direction, false);
	TVisObjRef game2 = gameControl()->GetVisionaire()->GetGame();
	TVisObjRef currentCharacter = game2.GetLink(kGameScrollCharacter);
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

// Confirmed (asm lines 454129-454880): the change to the scene `scene`. `character` (if there is one) is put in it at `pos`
// looking `direction`. `fade`: the game's fade effect (kGameFadeEffect, for kGameFadeDelay milliseconds) is played;
// `flag` (the scene is shown, not a character sent) keeps the music of the old scene when the scene says so
// (kSceneContinueMusic). The effects, with a fade: 1 the new scene comes out of the dark, 2 the old one goes into the dark
// first, 3 both (half the time each), 7 the same with an iris, the others (4, 5, 6, 8, 9, 11 ...) the old scene is kept
// and the new one is faded in over it (the scene control draws both, see Draw()). The background music follows
// the scene (and fades with the effect), the audio busses of the scene are switched on.
void TSceneControl::ToScene(bool fade, TVisObjRef &scene, const TVisObjRef &character, const wxPoint &pos, int direction,
                            bool flag) {
	TGameControl *control = gameControl();
	TVisObjRef game = control->GetVisionaire()->GetGame();
	const int effect = game.GetInt(kGameFadeEffect);
	const int fadeTime = game.GetInt(kGameFadeDelay);
	TSoundFFMPEG *sounds = control->GetSoundManager();

	_fadingToNewScene = true;
	control->GetCursorControl()->SetActive(false);

	if (!flag && scene.GetBool(kSceneContinueMusic))
		scene.SetValue(kSceneBackgroundMusic, _currentScene->GetRef().GetPath(kSceneBackgroundMusic), TSendEventEnum::kNoEvent);

	sounds->KeepCurrentBackgroundMusicIfSameAs(scene.GetPath(kSceneBackgroundMusic));

	// a menu is entered: the picture the savegames show is made while the game is still on the screen
	if (!_currentScene->GetRef().GetBool(kSceneIsMenu) && scene.GetBool(kSceneIsMenu))
		graphics->CreateSavegameScreenshot(false);

	if (effect != 0 && fade) {
		sounds->FinishSoundFade();

		if (effect == 2) {
			sounds->StartSoundFade(TFadeEnum::kOut, fadeTime, true);
			graphics->FadeIn(2, fadeTime, nullptr, nullptr, sounds, control);
		} else {
			if (effect == 3 || effect == 7) {
				sounds->StartSoundFade(TFadeEnum::kIn, fadeTime / 2, true);
				sounds->StartSoundFade(TFadeEnum::kOut, fadeTime / 2, true);
			}

			if (effect == 3)
				graphics->FadeIn(2, fadeTime / 2, nullptr, nullptr, sounds, control);
			else if (effect == 7)
				graphics->FadeIn(8, fadeTime / 2, nullptr, nullptr, sounds, control);
			else if (effect != 1)
				_hasOldScene = true;
		}
	}

	// the new scene is the one that was the old one; it is made the scene `scene`
	std::swap(_currentScene, _oldScene);
	_currentScene->SetRef(scene);
	static_cast<THScene *>(_currentScene)->RegisterEvents(scene);
	game = control->GetVisionaire()->GetGame();
	game.SetLink(kGameCurrentScene, scene, false);

	if (!character.IsEmpty())
		_currentScene->InitialiseCharacter(character, pos, direction, scene);

	if (_hasOldScene) {
		_currentScene->SetCharacters();
		_currentScene->SortAllObjects();
	} else {
		static_cast<THScene *>(_oldScene)->UnregisterEvents();
		_oldScene->EndScene();
	}

	_currentScene->BeginScene();
	_currentScene->BeforeFade();
	sounds->FinishSoundFade();

	TVList busses;

	scene.GetLinks(kSceneAudioBusses, TypeOrder::kValue0, busses);
	sounds->BusActivate(&busses);

	wxFileName music = scene.GetPath(kSceneBackgroundMusic);

	if (music.GetName().IsEmpty())
		sounds->StopBackgroundMusic(effect != 2);
	else
		sounds->Play(music, scene.GetInt(kSceneMusicVolume), scene.GetInt(kSceneMusicBalance), true, TSoundTypeEnum::kMusic,
		             effect != 2, 0);

	if (effect == 5 || effect == 6)
		sounds->StartSoundFade(TFadeEnum::kToNew, fadeTime, true);
	else if (effect == 3 || effect == 7)
		sounds->StartSoundFade(TFadeEnum::kIn, fadeTime / 2, true);
	else if (effect != 2)
		sounds->StartSoundFade(static_cast<TFadeEnum>(effect), fadeTime, true);

	if ((effect & ~2) != 0 && fade) {
		if (effect == 1)
			graphics->FadeIn(1, fadeTime, nullptr, nullptr, sounds, control);
		else if (effect == 3)
			graphics->FadeIn(1, fadeTime / 2, nullptr, nullptr, sounds, control);
		else if (effect == 7)
			graphics->FadeIn(9, fadeTime / 2, nullptr, nullptr, sounds, control);
		else
			graphics->FadeIn(effect, fadeTime, nullptr, nullptr, sounds, control);
	}

	// the old scene is over
	if (_hasOldScene) {
		_oldScene->EndScene();
		static_cast<THScene *>(_oldScene)->UnregisterEvents();
	}

	_hasOldScene = false;
	sounds->FinishSoundFade();
	control->GetCursorControl()->SetActive(!game.GetBool(kGameHideCursor));
	_fadingToNewScene = false;
	control->SetClearMessage();
}
