#include <chrono>
#include <cmath>
#include <chrono>
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
#include "fakepictures.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

class NullGraphics : public TGraphicsInterface {
};

// The hero says a text: the talk animation is shown, a click skips the text, and an unskipped text ends by itself.
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
		texts[0].text = TCharHolder("one<p2>two<p500ms>three<pt>four<p>five");
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
		say();
		mark("said = true");
		addPart(kCommandEndAction);
		gameRef.SetLink(kGameStartAction, action, true);

		// the same text said again
		TVisObjRef again = game.CreateObject(7, gameRef, kGameActions);
		again.SetName(TCharHolder("Again"));
		action = again;
		say();
		mark("said_again = true");

		wxFileName out(L"text.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"text.dat");
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

		// the text goes through its parts: the time of each is noted when it changes
		TVisObjRef game = control->GetVisionaire()->GetGame();
		std::vector<std::pair<std::string, int>> parts;
		std::string last;
		auto begin = std::chrono::steady_clock::now();
		int elapsed = 0;
		bool clicked = false;

		for (int frame = 0; frame < 3000 && !isSet("said"); frame++) {
			frames(1);
			elapsed = (int)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count();

			TVisObjRef active = game.GetLink(kGameCurrentText);
			std::string now = active.IsEmpty() ? std::string() : std::string(active.GetStr(kTextCurrentText).mb_str());

			if (now != last) {
				parts.push_back({now, elapsed});
				last = now;
			}

			// "four" waits until it is skipped
			if (now == "four" && !clicked && elapsed > parts.back().second + 700) {
				click(300, 300);
				clicked = true;
			}
		}

		for (const auto &part : parts)
			printf("part '%s' from %d ms\n", part.first.c_str(), part.second);
		CHECK(parts.size() >= 5);
		CHECK(isSet("said"));

		// the parts stay as long as their tags say: 2 seconds, 500 milliseconds, the length of "three" (130 ms a letter)
		auto duration = [&](size_t index) { return parts[index + 1].second - parts[index].second; };

		if (parts.size() >= 6) {
			CHECK(parts[0].first == "one" && parts[1].first == "two" && parts[2].first == "three" && parts[3].first == "four" &&
			      parts[4].first == "five");
			CHECK(std::abs(duration(0) - 2000) < 150);
			CHECK(std::abs(duration(1) - 500) < 150);
			CHECK(std::abs(duration(2) - 650) < 150);
			CHECK(duration(3) > 700);   // (until the click)
			CHECK(std::abs(duration(4) - 4 * 130) < 150);
		}

		// the player's text speed (percent) scales the times of the tags that go by a number or the length: twice as fast
		game.SetValue(kGameTextSpeed, 200, TSendEventEnum::kNoEvent);

		TVisObjRef againAction;
		control->GetVisionaire()->GetObjectByName(wxString(L"Again"), 7, againAction);
		TGAction::AddRunningAction(againAction);

		parts.clear();
		last.clear();
		clicked = false;
		begin = std::chrono::steady_clock::now();

		for (int frame = 0; frame < 3000 && !isSet("said_again"); frame++) {
			frames(1);
			elapsed = (int)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count();

			TVisObjRef active = game.GetLink(kGameCurrentText);
			std::string now = active.IsEmpty() ? std::string() : std::string(active.GetStr(kTextCurrentText).mb_str());

			if (now != last) {
				parts.push_back({now, elapsed});
				last = now;
			}

			if (now == "four" && !clicked && elapsed > parts.back().second + 300) {
				click(300, 300);
				clicked = true;
			}
		}

		for (const auto &part : parts)
			printf("faster: part '%s' from %d ms\n", part.first.c_str(), part.second);
		CHECK(parts.size() >= 6);
		if (parts.size() >= 6) {
			CHECK(std::abs(duration(0) - 1000) < 150);
			CHECK(std::abs(duration(1) - 250) < 150);
			CHECK(std::abs(duration(2) - 325) < 150);
			CHECK(std::abs(duration(4) - 2 * 130) < 150);
		}
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
