#include "THScene.h"

#include "AppGlobals.h"
#include "TGCharacter.h"
#include "TSoundFFMPEG.h"
#include "vsplayer/control/gameControl.h"
#include "vstables/fieldIds.h"

// TGameControl implements everything used from it here, but g_pGameControl is only
// declared as TMasterControl* (AppGlobals.h) - the same cast the other classes use.
static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

// Confirmed (asm lines 219729-219739)
THScene::THScene() : TGScene() {
}

// Confirmed (asm lines 219228-219262)
THScene::~THScene() {
	if (!_ref.IsEmpty())
		_ref.UnRegisterEventHandler(this);
}

// Confirmed (asm lines 219751-219802): the scene gets the data object to listen to (the
// one it listened to before, if any, is let go).
void THScene::RegisterEvents(TVisObjRef &scene) {
	if (scene.IsEmpty())
		return;

	if (!_ref.IsEmpty())
		_ref.UnRegisterEventHandler(this);

	scene.RegisterEventHandler(this, TEventEnum::kChanged);
}

// Confirmed (asm lines 219814-219844)
void THScene::UnregisterEvents() {
	if (!_ref.IsEmpty())
		_ref.UnRegisterEventHandler(this);
}

// Confirmed (asm lines 219326-219700). Which field changed is all that is given: the new
// value is read from the data.
void THScene::OnEvent(TEventEnum /*event*/, int field, TVisionaireObject * /*object*/) {
	TVisObjRef &scene = _ref;

	switch (field) {
	case kSceneMusicVolume:
	case kSceneMusicBalance:
		// the music goes on with the new volume and balance
		g_pGameControl->GetSoundManager()->SetStats(scene.GetPath(kSceneBackgroundMusic), scene.GetInt(kSceneMusicVolume),
		        scene.GetInt(kSceneMusicBalance), TSoundTypeEnum::kValue0, false, 0);
		break;
	case kSceneBackgroundMusic: {
		// new music: the old one is let go, the new one plays (and is faded in)
		TSoundFFMPEG *sounds = g_pGameControl->GetSoundManager();

		sounds->FinishSoundFade();
		g_pGameControl->GetSoundManager()->Play(scene.GetPath(kSceneBackgroundMusic), scene.GetInt(kSceneMusicVolume),
		                                        scene.GetInt(kSceneMusicBalance), true, TSoundTypeEnum::kValue0, true, 0);
		g_pGameControl->GetSoundManager()->StartSoundFade(TFadeEnum::kValue4, 3000, false);
		break;
	}
	case kSceneBrightness: {
		// kept between 0 and 100, also in the data
		int brightness = scene.GetInt(kSceneBrightness);

		if (brightness < 0) {
			brightness = 0;
			scene.SetValue(kSceneBrightness, 0, TSendEventEnum::kNoEvent);
		} else if (brightness > 100) {
			brightness = 100;
			scene.SetValue(kSceneBrightness, 100, TSendEventEnum::kNoEvent);
		}

		_brightness = (float)brightness / 100.0f;
		break;
	}
	case kSceneCurrentWaySystem: {
		// the characters in the scene walk on the new way system
		TVisObjRef waySystem = scene.GetLink(kSceneCurrentWaySystem);
		TVisObjRef waySystemScene = waySystem.GetParent();

		for (TGCharacter *character : gameControl()->GetAllCharacters()) {
			if (character->GetRef().GetLink(kCharacterScene) == waySystemScene)
				character->SetWaySystem(waySystem, true);
		}
		break;
	}
	case kSceneScrollOnEdges:
		SetIsScrollable(scene.GetBool(kSceneScrollOnEdges));
		break;
	case kSceneLightMap:
		SetCurrentLightmap();
		break;
	case kSceneScrollableArea:
		SetWorktopArea(*scene.GetRect(kSceneScrollableArea), GetWorktopWidth(), GetWorktopHeight());
		break;
	default:
		break;
	}
}
