#include <cstdio>
#include <filesystem>
#include <string>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TGCharacter.h"
#include "TGScene.h"
#include "TMSavegame.h"
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

// A picture format that also writes (the savegames keep a picture of the screen), and a screen to take it from.
class WritingPicture : public TFakePicture {
public:
	bool Write(const wxFileName &file, TPictureMEM &, int width, int height, int, bool) override {
		std::FILE *out = _wfopen(file.GetFullPath().ToStdWstring().c_str(), L"wb");

		if (!out)
			return false;

		std::fprintf(out, "FAKE %d %d", width, height);
		std::fclose(out);
		return true;
	}
};

static TPictureFormat *createWritingPicture(TPictureKind) {
	return new WritingPicture();
}

class ScreenGraphics : public TGraphicsInterface {
public:
	ScreenGraphics() {
		_frame.SetImageSize(64, 48);
		_frame.SetLoadedImage();
	}

	TPictureMEM *GetCapturedFrame() override {
		return &_frame;
	}

private:
	TPictureMEM _frame;
};

// A menu scene with three places for savegames: the savegames that exist are listed (the last page first), one is
// selected by the mouse or by a script, saved over, scrolled through and deleted.
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
		std::vector<wxRect> areas = {wxRect{20, 20, 100, 75}, wxRect{140, 20, 100, 75}, wxRect{260, 20, 100, 75}};
		menu.SetValue(kSceneSavegameAreas, areas, TSendEventEnum::kNoEvent);
		menu.SetValue(kSceneSavegameScrollStep, 3, TSendEventEnum::kNoEvent);

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
		auto addPart = [&](TVisObjRef owner, int command, int value) {
			TVisObjRef part = game.CreateObject(8, owner, kActionActionParts);
			part.SetValue(kActionPartCommand, command, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartInt, value, TSendEventEnum::kNoEvent);
			return part;
		};

		TVisObjRef a = makeAction("GoMenu");
		TVisObjRef go = addPart(a, kCommandChangeScene, 0);
		go.SetLink(kActionPartLink, menuStart, true);
		go.SetLink(kActionPartAltLink, hero, true);
		a = makeAction("GoHall");
		go = addPart(a, kCommandChangeScene, 0);
		go.SetLink(kActionPartLink, startObject, true);
		go.SetLink(kActionPartAltLink, hero, true);
		a = makeAction("Save");
		addPart(a, kCommandGameSave, 1);
		a = makeAction("Load");
		addPart(a, kCommandGameSave, 0);
		a = makeAction("Delete");
		addPart(a, kCommandDeleteSavegame, 0);
		a = makeAction("Forward");
		addPart(a, kCommandScrollSavegames, 0);
		a = makeAction("Backward");
		addPart(a, kCommandScrollSavegames, 1);
		a = makeAction("IfSelected");
		addPart(a, kCommandIfSavegame, 0);
		TVisObjRef part = addPart(a, kCommandRunPartScript, 0);
		part.SetValue(kActionPartString, wxString(L"trace = (trace or '') .. 'selected '"), TSendEventEnum::kNoEvent);

		wxFileName out(L"savemenu.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	useFakePictures();
	g_createPictureFormat = createWritingPicture;
	graphics = new ScreenGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"savemenu.dat");
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
		auto find = [&](const char *name) {
			TVisObjRef found;

			if (!control->GetVisionaire()->GetObjectByName(wxString(name), 7, found))
				printf("no action %s\n", name);
			return found;
		};
		auto run = [&](const char *name, int count = 30) {
			TGAction::AddRunningAction(find(name));
			frames(count);
		};
		auto number = [&](const char *expression) {
			LuaDoString((std::string("probe = ") + expression).c_str(), "probe");
			lua_getfield(L, LUA_GLOBALSINDEX, "probe");
			int result = (int)lua_tointeger(L, -1);
			lua_settop(L, 0);
			return result;
		};
		auto onDisk = [&]() {
			std::vector<int> slots;

			TMSavegame::GetExistingSaveGames(slots);
			return slots;
		};

		frames(20);
		TMSavegame::InitSaveGamePath();

		auto clean = [&]() {
			TStandardPaths paths;
			std::filesystem::path directory(std::wstring(paths.GetSavegamePath().c_str()));
			std::error_code error;

			for (const auto &entry : std::filesystem::directory_iterator(directory, error)) {
				if (entry.path().filename().wstring().rfind(L"savegame", 0) == 0)
					std::filesystem::remove(entry.path(), error);
			}
		};

		// (what an earlier run left)
		clean();
		CHECK(onDisk().empty());

		run("GoMenu");
		CHECK(control->GetScene()->IsMenu());
		CHECK(number("system.savegamesCount") == 0);
		CHECK(number("system.selectedSavegame") == -1);

		// selected by a click: the place under it (a click elsewhere leaves it)
		auto point = [&](int x, int y) {
			control->ProcessMessage(static_cast<TMouseMessageEnum>(1), wxPoint{x, y});
			control->ProcessMessage(static_cast<TMouseMessageEnum>(3), wxPoint{x, y});
			control->ProcessMessage(static_cast<TMouseMessageEnum>(4), wxPoint{x, y});
			frames(2);
		};

		point(50, 50);
		CHECK(number("system.selectedSavegame") == 0);
		point(170, 50);
		CHECK(number("system.selectedSavegame") == 1);
		point(290, 50);
		CHECK(number("system.selectedSavegame") == 2);
		point(500, 400);
		CHECK(number("system.selectedSavegame") == 2);

		// nothing in the selected place yet
		LuaDoString("trace = ''", "reset");
		run("IfSelected");
		CHECK(number("#trace") == 0);

		// saved over the selected: a savegame is made, in the three places and in a fourth by the script
		for (int i = 0; i < 3; i++) {
			point(50 + i * 120, 50);
			run("Save");
			printf("saved %d: %d savegames in the scene, %d on disk\n", i, number("system.savegamesCount"), (int)onDisk().size());
			CHECK(number("system.savegamesCount") == i + 1);
		}
		LuaDoString("system.selectedSavegame = 3", "select");
		run("Save");
		CHECK(number("system.savegamesCount") == 4);

		std::vector<int> slots = onDisk();

		printf("on disk: %d savegames\n", (int)slots.size());
		CHECK(slots.size() == 4);

		LuaDoString("trace = ''", "reset");
		run("IfSelected");
		CHECK(number("#trace") > 0);

		// scrolled by a page: forward is stopped at the end, backward at the start
		CHECK(number("system.savegamesScrollPos") == 0);
		run("Forward", 5);
		printf("scroll position %d\n", number("system.savegamesScrollPos"));
		CHECK(number("system.savegamesScrollPos") == 3);
		run("Forward", 5);
		CHECK(number("system.savegamesScrollPos") == 3);
		run("Backward", 5);
		CHECK(number("system.savegamesScrollPos") == 0);
		run("Backward", 5);
		CHECK(number("system.savegamesScrollPos") == 0);

		// the menu is entered again: the savegames are read from the disk, the last page is shown
		run("GoHall");
		CHECK(!control->GetScene()->IsMenu());
		run("GoMenu");
		CHECK(control->GetScene()->IsMenu());
		CHECK(number("system.savegamesCount") == 4);
		CHECK(number("system.savegamesScrollPos") == 3);
		CHECK(number("system.selectedSavegame") == -1);

		// one is deleted: from the scene and from the disk
		LuaDoString("system.selectedSavegame = 1", "select");
		run("Delete");
		CHECK(number("system.savegamesCount") == 3);
		CHECK(onDisk().size() == 3);

		// and the selected one is loaded
		LuaDoString("system.selectedSavegame = 0", "select");
		run("Load", 60);
		printf("loaded: menu %d\n", (int)control->GetScene()->IsMenu());

		// (the menu is not a scene to play in: the game goes on in the one before)
		CHECK(!control->GetScene()->IsMenu());
		clean();
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
