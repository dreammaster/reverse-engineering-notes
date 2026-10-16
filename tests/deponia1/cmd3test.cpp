#include <cstdio>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TGCharacter.h"
#include "TGObjectManager.h"
#include "TGScene.h"
#include "TManagedObject.h"
#include "TTText.h"
#include "graphicslib/graphics.h"
#include "vscommon/objAccess.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/id.h"
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

// The commands of the actions that are left: the font of a character, the light map of a scene, the item in the hand, the
// object that is saved and executed later, the cursor, a button that is shown active and the test of the language.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	{
		TVisionaireGame game;
		TVisObjRef gameRef = game.GetGame();
		gameRef.SetValue(kGameWindowResolution, wxPoint{640, 480}, TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameCompanyName, wxString(L"Test Company"), TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameGameName, wxString(L"Test Game"), TSendEventEnum::kNoEvent);

		TVisObjRef hall = game.CreateObject(4, gameRef, kGameSceneLinks);
		hall.SetName(TCharHolder("Hall"));
		TVisObjRef hero = game.CreateObject(0, gameRef, kGameCharacterLinks);
		hero.SetName(TCharHolder("Hero"));
		TVisObjRef startObject = game.CreateObject(6, hall, kSceneObjects);
		startObject.SetName(TCharHolder("Start"));
		startObject.SetValue(kObjectPosition, wxPoint{100, 200}, TSendEventEnum::kNoEvent);
		hero.SetLink(kCharacterStartObject, startObject, true);
		gameRef.SetLink(kGameFirstCharacter, hero, true);

		TVisObjRef ways = game.CreateObject(29, hall, kSceneWaySystems);
		std::vector<wxPoint> border = {{0, 0}, {640, 0}, {640, 480}, {0, 480}};
		ways.SetValue(kWaySystemBorder, border, TSendEventEnum::kNoEvent);
		hall.SetLink(kSceneCurrentWaySystem, ways, true);
		TVisObjRef outfit = game.CreateObject(17, hero, kCharacterOutfits);
		hero.SetLink(kCharacterCurrentOutfit, outfit, true);

		// a rock under the mouse, and an item
		TVisObjRef rock = game.CreateObject(6, hall, kSceneObjects);
		rock.SetName(TCharHolder("Rock"));
		rock.SetValue(kObjectPosition, wxPoint{400, 300}, TSendEventEnum::kNoEvent);
		std::vector<wxPoint> rockPolygon = {{380, 280}, {420, 280}, {420, 320}, {380, 320}};
		rock.SetValue(kObjectPolygon, rockPolygon, TSendEventEnum::kNoEvent);
		TVisObjRef key = game.CreateObject(6, hall, kSceneObjects);
		key.SetName(TCharHolder("Key"));
		key.SetValue(kObjectIsItem, true, TSendEventEnum::kNoEvent);

		TVisObjRef font = game.CreateObject(3, gameRef, kGameFontLinks);
		font.SetName(TCharHolder("Big"));
		TVisObjRef cursor = game.CreateObject(15, gameRef, kGameCursors);
		cursor.SetName(TCharHolder("Arrow"));
		TVisObjRef english = game.CreateObject(18, gameRef, kGameLanguages);
		english.SetName(TCharHolder("English"));
		TVisObjRef german = game.CreateObject(18, gameRef, kGameLanguages);
		german.SetName(TCharHolder("German"));

		// an interface of the hero with a button
		TVisObjRef interface = game.CreateObject(1, gameRef, kGameInterfaces);
		interface.SetName(TCharHolder("Panel"));
		std::vector<wxPoint> panel = {{0, 0}, {200, 0}, {200, 200}, {0, 200}};
		interface.SetValue(kInterfaceBorder, panel, TSendEventEnum::kNoEvent);
		interface.SetValue(kInterfaceDisplacement, 4, TSendEventEnum::kNoEvent);
		hero.SetLink(kCharacterInterfaces, interface, true);
		TVisObjRef button = game.CreateObject(2, interface, kInterfaceButtons);
		button.SetName(TCharHolder("Use"));
		std::vector<wxPoint> square = {{10, 10}, {60, 10}, {60, 60}, {10, 60}};
		button.SetValue(kButtonPolygon, square, TSendEventEnum::kNoEvent);
		TVisObjRef spare = game.CreateObject(2, interface, kInterfaceButtons);
		spare.SetName(TCharHolder("Spare"));
		std::vector<wxPoint> square2 = {{70, 10}, {120, 10}, {120, 60}, {70, 60}};
		spare.SetValue(kButtonPolygon, square2, TSendEventEnum::kNoEvent);

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
		auto addPart = [&](TVisObjRef owner, int command, TVisObjRef link, int value, int altValue = 0, TVisObjRef altLink = TVisObjRef()) {
			TVisObjRef part = game.CreateObject(8, owner, kActionActionParts);
			part.SetValue(kActionPartCommand, command, TSendEventEnum::kNoEvent);
			if (!link.IsEmpty())
				part.SetLink(kActionPartLink, link, true);
			if (!altLink.IsEmpty())
				part.SetLink(kActionPartAltLink, altLink, true);
			part.SetValue(kActionPartInt, value, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartAltInt, altValue, TSendEventEnum::kNoEvent);
			return part;
		};
		auto log = [&](TVisObjRef owner, const char *word) {
			TVisObjRef part = addPart(owner, kCommandRunPartScript, none, 0);
			part.SetValue(kActionPartString, wxString((std::string("trace = (trace or '') .. '") + word + " '").c_str()), TSendEventEnum::kNoEvent);
		};

		TVisObjRef a;

		a = makeAction("Font");
		addPart(a, kCommandSetFont, font, 0);
		a = makeAction("LightMap");
		{
			TVisObjRef part = addPart(a, kCommandSetLightMap, none, 0);

			part.SetValue(kActionPartPath, wxFileName(L"light.png"), TSendEventEnum::kNoEvent);
		}
		a = makeAction("Hold");
		addPart(a, kCommandSetItem, key, 1);
		a = makeAction("Drop");
		addPart(a, kCommandSetItem, key, 0);
		a = makeAction("SaveObject");
		addPart(a, kCommandSaveObject, none, 0);
		a = makeAction("ClearObject");
		addPart(a, kCommandClearSavedObject, none, 0);
		a = makeAction("ExecuteObject");
		addPart(a, kCommandExecuteSavedObject, none, 0);
		a = makeAction("Cursor");
		addPart(a, kCommandSetCursor, cursor, 0);
		a = makeAction("ButtonOff");
		addPart(a, kCommandSetObjectActive, spare, 0);
		a = makeAction("ButtonOn");
		addPart(a, kCommandSetObjectActive, spare, 1);
		a = makeAction("ActiveOff");
		addPart(a, kCommandSetObjectActive, button, 0);
		a = makeAction("IfEnglish");
		addPart(a, kCommandIfLanguage, english, 0);
		log(a, "english");
		a = makeAction("IfGerman");
		addPart(a, kCommandIfLanguage, german, 0);
		log(a, "german");

		wxFileName out(L"cmd3.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"cmd3.dat");
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
		auto trace = [&]() {
			lua_getfield(L, LUA_GLOBALSINDEX, "trace");
			std::string result = lua_tostring(L, -1) ? lua_tostring(L, -1) : "";
			lua_settop(L, 0);
			LuaDoString("trace = ''", "reset");
			return result;
		};
		auto run = [&](const char *name) {
			TGAction::AddRunningAction(find(name, 7));
			frames(20);
		};

		frames(20);

		TVisObjRef game = control->GetVisionaire()->GetGame();
		TVisObjRef hero = control->GetCurrentCharacter()->GetRef();
		TVisObjRef rock = find("Rock", 6);
		TVisObjRef key = find("Key", 6);
		TVisObjRef font = find("Big", 3);
		TVisObjRef button = find("Use", 2);
		TVisObjRef spare = find("Spare", 2);

		// the font of the character
		run("Font");
		CHECK(hero.GetLink(kCharacterFont) == font);

		// the light map of the scene
		run("LightMap");
		CHECK(control->GetScene()->GetRef().GetPath(kSceneLightMap).GetFullName().ToStdWstring() == L"light.png");

		// the item in the hand: held, and let go
		run("Hold");
		CHECK(game.GetLink(kGameUsedItem) == key);
		CHECK(game.GetBool(kGameUsedItemPicked));
		run("Drop");
		// (it stays the item that is used, but is not in the hand)
		CHECK(game.GetLink(kGameUsedItem) == key);
		CHECK(!game.GetBool(kGameUsedItemPicked));

		// the object that the mouse is on is saved, forgotten, and saved and executed again (the mouse has gone away meanwhile)
		control->ProcessMessage(static_cast<TMouseMessageEnum>(1), wxPoint{400, 300});
		frames(3);
		CHECK(control->GetObjectManager()->GetCurrentObject() == rock);
		run("SaveObject");
		CHECK(game.GetLink(kGameSavedObject) == rock);
		control->ProcessMessage(static_cast<TMouseMessageEnum>(1), wxPoint{50, 50});
		frames(3);
		CHECK(control->GetObjectManager()->GetCurrentObject().IsEmpty());
		run("ExecuteObject");
		CHECK(game.GetLink(kGameCurrentObject) == rock);
		run("ClearObject");
		CHECK(game.GetLink(kGameSavedObject).IsEmpty());

		// the cursor
		run("Cursor");

		// the button: shown not active, and active again
		TManagedObject *buttonObject = control->GetObject(spare);

		CHECK(buttonObject != nullptr);
		if (buttonObject) {
			CHECK(buttonObject->IsActive());
			TGAction::AddRunningAction(find("ButtonOff", 7));
			TGAction::ContinueRunningActions(false);
			CHECK(!buttonObject->IsActive());
			frames(5);
			CHECK(!buttonObject->IsActive());
			TGAction::AddRunningAction(find("ButtonOn", 7));
			TGAction::ContinueRunningActions(false);
			CHECK(buttonObject->IsActive());
		}

		// (the button of the active command of the interface is not switched off)
		TManagedObject *commandObject = control->GetObject(button);

		CHECK(commandObject != nullptr);
		if (commandObject && commandObject->IsActive() && button == button.GetParent().GetLink(kInterfaceActiveCommand)) {
			TGAction::AddRunningAction(find("ActiveOff", 7));
			TGAction::ContinueRunningActions(false);
			CHECK(commandObject->IsActive());
		}

		// the language: the actions of the one that is set run on
		TVisObjRef english = find("English", 18);
		TVisObjRef german = find("German", 18);

		TTText::SetLanguage(german);

		int languageId = TTText::GetLanguageId();

		printf("language %d, english %d, german %d\n", languageId, PackVisId(english.GetId()), PackVisId(german.GetId()));
		trace();
		run("IfEnglish");
		run("IfGerman");

		std::string seen = trace();

		printf("language trace: %s\n", seen.c_str());
		CHECK(languageId == PackVisId(german.GetId()));
		CHECK(seen.find("english") == std::string::npos);
		CHECK(seen.find("german") != std::string::npos);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
