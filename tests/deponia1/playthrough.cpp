#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "AppGlobals.h"
#include "SdlStub.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TGDialog.h"
#include "TGInterface.h"
#include "TGScene.h"
#include "TGCharacter.h"
#include "graphicslib/softwareGraphics.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/visLua.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"
#include "datastruct/vlist.h"
#include "datastruct/visionaireobject.h"
#include "soundengine/audioMixer.h"

// A player that follows the hotspots: it starts a new game of the real data (run from the game directory) and goes on by clicking on what
// the scene shows, the one that was tried the least first, answers the dialogs, skips the movies it may and writes down the scenes it
// gets to and the actions that run. A way to see how far the engine takes a game that nobody plays.
//   playthrough <out directory> [steps]       environment: PLAY_SCALE=<speed of the time, 8>, PLAY_SEED=<1>, PLAY_STOP=<scene prefix to stop at>
static THGameControl *control = nullptr;
static TSoftwareGraphics *backend = nullptr;
static std::string outDir;
static std::mt19937 randomNumbers;

static std::string name(const TVisObjRef &object) {
	return static_cast<const char *>(wxString(object.GetName()).mb_str());
}

static std::string sceneName() {
	TGScene *scene = control->GetScene();

	return scene ? name(scene->GetRef()) : "(none)";
}

/** One frame of the game as the main loop makes it (a movie that plays is run instead). */
static void frame(bool skipMovies = true) {
	if (control->IsVideoPlaying()) {
		if (skipMovies) {
			SDL_Event event;

			event.type = SDL_KEYDOWN;
			SDL_PushEvent(&event);
		}
		control->VideoFrame();
		wxMilliSleep(16);
		return;
	}

	TGAction::ContinueRunningActions(false);
	control->Update();
	control->Draw(true);
	wxMilliSleep(16);
}

/** A click as the main loop sends it: a move, the button going down, the button going up (ProcessMessage: the scripts' mouse
 *  handlers, then the game). */
static void click(int x, int y, bool right = false) {
	wxPoint point;

	point.x = x;
	point.y = y;
	control->ProcessMessage(TMouseMessageEnum::kMove, point);
	control->ProcessMessage(right ? TMouseMessageEnum::kRightDown : TMouseMessageEnum::kLeftDown, point);
	control->ProcessMessage(right ? TMouseMessageEnum::kRightUp : TMouseMessageEnum::kLeftUp, point);
}

static void wheel(bool down) {
	wxPoint point;

	point.x = 960;
	point.y = 540;
	control->ProcessMessage(down ? TMouseMessageEnum::kWheelDown : TMouseMessageEnum::kWheelUp, point);
}

static std::set<std::string> background = {"run_checks", "item_on_cursor_check"};   // the actions that always run (the checks of the game)

/** The names of the actions that run now and are not part of the background. */
static std::vector<std::string> foreground() {
	std::vector<std::string> result;

	for (TGAction *action : TGAction::GetRunningActions()) {
		std::string actionName = name(action->GetDataObject());

		if (!background.count(actionName))
			result.push_back(actionName);
	}

	return result;
}

static bool idle() {
	if (control->IsVideoPlaying())
		return false;

	// (a character that walks is on the way to what was clicked)
	TGCharacter *walker = control->GetCurrentCharacter();

	if (walker && walker->IsWalking())
		return false;

	if (!control->GetCursorControl()->IsActive())
		return false;

	if (!control->GetDialog()->IsEmpty())
		return true;   // (a dialog is waiting for the player)

	return control->IsNoTextDisplayed() && foreground().empty();
}

static std::set<std::string> everRun;           // the names of all the actions that ran
static std::vector<std::string> newActions;     // those that ran for the first time since the last look

static void noteActions() {
	for (const std::string &actionName : foreground()) {
		if (everRun.insert(actionName).second)
			newActions.push_back(actionName);
	}
}

/** Runs frames until the game waits for the player (a number of frames in a row), at most `limit` frames. */
static int settle(int limit = 1500, int quiet = 20) {
	int quietFrames = 0;
	int frames = 0;

	while (frames < limit && quietFrames < quiet) {
		frame();
		noteActions();
		quietFrames = idle() ? quietFrames + 1 : 0;
		frames++;
	}

	return frames;
}

