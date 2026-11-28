#include <cstdio>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
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

// An object without actions is clicked with a command: the hero says the comment for that command.
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

		// a rock without actions
		TVisObjRef rock = game.CreateObject(6, scene, kSceneObjects);
		rock.SetName(TCharHolder("Rock"));
		rock.SetValue(kObjectPosition, wxPoint{400, 300}, TSendEventEnum::kNoEvent);
		std::vector<wxPoint> rockPolygon = {{380, 280}, {420, 280}, {420, 320}, {380, 320}};
		rock.SetValue(kObjectPolygon, rockPolygon, TSendEventEnum::kNoEvent);

		// the commands the player can choose, and a language, a font and the comments of the hero
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
		hero.SetLink(kCharacterFont, font, true);

		TVisObjRef noParent;
		auto makeText = [&](const char *name, const char *words) {
			TVisObjRef text = game.CreateObject(14, noParent, -1);
			text.SetName(TCharHolder(name));
			std::vector<TTextLanguage> texts(1);
			texts[0].text = TCharHolder(words);
			texts[0].languageId = PackVisId(language.GetId());
			text.SetValue(kTextTextLanguages, texts, TSendEventEnum::kNoEvent);
			return text;
		};
		TVisObjRef useText = makeText("UseText", "i cannot use that");
		TVisObjRef lookText = makeText("LookText", "nothing special");

		TVisObjRef commandInterface = game.CreateObject(1, hero, kCharacterInterfaces);
		TVisObjRef useCommand = game.CreateObject(2, commandInterface, kInterfaceButtons);
		useCommand.SetName(TCharHolder("Use"));
		useCommand.SetValue(kButtonType, 3, TSendEventEnum::kNoEvent);
		useCommand.SetValue(kButtonCommandType, 1, TSendEventEnum::kNoEvent);
		TVisObjRef lookCommand = game.CreateObject(2, commandInterface, kInterfaceButtons);
		lookCommand.SetName(TCharHolder("Look"));
		lookCommand.SetValue(kButtonType, 3, TSendEventEnum::kNoEvent);
		lookCommand.SetValue(kButtonCommandType, 1, TSendEventEnum::kNoEvent);

		TVisObjRef comments = game.CreateObject(16, hero, kCharacterCommentSets);
		comments.SetName(TCharHolder("Comments"));
		auto makeEntry = [&](TVisObjRef command, TVisObjRef text) {
			TVisObjRef entry = game.CreateObject(27, comments, kCommentSetEntries);
			entry.SetLink(kCommentSetEntryText, text, true);
			if (!command.IsEmpty())
				entry.SetLink(kCommentSetEntryCommand, command, true);
		};
		makeEntry(useCommand, useText);
		makeEntry(lookCommand, lookText);
		hero.SetLink(kCharacterCurrentCommentSet, comments, true);

		// the start of the game: the scene is shown
		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		TVisObjRef change = game.CreateObject(8, action, kActionActionParts);
		change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		change.SetLink(kActionPartLink, startObject, true);
		change.SetLink(kActionPartAltLink, hero, true);
		gameRef.SetLink(kGameStartAction, action, true);

		wxFileName out(L"comment.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"comment.dat");
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
		auto click = [&](int x, int y) {
			control->ProcessMessage(static_cast<TMouseMessageEnum>(1), wxPoint{x, y});
			frames(3);
			control->ProcessMessage(static_cast<TMouseMessageEnum>(3), wxPoint{x, y});
			control->ProcessMessage(static_cast<TMouseMessageEnum>(4), wxPoint{x, y});
		};
		auto waitForText = [&](int frameLimit) {
			for (int i = 0; i < frameLimit; i++) {
				frames(1);
				if (!control->IsNoTextDisplayed())
					return true;
			}
			return false;
		};

		frames(20);

		TVisObjRef game = control->GetVisionaire()->GetGame();
		TVisObjRef useCommand;
		TVisObjRef lookCommand;
		TVisObjRef useText;
		TVisObjRef lookText;

		control->GetVisionaire()->GetObjectByName(wxString(L"UseText"), 14, useText);
		control->GetVisionaire()->GetObjectByName(wxString(L"LookText"), 14, lookText);
		control->GetVisionaire()->GetObjectByName(wxString(L"Use"), 2, useCommand);
		control->GetVisionaire()->GetObjectByName(wxString(L"Look"), 2, lookCommand);

		// the rock has no action for "use": the hero says what the comments say for that
		game.SetLink(kGameActiveCommand, useCommand, true);
		click(400, 300);
		bool said = waitForText(600);

		printf("the hero said something about using: %d\n", (int)said);
		CHECK(said);
		CHECK(control->IsTextActive(useText));
		CHECK(!control->IsTextActive(lookText));
		control->SkipCurrentText();
		frames(20);
		CHECK(control->IsNoTextDisplayed());

		// and for "look"
		game.SetLink(kGameActiveCommand, lookCommand, true);
		click(400, 300);
		said = waitForText(600);
		CHECK(said);
		CHECK(control->IsTextActive(lookText));
		control->SkipCurrentText();
		frames(20);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
