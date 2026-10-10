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

	// a small project of our own: one scene, one character
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
		startObject.SetValue(kObjectDirection, 180, TSendEventEnum::kNoEvent);
		hero.SetLink(kCharacterStartObject, startObject, true);
		gameRef.SetLink(kGameFirstCharacter, hero, true);

		// the start action: change the scene to the start object, wait, walk the character to a place, end
		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		TVisObjRef part1 = game.CreateObject(8, action, kActionActionParts);
		part1.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		part1.SetLink(kActionPartLink, startObject, true);
		part1.SetLink(kActionPartAltLink, hero, true);
		TVisObjRef part2 = game.CreateObject(8, action, kActionActionParts);
		part2.SetValue(kActionPartCommand, kCommandWait, TSendEventEnum::kNoEvent);
		part2.SetValue(kActionPartInt, 30, TSendEventEnum::kNoEvent);
		TVisObjRef ways = game.CreateObject(29, scene, kSceneWaySystems);
		std::vector<wxPoint> border = {{0, 0}, {640, 0}, {640, 480}, {0, 480}};
		ways.SetValue(kWaySystemBorder, border, TSendEventEnum::kNoEvent);
		scene.SetLink(kSceneCurrentWaySystem, ways, true);
		TVisObjRef outfit = game.CreateObject(17, hero, kCharacterOutfits);
		outfit.SetValue(kOutfitCharacterSpeed, 100, TSendEventEnum::kNoEvent);
		hero.SetLink(kCharacterCurrentOutfit, outfit, true);
		TVisObjRef counter = game.CreateObject(20, scene, kSceneValues);
		counter.SetName(TCharHolder("Counter"));
		TVisObjRef partValue = game.CreateObject(8, action, kActionActionParts);
		partValue.SetValue(kActionPartCommand, kCommandSetValue, TSendEventEnum::kNoEvent);
		partValue.SetLink(kActionPartLink, counter, true);
		partValue.SetValue(kActionPartInt, 0, TSendEventEnum::kNoEvent);
		partValue.SetValue(kActionPartAltInt, 5, TSendEventEnum::kNoEvent);
		auto addPart = [&](int command, TVisObjRef link, int intValue, int altInt) {
			TVisObjRef part = game.CreateObject(8, action, kActionActionParts);
			part.SetValue(kActionPartCommand, command, TSendEventEnum::kNoEvent);
			if (!link.IsEmpty())
				part.SetLink(kActionPartLink, link, true);
			part.SetValue(kActionPartInt, intValue, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartAltInt, altInt, TSendEventEnum::kNoEvent);
			return part;
		};
		addPart(kCommandIfValue, counter, 0, 5);
		addPart(kCommandSetValue, counter, 0, 7);
		addPart(kCommandElse, TVisObjRef(), 0, 0);
		addPart(kCommandSetValue, counter, 0, 99);
		addPart(kCommandEndIf, TVisObjRef(), 0, 0);
		TVisObjRef part3 = game.CreateObject(8, action, kActionActionParts);
		part3.SetValue(kActionPartCommand, kCommandCharacterGoTo, TSendEventEnum::kNoEvent);
		part3.SetLink(kActionPartLink, hero, true);
		part3.SetValue(kActionPartInt, 300, TSendEventEnum::kNoEvent);
		part3.SetValue(kActionPartAltInt, 300, TSendEventEnum::kNoEvent);
		// a language, a font and a text the hero says
		TVisObjRef language = game.CreateObject(18, gameRef, kGameLanguages);
		language.SetName(TCharHolder("English"));
		gameRef.SetLink(kGameStandardLanguage, language, true);
		TVisObjRef font = game.CreateObject(3, gameRef, kGameFontLinks);
		font.SetName(TCharHolder("Font"));
		std::vector<wxRect> rects;
		for (int i = 0; i < 26; i++) {
			wxRect r;
			r.x = i * 8;
			r.y = 0;
			r.width = 8;
			r.height = 12;
			rects.push_back(r);
		}
		font.SetValue(kFontLetters, rects, TSendEventEnum::kNoEvent);
		font.SetValue(kFontAlphabet, wxString(L"abcdefghijklmnopqrstuvwxyz"), TSendEventEnum::kNoEvent);
		gameRef.SetLink(kGameActionTextFont, font, true);
		TVisObjRef text = game.CreateObject(14, hero, kCharacterTexts);
		text.SetName(TCharHolder("Hello"));
		TVisObjRef partText = game.CreateObject(8, action, kActionActionParts);
		partText.SetValue(kActionPartCommand, kCommandShowText, TSendEventEnum::kNoEvent);
		partText.SetLink(kActionPartLink, text, true);
		partText.SetLink(kActionPartAltLink, hero, true);
		TVisObjRef part4 = game.CreateObject(8, action, kActionActionParts);
		part4.SetValue(kActionPartCommand, kCommandEndAction, TSendEventEnum::kNoEvent);
		gameRef.SetLink(kGameStartAction, action, true);

		wxFileName out(L"synthetic.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}
	printf("saved\n");

	graphics = new NullGraphics();

	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"synthetic.dat");
	wxString warning;

	printf("PreLoad\n");
	CHECK(control->PreLoad(file, warning, true));
	printf("PreLoad done\n");

	InitPlayerCommands(control->GetVisionaire(), wxString(L"."), wxString(L"."), wxString(L"."));
	wxString language;

	printf("LoadAndInitGame\n");
	bool loaded = control->LoadAndInitGame(file, warning, language, true);
	printf("LoadAndInitGame -> %d\n", loaded);
	CHECK(loaded);

	if (loaded) {
		control->RegisterEventHandler();
		control->InitAfterLoadingScreen();
		control->ExecuteStartingAction();
		printf("started\n");

		// run a few frames
		for (int i = 0; i < 5; i++) {
			TGAction::ContinueRunningActions(false);
			control->Update();
			control->Draw(true);
			wxMilliSleep(10);
			control->ProcessMessage(static_cast<TMouseMessageEnum>(1), wxPoint{50 + i, 60});
		}
		control->ProcessMessage(static_cast<TMouseMessageEnum>(3), wxPoint{300, 300});
		control->ProcessMessage(static_cast<TMouseMessageEnum>(4), wxPoint{300, 300});
		for (int i = 0; i < 20; i++) {
			TGAction::ContinueRunningActions(false);
			control->Update();
			control->Draw(true);
		}
		{
			TVisObjRef g = control->GetVisionaire()->GetGame();
			TVisObjRef sc = control->GetCurrentCharacter()->GetRef().GetLink(kCharacterStartObject).GetParent();
			printf("scene empty: %d", (int)sc.IsEmpty());
			control->GetSceneControl()->ChangeScene(control->GetCurrentCharacter()->GetRef(), control->GetCurrentCharacter()->GetRef().GetLink(kCharacterStartObject), false, -1);
			for (int i = 0; i < 40; i++) {
				TGAction::ContinueRunningActions(false);
				control->Update();
				control->Draw(true);
				control->ProcessMessage(static_cast<TMouseMessageEnum>(1), wxPoint{200, 200});
			}
			control->ProcessMessage(static_cast<TMouseMessageEnum>(3), wxPoint{320, 400});
			control->ProcessMessage(static_cast<TMouseMessageEnum>(4), wxPoint{320, 400});
			for (int i = 0; i < 200; i++) {
				TGAction::ContinueRunningActions(false);
				control->Update();
				control->Draw(true);
				wxMilliSleep(10);
			}
		}
		TVisObjRef cur = control->GetCurrentCharacter()->GetRef();
		printf("hero at %d,%d\n", cur.GetPoint(kCharacterPosition)->x, cur.GetPoint(kCharacterPosition)->y);
		// save the running game and load it again
		TMSavegame::InitSaveGamePath();
		control->SaveGame(1);
		{
			TMSavegame check(true, 1, 0, 0, control->GetVisionaire());
			CHECK(check.Exists());
			printf("savegame exists: %d" "\n", (int)check.Exists());
		}
		control->GetCurrentCharacter()->GetRef().SetValue(kCharacterPosition, wxPoint{5, 6}, TSendEventEnum::kNoEvent);
		bool loadedAgain = control->LoadGame(1);
		printf("LoadGame -> %d" "\n", (int)loadedAgain);
		TVisObjRef again = control->GetCurrentCharacter()->GetRef();
		printf("hero after load at %d,%d" "\n", again.GetPoint(kCharacterPosition)->x, again.GetPoint(kCharacterPosition)->y);
		CHECK(loadedAgain);
		// scripts see the game
		LuaDoString("local h = Characters['Hero']; luaName = tostring(h) .. ' ' .. tostring(h and h.Position and h.Position.x)");
		lua_getfield(L, LUA_GLOBALSINDEX, "luaName");
		printf("lua: %s" "\n", lua_tostring(L, -1));
		lua_settop(L, 0);
		LuaDoString("hero = Characters['Hero']; hero.Position = {x = 7, y = 8}");
		TVisObjRef afterLua = control->GetCurrentCharacter()->GetRef();
		printf("hero set by lua at %d,%d" "\n", afterLua.GetPoint(kCharacterPosition)->x, afterLua.GetPoint(kCharacterPosition)->y);
		{
			TVisObjRef sceneRef = control->GetCurrentCharacter()->GetRef().GetLink(kCharacterStartObject).GetParent();
			TVList values;
			sceneRef.GetLinks(kSceneValues, TypeOrder::kValue0, values);
			printf("values in the scene: %d" "\n", (int)values.size());
			if (!values.empty())
				printf("Counter = %d" "\n", TVisObjRef(values.front()).GetInt(kValueInt));
		}
		printf("frames done\n");
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
