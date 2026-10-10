#include "AppFunctions.h"

#include <cstdio>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cwchar>

#include "AppGlobals.h"
#include "Diagnostics.h"
#include "THGameControl.h"
#include "TGAction.h"
#include "TSceneControl.h"
#include "TMSavegame.h"
#include "TSoundFFMPEG.h"
#include "TStandardPaths.h"
#include "datastruct/table.h"
#include "datastruct/visionaire.h"
#include "graphicslib/graphicsBackend.h"
#include "graphicslib/preloadedPicManager.h"
#include "baselib/xmlCommon.h"
#include "vscommon/scripting/command.h"
#include "vsplayer/main/appConfig.h"
#include "vscommon/cfont.h"
#include "TGCharacter.h"
#include "TTimer.h"
#include "graphicslib/graphics.h"
#include "graphicslib/subsys.h"
#include "vsplayer/control/gameControl.h"
#include "vsplayer/control/gameController.h"
#include "vscommon/fontManager.h"
#include "vstables/fieldIds.h"

// Confirmed (asm lines 492124-492247): ends the player. With `forceExit` the volumes go back to config.ini; the game
// control, the commands and the data are let go, the graphics backend's cache is cleared and the backend deleted, the log
// closes and the window goes.
static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/vsplayer/main/mainSDL.cpp";

void CleanUp(bool forceExit) {
	collectProfileData();

	if (g_pGameControl) {
		TGameControl *control = dynamic_cast<TGameControl *>(g_pGameControl);
		TSoundFFMPEG *sound = g_pGameControl->GetSoundManager();

		if (sound && forceExit)
			WriteVolume(sound->GetMusicVolume(), sound->GetSoundVolume(), sound->GetSpeechVolume(), sound->GetMovieVolume(),
			            sound->GetGlobalVolume());

		TVisionaireGame *visionaire = control ? control->GetVisionaire() : nullptr;

		delete g_pGameControl;
		g_pGameControl = nullptr;
		ClosePlayerCommands();
		TVisionaire::CleanUp();
		delete visionaire;
	} else {
		ClosePlayerCommands();
		TVisionaire::CleanUp();
	}

	if (graphics) {
		graphics->ClearCache();
		delete graphics;
		graphics = nullptr;
	}

	delete wxLog::SetActiveTarget(nullptr);

	if (LogFile)
		std::fclose(LogFile);

	SDL_DestroyWindow(VSPlayerWindow);
	wxUninitialize();
}

// Confirmed (asm lines 491982-492100): asks the main loop to end with an SDL_QUIT event; when that cannot be pushed the
// program ends at once.
void TerminateApplication() {
	SDL_Event event;

	std::memset(&event, 0, sizeof(event));
	event.type = SDL_QUIT;

	if (SDL_PushEvent(&event) == -1) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"SDL_QUIT event can't be pushed: %s\n", wxString(SDL_GetError()).wc_str());

		std::exit(1);
	}
}

// Confirmed (asm lines 492271-492892): makes the window of the player and its OpenGL context, and asks the backend to work
// out the part of the window where the game is drawn (into g_displayedArea). `windowSize` is the size of the window,
// `renderSize` that of the game; `brightness` is 0 to 100. False (with the cause in the log) when it fails.
// The original also calls SDL_GetRenderer() and SDL_RenderFillRect() on the window after setting the swap interval; a
// window with an OpenGL context has no renderer, so that call can only fail and is left out.
bool CreateWindowGL(const wxSize &windowSize, wxSize &renderSize, long brightness) {
	TDiagnostic::BeginFixedRegion(wxString(L"GL Window"));

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	wxString title = VSPlayerTitle;

	VSPlayerWindow = SDL_CreateWindow(static_cast<const char *>(title.mb_str()), SDL_WINDOWPOS_CENTERED,
	                                  SDL_WINDOWPOS_CENTERED, windowSize.width, windowSize.height, Vflags);

	if (!VSPlayerWindow) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"problem with SDL_SetVideoMode (%d, %d): %s. Make sure your screen color depth is set to 32 "
			                   L"bit.", windowSize.width, windowSize.height, wxString(SDL_GetError()).wc_str());

		return false;
	}

	// the context is made by the OpenGL backend's sub system
	if (g_subSys && !VSPlayerContext)
		VSPlayerContext = SDL_GL_CreateContext(VSPlayerWindow);

	TDiagnostic::EndFixedRegion();



	if (!graphics->InitGraphics(windowSize, renderSize, &g_displayedArea)) {
		if (wxLog::loglevel >= 0) {
			wxLog::logexpanded(L"problem with initialization of graphics interface: %s", wxString(SDL_GetError()).wc_str());
			wxLog::logexpanded(L"Make sure graphics driver is up-to-date.");
		}

		return false;
	}

	if (g_subSys) {
		if (!VSPlayerContext)
			VSPlayerContext = SDL_GL_CreateContext(VSPlayerWindow);

		SDL_GL_SetSwapInterval(1);
	}

	TDiagnostic::EndFixedRegion();

	int red = 0, green = 0, blue = 0, alpha = 0, depth = 0, doubleBuffer = 0, accelerated = 0;

	SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &red);
	SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &green);
	SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &blue);
	SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &alpha);
	SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &depth);
	SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &doubleBuffer);
	SDL_GL_GetAttribute(SDL_GL_ACCELERATED_VISUAL, &accelerated);

	if (wxLog::loglevel > 1)
		wxLog::logexpanded(L"red, green, blue, alpha, depth size: <%d, %d, %d, %d, %d>, doublebuffer <%d>, accelerated <%d>", red,
		                   green, blue, alpha, depth, doubleBuffer, accelerated);

	float brightnessFactor = static_cast<float>(brightness) / 100.0f;

	if (brightnessFactor != 1.0f && SDL_SetWindowBrightness(VSPlayerWindow, brightnessFactor) != 0 && wxLog::loglevel >= 0)
		wxLog::logexpanded(L"Unable to set brightness: %s", wxString(SDL_GetError()).wc_str());

	return true;
}

