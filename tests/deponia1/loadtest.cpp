#include <cstdio>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "graphicslib/graphics.h"
#include "vscommon/objAccess.h"
#include "vscommon/scripting/visLua.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

class NullGraphics : public TGraphicsInterface {
};

int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	// a small project of our own: one scene, one character
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
		startObject.SetValue(kObjectDirection, 180, TSendEventEnum::kNoEvent);
		hero.SetLink(kCharacterStartObject, startObject, true);
		gameRef.SetLink(kGameFirstCharacter, hero, true);

		wxFileName out(L"synthetic.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}
	printf("saved\n");

	graphics = new NullGraphics();

	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"synthetic.dat");
	wxString warning;

	printf("PreLoad\n");
	CHECK(control->PreLoad(file, warning, true));
	printf("PreLoad done\n");

	wxString language;

	printf("LoadAndInitGame\n");
	bool loaded = control->LoadAndInitGame(file, warning, language, true);
	printf("LoadAndInitGame -> %d\n", loaded);
	CHECK(loaded);

	if (loaded) {
		control->RegisterEventHandler();
		control->InitAfterLoadingScreen();
		control->ExecuteStartingAction();
		printf("started\n");

		// run a few frames
		for (int i = 0; i < 5; i++) {
			TGAction::ContinueRunningActions(false);
			control->Update();
			control->Draw(true);
			control->ProcessMessage(static_cast<TMouseMessageEnum>(1), wxPoint{50 + i, 60});
		}
		control->ProcessMessage(static_cast<TMouseMessageEnum>(3), wxPoint{300, 300});
		control->ProcessMessage(static_cast<TMouseMessageEnum>(4), wxPoint{300, 300});
		for (int i = 0; i < 20; i++) {
			TGAction::ContinueRunningActions(false);
			control->Update();
			control->Draw(true);
		}
		{
			TVisObjRef g = control->GetVisionaire()->GetGame();
			TVisObjRef sc = control->GetCurrentCharacter()->GetRef().GetLink(kCharacterStartObject).GetParent();
			printf("scene empty: %d", (int)sc.IsEmpty());
			control->GetSceneControl()->ChangeScene(control->GetCurrentCharacter()->GetRef(), control->GetCurrentCharacter()->GetRef().GetLink(kCharacterStartObject), false, -1);
			for (int i = 0; i < 40; i++) {
				TGAction::ContinueRunningActions(false);
				control->Update();
				control->Draw(true);
				control->ProcessMessage(static_cast<TMouseMessageEnum>(1), wxPoint{200, 200});
			}
			control->ProcessMessage(static_cast<TMouseMessageEnum>(3), wxPoint{320, 400});
			control->ProcessMessage(static_cast<TMouseMessageEnum>(4), wxPoint{320, 400});
			for (int i = 0; i < 200; i++) {
				TGAction::ContinueRunningActions(false);
				control->Update();
				control->Draw(true);
			}
		}
		TVisObjRef cur = control->GetCurrentCharacter()->GetRef();
		printf("hero at %d,%d\n", cur.GetPoint(kCharacterPosition)->x, cur.GetPoint(kCharacterPosition)->y);
		printf("frames done\n");
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
