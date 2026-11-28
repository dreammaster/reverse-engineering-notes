#include <cstdio>
#include <string>
#include <vector>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TGScene.h"
#include "graphicslib/graphics.h"
#include "graphicslib/particleSystem.h"
#include "vscommon/objAccess.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/visLua.h"
#include "vstables/eCommand.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

// A backend that notes what the particles ask of it.
class RecordingGraphics : public TGraphicsInterface {
public:
	void SetOffset(float x, float y) override {
		offsets.push_back(wxPoint{(int)x, (int)y});
	}
	void Draw(TParticleSystem &system, bool first, bool second) override {
		systems.push_back(&system);
		lastFlags = (first ? 1 : 0) | (second ? 2 : 0);
		lastScroll = wxPoint{(int)system._scrollX, (int)system._scrollY};
		scrolls.push_back(lastScroll);
	}

	std::vector<wxPoint> offsets;
	std::vector<TParticleSystem *> systems;
	int lastFlags = -1;
	wxPoint lastScroll;
	std::vector<wxPoint> scrolls;
};

// The particles of a scene and of its objects: the backend is told to move by the scroll position, and to draw the
// particle systems.
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
		hero.SetLink(kCharacterCurrentOutfit, outfit, true);

		// a particle system without emitters, for the scene and for an object
		TVisObjRef particles = game.CreateObject(23, gameRef, kGameParticleContainerLinks);
		particles.SetName(TCharHolder("Dust"));
		scene.SetLink(kSceneParticleSystem, particles, true);

		TVisObjRef dusty = game.CreateObject(6, scene, kSceneObjects);
		dusty.SetName(TCharHolder("Dusty"));
		dusty.SetValue(kObjectPosition, wxPoint{300, 300}, TSendEventEnum::kNoEvent);
		dusty.SetLink(kObjectParticleSystem, particles, true);

		// and an object with a particle system made by a script
		TVisObjRef scripted = game.CreateObject(23, gameRef, kGameParticleContainerLinks);
		scripted.SetName(TCharHolder("Scripted"));
		scripted.SetValue(kParticleContainerSettings, wxString(L"{emitter={emitterType=\"point\"}}"), TSendEventEnum::kNoEvent);

		TVisObjRef smoky = game.CreateObject(6, scene, kSceneObjects);
		smoky.SetName(TCharHolder("Smoky"));
		smoky.SetValue(kObjectPosition, wxPoint{200, 100}, TSendEventEnum::kNoEvent);
		smoky.SetLink(kObjectParticleSystem, scripted, true);

		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		TVisObjRef change = game.CreateObject(8, action, kActionActionParts);
		change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		change.SetLink(kActionPartLink, startObject, true);
		change.SetLink(kActionPartAltLink, hero, true);
		gameRef.SetLink(kGameStartAction, action, true);

		wxFileName out(L"particle.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	RecordingGraphics *recorder = new RecordingGraphics();
	graphics = recorder;
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"particle.dat");
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

		control->GetScene()->SetWorktopSize(1000, 800);
		control->GetScene()->SetScrollPos(wxPoint{50, 20});
		recorder->offsets.clear();

		for (int i = 0; i < 30; i++) {
			TGAction::ContinueRunningActions(false);
			control->Update();
			control->Draw(true);
			wxMilliSleep(5);
		}

		printf("%d systems drawn, last flags %d\n", (int)recorder->systems.size(), recorder->lastFlags);
		// the scene and the object: two systems in each frame at least once the first starts (the timer lets the first frame pass)
		CHECK(recorder->systems.size() >= 2);
		CHECK(recorder->lastFlags == 2);
		// the scripted ones are moved by the scroll position (the scene's own by the position itself, the object's by
		// that and its own percent) and put back
		printf("%d offsets, scroll %d,%d\n", (int)recorder->offsets.size(), control->GetScene()->GetScrollPos().x,
		       control->GetScene()->GetScrollPos().y);
		CHECK(recorder->offsets.size() >= 2);
		CHECK(recorder->offsets.size() % 2 == 0);
		CHECK(recorder->offsets.back().x == 0 && recorder->offsets.back().y == 0);
		bool moved = false;

		for (const wxPoint &offset : recorder->offsets) {
			if (offset.y == -control->GetScene()->GetScrollPos().y && offset.y != 0)
				moved = true;
		}
		CHECK(moved);
		CHECK(recorder->offsets.size() >= 2 && recorder->offsets[recorder->offsets.size() - 2].x == -control->GetScene()->GetScrollPos().x);
		// the system of the object knows where the scene is scrolled to (the scene's own does not)
		bool known = false;

		for (const wxPoint &scroll : recorder->scrolls) {
			if (scroll.y == -control->GetScene()->GetScrollPos().y)
				known = true;
		}
		CHECK(known);

		// an earthquake shakes the picture for the time of a frame: what the particles are moved by changes, and the scroll
		// position of the scene is the same after the frame
		wxPoint stable = control->GetScene()->GetScrollPos();

		recorder->offsets.clear();
		control->StartEarthquake(20, 1);

		bool shaken = false;

		for (int i = 0; i < 20; i++) {
			TGAction::ContinueRunningActions(false);
			control->Update();
			control->Draw(true);
			wxMilliSleep(5);

			CHECK(control->GetScene()->GetScrollPos() == stable);
		}

		for (const wxPoint &offset : recorder->offsets) {
			if (offset.x != 0 && offset.x != -stable.x)
				shaken = true;
		}
		CHECK(shaken);
		control->StopEarthquake();
		recorder->offsets.clear();
		control->Draw(true);
		CHECK(recorder->offsets.size() >= 2 && recorder->offsets[0].x == -stable.x);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