// The options of the command line (the table that ParseCommandLine() of the original builds on its stack, asm 492892-493075).
// Short names are several letters long here (-tc, -ll ...). The one parameter, the input file, is mandatory.
static const wxCmdLineEntryDesc kCommandLineOptions[] = {
	{wxCMD_LINE_SWITCH, "w", "window", "no full screen", wxCMD_LINE_VAL_STRING, 0},
	{wxCMD_LINE_SWITCH, "re", "resizeable", "window is resizeable", wxCMD_LINE_VAL_STRING, 0},
	{wxCMD_LINE_SWITCH, "tc", "compression", "texture compression", wxCMD_LINE_VAL_STRING, 0},
	{wxCMD_LINE_SWITCH, "ns", "nosounds", "disable sounds", wxCMD_LINE_VAL_STRING, 0},
	{wxCMD_LINE_SWITCH, "nv", "novideos", "disable videos", wxCMD_LINE_VAL_STRING, 0},
	{wxCMD_LINE_OPTION, "prof", "profile", "profiling mode", wxCMD_LINE_VAL_STRING, 0},
	{wxCMD_LINE_OPTION, "r", "resolution", "resolution of full screen mode (game|auto|desktop) or window size in windowed "
	                                       "mode (e.g. 640x480 for a game with resolution 320x240)",
	 wxCMD_LINE_VAL_STRING, 0},
	{wxCMD_LINE_OPTION, "utw", "usetexforwidescreen", "Defines if internally a texture is used for widescreen support "
	                                                   "(r=auto|desktop). Currently in development because both techniques make "
	                                                   "problem on some hardware. Possible settings: true|false",
	 wxCMD_LINE_VAL_STRING, 0},
	{wxCMD_LINE_OPTION, "ll", "loglevel", "log level for log messages (error|warning|info)", wxCMD_LINE_VAL_STRING, 0},
	{wxCMD_LINE_OPTION, "lf", "logfile", "optional path and filename for log file. this can be useful to create a log file "
	                                      "before the input file is loaded. Otherwise the log file is created after the input "
	                                      "file was loaded.",
	 wxCMD_LINE_VAL_STRING, 0},
	{wxCMD_LINE_OPTION, "s", "savegame", "load savegame", wxCMD_LINE_VAL_NUMBER, 0},
	{wxCMD_LINE_OPTION, "g", "graphics", "graphics interface", wxCMD_LINE_VAL_STRING, 0},
	{wxCMD_LINE_OPTION, "l", "language", "game language", wxCMD_LINE_VAL_STRING, 0},
	{wxCMD_LINE_OPTION, "uld", "uselocaldir", "use local directory (default) for savegames, otherwise a common directory "
	                                           "will be used (true|false)",
	 wxCMD_LINE_VAL_STRING, 0},
	{wxCMD_LINE_OPTION, "dbo", "depthbufferopt", "", wxCMD_LINE_VAL_STRING, 0},
	{wxCMD_LINE_OPTION, "dev", "device", "graphics device: dx11 | dx9 | gl", wxCMD_LINE_VAL_STRING, 0},
	{wxCMD_LINE_OPTION, "p", "password", "", wxCMD_LINE_VAL_STRING, 0},
	{wxCMD_LINE_OPTION, "sc", "scene", "name of start scene", wxCMD_LINE_VAL_STRING, 0},
	{wxCMD_LINE_OPTION, "deb", "debugger", "debugger adress", wxCMD_LINE_VAL_STRING, 0},
	{wxCMD_LINE_PARAM, nullptr, nullptr, "input file", wxCMD_LINE_VAL_STRING, 0},
	{wxCMD_LINE_NONE, nullptr, nullptr, nullptr, wxCMD_LINE_VAL_STRING, 0}
};

