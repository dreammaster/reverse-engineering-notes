#include "AppFunctions.h"

#include <cstdio>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cwchar>

#include "AppGlobals.h"
#include "Diagnostics.h"
#include "TCFont.h"
#include "TGCharacter.h"
#include "TTimer.h"
#include "graphicslib/graphics.h"
#include "graphicslib/subsys.h"
#include "vsplayer/control/gameControl.h"
#include "vsplayer/control/gameController.h"
#include "vscommon/fontManager.h"
#include "vstables/fieldIds.h"

void CleanUp(bool forceExit) {
	std::printf("[stub] CleanUp(%s)\n", forceExit ? "true" : "false");
}

bool ParseCommandLine(int /*argc*/, char **/*argv*/, wxCmdLineParser &/*parser*/) {
	// Stub: pretend the command line always parses successfully.
	return true;
}

bool Init(const wxString &/*appName*/, wxSize &/*surfaceSize*/, wxSize &/*renderSize*/, int /*argc*/,
          char **/*argv*/, wxCmdLineParser &/*parser*/) {
	// Stub: pretend startup always succeeds so the reconstructed main() can
	// be smoke-tested end to end.
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
