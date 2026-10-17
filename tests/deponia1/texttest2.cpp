#define private public
#define protected public
#include <cmath>
#include <chrono>
#include <cstdio>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TGCharacter.h"
#include "TTText.h"
#include "graphicslib/graphics.h"
#include "vscommon/objAccess.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/visLua.h"
#include "vstables/eCommand.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"
#include "fakepictures.h"
#include "TGText.h"
#include "TManagedObject.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

class NullGraphics : public TGraphicsInterface {
};

// The single text of a stopped game: ContinueStoppedText, DetachOwner and SetOwner.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	useFakePictures();
	writeFakePicture("pics/h1.png", 30, 60);
	writeFakePicture("pics/h2.png", 30, 60);
	writeFakePicture("pics/h3.png", 30, 60);

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

		// standing and talking, for four directions
		for (int kind = 0; kind < 2; kind++) {
			for (int direction = 0; direction < 360; direction += 90) {
				TVisObjRef anim = game.CreateObject(9, outfit, kind == 0 ? kOutfitTalkAnimations : kOutfitStandingAnimations);
				anim.SetName(TCharHolder(kind == 0 ? "Talk" : "Stand"));
				anim.SetValue(kAnimationDirection, direction, TSendEventEnum::kNoEvent);
				std::vector<TSprite> sprites(3);
				for (int k = 0; k < 3; k++) {
					sprites[k].SetPath(TCharHolder(k == 0 ? "pics/h1.png" : k == 1 ? "pics/h2.png" : "pics/h3.png"));
					sprites[k].SetPause(40);
				}
				anim.SetValue(kAnimationSprites, sprites, TSendEventEnum::kNoEvent);
			}
		}

		// a language, a font and a text the hero says
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
		TVisObjRef noParent;
		TVisObjRef text = game.CreateObject(14, noParent, -1);
		text.SetName(TCharHolder("Hello"));
		std::vector<TTextLanguage> texts(1);
		texts[0].text = TCharHolder("hello world");
		texts[0].languageId = PackVisId(language.GetId());
		text.SetValue(kTextTextLanguages, texts, TSendEventEnum::kNoEvent);

		// the start of the game: the scene is shown, the hero says the text twice, with marks between
		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		auto addPart = [&](int command) {
			TVisObjRef part = game.CreateObject(8, action, kActionActionParts);
			part.SetValue(kActionPartCommand, command, TSendEventEnum::kNoEvent);
			return part;
		};
		TVisObjRef change = addPart(kCommandChangeScene);
		change.SetLink(kActionPartLink, startObject, true);
		change.SetLink(kActionPartAltLink, hero, true);
		auto say = [&]() {
			TVisObjRef part = addPart(kCommandShowText);
			part.SetLink(kActionPartLink, text, true);
			part.SetLink(kActionPartAltLink, hero, true);
			part.SetValue(kActionPartInt, 0, TSendEventEnum::kNoEvent);
		};
		auto mark = [&](const char *code) {
			TVisObjRef part = addPart(kCommandRunPartScript);
			part.SetValue(kActionPartString, wxString(code), TSendEventEnum::kNoEvent);
		};
		mark("mark1 = true");
		say();
		mark("mark2 = true");
		say();
		mark("mark3 = true");
		addPart(kCommandEndAction);
		gameRef.SetLink(kGameStartAction, action, true);

		wxFileName out(L"text2.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"text2.dat");
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

		auto isSet = [&](const char *name) {
			lua_getfield(L, LUA_GLOBALSINDEX, name);
			bool result = !lua_isnil(L, -1);

			lua_settop(L, 0);
			return result;
		};

		TGCharacter *character = control->GetCurrentCharacter();
		TVisObjRef hero = character->GetRef();

		frames(30);
		printf("saying: mark1 %d, mark2 %d, anim state %d\n", (int)isSet("mark1"), (int)isSet("mark2"), hero.GetInt(kCharacterAnimState));
		CHECK(isSet("mark1"));
		CHECK(!isSet("mark2"));
		CHECK(hero.GetInt(kCharacterAnimState) == (int)TCharacterAnimEnum::kTalk);

		TGText *shown = control->_currentText;

		CHECK(shown != nullptr);
		if (shown) {
			CHECK(shown->GetSpeaker() == character);
			CHECK(!shown->_stopped);

			// the game stops: the text is stopped and does not run out while it waits
			TGText::StopRunningTexts();
			CHECK(shown->_stopped);
			wxMilliSleep(50);
			character->StopTalkAnim();
			CHECK(hero.GetInt(kCharacterAnimState) != (int)TCharacterAnimEnum::kTalk);

			// only this text goes on: the speaker talks again, the time of the stop is not counted
			shown->ContinueStoppedText();
			CHECK(!shown->_stopped);
			CHECK(hero.GetInt(kCharacterAnimState) == (int)TCharacterAnimEnum::kTalk);

			// the owner of the text can let go of it; the text then ends without telling anyone
			TManagedObject *owner = character;

			shown->SetOwner(owner);
			shown->DetachOwner(owner);
			CHECK(shown->_owner == nullptr);

			shown->SetOwner(owner);
			CHECK(shown->_owner == owner);
			shown->DetachOwner(owner);
		}

		// the text ends as usual after all that
		int frame = 0;

		for (; frame < 2000 && !isSet("mark2"); frame++)
			frames(1);
		printf("the text ended after %d frames\n", frame);
		CHECK(isSet("mark2"));
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