static void snapshot(const std::string &label) {
	backend->SetRendering(true);
	frame();
	backend->SavePNG((outDir + "/" + label + ".png").c_str(), 4);
	backend->SetRendering(false);
}

struct Hotspot {
	std::string _name;
	long long _x = 0;
	long long _y = 0;
	int _points = 0;
};

/** What the scene shows that the player can click on: the objects found on a grid of the screen, each at the middle of its points. */
static std::vector<Hotspot> scan() {
	std::map<std::string, Hotspot> found;
	TGScene *scene = control->GetScene();

	if (!scene)
		return {};

	for (int y = 20; y < 1080; y += 40) {
		for (int x = 20; x < 1920; x += 40) {
			wxPoint point;

			point.x = x;
			point.y = y;

			TManagedObject *object = scene->GetObject(point);

			if (!object)
				continue;

			Hotspot &hotspot = found[name(object->GetRef())];

			hotspot._name = name(object->GetRef());
			hotspot._x += x;
			hotspot._y += y;
			hotspot._points++;
		}
	}

	std::vector<Hotspot> result;

	for (auto &entry : found) {
		entry.second._x /= entry.second._points;
		entry.second._y /= entry.second._points;
		result.push_back(entry.second);
	}

	return result;
}

/** The objects of an interface of the game (by its name) at the places of a grid, each at the middle of its points. */
static std::vector<Hotspot> scanInterface(const std::string &interfaceName) {
	std::map<std::string, Hotspot> found;

	for (TGInterface *interface : control->GetActiveInterfaces()) {
		if (name(interface->GetRef()) != interfaceName)
			continue;

		for (int y = 10; y < 1080; y += 20) {
			for (int x = 10; x < 1920; x += 20) {
				wxPoint point;

				point.x = x;
				point.y = y;

				if (!interface->IsInside(point))
					continue;

				TManagedObject *object = interface->GetObject(point);

				if (!object)
					continue;

				Hotspot &hotspot = found[name(object->GetRef())];

				hotspot._name = name(object->GetRef());
				hotspot._x += x;
				hotspot._y += y;
				hotspot._points++;
			}
		}
	}

	std::vector<Hotspot> result;

	for (auto &entry : found) {
		entry.second._x /= entry.second._points;
		entry.second._y /= entry.second._points;
		result.push_back(entry.second);
	}

	return result;
}

/** The items of the inventory (when it is open): the objects of its interface that are not a slot or a button. */
static std::vector<Hotspot> scanInventory() {
	std::map<std::string, Hotspot> found;

	for (TGInterface *interface : control->GetActiveInterfaces()) {
		if (name(interface->GetRef()) != "Inventory")
			continue;

		for (int y = 10; y < 1080; y += 20) {
			for (int x = 10; x < 1920; x += 20) {
				wxPoint point;

				point.x = x;
				point.y = y;

				if (!interface->IsInside(point))
					continue;

				TManagedObject *object = interface->GetObject(point);

				if (!object)
					continue;

				std::string objectName = name(object->GetRef());

				if (objectName.compare(0, 4, "Slot") == 0 || objectName.compare(0, 4, "btn_") == 0)
					continue;

				Hotspot &hotspot = found[objectName];

				hotspot._name = objectName;
				hotspot._x += x;
				hotspot._y += y;
				hotspot._points++;
			}
		}
	}

	std::vector<Hotspot> result;

	for (auto &entry : found) {
		entry.second._x /= entry.second._points;
		entry.second._y /= entry.second._points;
		result.push_back(entry.second);
	}

	return result;
}