// Confirmed (asm lines 492892-493078): a command line of just the name of the program is not looked at; else it is
// parsed against the table above. False when it is wrong (or help was asked for).
bool ParseCommandLine(int argc, char **argv, wxCmdLineParser &parser) {
	if (argc <= 1)
		return true;

	parser.SetCmdLine(argc, argv);
	parser.SetDesc(kCommandLineOptions);
	return parser.Parse() == 0;
}

static bool isNoCase(const wxString &text, const wchar_t *expected) {
	return text.CmpNoCase(wxString(expected)) == 0;
}

// Confirmed (asm lines 493088-496900, ~4300 lines): the start-up of the player. See NOTES.md ("The player's start-up") for
// the order of things. `surfaceSize` is the size of the window and `renderSize` that of the game, both are set here. The
// original's repeated copies of the clean-up of its strings at every exit are not repeated.
//
// Not as in the original: the OpenGL backend is made by CreateGraphicsBackend(), because it is not reconstructed. The
// command line options -uld (uselocaldir), -dbo, -g and -dev are in the table but Init() does not look at them (it calls
// TStandardPaths::SetUseLocalDir(false) whatever is given), and config.ini cannot choose the graphics device: it is
// always OpenGL after the file was read.
bool Init(const wxString &/*appName*/, wxSize &surfaceSize, wxSize &renderSize, int argc, char **/*argv*/,
          wxCmdLineParser &parser) {
	wxString file;
	wxString language;
	bool fullscreen = true;
	bool intro = true;
	bool resizeable = false;
	bool textureCompression = false;
	bool lockCursor = false;
	bool textureForWidescreen = true;
	eResolution resolutionMode = eResolution::kAuto;
	eGraphicsInterface device = static_cast<eGraphicsInterface>(0);
	eLogLevel logLevel = eLogLevel::kInfo;
	long musicVolume = 100, soundVolume = 100, speechVolume = 100, movieVolume = 100, globalVolume = 100;
	long brightness = 100;
	long savegame = -1;

	// what the command line gave (it wins over config.ini)
	bool windowGiven = false;
	bool resizeableGiven = false;
	bool resolutionGiven = false;
	bool textureForWidescreenGiven = false;
	bool logLevelGiven = false;
	bool logFileGiven = false;
	bool languageGiven = false;
	bool noSounds = false;
	bool noVideos = false;

	SDL_EventState(SDL_TEXTINPUT, SDL_ENABLE);
	Vflags = 0x2004;
	wxLog::SetLogLevel(static_cast<int>(logLevel));

	if (argc > 1) {
		if (parser.GetParamCount() > 0)
			file = parser.GetParam(0);

		if (parser.Found(wxString(L"w"))) {
			fullscreen = false;
			windowGiven = true;
		}

		if (parser.Found(wxString(L"re"))) {
			resizeable = true;
			resizeableGiven = true;
		}

		if (parser.Found(wxString(L"tc")))
			textureCompression = true;

		noSounds = parser.Found(wxString(L"ns"));
		noVideos = parser.Found(wxString(L"nv"));

		wxString value;

		if (parser.Found(wxString(L"p"), &value))
			passw = value;

		if (parser.Found(wxString(L"r"), &value)) {
			if (isNoCase(value, L"Desktop")) {
				resolutionMode = eResolution::kDesktop;
			} else if (isNoCase(value, L"Game")) {
				resolutionMode = eResolution::kGame;
			} else if (isNoCase(value, L"Auto")) {
				resolutionMode = eResolution::kAuto;
			} else if (ConvertStringToSize(value, surfaceSize)) {
				resolutionMode = eResolution::kSize;
			} else if (wxLog::loglevel > 0) {
				wxLog::logexpanded(L"Invalid value for command line parameter 'resolution'.");
			}

			resolutionGiven = true;
		}

		if (parser.Found(wxString(L"utw"), &value)) {
			if (isNoCase(value, L"true")) {
				textureForWidescreen = true;
			} else if (isNoCase(value, L"false")) {
				textureForWidescreen = false;
			} else if (wxLog::loglevel > 0) {
				wxLog::logexpanded(L"Invalid value for command line parameter 'usetexforwidescreen'.");
			}

			textureForWidescreenGiven = true;
		}

		if (parser.Found(wxString(L"ll"), &value)) {
			if (isNoCase(value, L"Error")) {
				logLevel = eLogLevel::kError;
			} else if (isNoCase(value, L"Warning")) {
				logLevel = eLogLevel::kWarning;
			} else if (isNoCase(value, L"Info")) {
				logLevel = eLogLevel::kInfo;
			} else if (isNoCase(value, L"Max")) {
				logLevel = eLogLevel::kMax;
			} else if (wxLog::loglevel > 0) {
				wxLog::logexpanded(L"Invalid value for command line parameter 'loglevel'.");
			}

			logLevelGiven = true;
		}

		// main() has already taken the name of the log file into g_logfile; the file is opened again here
		if (parser.Found(wxString(L"lf"), &value)) {
			LogFile = std::fopen(static_cast<const char *>(g_logfile.GetFullPath().mb_str()), "w");
			logFileGiven = true;
		}

		long number;

		if (parser.Found(wxString(L"savegame"), &number))
			savegame = number;

		if (parser.Found(wxString(L"language"), &language))
			languageGiven = true;

		if (parser.Found(wxString(L"deb"), &value) && value.Contains(wxString(L":"))) {
			int colon = value.Find(wxString(L":"));

			debugger_addr = static_cast<const char *>(value.Mid(0, colon).mb_str());
			debugger_port = static_cast<int>(dtol(static_cast<const char *>(value.Mid(colon + 1).mb_str())));
		}

		// both need the address of the debugger (the original tests the address, not the value)
		if (parser.Found(wxString(L"prof"), &value) && !debugger_addr.empty()) {
			if (value.ToStdWstring() == L"lua")
				profile = true;

			if (value.ToStdWstring() == L"frame")
				profileAreas = true;
		}

		parser.Found(wxString(L"sc"), &FirstSceneName);
	}

	TStandardPaths::SetUseLocalDir(false);

	// The settings of config.ini: those the command line gave are not looked for (a null place).
	TConfigTargets targets;

	targets.fullscreen = windowGiven ? nullptr : &fullscreen;
	targets.resizeable = resizeableGiven ? nullptr : &resizeable;
	targets.intro = &intro;
	targets.resolution = resolutionGiven ? nullptr : &resolutionMode;
	targets.textureForWidescreen = textureForWidescreenGiven ? nullptr : &textureForWidescreen;
	targets.resolutionSize = &surfaceSize;
	targets.logLevel = logLevelGiven ? nullptr : &logLevel;
	targets.language = languageGiven ? nullptr : &language;
	targets.musicVolume = &musicVolume;
	targets.soundVolume = &soundVolume;
	targets.speechVolume = &speechVolume;
	targets.movieVolume = &movieVolume;
	targets.globalVolume = &globalVolume;
	targets.brightness = &brightness;
	targets.textureCompression = &textureCompression;
	targets.device = &device;
	targets.lockCursor = &lockCursor;

	if (file.IsEmpty()) {
		// no input file: the config.ini next to the program may name the game (File = ...)
		TConfigTargets first = targets;

		first.file = &file;
		first.password = &passw;
		LoadConfigFile(standardPaths.GetResourcesDir(false) + wxString(L"/config.ini"), first);
	}

	wxLog::SetLogLevel(static_cast<int>(logLevel));

	THGameControl *gameControl = new THGameControl();

	g_pGameControl = gameControl;
	TVisionaire::SetVisPlayerMode(true);

	TTimer timer;

	timer.SetTime();

	wxString warning;

	if (!gameControl->PreLoad(file, warning, true)) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"%s", (wxString(L"Loading game from file '") + file + wxString(L"' failed!")).wc_str());

		return false;
	}

	long preloadTime = timer.GetTime();

	timer.SetTime();

	TVisObjRef game = gameControl->GetVisionaire()->GetGame();
	wxFileName gameFile(file.ToStdWstring());

	gameFile.NormalizePath();

	wxString fileName = gameFile.GetName();
	wxString companyName = game.GetStr(kGameCompanyName);
	wxString gameName = game.GetStr(kGameGameName);

	VSPlayerTitle = gameName.IsEmpty() ? fileName : gameName;
	TStandardPaths::InitGameAndCompanyName(companyName, gameName, fileName);
	TMSavegame::InitSaveGamePath();

	if (!logFileGiven) {
		wxFileName logFile(standardPaths.GetLogFileDir() + L"messages.log");

		logFile.NormalizePath();
		g_logfile = logFile;

		if (LogFile) {
			std::fclose(LogFile);
			LogFile = nullptr;
		}

		x_assert(true, "LogFile == NULL", kSourceFile, 0x368);
		LogFile = std::fopen(static_cast<const char *>(g_logfile.GetFullPath().mb_str()), "w");
		wxLog::SetVerbose(true);
		delete wxLog::SetActiveTarget(new wxLogStderr(LogFile));
	} else {
		x_assert(LogFile != nullptr, "LogFile != NULL", kSourceFile, 0x375);
	}

	if (wxLog::loglevel > 1) {
		wxLog::logexpanded(L"Engine Version: %s (Build %d from Build date: %s)", L"5.0.1", 1189, L"Feb  8 2019");
		wxLog::logexpanded(L"Time needed for preloading game: %ld msec", preloadTime);
	}

	// the settings of the user's own config.ini (the directory of the game); what the command line gave stays
	LoadConfigFile(wxString(standardPaths.GetConfigDir()) + wxString(L"/config.ini"), targets);

	if (!fullscreen && resolutionGiven && resolutionMode != eResolution::kSize && wxLog::loglevel > 0)
		wxLog::logexpanded(L"The command line parameter 'resolution' can only be used in fullscreen mode. Parameter is ignored.");

	// the size of the picture of the game; the window has that size unless the command line gave one
	const wxPoint *gameResolution = game.GetPoint(kGameWindowResolution);

	renderSize.Set(gameResolution->x, gameResolution->y);
	g_displayedArea.x = 0;
	g_displayedArea.y = 0;
	g_displayedArea.width = renderSize.width;
	g_displayedArea.height = renderSize.height;

	// config.ini cannot choose the graphics device: it is OpenGL from here on
	device = eGraphicsInterface::kOpenGL;

	TGraphicsStartup startup;

	startup.picBufferSize = game.GetInt(kGamePicBufferSize);
	startup.preloadPicThreads = game.GetInt(kGamePreloadPicThreads);
	startup.preloadedPicBufferSize = game.GetInt(kGamePreloadedPicBufferSize);
	startup.nearestNeighbor = game.GetBool(kGameNearestNeighborInterpolation);
	startup.gameResolution = renderSize;
	startup.windowSize = surfaceSize;
	startup.textureForWidescreen = textureForWidescreen;
	startup.fullscreen = fullscreen;
	startup.resizeable = resizeable;
	startup.textureCompression = textureCompression;

	if (!CreateGraphicsBackend(startup))
		return false;

	gameControl->GetSoundManager()->SetVolume(static_cast<int>(musicVolume), static_cast<int>(soundVolume),
	                                          static_cast<int>(speechVolume), static_cast<int>(movieVolume),
	                                          static_cast<int>(globalVolume));

	if (noSounds)
		gameControl->GetSoundManager()->DisableSounds(true);

	if (noVideos)
		gameControl->EnableMovies(false);

	InitPlayerCommands(gameControl->GetVisionaire(), wxString(standardPaths.GetConfigDir()),
	                   standardPaths.GetResourcesDir(true), wxString(standardPaths.GetLogFileDir()));

	if (!debugger_addr.empty())
		debugger.Activate(debugger_addr.c_str(), debugger_port);

	if (resolutionMode != eResolution::kSize)
		surfaceSize = renderSize;

	GameMinDownTime = game.GetInt(kGameHoldTime);

	if (fullscreen) {
		SDL_DisplayMode desktop;

		Vflags |= SDL_WINDOW_FULLSCREEN;
		SDL_GetDesktopDisplayMode(0, &desktop);

		double ratio = static_cast<double>(desktop.w) / static_cast<double>(desktop.h);

		if ((ratio > 1.5 && resolutionMode == eResolution::kAuto) || resolutionMode == eResolution::kDesktop)
			surfaceSize.Set(desktop.w, desktop.h);
	}

	if (resizeable)
		Vflags |= SDL_WINDOW_RESIZABLE;

	if (device == eGraphicsInterface::kOpenGL)
		Vflags |= SDL_WINDOW_OPENGL;

	if (!CreateWindowGL(surfaceSize, renderSize, brightness)) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"Unable to open screen surface.");

		return false;
	}

	if (lockCursor)
		SDL_SetRelativeMouseMode(1);

	SDL_DisableScreenSaver();
	SDL_ShowCursor(0);

	graphics->GetPreloadedPicManager()->Pause();

	// (the language the game starts in goes to LoadAndInitGame)
	if (!gameControl->LoadAndInitGame(file, warning, language, true))
		return false;

	graphics->GetPreloadedPicManager()->Continue();

	if (wxLog::loglevel > 1)
		wxLog::logexpanded(L"Time needed for loading game: %ld msec", timer.GetTime());

	gameControl->RegisterEventHandler();

	if (intro) {
		wxFileName introFile = game.GetPath(kGameIntro);

		gameControl->PlayAVI(introFile, game.GetBool(kGameFullScreenIntro), static_cast<HandleSoundsEnum>(3));
	}

	if (savegame < 0) {
		gameControl->InitAfterLoadingScreen();
		gameControl->ExecuteStartingAction();
	} else {
		// whatever the number was, it is the savegame of slot 0 that is loaded
		TMSavegame *savegameToLoad = new TMSavegame(false, 0, -1, -1, gameControl->GetVisionaire());

		gameControl->LoadGame(savegameToLoad);
		delete savegameToLoad;
	}

	if (!FirstSceneName.IsEmpty()) {
		// `-sc`: go to the scene. First the actions of the start run out (at most 200 turns of 10 ms).
		graphics->AfterDrawScene(true, false);
		graphics->BeforeDrawScene(true, 1);

		TVList actions;

		TGAction::GetActions(actions);

		for (int turns = 0; actions.size() != 0;) {
			TGAction::ContinueRunningActions(false);
			TGAction::DeleteFinishedActions();
			TGAction::GetActions(actions);
			wxMilliSleep(10);

			if (++turns == 200) {
				if (actions.size() != 0 && wxLog::loglevel > 0)
					wxLog::logexpanded(L"Change to scene took longer because the action '%s' failed to become idle.",
					                   wxString(actions.at(0)->GetName().mb_str()).wc_str());

				break;
			}
		}

		TTable *scenes = nullptr;

		gameControl->GetVisionaire()->GetTable(kScene, &scenes);

		TVisObjRef scene;

		if (scenes->GetByName(FirstSceneName, scene)) {
			// the first object of the scene that has a place is where the character goes
			TVList objects;

			scene.GetLinks(kSceneObjects, static_cast<TypeOrder>(0), objects);

			TVisObjRef target;

			for (TVisionaireObject *object : objects) {
				const wxPoint *position = object->GetPoint(kObjectPosition);

				if (position->x > 0 && position->y > 0) {
					target = TVisObjRef(object);
					break;
				}
			}

			if (target.IsEmpty())
				gameControl->GetSceneControl()->ShowScene(scene, false, false);
			else
				gameControl->GetSceneControl()->ChangeScene(gameControl->GetCurrentCharacter()->GetRef(), target, false, -1);
		}
	}

	return true;
}

