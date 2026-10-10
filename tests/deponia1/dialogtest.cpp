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

		// a dialog with two answers; each runs an action that prints
		TVisObjRef dialog = game.CreateObject(11, hero, kCharacterDialogs);
		dialog.SetName(TCharHolder("Chat"));
		const char *answers[2] = {"hello there", "good bye"};
		const char *prints[2] = {"picked_hello = (picked_hello or 0) + 1", "picked_bye = (picked_bye or 0) + 1"};
		for (int i = 0; i < 2; i++) {
			TVisObjRef part = game.CreateObject(12, dialog, kDialogDialogParts);
			part.SetValue(kDialogPartAvailable, true, TSendEventEnum::kNoEvent);
			TVisObjRef text = part.GetLink(kDialogPartText);
			std::vector<TTextLanguage> texts(1);
			texts[0].text = TCharHolder(answers[i]);
			texts[0].languageId = PackVisId(language.GetId());
			text.SetValue(kTextTextLanguages, texts, TSendEventEnum::kNoEvent);

			TVisObjRef reaction = game.CreateObject(7, gameRef, kGameActions);
			TVisObjRef script = game.CreateObject(8, reaction, kActionActionParts);
			script.SetValue(kActionPartCommand, kCommandRunPartScript, TSendEventEnum::kNoEvent);
			script.SetValue(kActionPartString, wxString(prints[i]), TSendEventEnum::kNoEvent);
			if (i == 1) {
				TVisObjRef end = game.CreateObject(8, reaction, kActionActionParts);
				end.SetValue(kActionPartCommand, kCommandEndDialog, TSendEventEnum::kNoEvent);
			}
			part.SetLink(kDialogPartAction, reaction, true);
		}

		addPart(kCommandWait, 30, 0, nullptr);
		TVisObjRef start = addPart(kCommandStartDialog, 0, 0, nullptr);
		start.SetLink(kActionPartLink, dialog, true);
		TVisObjRef after = addPart(kCommandRunPartScript, 0, 0, nullptr);
		after.SetValue(kActionPartString, wxString("print('dialog over')"), TSendEventEnum::kNoEvent);
		addPart(kCommandEndAction, 0, 0, nullptr);
		gameRef.SetLink(kGameStartAction, action, true);

		wxFileName out(L"dialog.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"dialog.dat");
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
		printf("dialog active: %d hovered %d\n", (int)!control->GetDialog()->IsEmpty(), control->GetDialog()->GetCurrentDialogPart());
		for (int click = 0; click < 3; click++) {
			control->ProcessMessage(static_cast<TMouseMessageEnum>(1), wxPoint{20, 5});
			control->ProcessMessage(static_cast<TMouseMessageEnum>(3), wxPoint{20, 5});
			control->ProcessMessage(static_cast<TMouseMessageEnum>(4), wxPoint{20, 5});
			frames(30);
			printf("dialog active after click %d: %d hovered %d\n", click, (int)!control->GetDialog()->IsEmpty(), control->GetDialog()->GetCurrentDialogPart());
		}
		lua_getfield(L, LUA_GLOBALSINDEX, "picked_hello");
		CHECK(lua_tointeger(L, -1) == 3);
		lua_settop(L, 0);
		printf("frames done\n");
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
