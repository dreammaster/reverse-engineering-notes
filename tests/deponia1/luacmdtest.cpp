#include <cmath>
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
#include "vscommon/scripting/visLua.h"
#include "vsplayer/animationGame.h"
#include "vstables/eCommand.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"
#include "fakepictures.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

class NullGraphics : public TGraphicsInterface {
};

// The scripting commands of the player that work on the game: start and stop an action and an animation, the volumes,
// the position of the cursor, getObject / getTime / getProperty, hooks and created events.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	useFakePictures();
	writeFakePicture("pics/a.png", 20, 20);
	writeFakePicture("pics/b.png", 20, 20);

	{
		TVisionaireGame game;
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
		hero.SetLink(kCharacterCurrentOutfit, outfit, true);

		TVisObjRef door = game.CreateObject(6, scene, kSceneObjects);
		door.SetName(TCharHolder("Door"));
		door.SetValue(kObjectPosition, wxPoint{500, 250}, TSendEventEnum::kNoEvent);

		// an animation of the door that never ends
		TVisObjRef blink = game.CreateObject(9, door, kObjectAnimations);
		blink.SetName(TCharHolder("Blink"));
		std::vector<TSprite> sprites(2);
		sprites[0].SetPath(TCharHolder("pics/a.png"));
		sprites[1].SetPath(TCharHolder("pics/b.png"));
		for (TSprite &sprite : sprites)
			sprite.SetPause(40);
		blink.SetValue(kAnimationSprites, sprites, TSendEventEnum::kNoEvent);
		blink.SetValue(kAnimationPosition, wxPoint{100, 100}, TSendEventEnum::kNoEvent);
		blink.SetValue(kAnimationNumberOfLoops, 0, TSendEventEnum::kNoEvent);

		// the start of the game: the scene is shown
		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		TVisObjRef change = game.CreateObject(8, action, kActionActionParts);
		change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		change.SetLink(kActionPartLink, startObject, true);
		change.SetLink(kActionPartAltLink, hero, true);
		gameRef.SetLink(kGameStartAction, action, true);

		// an action that marks, waits two seconds and marks again
		TVisObjRef longAction = game.CreateObject(7, gameRef, kGameActions);
		longAction.SetName(TCharHolder("Long"));
		auto addPart = [&](int command, int value, const char *code) {
			TVisObjRef part = game.CreateObject(8, longAction, kActionActionParts);
			part.SetValue(kActionPartCommand, command, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartInt, value, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartAltInt, 1, TSendEventEnum::kNoEvent);
			if (code)
				part.SetValue(kActionPartString, wxString(code), TSendEventEnum::kNoEvent);
		};
		addPart(kCommandRunPartScript, 0, "long_a = (long_a or 0) + 1");
		addPart(kCommandWait, 2, nullptr);
		addPart(kCommandRunPartScript, 0, "long_b = (long_b or 0) + 1");

		wxFileName out(L"luacmd.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"luacmd.dat");
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
		auto run = [&](const char *code) {
			LuaDoString(code);
		};
		auto number = [&](const char *name) {
			lua_getfield(L, LUA_GLOBALSINDEX, name);
			double result = lua_isnil(L, -1) ? -1e9 : lua_tonumber(L, -1);

			lua_settop(L, 0);
			return result;
		};
		auto text = [&](const char *name) {
			lua_getfield(L, LUA_GLOBALSINDEX, name);
			std::string result = lua_isstring(L, -1) ? lua_tostring(L, -1) : "(not a string)";

			lua_settop(L, 0);
			return result;
		};
		auto flag = [&](const char *name) {
			lua_getfield(L, LUA_GLOBALSINDEX, name);
			int result = lua_isnil(L, -1) ? -1 : lua_toboolean(L, -1);

			lua_settop(L, 0);
			return result;
		};

		frames(20);

		// startAction gives the active action; the action runs, and stopAction ends it in its wait
		run("started = startAction(Actions['Long']); started_empty = started:isEmpty()");
		frames(30);
		CHECK(flag("started_empty") == 0);
		CHECK(number("long_a") == 1);
		CHECK(number("long_b") == -1e9);
		run("stopAction(Actions['Long'])");
		frames(500);
		CHECK(number("long_b") == -1e9);

		// started again, it runs to its end
		run("startAction(Actions['Long'])");
		frames(500);
		printf("long action: a %g, b %g\n", number("long_a"), number("long_b"));
		CHECK(number("long_a") == 2);
		CHECK(number("long_b") == 1);

		// an animation is started and stopped
		int before = (int)TGAnimation::GetRunningAnimations().size();

		run("state = startAnimation(Animations['Blink']); state_empty = state:isEmpty()");
		frames(10);
		int during = (int)TGAnimation::GetRunningAnimations().size();

		CHECK(flag("state_empty") == 0);
		CHECK(during == before + 1);
		run("stopAnimation(Animations['Blink'])");
		frames(10);
		CHECK((int)TGAnimation::GetRunningAnimations().size() == before);
		run("state = startAnimation(Animations['Blink'], true)");
		frames(10);
		CHECK((int)TGAnimation::GetRunningAnimations().size() == before + 1);
		run("stopAnimation(Animations['Blink'])");

		// the volumes
		run("setVolume(0, 33); setVolume(1, 44); setVolume(2, 55); setVolume(3, 66); setVolume(4, 77); "
		    "v_music = getVolume(0); v_sound = getVolume(1); v_speech = getVolume(2); v_movie = getVolume(3); v_global = getVolume(4)");
		printf("volumes: %g %g %g %g %g\n", number("v_music"), number("v_sound"), number("v_speech"), number("v_movie"), number("v_global"));
		CHECK(number("v_music") == 33);
		CHECK(number("v_sound") == 44);
		CHECK(number("v_speech") == 55);
		CHECK(number("v_movie") == 66);
		CHECK(number("v_global") == 77);
		// (a script that fails goes on being run: the error handler writes the traceback, which crashed once the stack of Lua
		// had been damaged by a script that worked)
		run("setVolume(1, 200); v_sound = getVolume(1)");
		run("v_sound = getVolume(1)");
		CHECK(number("v_sound") == 44);

		// the cursor
		run("setCursorPos({x = 120, y = 80}); cursor = getCursorPos(); cursor_x = cursor.x; cursor_y = cursor.y");
		printf("cursor at %g,%g\n", number("cursor_x"), number("cursor_y"));

		// the object of a path, the time, the properties
		run("found = getObject('Characters[Hero]'); found_name = found:getName(); missing = getObject('Characters[Nobody]', false); "
		    "missing_empty = missing:isEmpty()");
		CHECK(text("found_name") == "Hero");
		CHECK(flag("missing_empty") == 1);
		run("t1 = getTime({flags = true, reset = true}); t2 = getTime()");
		CHECK(number("t1") >= 0);
		CHECK(number("t2") >= 0);
		run("platform = getProperty('platform'); steam = getProperty('steam_initialized'); lang = getProperty('system_language'); "
		    "area = getProperty('display_resolution')");
		CHECK(text("platform") == "linux");
		printf("steam %d, language %s\n", flag("steam"), text("lang").c_str());

		// a hook for the text of the action line, and one that does not exist
		run("function actionLineText(pos) return 'hooked' end; registerHookFunction('getActionText', 'actionLineText')");
		TVisObjRef door;
		control->GetVisionaire()->GetObjectByName(wxString(L"Door"), 6, door);
		CHECK(control->GetObjectManager()->GetActionText().ToStdWstring() == L"hooked");
		run("registerHookFunction('getActionText', '')");
		run("ok = pcall(registerHookFunction, 'nonsense', 'f')");

		// an event is created: the game takes it from the queue of events
		run("pushed = createEvent('eEvtKeyDown', {x = 0, y = 0}, 120, 0)");
		printf("event pushed: %d\n", flag("pushed"));
		run("pushed_wrong = pcall(createEvent, 'eEvtNothing')");
		CHECK(flag("pushed_wrong") == 0);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
