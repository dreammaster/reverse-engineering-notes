#include <cstdio>
#include <string>

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

// Dialogs that lead to others: a part that is used up (remove) and returns to the same dialog, a part that goes to
// another dialog, one that goes back, and one that ends the dialog.
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
		auto addPart = [&](TVisObjRef owner, int command, int intValue) {
			TVisObjRef part = game.CreateObject(8, owner, kActionActionParts);
			part.SetValue(kActionPartCommand, command, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartInt, intValue, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartAltInt, 0, TSendEventEnum::kNoEvent);
			return part;
		};
		TVisObjRef change = addPart(action, kCommandChangeScene, 0);
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

		TVisObjRef root = game.CreateObject(11, hero, kCharacterDialogs);
		root.SetName(TCharHolder("Root"));
		TVisObjRef sub = game.CreateObject(11, hero, kCharacterDialogs);
		sub.SetName(TCharHolder("Sub"));

		auto makePart = [&](TVisObjRef dialog, const char *line, const char *mark) {
			TVisObjRef part = game.CreateObject(12, dialog, kDialogDialogParts);
			part.SetValue(kDialogPartAvailable, true, TSendEventEnum::kNoEvent);
			TVisObjRef text = part.GetLink(kDialogPartText);
			std::vector<TTextLanguage> texts(1);
			texts[0].text = TCharHolder(line);
			texts[0].languageId = PackVisId(language.GetId());
			text.SetValue(kTextTextLanguages, texts, TSendEventEnum::kNoEvent);

			TVisObjRef reaction = game.CreateObject(7, gameRef, kGameActions);
			TVisObjRef script = addPart(reaction, kCommandRunPartScript, 0);
			script.SetValue(kActionPartString, wxString((std::string("trace = (trace or '') .. '") + mark + " '").c_str()), TSendEventEnum::kNoEvent);
			part.SetLink(kDialogPartAction, reaction, true);
			return std::make_pair(part, reaction);
		};

		// the root: a part that is used up and returns to the dialog, one that goes to the sub dialog, one that ends it
		auto ask = makePart(root, "ask something", "ask");
		ask.first.SetValue(kDialogPartRemove, true, TSendEventEnum::kNoEvent);
		ask.first.SetValue(kDialogPartReturn, 0, TSendEventEnum::kNoEvent);
		auto deeper = makePart(root, "go deeper", "deeper");
		deeper.first.SetLink(kDialogPartNextDialog, sub, true);
		auto bye = makePart(root, "good bye", "bye");
		addPart(bye.second, kCommandEndDialog, 0);

		// the sub dialog: a part that returns to the same dialog, and one that goes back to the root
		auto again = makePart(sub, "say it again", "again");
		again.first.SetValue(kDialogPartReturn, 0, TSendEventEnum::kNoEvent);
		auto back = makePart(sub, "go back", "back");
		back.first.SetLink(kDialogPartNextDialog, root, true);

		addPart(action, kCommandWait, 30).SetValue(kActionPartAltInt, 0, TSendEventEnum::kNoEvent);
		TVisObjRef start = addPart(action, kCommandStartDialog, 0);
		start.SetLink(kActionPartLink, root, true);
		gameRef.SetLink(kGameStartAction, action, true);

		wxFileName out(L"dialog2.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"dialog2.dat");
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
		auto trace = [&]() {
			lua_getfield(L, LUA_GLOBALSINDEX, "trace");
			std::string result = lua_tostring(L, -1) ? lua_tostring(L, -1) : "";
			lua_settop(L, 0);
			return result;
		};
		// what is said is read first (it takes a time), then the dialog is there again (or is over)
		auto waitForDialog = [&]() {
			for (int wait = 0; wait < 4000 && control->GetDialog()->IsEmpty(); wait++)
				frames(1);
		};
		// the number of parts that can be picked: the lines the mouse can hover over
		auto partCount = [&]() {
			int highest = -1;

			for (int y = 0; y < 100; y += 2) {
				control->ProcessMessage(static_cast<TMouseMessageEnum>(1), wxPoint{20, y});
				highest = std::max(highest, control->GetDialog()->GetCurrentDialogPart());
			}
			return highest + 1;
		};
		// a click on a part
		auto pick = [&](int part) {
			for (int y = 0; y < 100; y += 2) {
				control->ProcessMessage(static_cast<TMouseMessageEnum>(1), wxPoint{20, y});
				if (control->GetDialog()->GetCurrentDialogPart() == part) {
					control->ProcessMessage(static_cast<TMouseMessageEnum>(3), wxPoint{20, y});
					control->ProcessMessage(static_cast<TMouseMessageEnum>(4), wxPoint{20, y});
					frames(5);
					waitForDialog();
					return true;
				}
			}
			printf("no part %d to pick\n", part);
			return false;
		};

		frames(40);
		CHECK(!control->GetDialog()->IsEmpty());
		printf("the root has %d parts\n", partCount());
		CHECK(partCount() == 3);

		// the first part: used up, the root again with two parts
		CHECK(pick(0));
		printf("trace: %s; parts %d\n", trace().c_str(), partCount());
		CHECK(trace() == "ask ");
		CHECK(partCount() == 2);

		// "go deeper": the sub dialog, with two parts
		CHECK(pick(0));
		printf("trace: %s; parts %d\n", trace().c_str(), partCount());
		CHECK(trace() == "ask deeper ");
		CHECK(partCount() == 2);

		// "say it again" returns to the sub dialog
		CHECK(pick(0));
		CHECK(trace() == "ask deeper again ");
		CHECK(partCount() == 2);

		// "go back": the root, with its two parts that are left
		CHECK(pick(1));
		CHECK(trace() == "ask deeper again back ");
		CHECK(partCount() == 2);

		// "good bye" ends the dialog, and the action goes on
		CHECK(pick(1));
		frames(50);
		printf("trace: %s\n", trace().c_str());
		CHECK(trace() == "ask deeper again back bye ");
		CHECK(control->GetDialog()->IsEmpty());
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
