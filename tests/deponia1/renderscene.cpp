#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

#include "AppGlobals.h"
#include "SdlStub.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TGScene.h"
#include "TGCharacter.h"
#include "graphicslib/softwareGraphics.h"
#include "soundengine/audioMixer.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/visLua.h"
#include "vstables/eCommand.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"
#include "datastruct/vlist.h"
#include "datastruct/visionaireobject.h"

// Runs the real game data without a window and writes the frames as PNG pictures. Run it from the game directory:
//   renderscene <out directory> [frames [every [reduce]]]     (frames to run, a picture every `every` frames, made `reduce` times smaller)
int main(int argc, char **argv) {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = getenv("RENDER_LOG") ? atoi(getenv("RENDER_LOG")) : 1;

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
	// the directory of the settings and savegames of the player (a new one for each run: the game starts as it does for a
	// player who has not played yet); the scripts add "Savegames/..." to it
	std::string appDir = outDir + "/appdata/";

	std::filesystem::remove_all(appDir);
	std::filesystem::create_directories(appDir + "Savegames");
	InitPlayerCommands(control->GetVisionaire(), wxString(appDir.c_str()), wxString(L"."), wxString(L"."));

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

	if (getenv("RENDER_ACTION")) {
		TVisObjRef action = getenv("RENDER_ACTION")[0] >= '1' && getenv("RENDER_ACTION")[0] <= '9'
			? TVisObjRef(control->GetVisionaire()->GetObjectById(TId(atoi(getenv("RENDER_ACTION")), 7)))
			: TVisObjRef(control->GetVisionaire()->GetGame().GetLink(kGameStartAction));
		TVList parts;

		printf("start action '%s'", static_cast<const char *>(wxString(action.GetName()).mb_str()));
		putchar(10);
		action.GetLinks(kActionActionParts, TypeOrder::kValue1, parts);
		for (TVisionaireObject *part : parts) {
			printf("  command %d int %d string '%s' link '%s'", part->GetInt(kActionPartCommand), part->GetInt(kActionPartInt),
			       static_cast<const char *>(wxString(part->GetStr(kActionPartString)).mb_str()),
			       static_cast<const char *>(wxString(part->GetLink(kActionPartLink) ? part->GetLink(kActionPartLink)->GetName() : TCharHolder()).mb_str()));
			putchar(10);
		}
		return 0;
	}

	if (getenv("RENDER_VIDEOS")) {
		// the actions that play a movie, with the file
		TVList actions;

		control->GetVisionaire()->GetList(7, actions, false);
		for (TVisionaireObject *action : actions) {
			TVList parts;

			TVisObjRef(action).GetLinks(kActionActionParts, TypeOrder::kValue1, parts);
			for (TVisionaireObject *part : parts) {
				if (part->GetInt(kActionPartCommand) != kCommandPlayVideo)
					continue;

				printf("action %d '%s' plays %s\n", action->GetId24(), static_cast<const char *>(wxString(action->GetName()).mb_str()),
				       static_cast<const char *>(part->GetPath(kActionPartPath).GetFullPath().mb_str()));
			}
		}
		return 0;
	}

	if (getenv("RENDER_FIND")) {
		// the actions that go to a scene whose name starts with the argument
		std::string wanted = getenv("RENDER_FIND");
		TVList actions;

		control->GetVisionaire()->GetList(7, actions, false);
		for (TVisionaireObject *action : actions) {
			TVList parts;

			TVisObjRef(action).GetLinks(kActionActionParts, TypeOrder::kValue1, parts);
			for (TVisionaireObject *part : parts) {
				int command = part->GetInt(kActionPartCommand);

				if (command != 10 && command != 46)
					continue;

				TVisObjRef target(part->GetLink(kActionPartLink));

				if (target.IsEmpty())
					continue;

				TVisObjRef scene = target.GetParent();
				std::string name(static_cast<const char *>(wxString(scene.GetName()).mb_str()));

				if (name.compare(0, wanted.size(), wanted) == 0) {
					printf("action %d '%s' -> scene '%s' (command %d)", action->GetId24(), static_cast<const char *>(wxString(action->GetName()).mb_str()), name.c_str(), command);
					putchar(10);
				}
			}
		}
		return 0;
	}

	if (getenv("RENDER_DUMP")) {
		std::string wanted = getenv("RENDER_DUMP");
		TVList scenes;
		TVisObjRef first(control->GetVisionaire()->GetGame().GetLink(kGameFirstScene));

		printf("first scene '%s', first character '%s'", static_cast<const char *>(wxString(first.GetName()).mb_str()),
		       static_cast<const char *>(wxString(TVisObjRef(control->GetVisionaire()->GetGame().GetLink(kGameFirstCharacter)).GetName()).mb_str()));
		putchar(10);
		control->GetVisionaire()->GetList(4, scenes, false);
		for (TVisionaireObject *scene : scenes) {
			std::string name(static_cast<const char *>(wxString(scene->GetName()).mb_str()));

			if (name.compare(0, wanted.size(), wanted) != 0)
				continue;

			printf("scene '%s'", name.c_str());
			putchar(10);

			TVList objects;
			TVList actions;

			TVisObjRef(scene).GetLinks(kSceneActions, TypeOrder::kValue1, actions);
			for (TVisionaireObject *action : actions) {
				printf("  scene action %d '%s' type %d", action->GetId24(), static_cast<const char *>(wxString(action->GetName()).mb_str()), action->GetInt(kActionExecutionType));
				putchar(10);
			}
			TVisObjRef(scene).GetLinks(kSceneObjects, TypeOrder::kValue1, objects);
			for (TVisionaireObject *object : objects) {
				TVList objectActions;

				TVisObjRef(object).GetLinks(kObjectActions, TypeOrder::kValue1, objectActions);
				printf("  object '%s' condition %d, %d actions", static_cast<const char *>(wxString(object->GetName()).mb_str()), (int)object->GetInt(kObjectCondition), (int)objectActions.size());
				putchar(10);
			}
		}
		return 0;
	}

	// (the first events the headless SDL gives are that the window is hidden and closed: not for a run that plays)
	SDL_Event drained;

	while (SDL_PeepEvents(&drained, 1, SDL_GETEVENT, 0, 0xFFFF) > 0) {
	}

	// RENDER_AUDIO=<file.wav>: what the mixer renders (the sounds, the speech and the movies) is written there
	soundengine::WavWriter wav;

	if (getenv("RENDER_AUDIO") && wav.Open(getenv("RENDER_AUDIO"), soundengine::AudioMixer::kSampleRate, 2)) {
		soundengine::AudioMixer::Get().SetSink([&wav](const int16_t *samples, int frames) {
			wav.Write(samples, frames);
		});
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
		if (frame == (getenv("RENDER_LUA_FRAME") ? atoi(getenv("RENDER_LUA_FRAME")) : 3) && getenv("RENDER_LUA"))
			LuaDoString(getenv("RENDER_LUA"), "render");
		if (frame == 5 && getenv("RENDER_RUN")) {
			// starts the action with this id
			TVisObjRef action(control->GetVisionaire()->GetObjectById(TId(atoi(getenv("RENDER_RUN")), 7)));

			printf("running action '%s'\n", static_cast<const char *>(wxString(action.GetName()).mb_str()));
			TGAction::AddRunningAction(action);
		}
		if (getenv("RENDER_CLICK")) {
			// RENDER_CLICK=frame:x:y,frame:x:y... clicks with the left button at the position (of the 1920x1080 frame)
			std::string clicks = getenv("RENDER_CLICK");
			size_t at = 0;

			while (at < clicks.size()) {
				int clickFrame = 0, x = 0, y = 0, used = 0;

				if (sscanf(clicks.c_str() + at, "%d:%d:%d%n", &clickFrame, &x, &y, &used) < 3)
					break;
				at += used;
				if (at < clicks.size() && clicks[at] == ',')
					at++;
				if (clickFrame == frame) {
					printf("click %d, %d\n", x, y);
					wxPoint point;

					point.x = x;
					point.y = y;
					control->HandleMouseMove(point, false);
					control->HandleMouseUp(point, TMouseMessageEnum::kLeftDown);
					control->HandleMouseUp(point, TMouseMessageEnum::kLeftUp);
				}
			}
		}
		if (control->IsVideoPlaying()) {
			// (the main loop only runs the movie while one plays; without a player it ends at once)
			bool playing = control->VideoFrame();

			if (frame % every == every - 1) {
				std::string name = outDir + "/frame_" + std::to_string(frame + 1) + ".png";

				printf("frame %d: movie %s -> %s\n", frame + 1, playing ? "plays" : "is over", name.c_str());
				backend->SavePNG(name.c_str(), reduce);
			}
			wxMilliSleep(16);
			continue;
		}
		TGAction::ContinueRunningActions(false);
		control->Update();
		control->Draw(true);

		if (getenv("RENDER_RUNNING") && frame % every == every - 1) {
			std::list<wxString> lines;

			TGAction::PrintRunningActions(lines);
			for (const wxString &line : lines)
				printf("  running: %s", static_cast<const char *>(line.mb_str()));
			putchar(10);
		}
		if (frame % every == every - 1) {
			std::string name = outDir + "/frame_" + std::to_string(frame + 1) + ".png";
			TGScene *scene = control->GetScene();

			printf("frame %d: brightness %.2f, scene '%s', %d drawn -> %s\n", frame + 1, scene ? scene->GetBrightness() : -1.0f, scene ? static_cast<const char *>(scene->GetRef().GetName().mb_str()) : "(none)", backend->GetDrawnCount(), name.c_str());
			backend->SavePNG(name.c_str(), reduce);
		}
		wxMilliSleep(16);
	}
	soundengine::AudioMixer::Get().SetSink(nullptr);
	wav.Close();
	return 0;
}
