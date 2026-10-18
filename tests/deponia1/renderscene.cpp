#include <cstdio>
#include <cstdlib>
#include <string>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TGScene.h"
#include "TGCharacter.h"
#include "graphicslib/softwareGraphics.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/visLua.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"
#include "datastruct/vlist.h"
#include "datastruct/visionaireobject.h"

// Runs the real game data without a window and writes the frames as PNG pictures. Run it from the game directory:
//   renderscene <out directory> [frames [every [reduce]]]     (frames to run, a picture every `every` frames, made `reduce` times smaller)
int main(int argc, char **argv) {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 1;

	std::string outDir = argc > 1 ? argv[1] : ".";
	int frames = argc > 2 ? atoi(argv[2]) : 120;
	int every = argc > 3 ? atoi(argv[3]) : 30;
	int reduce = argc > 4 ? atoi(argv[4]) : 2;

	TSoftwareGraphics *backend = new TSoftwareGraphics(1920, 1080);

	graphics = backend;
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"data.vis");
	wxString warning;

	if (!control->PreLoad(file, warning, true))
		return 1;
	InitPlayerCommands(control->GetVisionaire(), wxString(L"."), wxString(L"."), wxString(L"."));

	wxString language;

	if (!control->LoadAndInitGame(file, warning, language, true))
		return 2;

	if (getenv("RENDER_LIST")) {
		TVList scenes;

		control->GetVisionaire()->GetList(4, scenes, false);
		for (TVisionaireObject *scene : scenes) {
			printf("scene %d '%s' brightness %d lightmap '%s'", scene->GetId24(), static_cast<const char *>(wxString(scene->GetName()).mb_str()),
			       scene->GetInt(kSceneBrightness), static_cast<const char *>(scene->GetPath(kSceneLightMap).GetFullPath().mb_str()));
			putchar(10);
		}
		return 0;
	}

	control->RegisterEventHandler();
	control->InitAfterLoadingScreen();
	control->ExecuteStartingAction();

	for (int frame = 0; frame < frames; frame++) {
		if (frame == 10 && getenv("RENDER_SCENE")) {
			// goes to the first object of the scene that has this name (the first characters of it)
			std::string wanted = getenv("RENDER_SCENE");
			TVList scenes;

			control->GetVisionaire()->GetList(4, scenes, false);
			for (TVisionaireObject *scene : scenes) {
				std::string name(static_cast<const char *>(wxString(scene->GetName()).mb_str()));

				if (name.compare(0, wanted.size(), wanted) != 0)
					continue;

				TVList objects;

				TVisObjRef(scene).GetLinks(kSceneObjects, TypeOrder::kValue0, objects);
				if (objects.empty())
					continue;

				TVisObjRef target(objects.front());

				printf("going to '%s'\n", name.c_str());
				control->GetSceneControl()->ChangeScene(control->GetCurrentCharacter()->GetRef(), target, true, 0);
				// (the game's own actions take the scenes from the dark: the brightness is set by them)
				TVisObjRef(scene).SetValue(kSceneBrightness, 100, TSendEventEnum::kSendEvent);
				break;
			}
		}
		TGAction::ContinueRunningActions(false);
		control->Update();
		control->Draw(true);

		if (frame % every == every - 1) {
			std::string name = outDir + "/frame_" + std::to_string(frame + 1) + ".png";
			TGScene *scene = control->GetScene();

			printf("frame %d: brightness %.2f, scene '%s', %d drawn -> %s\n", frame + 1, scene ? scene->GetBrightness() : -1.0f, scene ? static_cast<const char *>(scene->GetRef().GetName().mb_str()) : "(none)", backend->GetDrawnCount(), name.c_str());
			backend->SavePNG(name.c_str(), reduce);
		}
		wxMilliSleep(16);
	}
	return 0;
}
