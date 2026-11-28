#include <chrono>
#include <cstdio>
#include <string>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TGCharacter.h"
#include "TGScene.h"
#include "graphicslib/graphics.h"
#include "vscommon/objAccess.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/lua.h"
#include "vscommon/scripting/visLua.h"
#include "vstables/eCommand.h"
#include "vstables/fieldIds.h"
#include "vstables/records.h"
#include "vstables/visionaireGame.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

class NullGraphics : public TGraphicsInterface {
};

// Going to a menu freezes the game: the actions that run and the texts that are shown wait, and go on when the game is
// entered again, with the time they had left.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	{
		TVisionaireGame game;
		TVisObjRef gameRef = game.GetGame();
		gameRef.SetValue(kGameWindowResolution, wxPoint{640, 480}, TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameCompanyName, wxString(L"Test Company"), TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameGameName, wxString(L"Test Game"), TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameFadeEffect, 0, TSendEventEnum::kNoEvent);

		TVisObjRef hall = game.CreateObject(4, gameRef, kGameSceneLinks);
		hall.SetName(TCharHolder("Hall"));
		TVisObjRef menu = game.CreateObject(4, gameRef, kGameSceneLinks);
		menu.SetName(TCharHolder("Menu"));
		menu.SetValue(kSceneIsMenu, true, TSendEventEnum::kNoEvent);
		TVisObjRef hero = game.CreateObject(0, gameRef, kGameCharacterLinks);
		hero.SetName(TCharHolder("Hero"));
		TVisObjRef startObject = game.CreateObject(6, hall, kSceneObjects);
		startObject.SetName(TCharHolder("Start"));
		startObject.SetValue(kObjectPosition, wxPoint{100, 200}, TSendEventEnum::kNoEvent);
		TVisObjRef menuStart = game.CreateObject(6, menu, kSceneObjects);
		menuStart.SetName(TCharHolder("MenuStart"));
		menuStart.SetValue(kObjectPosition, wxPoint{100, 200}, TSendEventEnum::kNoEvent);
		hero.SetLink(kCharacterStartObject, startObject, true);
		gameRef.SetLink(kGameFirstCharacter, hero, true);
		for (TVisObjRef scene : {hall, menu}) {
			TVisObjRef ways = game.CreateObject(29, scene, kSceneWaySystems);
			std::vector<wxPoint> border = {{0, 0}, {640, 0}, {640, 480}, {0, 480}};
			ways.SetValue(kWaySystemBorder, border, TSendEventEnum::kNoEvent);
			scene.SetLink(kSceneCurrentWaySystem, ways, true);
		}
		TVisObjRef outfit = game.CreateObject(17, hero, kCharacterOutfits);
		hero.SetLink(kCharacterCurrentOutfit, outfit, true);
		TVisObjRef statue = game.CreateObject(6, hall, kSceneObjects);
		statue.SetName(TCharHolder("Statue"));
		statue.SetValue(kObjectPosition, wxPoint{400, 300}, TSendEventEnum::kNoEvent);

		// a text
		TVisObjRef language = game.CreateObject(18, gameRef, kGameLanguages);
		language.SetName(TCharHolder("English"));
		gameRef.SetLink(kGameStandardLanguage, language, true);
		TVisObjRef font = game.CreateObject(3, gameRef, kGameFontLinks);
		font.SetName(TCharHolder("Font"));
		std::vector<wxRect> rects;
		for (int k = 0; k < 26; k++) {
			wxRect r;
			r.x = k * 8;
			r.y = 0;
			r.width = 8;
			r.height = 12;
			rects.push_back(r);
		}
		font.SetValue(kFontLetters, rects, TSendEventEnum::kNoEvent);
		font.SetValue(kFontAlphabet, wxString(L"abcdefghijklmnopqrstuvwxyz"), TSendEventEnum::kNoEvent);
		gameRef.SetLink(kGameObjectFont, font, true);
		TVisObjRef noParent;
		TVisObjRef text = game.CreateObject(14, noParent, -1);
		text.SetName(TCharHolder("Hello"));
		std::vector<TTextLanguage> texts(1);
		texts[0].text = TCharHolder("hello world from the statue");
		texts[0].languageId = PackVisId(language.GetId());
		text.SetValue(kTextTextLanguages, texts, TSendEventEnum::kNoEvent);

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
		auto addPart = [&](TVisObjRef owner, int command, TVisObjRef link, int value, int altValue = 0, int altValue2 = 0,
		                   TVisObjRef altLink = TVisObjRef()) {
			TVisObjRef part = game.CreateObject(8, owner, kActionActionParts);
			part.SetValue(kActionPartCommand, command, TSendEventEnum::kNoEvent);
			if (!link.IsEmpty())
				part.SetLink(kActionPartLink, link, true);
			if (!altLink.IsEmpty())
				part.SetLink(kActionPartAltLink, altLink, true);
			part.SetValue(kActionPartInt, value, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartAltInt, altValue, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartAltInt2, altValue2, TSendEventEnum::kNoEvent);
			return part;
		};
		TVisObjRef none;

		TVisObjRef a = makeAction("GoMenu");
		addPart(a, kCommandChangeScene, menuStart, 0, 0, 0, hero);
		a = makeAction("GoHall");
		addPart(a, kCommandChangeScene, startObject, 0, 0, 0, hero);
		a = makeAction("Say");
		addPart(a, kCommandShowTextAt, text, 200, 100, 0);
		// (a wait of a second and a half, and then the word)
		a = makeAction("Slow");
		addPart(a, kCommandWait, none, 1500);
		TVisObjRef part = addPart(a, kCommandRunPartScript, none, 0);
		part.SetValue(kActionPartString, wxString(L"slow_done = true"), TSendEventEnum::kNoEvent);

		wxFileName out(L"menu2.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"menu2.dat");
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
		auto run = [&](const char *name, int count = 10) {
			TGAction::AddRunningAction(find(name, 7));
			frames(count);
		};
		auto flag = [&](const char *name) {
			lua_getfield(L, LUA_GLOBALSINDEX, name);
			bool result = lua_toboolean(L, -1) != 0;
			lua_settop(L, 0);
			return result;
		};

		auto now = []() {
			return (long)std::chrono::duration_cast<std::chrono::milliseconds>(
			    std::chrono::steady_clock::now().time_since_epoch()).count();
		};
		// frames for this many milliseconds (or until the condition holds)
		auto spend = [&](long milliseconds) {
			long end = now() + milliseconds;

			while (now() < end)
				frames(1);
		};

		frames(20);
		TVisObjRef text = find("Hello", 14);

		// how long the text is shown
		run("Say", 5);
		CHECK(control->IsTextActive(text));

		long begin = now();

		while (control->IsTextActive(text) && now() - begin < 20000)
			frames(1);

		long shown = now() - begin;

		printf("the text is shown for %ld ms\n", shown);
		CHECK(shown > 600 && shown < 15000);
		frames(20);

		// shown again; the game is left for the time of two thirds of that, and then entered again: the text goes on for as long as
		// it had left (it is still there after the time that it is shown for, counted from its start)
		begin = now();
		run("Say", 5);
		spend(shown / 5);
		run("GoMenu", 10);
		CHECK(control->GetScene()->IsMenu());
		CHECK(control->IsTextActive(text));
		spend(shown * 2 / 5);
		CHECK(control->IsTextActive(text));
		run("GoHall", 10);
		CHECK(!control->GetScene()->IsMenu());
		CHECK(control->IsTextActive(text));

		long target = begin + shown * 12 / 10;

		while (now() < target)
			frames(1);

		printf("after %ld ms from the start (the text is shown for %ld ms): active %d\n", now() - begin, shown,
		       (int)control->IsTextActive(text));
		CHECK(control->IsTextActive(text));

		while (control->IsTextActive(text) && now() - begin < 30000)
			frames(1);
		CHECK(!control->IsTextActive(text));
		CHECK(now() - begin > shown * 12 / 10);

		// an action that waits: the same
		run("Slow", 5);
		spend(300);
		run("GoMenu", 10);
		spend(1800);
		CHECK(!flag("slow_done"));
		run("GoHall", 10);
		CHECK(!flag("slow_done"));
		spend(600);
		CHECK(!flag("slow_done"));

		for (int i = 0; i < 600 && !flag("slow_done"); i++)
			frames(1);
		CHECK(flag("slow_done"));
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