void ShowMessageBox(const wxString &title, const wxString &message) {
	std::fwprintf(stderr, L"[%ls] %ls\n", title.wc_str(), message.wc_str());
}

// Confirmed (asm lines 497603-497630)
static bool IsMultigesture() {
	if (lastMultigestureTicks == 0xFFFFFFFFu) {
		lastMultigestureTicks = SDL_GetTicks();
		return false;
	}

	Uint32 now = SDL_GetTicks();
	Uint32 elapsed = now - lastMultigestureTicks;

	lastMultigestureTicks = now;
	return elapsed <= 300;
}

// Confirmed (asm lines 497640-497735): the position of the mouse in the window as a position in the game. A mouse
// outside the part where the game is drawn is put back inside (the cursor of the window is moved there).
static void ToScreenPos(int x, int y) {
	int relativeX = x - g_displayedArea.x;
	int relativeY = y - g_displayedArea.y;
	bool warp = false;

	if (relativeX < 0) {
		relativeX = 0;
		warp = true;
	} else if (relativeX >= g_displayedArea.width) {
		relativeX = g_displayedArea.width - 1;
		warp = true;
	}

	if (relativeY < 0) {
		relativeY = 0;
		warp = true;
	} else if (relativeY >= g_displayedArea.height) {
		relativeY = g_displayedArea.height - 1;
		warp = true;
	}

	if (warp)
		SDL_WarpMouseInWindow(VSPlayerWindow, g_displayedArea.x + relativeX, g_displayedArea.y + relativeY);

	mousePos.x = static_cast<int>(static_cast<double>(relativeX) / g_displayedArea.GetWidth() * renderSize.width);
	mousePos.y = static_cast<int>(static_cast<double>(relativeY) / g_displayedArea.GetHeight() * renderSize.height);
}

