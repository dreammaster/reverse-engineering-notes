#include <cstdio>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TMSavegame.h"
#include "graphicslib/graphics.h"
#include "vscommon/objAccess.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/visLua.h"
#include "vstables/eCommand.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

class NullGraphics : public TGraphicsInterface {
};

int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	{
		TVisionaireGame game;
		game.NewGame();
		TVisObjRef gameRef = game.GetGame();
		gameRef.SetValue(kGameWindowResolution, wxPoint{640, 480}, TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameCompanyName, wxString(L"Test Company"), TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameGameName, wxString(L"Test Game"), TSendEventEnum::kNoEvent);

		TVisObjRef scene = game.CreateObject(4, gameRef, kGameSceneLinks);
		scene.SetName(TCharHolder("Hall"));
		TVisObjRef hero = game.CreateObject(0, gameRef, kGameCharacterLinks);
		hero.SetName(TCharHolder("Hero"));
		TVisObjRef startObject = game.CreateObject(6, scene, kSceneObjects);
		startObject.SetName(TCharHolder("Start"));
		startObject.SetValue(kObjectPosition, wxPoint{100, 200}, TSendEventEnum::kNoEvent);
		hero.SetLink(kCharacterStartObject, startObject, true);
		gameRef.SetLink(kGameFirstCharacter, hero, true);
		TVisObjRef ways = game.CreateObject(29, scene, kSceneWaySystems);
		std::vector<wxPoint> border = {{0, 0}, {640, 0}, {640, 480}, {0, 480}};
		ways.SetValue(kWaySystemBorder, border, TSendEventEnum::kNoEvent);
		scene.SetLink(kSceneCurrentWaySystem, ways, true);
		TVisObjRef outfit = game.CreateObject(17, hero, kCharacterOutfits);
		outfit.SetValue(kOutfitCharacterSpeed, 100, TSendEventEnum::kNoEvent);
		hero.SetLink(kCharacterCurrentOutfit, outfit, true);

		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		auto addPart = [&](int command, int intValue, int altInt, const wchar_t *path) {
			TVisObjRef part = game.CreateObject(8, action, kActionActionParts);
			part.SetValue(kActionPartCommand, command, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartInt, intValue, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartAltInt, altInt, TSendEventEnum::kNoEvent);
			if (path)
				part.SetValue(kActionPartPath, wxFileName(path), TSendEventEnum::kNoEvent);
			return part;
		};
		TVisObjRef change = addPart(kCommandChangeScene, 0, 0, nullptr);
		change.SetLink(kActionPartLink, startObject, true);
		change.SetLink(kActionPartAltLink, hero, true);

		// an animation of three pictures on the object
		TVisObjRef anim = game.CreateObject(9, startObject, kObjectAnimations);
		anim.SetName(TCharHolder("Blink"));
		std::vector<TSprite> sprites(3);
		for (int i = 0; i < 3; i++) {
			sprites[i].SetPath(TCharHolder(i == 0 ? "pics/a.png" : i == 1 ? "pics/b.png" : "pics/c.png"));
			sprites[i].SetPause(40);
		}
		anim.SetValue(kAnimationSprites, sprites, TSendEventEnum::kNoEvent);
		anim.SetValue(kAnimationPosition, wxPoint{100, 100}, TSendEventEnum::kNoEvent);

		auto showAnimation = [&](bool hide, bool wait) {
			TVisObjRef part = addPart(kCommandShowAnimation, hide ? 1 : 0, 0, nullptr);
			part.SetLink(kActionPartLink, anim, true);
			part.SetValue(kActionPartAltInt2, wait ? 1 : 0, TSendEventEnum::kNoEvent);
		};
		auto mark = [&](const char *text) {
			TVisObjRef part = addPart(kCommandRunPartScript, 0, 0, nullptr);
			part.SetValue(kActionPartString, wxString(text), TSendEventEnum::kNoEvent);
		};
		mark("print('mark 1')");
		showAnimation(false, true);
		mark("print('mark 2')");
		addPart(kCommandWait, 10, 0, nullptr);
		showAnimation(false, false);
		addPart(kCommandWait, 30, 0, nullptr);
		showAnimation(true, false);
		mark("print('mark 3')");
		TVisObjRef visibility = addPart(kCommandSetObjectVisibility, 0, 0, nullptr);
		visibility.SetLink(kActionPartLink, startObject, true);
		addPart(kCommandEarthquake, 100, 5, nullptr);
		addPart(kCommandFade, 0, 0, nullptr);
		addPart(kCommandWait, 20, 0, nullptr);
		mark("print('mark 4')");
		addPart(kCommandEndAction, 0, 0, nullptr);
		gameRef.SetLink(kGameStartAction, action, true);

		wxFileName out(L"anim.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"anim.dat");
	wxString warning;

	CHECK(control->PreLoad(file, warning, true));
	InitPlayerCommands(control->GetVisionaire(), wxString(L"."), wxString(L"."), wxString(L"."));
	wxString language;
	bool loaded = control->LoadAndInitGame(file, warning, language, true);

	CHECK(loaded);
	if (loaded) {
		control->RegisterEventHandler();
		control->InitAfterLoadingScreen();
		control->ExecuteStartingAction();

		for (int i = 0; i < 100; i++) {
			TGAction::ContinueRunningActions(false);
			control->Update();
			control->Draw(true);
			wxMilliSleep(5);
		}
		printf("frames done\n");
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
