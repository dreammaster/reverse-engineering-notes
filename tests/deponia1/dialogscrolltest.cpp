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
#include "vscommon/scripting/visLua.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

class NullGraphics : public TGraphicsInterface {
};

int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

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

		// the language and the fonts of the dialog
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
		hero.SetLink(kCharacterActiveDialogFont, font, true);
		hero.SetLink(kCharacterInactiveDialogFont, font, true);
		hero.SetValue(kCharacterDialogArea, wxRect{0, 0, 300, 100}, TSendEventEnum::kNoEvent);

		// a dialog with fourteen answers (the area shows about eight): each runs a script that notes which it is
		TVisObjRef dialog = game.CreateObject(11, hero, kCharacterDialogs);
		dialog.SetName(TCharHolder("Chat"));
		for (int i = 0; i < 14; i++) {
			TVisObjRef part = game.CreateObject(12, dialog, kDialogDialogParts);
			part.SetValue(kDialogPartAvailable, true, TSendEventEnum::kNoEvent);
			TVisObjRef text = part.GetLink(kDialogPartText);
			std::vector<TTextLanguage> texts(1);
			std::string answer = std::string("pick ") + char('a' + i);
			texts[0].text = TCharHolder(answer.c_str());
			texts[0].languageId = PackVisId(language.GetId());
			text.SetValue(kTextTextLanguages, texts, TSendEventEnum::kNoEvent);

			TVisObjRef reaction = game.CreateObject(7, gameRef, kGameActions);
			TVisObjRef script = game.CreateObject(8, reaction, kActionActionParts);
			script.SetValue(kActionPartCommand, kCommandRunPartScript, TSendEventEnum::kNoEvent);
			script.SetValue(kActionPartString, wxString((std::string("picked = ") + std::to_string(i)).c_str()), TSendEventEnum::kNoEvent);
			TVisObjRef end = game.CreateObject(8, reaction, kActionActionParts);
			end.SetValue(kActionPartCommand, kCommandEndDialog, TSendEventEnum::kNoEvent);
			part.SetLink(kDialogPartAction, reaction, true);
		}

		addPart(kCommandWait, 30, 0, nullptr);
		TVisObjRef start = addPart(kCommandStartDialog, 0, 0, nullptr);
		start.SetLink(kActionPartLink, dialog, true);
		TVisObjRef after = addPart(kCommandRunPartScript, 0, 0, nullptr);
		after.SetValue(kActionPartString, wxString("print('dialog over')"), TSendEventEnum::kNoEvent);
		addPart(kCommandEndAction, 0, 0, nullptr);
		gameRef.SetLink(kGameStartAction, action, true);

		wxFileName out(L"dialogscroll.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"dialogscroll.dat");
	wxString warning;

	CHECK(control->PreLoad(file, warning, true));
	InitPlayerCommands(control->GetVisionaire(), wxString(L"."), wxString(L"."), wxString(L"."));
	wxString language;
	bool loaded = control->LoadAndInitGame(file, warning, language, true);

	CHECK(loaded);
	if (loaded) {
		control->RegisterEventHandler();
		control->InitAfterLoadingScreen();
		control->Draw(true);
		control->ExecuteStartingAction();

		auto frames = [&](int count) {
			for (int i = 0; i < count; i++) {
				TGAction::ContinueRunningActions(false);
				control->Update();
				control->Draw(true);
				wxMilliSleep(5);
			}
		};
		frames(30);
		CHECK(!control->GetDialog()->IsEmpty());

		auto pickAt = [&](int x, int y) {
			control->ProcessMessage(static_cast<TMouseMessageEnum>(1), wxPoint{x, y});
			frames(2);
			printf("hovered part %d at %d,%d\n", control->GetDialog()->GetCurrentDialogPart(), x, y);
			return control->GetDialog()->GetCurrentDialogPart();
		};
		auto wheel = [&](int message) {
			wxMilliSleep(30);
			// (a move of the mouse first: what the mouse was on, the arrow, stays set until it moves)
			control->ProcessMessage(static_cast<TMouseMessageEnum>(1), wxPoint{20, 5});
			control->ProcessMessage(static_cast<TMouseMessageEnum>(message), wxPoint{20, 5});
			frames(2);
		};

		// the answers that can be picked: those whose line is whole in the area (a line is some pixels high)
		auto visible = [&](int *lastY) {
			std::vector<int> parts;

			for (int y = 0; y < 100; y++) {
				control->ProcessMessage(static_cast<TMouseMessageEnum>(1), wxPoint{20, y});

				int part = control->GetDialog()->GetCurrentDialogPart();

				if (part >= 0 && (parts.empty() || parts.back() != part)) {
					parts.push_back(part);
					if (lastY)
						*lastY = y;
				}
			}
			return parts;
		};

		int lastY = 0;
		std::vector<int> parts = visible(&lastY);

		printf("visible at the start: %d answers, the first %d, the last %d\n", (int)parts.size(), parts.empty() ? -1 : parts.front(),
		       parts.empty() ? -1 : parts.back());
		CHECK(parts.size() >= 5 && parts.size() < 14);
		CHECK(!parts.empty() && parts.front() == 0);
		CHECK(pickAt(20, 400) == -1);

		// the wheel down scrolls a line: the first line is the second answer
		wheel(13);
		parts = visible(nullptr);
		CHECK(!parts.empty() && parts.front() == 1);
		wheel(13);
		wheel(13);
		parts = visible(nullptr);
		CHECK(!parts.empty() && parts.front() == 3);

		// and up again, and not above the first
		wheel(12);
		parts = visible(nullptr);
		CHECK(!parts.empty() && parts.front() == 2);
		for (int i = 0; i < 6; i++)
			wheel(12);
		parts = visible(nullptr);
		CHECK(!parts.empty() && parts.front() == 0);

		// down as far as it goes: the last answer is the last line, and no further
		for (int i = 0; i < 30; i++)
			wheel(13);
		parts = visible(&lastY);
		printf("visible at the end: %d answers, the first %d, the last %d (at y %d)\n", (int)parts.size(), parts.empty() ? -1 : parts.front(),
		       parts.empty() ? -1 : parts.back(), lastY);
		CHECK(!parts.empty() && parts.back() == 13);

		// a click picks the answer
		control->ProcessMessage(static_cast<TMouseMessageEnum>(1), wxPoint{20, lastY});
		control->ProcessMessage(static_cast<TMouseMessageEnum>(3), wxPoint{20, lastY});
		control->ProcessMessage(static_cast<TMouseMessageEnum>(4), wxPoint{20, lastY});
		// (what is said is read first, which takes a time, and then the action of the answer runs)
		for (int wait = 0; wait < 3000; wait++) {
			lua_getfield(L, LUA_GLOBALSINDEX, "picked");

			bool done = !lua_isnil(L, -1);

			lua_settop(L, 0);
			if (done)
				break;
			frames(1);
		}
		lua_getfield(L, LUA_GLOBALSINDEX, "picked");
		printf("picked %d\n", (int)lua_tointeger(L, -1));
		CHECK(lua_tointeger(L, -1) == 13);
		lua_settop(L, 0);
		printf("frames done\n");
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
