#include <cstdio>
#include <list>
#include <string>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TGCharacter.h"
#include "graphicslib/graphics.h"
#include "vscommon/objAccess.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/lua.h"
#include "vscommon/scripting/visLua.h"
#include "vstables/eCommand.h"
#include "vstables/fieldIds.h"
#include "vstables/records.h"
#include "vstables/visionaireGame.h"
#include "fakepictures.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

class NullGraphics : public TGraphicsInterface {
};

// Showing the scene of a character, loading the animations of a character that is somewhere else, and the lists of the
// actions that run.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	useFakePictures();
	writeFakePicture("pics/f1.png", 20, 40);

	{
		TVisionaireGame game;
		TVisObjRef gameRef = game.GetGame();
		gameRef.SetValue(kGameWindowResolution, wxPoint{640, 480}, TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameCompanyName, wxString(L"Test Company"), TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameGameName, wxString(L"Test Game"), TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameFadeEffect, 0, TSendEventEnum::kNoEvent);

		TVisObjRef hall = game.CreateObject(4, gameRef, kGameSceneLinks);
		hall.SetName(TCharHolder("Hall"));
		TVisObjRef cellar = game.CreateObject(4, gameRef, kGameSceneLinks);
		cellar.SetName(TCharHolder("Cellar"));
		TVisObjRef hero = game.CreateObject(0, gameRef, kGameCharacterLinks);
		hero.SetName(TCharHolder("Hero"));
		TVisObjRef buddy = game.CreateObject(0, gameRef, kGameCharacterLinks);
		buddy.SetName(TCharHolder("Buddy"));
		TVisObjRef startObject = game.CreateObject(6, hall, kSceneObjects);
		startObject.SetName(TCharHolder("Start"));
		startObject.SetValue(kObjectPosition, wxPoint{100, 200}, TSendEventEnum::kNoEvent);
		TVisObjRef buddyStart = game.CreateObject(6, cellar, kSceneObjects);
		buddyStart.SetName(TCharHolder("BuddyStart"));
		buddyStart.SetValue(kObjectPosition, wxPoint{50, 50}, TSendEventEnum::kNoEvent);
		hero.SetLink(kCharacterStartObject, startObject, true);
		buddy.SetLink(kCharacterStartObject, buddyStart, true);
		gameRef.SetLink(kGameFirstCharacter, hero, true);

		for (TVisObjRef scene : {hall, cellar}) {
			TVisObjRef ways = game.CreateObject(29, scene, kSceneWaySystems);
			std::vector<wxPoint> border = {{0, 0}, {640, 0}, {640, 480}, {0, 480}};
			ways.SetValue(kWaySystemBorder, border, TSendEventEnum::kNoEvent);
			scene.SetLink(kSceneCurrentWaySystem, ways, true);
		}
		for (TVisObjRef character : {hero, buddy}) {
			TVisObjRef outfit = game.CreateObject(17, character, kCharacterOutfits);

			character.SetLink(kCharacterCurrentOutfit, outfit, true);

			TVList animations;

			outfit.GetLinks(kOutfitStandingAnimations, TypeOrder::kValue0, animations);

			for (TVisionaireObject *object : animations) {
				TVisObjRef anim(object);
				std::vector<TSprite> sprites(1);

				sprites[0].SetPath(TCharHolder("pics/f1.png"));
				sprites[0].SetPause(40);
				anim.SetValue(kAnimationSprites, sprites, TSendEventEnum::kNoEvent);
			}
		}

		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		TVisObjRef change = game.CreateObject(8, action, kActionActionParts);
		change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		change.SetLink(kActionPartLink, startObject, true);
		change.SetLink(kActionPartAltLink, hero, true);
		gameRef.SetLink(kGameStartAction, action, true);

		auto makeAction = [&](const char *name) {
			TVisObjRef result = game.CreateObject(7, gameRef, kGameActions);
			result.SetName(TCharHolder(name));
			return result;
		};
		auto addPart = [&](TVisObjRef owner, int command, TVisObjRef link, int value) {
			TVisObjRef part = game.CreateObject(8, owner, kActionActionParts);
			part.SetValue(kActionPartCommand, command, TSendEventEnum::kNoEvent);
			if (!link.IsEmpty())
				part.SetLink(kActionPartLink, link, true);
			part.SetValue(kActionPartInt, value, TSendEventEnum::kNoEvent);
			return part;
		};
		TVisObjRef none;

		TVisObjRef a = makeAction("Show");
		addPart(a, kCommandShowCharacter, buddy, 0);
		a = makeAction("ShowHero");
		addPart(a, kCommandShowCharacter, hero, 0);
		a = makeAction("PreloadBuddy");
		addPart(a, kCommandPreloadCharacter, buddy, 0);
		a = makeAction("UnloadBuddy");
		addPart(a, kCommandPreloadCharacter, buddy, 1);
		a = makeAction("PreloadHero");
		addPart(a, kCommandPreloadCharacter, hero, 0);
		a = makeAction("Long");
		TVisObjRef wait = addPart(a, kCommandWait, none, 60000);
		wait.SetName(TCharHolder("Waiting"));
		addPart(a, kCommandRunPartScript, none, 0).SetValue(kActionPartString, wxString(L"long_done = true"), TSendEventEnum::kNoEvent);

		wxFileName out(L"cmd5.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"cmd5.dat");
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

		auto frames = [&](int count) {
			for (int i = 0; i < count; i++) {
				TGAction::ContinueRunningActions(false);
				control->Update();
				control->Draw(true);
				wxMilliSleep(5);
			}
		};
		auto find = [&](const char *name, int table) {
			TVisObjRef found;

			if (!control->GetVisionaire()->GetObjectByName(wxString(name), table, found))
				printf("no object %s\n", name);
			return found;
		};
		auto run = [&](const char *name, int count = 30) {
			TGAction::AddRunningAction(find(name, 7));
			frames(count);
		};
		auto sceneName = [&]() {
			return control->GetScene()->GetRef().GetName().GetFullPath().ToStdWstring();
		};

		frames(20);
		CHECK(sceneName() == L"Hall");

		// a character in the shown scene is not loaded or unloaded
		run("PreloadHero");
		run("ShowHero");
		CHECK(sceneName() == L"Hall");

		// the character of the other scene: its animations are loaded and let go
		run("PreloadBuddy");
		run("UnloadBuddy");
		CHECK(sceneName() == L"Hall");

		// and its scene is shown
		run("Show", 60);
		printf("scene: %ls\n", sceneName().c_str());
		CHECK(sceneName() == L"Cellar");

		// the actions that run: by name and by id, in a list, and what the console prints
		TGAction::AddRunningAction(find("Long", 7));
		frames(5);

		TVisObjRef running = TGAction::GetActionByName(wxString(L"Long"));

		CHECK(!running.IsEmpty());
		CHECK(TGAction::GetActionByName(wxString(L"Nothing")).IsEmpty());

		int id = PackVisId(running.GetId());

		CHECK(TGAction::GetActionById(id) == running);
		CHECK(TGAction::GetActionById(id + 100000).IsEmpty());

		TVList actions;

		TGAction::GetActions(actions);
		CHECK(actions.size() >= 1);

		std::list<wxString> lines;

		TGAction::PrintRunningActions(lines);
		bool named = false;

		for (const wxString &line : lines) {
			printf("| %ls\n", line.ToStdWstring().c_str());
			if (line.ToStdWstring().find(L"Long") != std::wstring::npos)
				named = true;
		}
		CHECK(lines.size() >= 3);
		CHECK(named);

		// all of them are finished: the part that follows the wait does not run
		TGAction::FinishAllActions();
		frames(10);
		CHECK(TGAction::GetActionByName(wxString(L"Long")).IsEmpty());
		lines.clear();
		TGAction::PrintRunningActions(lines);
		CHECK(lines.size() == 2);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
