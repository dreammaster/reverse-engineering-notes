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
#include "vsplayer/animationGame.h"
#include "vstables/eCommand.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"
#include "fakepictures.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

class NullGraphics : public TGraphicsInterface {
};

// The commands of an action that work on animations: show and wait for the end, move, first and last frame, preload.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	useFakePictures();
	writeFakePicture("pics/a.png", 20, 20);
	writeFakePicture("pics/b.png", 20, 20);
	writeFakePicture("pics/c.png", 20, 20);

	{
		TVisionaireGame game;
		TVisObjRef gameRef = game.GetGame();
		gameRef.SetValue(kGameWindowResolution, wxPoint{640, 480}, TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameFadeEffect, 0, TSendEventEnum::kNoEvent);
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

		// two animations of the start object: one that runs twice, one that never ends
		auto makeAnimation = [&](const char *name, int loops) {
			TVisObjRef animation = game.CreateObject(9, startObject, kObjectAnimations);
			animation.SetName(TCharHolder(name));
			std::vector<TSprite> sprites(3);
			sprites[0].SetPath(TCharHolder("pics/a.png"));
			sprites[1].SetPath(TCharHolder("pics/b.png"));
			sprites[2].SetPath(TCharHolder("pics/c.png"));
			for (TSprite &sprite : sprites)
				sprite.SetPause(40);
			animation.SetValue(kAnimationSprites, sprites, TSendEventEnum::kNoEvent);
			animation.SetValue(kAnimationPosition, wxPoint{100, 100}, TSendEventEnum::kNoEvent);
			animation.SetValue(kAnimationNumberOfLoops, loops, TSendEventEnum::kNoEvent);
			return animation;
		};
		TVisObjRef snoop = makeAnimation("Snoop", 0);
		startObject.SetLink(kObjectSnoopAnimation, snoop, true);
		TVisObjRef twice = makeAnimation("Twice", 2);
		TVisObjRef forever = makeAnimation("Forever", 0);

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
		auto addPart = [&](TVisObjRef owner, int command, TVisObjRef link, int value, int altValue = 0, int altValue2 = 0) {
			TVisObjRef part = game.CreateObject(8, owner, kActionActionParts);
			part.SetValue(kActionPartCommand, command, TSendEventEnum::kNoEvent);
			if (!link.IsEmpty())
				part.SetLink(kActionPartLink, link, true);
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

		a = makeAction("ShowAndWait");
		addPart(a, kCommandShowAnimation, twice, 0, 0, 1);
		log(a, "twice-done");
		a = makeAction("ShowAndWaitLater");
		addPart(a, kCommandShowAnimation, twice, 0, 0, 0);
		log(a, "shown");
		addPart(a, kCommandWaitAnimation, twice, 0);
		log(a, "waited");
		a = makeAction("ShowForever");
		addPart(a, kCommandShowAnimation, forever, 0, 0, 0);
		a = makeAction("HideForever");
		addPart(a, kCommandShowAnimation, forever, 1);
		a = makeAction("MoveTo");
		addPart(a, kCommandMoveAnimation, forever, 300, 200, 0);
		a = makeAction("MoveMore");
		addPart(a, kCommandMoveAnimation, forever, 10, 5, 1);
		a = makeAction("MoveLess");
		addPart(a, kCommandMoveAnimation, forever, 4, 3, 2);
		a = makeAction("FirstFrame");
		addPart(a, kCommandSetFirstFrame, forever, 1);
		a = makeAction("LastFrame");
		addPart(a, kCommandSetLastFrame, forever, 1);
		a = makeAction("Preload");
		addPart(a, kCommandPreloadAnimation, forever, 0);
		a = makeAction("SnoopOn");
		addPart(a, kCommandSnoopAnimations, none, 0, 300);
		a = makeAction("SnoopOff");
		addPart(a, kCommandSnoopAnimations, none, 1, 300);
		a = makeAction("Unload");
		addPart(a, kCommandPreloadAnimation, forever, 1);

		wxFileName out(L"cmdanim.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"cmdanim.dat");
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

		// a part that waits until the animation has run twice (three pictures of 40 ms)
		run("ShowAndWait", 5);
		CHECK(trace() == "");
		frames(100);
		CHECK(trace() == "twice-done ");

		// the same with a part of its own that waits
		run("ShowAndWaitLater", 3);
		CHECK(trace() == "shown ");
		frames(100);
		CHECK(trace() == "waited ");

		// an animation that never ends can be moved, limited in its frames and hidden
		TVisObjRef forever;
		control->GetVisionaire()->GetObjectByName(wxString(L"Forever"), 9, forever);
		run("ShowForever", 20);

		TVisObjRef running = control->GetVisionaire()->GetActiveObject(26, TId(PackVisId(forever.GetId()), forever.GetId()[3]));

		CHECK(!running.IsEmpty());
		printf("running animation at %d,%d\n", running.GetPoint(kAnimationCurrentPosition)->x, running.GetPoint(kAnimationCurrentPosition)->y);
		run("MoveTo");
		CHECK(running.GetPoint(kAnimationCurrentPosition)->x == 300 && running.GetPoint(kAnimationCurrentPosition)->y == 200);
		run("MoveMore");
		CHECK(running.GetPoint(kAnimationCurrentPosition)->x == 310 && running.GetPoint(kAnimationCurrentPosition)->y == 205);
		run("MoveLess");
		CHECK(running.GetPoint(kAnimationCurrentPosition)->x == 306 && running.GetPoint(kAnimationCurrentPosition)->y == 202);
		run("FirstFrame");
		CHECK(running.GetInt(kAnimationFirstFrame) == 1);
		run("LastFrame");
		CHECK(running.GetInt(kAnimationLastFrame) == 1);
		frames(30);
		run("Unload");
		run("Preload");

		// the snoop animations of the objects fade in with the time of the part, and out again
		TGScene *scene = control->GetScene();
		int before = (int)TGAnimation::GetRunningAnimations().size();

		CHECK(scene->GetSnoopAnimAlpha() == 0.0f);
		run("SnoopOn", 5);
		for (int i = 0; i < 10; i++)
			frames(1);
		float midway = scene->GetSnoopAnimAlpha();

		printf("snoop alpha early: %g, animations %d (before %d)\n", midway, (int)TGAnimation::GetRunningAnimations().size(), before);
		CHECK((int)TGAnimation::GetRunningAnimations().size() == before + 1);
		frames(150);
		CHECK(scene->GetSnoopAnimAlpha() == 1.0f);
		CHECK(midway <= 1.0f);
		run("SnoopOff", 5);
		frames(150);
		printf("snoop alpha at the end: %g, animations %d\n", scene->GetSnoopAnimAlpha(), (int)TGAnimation::GetRunningAnimations().size());
		CHECK(scene->GetSnoopAnimAlpha() == 0.0f);
		CHECK((int)TGAnimation::GetRunningAnimations().size() == before);
		run("HideForever", 20);
		printf("animations running at the end: %d\n", (int)TGAnimation::GetRunningAnimations().size());
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
