#include <cmath>
#include <cstdio>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TGCharacter.h"
#include "graphicslib/graphics.h"
#include "vscommon/objAccess.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/visLua.h"
#include "vstables/eCommand.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"
#include "TGScene.h"
#include "vscommon/scripting/lua.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

class NullGraphics : public TGraphicsInterface {
};

// replaceGame('other.dat') from a script: the game is another one afterwards, and the start action of that one runs.
static void makeGame(const wchar_t *file, const char *sceneName, const char *heroName) {
	TVisionaireGame game;
	TVisObjRef gameRef = game.GetGame();

	gameRef.SetValue(kGameWindowResolution, wxPoint{640, 480}, TSendEventEnum::kNoEvent);
	gameRef.SetValue(kGameCompanyName, wxString(L"Test Company"), TSendEventEnum::kNoEvent);
	gameRef.SetValue(kGameGameName, wxString(L"Test Game"), TSendEventEnum::kNoEvent);

	TVisObjRef scene = game.CreateObject(4, gameRef, kGameSceneLinks);
	scene.SetName(TCharHolder(sceneName));
	TVisObjRef hero = game.CreateObject(0, gameRef, kGameCharacterLinks);
	hero.SetName(TCharHolder(heroName));
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
	hero.SetLink(kCharacterCurrentOutfit, outfit, true);

	TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
	action.SetName(TCharHolder("Start"));
	TVisObjRef change = game.CreateObject(8, action, kActionActionParts);
	change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
	change.SetLink(kActionPartLink, startObject, true);
	change.SetLink(kActionPartAltLink, hero, true);
	TVisObjRef script = game.CreateObject(8, action, kActionActionParts);
	script.SetValue(kActionPartCommand, kCommandRunPartScript, TSendEventEnum::kNoEvent);
	script.SetValue(kActionPartString, wxString((std::string("started_game = '") + sceneName + "'").c_str()), TSendEventEnum::kNoEvent);
	gameRef.SetLink(kGameStartAction, action, true);

	wxFileName out(file);
	game.BinarySave(out, nullptr, false, nullptr);
}

int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	makeGame(L"replacea.dat", "FirstScene", "FirstHero");
	makeGame(L"replaceb.dat", "SecondScene", "SecondHero");

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"replacea.dat");
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
		auto text = [&](const char *name) {
			lua_getfield(L, LUA_GLOBALSINDEX, name);

			std::string result = lua_tostring(L, -1) ? lua_tostring(L, -1) : "";

			lua_settop(L, 0);
			return result;
		};

		frames(30);
		CHECK(text("started_game") == "FirstScene");
		CHECK(std::string(control->GetScene()->GetRef().GetName().c_str().mb_str()) == "FirstScene");
		CHECK(std::string(control->GetCurrentCharacter()->GetRef().GetName().c_str().mb_str()) == "FirstHero");

		// a game file that is not there
		LuaDoString("replaceGame('nothing.dat')", "missing");
		frames(5);
		CHECK(text("started_game") == "FirstScene");
		CHECK(std::string(control->GetScene()->GetRef().GetName().c_str().mb_str()) == "FirstScene");

		// the other game
		LuaDoString("replaceGame('replaceb.dat')", "replace");
		frames(60);
		printf("started: %s, scene %s, hero %s\n", text("started_game").c_str(),
		       std::string(control->GetScene()->GetRef().GetName().c_str().mb_str()).c_str(),
		       std::string(control->GetCurrentCharacter()->GetRef().GetName().c_str().mb_str()).c_str());
		CHECK(text("started_game") == "SecondScene");
		CHECK(std::string(control->GetScene()->GetRef().GetName().c_str().mb_str()) == "SecondScene");
		CHECK(std::string(control->GetCurrentCharacter()->GetRef().GetName().c_str().mb_str()) == "SecondHero");
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
