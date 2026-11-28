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
#include "TManagedObject.h"
#include "graphicslib/graphics.h"
#include "vscommon/objAccess.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/visLua.h"
#include "vstables/eCommand.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"
#include "fakepictures.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

class NullGraphics : public TGraphicsInterface {
};

// The commands of an action that show texts: a text at a point (waiting, or in the background), a text at an object, and
// the commands that wait for a character to be done talking and skip the text.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	useFakePictures();

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

		TVisObjRef statue = game.CreateObject(6, scene, kSceneObjects);
		statue.SetName(TCharHolder("Statue"));
		statue.SetValue(kObjectPosition, wxPoint{400, 300}, TSendEventEnum::kNoEvent);

		// a language, a font and a text
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
		gameRef.SetLink(kGameActionTextFont, font, true);
		gameRef.SetLink(kGameObjectFont, font, true);
		TVisObjRef noParent;
		TVisObjRef text = game.CreateObject(14, noParent, -1);
		text.SetName(TCharHolder("Hello"));
		std::vector<TTextLanguage> texts(1);
		texts[0].text = TCharHolder("hello world from the statue");
		texts[0].languageId = PackVisId(language.GetId());
		text.SetValue(kTextTextLanguages, texts, TSendEventEnum::kNoEvent);

		// the start of the game: the scene is shown
		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		TVisObjRef change = game.CreateObject(8, action, kActionActionParts);
		change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		change.SetLink(kActionPartLink, startObject, true);
		change.SetLink(kActionPartAltLink, hero, true);
		gameRef.SetLink(kGameStartAction, action, true);

		TVisObjRef none;
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
		auto log = [&](TVisObjRef owner, const char *word) {
			TVisObjRef part = addPart(owner, kCommandRunPartScript, none, 0);
			part.SetValue(kActionPartString, wxString((std::string("trace = (trace or '') .. '") + word + " '").c_str()), TSendEventEnum::kNoEvent);
		};

		TVisObjRef a;

		a = makeAction("TextAt");
		addPart(a, kCommandShowTextAt, text, 200, 100, 0);
		log(a, "at-done");
		a = makeAction("TextAtBackground");
		addPart(a, kCommandShowTextAt, text, 200, 100, 1);
		log(a, "background-started");
		a = makeAction("SkipText");
		addPart(a, kCommandSkipText, none, 0);
		a = makeAction("ObjectText");
		addPart(a, kCommandShowObjectText, text, 10, 20, 1, statue);
		log(a, "object-text");
		a = makeAction("ClearObjectText");
		addPart(a, kCommandClearObjectText, statue, 0);
		log(a, "object-text-cleared");
		a = makeAction("WaitTalking");
		addPart(a, kCommandWaitTalking, hero, 0);
		log(a, "not-talking");

		wxFileName out(L"cmdtext.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"cmdtext.dat");
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
		auto actionByName = [&](const char *wanted) {
			TVList actions;

			control->GetVisionaire()->GetGame().GetLinks(kGameActions, TypeOrder::kValue0, actions);
			for (TVisionaireObject *object : actions) {
				TVisObjRef ref(object);

				if (std::string(ref.GetName().c_str().mb_str()) == wanted)
					return ref;
			}
			printf("no action %s\n", wanted);
			return TVisObjRef();
		};
		auto trace = [&]() {
			lua_getfield(L, LUA_GLOBALSINDEX, "trace");
			std::string result = lua_tostring(L, -1) ? lua_tostring(L, -1) : "";
			lua_settop(L, 0);
			LuaDoString("trace = ''", "reset");
			return result;
		};
		auto run = [&](const char *name, int count = 10) {
			TGAction::AddRunningAction(actionByName(name));
			frames(count);
		};

		frames(20);
		LuaDoString("trace = ''", "reset");

		TVisObjRef text;
		control->GetVisionaire()->GetObjectByName(wxString(L"Hello"), 14, text);

		// a text at a point: the part waits while it is shown, until it is skipped
		run("TextAt", 30);
		CHECK(trace() == "");
		CHECK(control->IsTextActive(text));
		run("SkipText", 30);
		CHECK(trace() == "at-done ");
		CHECK(!control->IsTextActive(text));

		// the same, in the background: the action goes on at once
		run("TextAtBackground", 5);
		CHECK(trace() == "background-started ");
		CHECK(control->IsNoTextDisplayed());

		// a text at an object, and the object's text cleared
		run("ObjectText", 20);
		CHECK(trace() == "object-text ");
		run("ClearObjectText", 5);
		CHECK(trace() == "object-text-cleared ");

		// nobody talks
		run("WaitTalking", 5);
		CHECK(trace() == "not-talking ");

		// the scripts are told when a text starts and stops, and may change what is shown, where it is put and how it is drawn
		LuaDoString("events = ''; hook_calls = ''; "
		            "function onTextStarted(t) events = events .. 'started ' end; "
		            "function onTextStopped(t) events = events .. 'stopped ' end; "
		            "function textTextHook(t) hook_calls = hook_calls .. 'text '; return 'hookedtext' end; "
		            "function positionHook(t) hook_calls = hook_calls .. 'position '; return true end; "
		            "function renderHook(t, lines, widths, pos, alignment, alpha) "
		            "hook_calls = hook_calls .. 'render:' .. #lines .. ' '; return true end; "
		            "registerEventHandler('textStarted', 'onTextStarted'); registerEventHandler('textStopped', 'onTextStopped'); "
		            "registerHookFunction('textText', 'textTextHook'); registerHookFunction('setTextPosition', 'positionHook'); "
		            "registerHookFunction('textRender', 'renderHook')",
		            "hooks");
		run("ObjectText", 20);
		{
			lua_getfield(L, LUA_GLOBALSINDEX, "events");
			std::string events = lua_tostring(L, -1) ? lua_tostring(L, -1) : "";
			lua_getfield(L, LUA_GLOBALSINDEX, "hook_calls");
			std::string calls = lua_tostring(L, -1) ? lua_tostring(L, -1) : "";
			lua_settop(L, 0);
			printf("events: %s| hook calls: %s\n", events.c_str(), calls.c_str());
			CHECK(events.find("started") != std::string::npos);
			CHECK(calls.find("text ") != std::string::npos);
			CHECK(calls.find("position ") != std::string::npos);
			CHECK(calls.find("render:") != std::string::npos);
			CHECK(calls.find("render:1 ") != std::string::npos);
		}
		run("ClearObjectText", 5);
		{
			lua_getfield(L, LUA_GLOBALSINDEX, "events");
			std::string events = lua_tostring(L, -1) ? lua_tostring(L, -1) : "";
			lua_settop(L, 0);
			CHECK(events.find("stopped") != std::string::npos);
		}
		LuaDoString("registerHookFunction('textText', ''); registerHookFunction('setTextPosition', ''); "
		            "registerHookFunction('textRender', ''); registerEventHandler('textStarted', ''); "
		            "registerEventHandler('textStopped', '')",
		            "unhook");
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