static void listInterfaces() {
	// the objects of the interfaces (the inventory, the menu button ...) at the places of a grid, with the inventory open and not
	if (getenv("PLAY_LUA"))
		LuaDoString(getenv("PLAY_LUA"), "play");

	for (int open = 0; open < 2; open++) {
		if (open) {
			wheel(true);
			settle(600, 30);
		}

		for (TGInterface *interface : control->GetAllInterfaces())
			printf("  (interface '%s', active %d)\n", name(interface->GetRef()).c_str(), (int)interface->IsActive());

		std::map<std::string, int> seen;

		for (TGInterface *interface : control->GetActiveInterfaces()) {
			for (int y = 10; y < 1080; y += 20) {
				for (int x = 10; x < 1920; x += 20) {
					wxPoint point;

					point.x = x;
					point.y = y;

					if (!interface->IsInside(point))
						continue;

					TManagedObject *object = interface->GetObject(point);

					if (object)
						seen[name(interface->GetRef()) + " / " + name(object->GetRef())]++;
				}
			}
		}

		printf("interface objects, inventory %s:\n", open ? "open" : "closed");
		for (const auto &entry : seen)
			printf("  %s (%d points)\n", entry.first.c_str(), entry.second);
	}
	}

int main(int argc, char **argv) {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	outDir = argc > 1 ? argv[1] : ".";

	const int maxSteps = argc > 2 ? atoi(argv[2]) : 200;
	const double scale = getenv("PLAY_SCALE") ? atof(getenv("PLAY_SCALE")) : 8.0;
	const std::string stopAt = getenv("PLAY_STOP") ? getenv("PLAY_STOP") : "";

	randomNumbers.seed(getenv("PLAY_SEED") ? atoi(getenv("PLAY_SEED")) : 1);

	backend = new TSoftwareGraphics(1920, 1080);
	backend->SetRendering(false);
	graphics = backend;
	control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"data.vis");
	wxString warning;

	if (!control->PreLoad(file, warning, true))
		return 1;

	std::string appDir = outDir + "/appdata/";

	std::filesystem::remove_all(appDir);
	std::filesystem::create_directories(appDir + "Savegames");
	InitPlayerCommands(control->GetVisionaire(), wxString(appDir.c_str()), wxString(L"."), wxString(L"."));

	wxString language;

	if (!control->LoadAndInitGame(file, warning, language, true))
		return 2;

	SDL_Event drained;

	while (SDL_PeepEvents(&drained, 1, SDL_GETEVENT, 0, 0xFFFF) > 0) {
	}

	control->RegisterEventHandler();
	control->InitAfterLoadingScreen();
	control->ExecuteStartingAction();
	SetEngineTimeScale(scale);

	// the start: the language select, the menus, a new game
	TVisObjRef beginAction(control->GetVisionaire()->GetObjectById(TId(12914, 7)));

	TGAction::AddRunningAction(beginAction);
	for (int i = 0; i < 60; i++)
		frame();

	auto waitScene = [&](const char *wanted, int frames) {
		for (int i = 0; i < frames && sceneName() != wanted; i++)
			frame();
		settle(2000, 30);
	};

	// (the places of what is clicked are those of the screens of the game: the check of the language, New Game, the questions)
	click(944, 880);
	waitScene("sys_main_menu", 3000);
	click(300, 380);
	settle(600, 30);
	click(1008, 560);
	settle(600, 30);
	click(960, 600);
	waitScene("0000 - Tutorial", 3000);

	// the actions that always run
	for (int i = 0; i < 120; i++)
		frame();
	for (const std::string &actionName : foreground())
		background.insert(actionName);

	printf("playing: in '%s', %d background actions\n", sceneName().c_str(), (int)background.size());

	if (!getenv("PLAY_NOSKIP")) {
		// the game lets the player skip the tutorial: the skip button, and a question that has to be answered
		for (const Hotspot &hotspot : scanInterface("skipButton")) {
			if (hotspot._name != "hs_skip")
				continue;

			click(static_cast<int>(hotspot._x), static_cast<int>(hotspot._y));
			settle(300, 20);
			printf("skip tutorial: clicked '%s'; the question:", hotspot._name.c_str());

			std::vector<Hotspot> answers = scanInterface("skipQuestion");

			for (const Hotspot &answer : answers)
				printf(" [%s at %lld,%lld]", answer._name.c_str(), answer._x, answer._y);
			printf("\n");

			for (const Hotspot &answer : answers) {
				std::string lower = answer._name;

				std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });

				if (lower.find("yes") != std::string::npos || lower.find("ja") != std::string::npos || lower.find("ok") != std::string::npos) {
					click(static_cast<int>(answer._x), static_cast<int>(answer._y));
					settle(1500, 30);
					printf("skip tutorial: answered '%s', now in '%s'\n", answer._name.c_str(), sceneName().c_str());
					break;
				}
			}
			break;
		}
	}

	if (getenv("PLAY_JUMP")) {
		// goes to a scene (the first characters of its name) as the dev tools do: to see what the game does there
		std::string wanted = getenv("PLAY_JUMP");
		TVList sceneList;

		control->GetVisionaire()->GetList(4, sceneList, false);
		for (TVisionaireObject *scene : sceneList) {
			std::string sceneObjectName = name(TVisObjRef(scene));

			if (sceneObjectName.compare(0, wanted.size(), wanted) != 0)
				continue;

			TVList objects;

			TVisObjRef(scene).GetLinks(kSceneObjects, TypeOrder::kValue0, objects);
			if (objects.empty())
				continue;

			control->GetSceneControl()->ChangeScene(control->GetCurrentCharacter()->GetRef(), TVisObjRef(objects.front()), true, 0);
			printf("jumped to '%s'\n", sceneObjectName.c_str());
			settle(600, 30);
			break;
		}
	}

	std::map<std::string, int> tries;
	std::vector<std::string> scenes;
	int sinceNew = 0;

	for (int step = 0; step < maxSteps; step++) {
		int frames = settle();

		if (getenv("PLAY_LISTUI") && step == atoi(getenv("PLAY_LISTUI"))) {
			listInterfaces();
			return 0;
		}
		std::string scene = sceneName();

		if (scenes.empty() || scenes.back() != scene) {
			scenes.push_back(scene);
			printf("== scene '%s' (step %d)\n", scene.c_str(), step);
			snapshot("scene_" + std::to_string(scenes.size()));
			sinceNew = 0;
		}

		if (!stopAt.empty() && scene.compare(0, stopAt.size(), stopAt) == 0) {
			printf("reached '%s'\n", scene.c_str());
			break;
		}

		std::string what;

		if (!control->GetDialog()->IsEmpty()) {
			// a dialog: a line, the one tried the least
			const std::vector<wxRect> lines = control->GetDialog()->GetLineRects();
			const std::string dialogName = name(control->GetDialog()->GetTarget());

			if (getenv("PLAY_DIALOGSHOTS"))
				snapshot("dialog_" + std::to_string(step));

			if (!lines.empty()) {
				int best = 0;
				int bestTries = 1 << 30;

				for (size_t i = 0; i < lines.size(); i++) {
					int count = tries["dialog " + dialogName + " " + std::to_string(i)];

					if (count < bestTries) {
						bestTries = count;
						best = static_cast<int>(i);
					}
				}

				tries["dialog " + dialogName + " " + std::to_string(best)]++;
				click(lines[best].x + lines[best].width / 2, lines[best].y + lines[best].height / 2);
				what = "dialog '" + dialogName + "' line " + std::to_string(best) + " of " + std::to_string(lines.size());
			} else {
				click(960, 900);
				what = "dialog (no lines)";
			}
		} else {
			TVisObjRef usedItem = control->GetVisionaire()->GetGame().GetLink(kGameUsedItem);
			std::string itemName = usedItem.IsEmpty() ? std::string() : name(usedItem);
			std::vector<Hotspot> hotspots = scan();

			// the button that leaves a close-up (an object of an interface, not of the scene)
			for (const Hotspot &button : scanInterface("skipButton")) {
				if (button._name == "hs_leave") {
					Hotspot leave = button;

					leave._name = "(interface) " + button._name;
					hotspots.push_back(leave);
				}
			}

			if (sinceNew >= 12 && sinceNew % 4 == 0) {
				// nothing happens: a way out is an empty place (a close-up is left so) or the right button
				TGScene *current = control->GetScene();
				wxPoint empty;

				empty.x = 960;
				empty.y = 100;

				for (int attempt = 0; attempt < 60 && current; attempt++) {
					wxPoint candidate;

					candidate.x = static_cast<int>(randomNumbers() % 1920);
					candidate.y = static_cast<int>(randomNumbers() % 1080);

					if (!current->GetObject(candidate)) {
						empty = candidate;
						break;
					}
				}

				bool right = (sinceNew / 4) % 2 == 0;

				click(empty.x, empty.y, right);
				what = std::string("stuck: a ") + (right ? "right" : "left") + " click on an empty place";
			}

			if (what.empty() && itemName.empty() && randomNumbers() % 4 == 0) {
				// the inventory is opened (a turn of the wheel) and an item put on the cursor, the one tried the least
				wheel(true);
				settle(300, 15);

				std::vector<Hotspot> items = scanInventory();

				if (!items.empty()) {
					Hotspot chosen;
					int bestTries = 1 << 30;

					for (const Hotspot &item : items) {
						int count = tries["take/" + item._name] * 2 + static_cast<int>(randomNumbers() % 2);

						if (count < bestTries) {
							bestTries = count;
							chosen = item;
						}
					}

					tries["take/" + chosen._name]++;
					click(static_cast<int>(chosen._x), static_cast<int>(chosen._y));
					settle(120, 10);

					TVisObjRef nowUsed = control->GetVisionaire()->GetGame().GetLink(kGameUsedItem);

					what = "took the item '" + chosen._name + "' (cursor: '" + (nowUsed.IsEmpty() ? std::string("nothing") : name(nowUsed)) + "' at " +
					       std::to_string(chosen._x) + "," + std::to_string(chosen._y) + ")";
				}
			}

			if (what.empty() && !itemName.empty()) {
				// an item on the cursor: a hotspot of the scene (or another item of the inventory) to use it on, the one tried the least;
				// an item that has been tried enough is put back (the right button)
				std::vector<Hotspot> targets = hotspots;

				wheel(true);
				settle(300, 15);
				for (const Hotspot &item : scanInventory())
					targets.push_back(item);

				Hotspot chosen;
				int bestTries = 1 << 30;

				for (const Hotspot &target : targets) {
					int count = tries["use/" + itemName + "/" + scene + "/" + target._name] * 2 + static_cast<int>(randomNumbers() % 2);

					if (count < bestTries) {
						bestTries = count;
						chosen = target;
					}
				}

				if (targets.empty() || bestTries >= 2) {
					click(960, 540, true);
					what = "put the item '" + itemName + "' back";
				} else {
					tries["use/" + itemName + "/" + scene + "/" + chosen._name]++;
					click(static_cast<int>(chosen._x), static_cast<int>(chosen._y));
					what = "used '" + itemName + "' on '" + chosen._name + "'";
				}
			}

			if (what.empty()) {
				if (hotspots.empty()) {
					click(randomNumbers() % 1920, randomNumbers() % 1080);
					what = "nothing found: a click at random";
				} else {
					// the one tried the least (the left button, the right one after it)
					Hotspot chosen;
					bool right = false;
					int bestTries = 1 << 30;

					for (const Hotspot &hotspot : hotspots) {
						for (int button = 0; button < 2; button++) {
							int count = tries[scene + "/" + hotspot._name + (button ? "/R" : "/L")] * 2 + button + static_cast<int>(randomNumbers() % 2);

							if (count < bestTries) {
								bestTries = count;
								chosen = hotspot;
								right = button != 0;
							}
						}
					}

					tries[scene + "/" + chosen._name + (right ? "/R" : "/L")]++;
					click(static_cast<int>(chosen._x), static_cast<int>(chosen._y), right);
					what = std::string(right ? "right" : "left") + " click on '" + chosen._name + "'";
				}
			}
		}

		noteActions();
		printf("step %d: %s (%d frames to settle)\n", step, what.c_str(), frames);

		if (!newActions.empty()) {
			sinceNew = 0;
			for (const std::string &actionName : newActions)
				printf("    action: %s\n", actionName.c_str());
			newActions.clear();
		} else if (++sinceNew >= 150) {
			printf("nothing new in 150 steps: stopping\n");
			break;
		}
	}

	printf("\nscenes visited (%d):\n", (int)scenes.size());
	for (const std::string &scene : scenes)
		printf("  %s\n", scene.c_str());
	printf("%d actions ran\n", (int)everRun.size());
	return 0;
}