static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

static TTimer leftClickStarted;
static TTimer lastClicked;

/** The window changed its size: tells the video, works out where the game is drawn and fits the aspect ratio. */
static void windowResized() {
	int width = 0;
	int height = 0;

	SDL_GetWindowSize(VSPlayerWindow, &width, &height);

	Uint32 flags = SDL_GetWindowFlags(VSPlayerWindow);
	bool fullscreen = (flags & SDL_WINDOW_FULLSCREEN) ? true : ((flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0);

	if (g_subSys)
		g_subSys->WindowResized(width, height, fullscreen);

	TVisObjRef game = gameControl()->GetVisionaire()->GetGame();
	const wxPoint *resolution = game.GetPoint(kGameWindowResolution);

	renderSize.Set(resolution->x, resolution->y);

	wxSize windowSize;

	windowSize.Set(width, height);
	graphics->CalculateDisplayedArea(windowSize, renderSize, &g_displayedArea);
	gameControl()->UpdateAspectRatio();
}

/** What a mouse button going down or up makes (the left one with the control key, or with three fingers on the
 *  touch screen, is a right one). */
static void mouseButton(const SDL_MouseButtonEvent &event, bool down) {
	if (event.which == 0xFFFFFFFFu)
		return;

	switch (event.button) {
	case 1:
		if ((SDL_GetModState() & KMOD_CTRL) || numFingers == (down ? 3 : 2)) {
			eMouseMessage = down ? 8 : 9;
		} else if (down) {
			bLeftButtonPressed = 1;
			leftClickStarted.SetTime();
			eMouseMessage = 3;
		} else {
			bLeftButtonPressed = 0;
			eMouseMessage = 4;

			if (leftClickStarted.GetTime() < GameMinDownTime) {
				// a short click; two within 449 ms are a double click
				if (lastClicked.GetTime() <= 449)
					eMouseMessage = 2;

				lastClicked.SetTime();
			} else {
				eMouseMessage = 5;
			}
		}

		break;
	case 3:
		eMouseMessage = down ? 8 : 9;
		break;
	case 2:
		eMouseMessage = down ? 10 : 11;
		break;
	default:
		break;
	}
}

// Confirmed (asm lines 497745-498889): one turn of the main loop. A video that plays is stepped. Else the events of
// SDL are made messages of the game (mouse, keys, controllers, window), and a frame is made: the mouse (moved by a
// controller) and the character (walked by a controller) are moved, the messages of the mouse are sent to the
// scripts and to the game, the game is updated (when the window has the focus) and drawn.
void ShowFrame(void */*userData*/) {
	if (!isProgramLooping)
		return;

	TMasterControl *control = g_pGameControl;

	// (mainloopTCPLoop(): the connection to the debugger of the editor is not reconstructed)
	for (TCFont *font : control->GetFontManager()->GetFonts())
		font->CheckFreetypeFont();

	if (control->IsVideoPlaying()) {
		SDL_PumpEvents();
		debugger.BeginArea(ProfileArea::kValue3, std::string(), -1);
		control->VideoFrame();
		debugger.EndArea(ProfileArea::kValue3, -1);
		debugger.NextFrame();
		return;
	}

	SDL_PumpEvents();

	SDL_Event event;

	while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, 0, 0xFFFF) > 0) {
		switch (event.type) {
		case SDL_QUIT:
			isProgramLooping = 0;
			break;
		case SDL_WINDOWEVENT:
			switch (event.window.event) {
			case SDL_WINDOWEVENT_SHOWN:
				AppStatus = 1;
				break;
			case SDL_WINDOWEVENT_HIDDEN:
				AppStatus = 0;
				break;
			case SDL_WINDOWEVENT_RESIZED:
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				windowResized();
				break;
			case SDL_WINDOWEVENT_ENTER:
				byte_11F8B01 = 1;
				break;
			case SDL_WINDOWEVENT_LEAVE:
				byte_11F8B01 = 0;
				break;
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				byte_11F8B02 = 1;
				control->GetSoundManager()->ContinueAll();
				break;
			case SDL_WINDOWEVENT_FOCUS_LOST:
				if (CanLoseFocus) {
					byte_11F8B02 = 0;
					control->GetSoundManager()->PauseAll();
				}

				break;
			default:
				break;
			}

			break;
		case SDL_KEYDOWN:
			// alt + return changes between the window and the full screen
			if (event.key.keysym.sym == 0xD && (event.key.keysym.mod & KMOD_ALT))
				graphics->ToggleWindowMode();

			gameControl()->HandleKeyEvent(TKeyboardMessageEnum::kKeyDown, wxString(), event.key.keysym.sym,
			                              event.key.keysym.mod);
			break;
		case SDL_KEYUP:
			gameControl()->HandleKeyEvent(TKeyboardMessageEnum::kKeyUp, wxString(), event.key.keysym.sym,
			                              event.key.keysym.mod);
			break;
		case SDL_TEXTINPUT: {
			// (the numbers are what follows the text in the event, as the original passes them)
			wxString text;
			Sint32 first;
			Uint16 second;

			toUTF(&text, event.text.text);
			std::memcpy(&first, reinterpret_cast<const Uint8 *>(&event) + 0x14, sizeof(first));
			std::memcpy(&second, reinterpret_cast<const Uint8 *>(&event) + 0x18, sizeof(second));
			gameControl()->HandleKeyEvent(TKeyboardMessageEnum::kText, text, first, second);
			break;
		}
		case SDL_MOUSEMOTION:
			ToScreenPos(event.motion.x, event.motion.y);
			break;
		case SDL_MOUSEBUTTONDOWN:
			mouseButton(event.button, true);
			break;
		case SDL_MOUSEBUTTONUP:
			mouseButton(event.button, false);
			break;
		case SDL_MOUSEWHEEL:
			if (byte_11F8B02) {
				if (event.wheel.y > 0)
					eMouseMessage = 12;
				else if (event.wheel.y != 0)
					eMouseMessage = 13;
			}

			break;
		case SDL_CONTROLLERAXISMOTION:
			if (event.caxis.axis <= 5) {
				gameControl()->HandleControllerAxis(static_cast<SDL_GameControllerAxis>(event.caxis.axis),
				                                    event.caxis.value, event.caxis.which);
			}

			break;
		case SDL_CONTROLLERBUTTONDOWN:
			gameControl()->HandleControllerButtonHit(event.cbutton, event.cbutton.which);
			break;
		case SDL_CONTROLLERBUTTONUP:
			gameControl()->HandleControllerButtonRelease(event.cbutton, event.cbutton.which);
			break;
		case SDL_CONTROLLERDEVICEADDED: {
			int id = control->GetGameController()->AddGameController(event.cdevice.which);

			gameControl()->HandleKeyEvent(TKeyboardMessageEnum::kControllerAdded, wxString(), 0,
			                              static_cast<unsigned short>(id));
			break;
		}
		case SDL_CONTROLLERDEVICEREMOVED:
			gameControl()->HandleKeyEvent(TKeyboardMessageEnum::kControllerRemoved, wxString(), 0,
			                              static_cast<unsigned short>(event.cdevice.which));
			control->GetGameController()->RemoveGameController(event.cdevice.which);
			break;
		case SDL_CONTROLLERDEVICEREMAPPED:
			gameControl()->HandleKeyEvent(TKeyboardMessageEnum::kControllerRemapped, wxString(), 0,
			                              static_cast<unsigned short>(event.cdevice.which));
			break;
		default:
			break;
		}
	}

	if (AppStatus == 0) {
		// (the window is hidden: nothing is drawn until something happens)
		SDL_WaitEvent(nullptr);

		if (!isProgramLooping) {
			CleanUp(true);
			SDL_Quit();
		}

		return;
	}

	// The mouse is moved by a controller, but stays in the picture.
	mousePos.x = std::max(0, std::min(mousePos.x + movex, renderSize.width));
	mousePos.y = std::max(0, std::min(mousePos.y + movey, renderSize.height));

	TGCharacter *character = gameControl()->GetCurrentCharacter();
	TVisObjRef game = gameControl()->GetGameSystem()->GetGame();

	// The character is walked by a controller (not while a cutscene runs).
	if (game.GetLink(kGameCutsceneAction).IsEmpty()) {
		int x = charmovex;
		int y = charmovey;
		bool move = true;

		if (x != 0 || y != 0) {
			double length = std::sqrt(static_cast<double>(y * y + x * x)) / 100.0;

			if (length != 0.0) {
				x = static_cast<int>(charmovex / length);
				y = static_cast<int>(charmovey / length);
			} else {
				x = 0;
				y = 0;
			}
		} else if (!stopped_char) {
			move = false;
		} else {
			x = 0;
			y = 0;
		}

		if (move) {
			charmovex = x;
			charmovey = y;
			stopped_char = 0;

			TVisObjRef reference = character->GetRef();
			const wxPoint position = *reference.GetPoint(kCharacterPosition);
			TVisObjRef outfit = reference.GetLink(kCharacterCurrentOutfit);
			float size = reference.GetFloat(kCharacterSize);
			double destinationX = position.x + outfit.GetInt(kOutfitCharacterSpeed) * x * size * 0.02 / 30.0 / 100.0;
			double destinationY = position.y + outfit.GetInt(kOutfitCharacterSpeed) * y * size * 0.02 / 30.0 / 100.0;
			wxPoint destination;

			destination.x = static_cast<int>(destinationX);
			destination.y = static_cast<int>(destinationY);
			character->SetHarmonizeWalk(true);
			character->SetFreeDestination(destination, false, true, true);
		}
	}

	control->ProcessMessage(static_cast<TMouseMessageEnum>(1), mousePos);

	if (bLeftButtonPressed && leftClickStarted.GetTime() >= GameMinDownTime) {
		// the button is held: a long click begins
		control->ProcessMessage(static_cast<TMouseMessageEnum>(6), mousePos);
		bLeftButtonPressed = 0;
	}

	if (control->GetClearMessage()) {
		eMouseMessage = 0;
	} else if (eMouseMessage != 0) {
		control->ProcessMessage(static_cast<TMouseMessageEnum>(eMouseMessage), mousePos);
		eMouseMessage = 0;
	}

	debugger.BeginArea(ProfileArea::kValue4, std::string(), -1);

	if (byte_11F8B02)
		control->Update();

	debugger.EndArea(ProfileArea::kValue4, -1);
	debugger.BeginArea(ProfileArea::kValue3, std::string(), -1);
	control->Draw(true);
	control->GetSoundManager()->BusValuesUpdate();
	debugger.NextFrame();

	if (control->GetQuitGame()) {
		isProgramLooping = 0;
		CleanUp(true);
		SDL_Quit();
	}
}
