#include "SdlStub.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

int SDL_Init(Uint32 /*flags*/) {
	return 0;
}

void SDL_Quit(void) {
}

static const char *g_error = "";

const char *SDL_GetError(void) {
	return g_error;
}

Uint32 SDL_GetTicks(void) {
	using namespace std::chrono;
	return static_cast<Uint32>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

const char *SDL_GetKeyName(int /*key*/) {
	return "";
}

int SDL_ShowCursor(int /*toggle*/) {
	return 0;
}

Uint8 SDL_EventState(Uint32 /*type*/, int /*state*/) {
	return 0;
}

int SDL_NumJoysticks(void) {
	return 0;
}

SDL_RWops *SDL_RWFromFile(const char */*file*/, const char */*mode*/) {
	return nullptr;
}

int SDL_GameControllerAddMappingsFromRW(SDL_RWops */*rw*/, int /*freesrc*/) {
	return 0;
}

int SDL_IsGameController(int /*joystickIndex*/) {
	return 0;
}

SDL_GameController *SDL_GameControllerOpen(int /*joystickIndex*/) {
	return nullptr;
}

void SDL_GameControllerClose(SDL_GameController */*controller*/) {
}

SDL_Joystick *SDL_GameControllerGetJoystick(SDL_GameController */*controller*/) {
	return nullptr;
}

const char *SDL_GameControllerName(SDL_GameController */*controller*/) {
	return "";
}

int SDL_JoystickInstanceID(SDL_Joystick */*joystick*/) {
	return -1;
}

const char *SDL_JoystickName(SDL_Joystick */*joystick*/) {
	return "";
}

SDL_JoystickGUID SDL_JoystickGetGUID(SDL_Joystick */*joystick*/) {
	SDL_JoystickGUID guid = {};

	return guid;
}

void SDL_JoystickGetGUIDString(SDL_JoystickGUID guid, char *pszGUID, int cbGUID) {
	static const char kHex[] = "0123456789abcdef";

	for (int i = 0; i < 16 && 2 * i + 2 < cbGUID; i++) {
		pszGUID[2 * i] = kHex[guid.data[i] >> 4];
		pszGUID[2 * i + 1] = kHex[guid.data[i] & 0xF];
		pszGUID[2 * i + 2] = '\0';
	}
}

int SDL_GetCPUCount(void) {
	return 1;
}

int SDL_GetNumDisplayModes(int /*displayIndex*/) {
	return 0;
}

int SDL_GetDisplayMode(int /*displayIndex*/, int /*modeIndex*/, SDL_DisplayMode */*mode*/) {
	return -1;
}

void SDL_PumpEvents(void) {
}

// Headless: there is no window, so the first events are that it is hidden and then that it is closed (a run of the
// main loop of this reconstruction ends by itself).
int SDL_PeepEvents(SDL_Event *events, int /*numevents*/, int /*action*/, Uint32 /*minType*/, Uint32 /*maxType*/) {
	static int sent = 0;

	if (sent == 0) {
		events->window.type = SDL_WINDOWEVENT;
		events->window.event = SDL_WINDOWEVENT_HIDDEN;
	} else if (sent == 1) {
		events->type = SDL_QUIT;
	} else {
		return 0;
	}

	sent++;
	return 1;
}

int SDL_WaitEvent(SDL_Event */*event*/) {
	return 0;
}

int SDL_GetModState(void) {
	return 0;
}

void SDL_GetWindowSize(SDL_Window */*window*/, int *w, int *h) {
	*w = 0;
	*h = 0;
}

Uint32 SDL_GetWindowFlags(SDL_Window */*window*/) {
	return 0;
}

int SDL_JoystickIsHaptic(SDL_Joystick */*joystick*/) {
	return 0;
}

SDL_Haptic *SDL_HapticOpenFromJoystick(SDL_Joystick */*joystick*/) {
	return nullptr;
}

void SDL_HapticClose(SDL_Haptic */*haptic*/) {
}

int SDL_HapticQuery(SDL_Haptic */*haptic*/) {
	return 0;
}

int SDL_HapticNumAxes(SDL_Haptic */*haptic*/) {
	return 0;
}

int SDL_HapticNumEffects(SDL_Haptic */*haptic*/) {
	return 0;
}

int SDL_HapticRumbleSupported(SDL_Haptic */*haptic*/) {
	return 0;
}

int SDL_HapticRumbleInit(SDL_Haptic */*haptic*/) {
	return 0;
}

int SDL_HapticRumblePlay(SDL_Haptic */*haptic*/, float /*strength*/, Uint32 /*lengthMs*/) {
	return 0;
}

int SDL_HapticRumbleStop(SDL_Haptic */*haptic*/) {
	return 0;
}

int SDL_HapticStopAll(SDL_Haptic */*haptic*/) {
	return 0;
}

int SDL_HapticNewEffect(SDL_Haptic */*haptic*/, SDL_HapticEffect */*effect*/) {
	return -1;
}

int SDL_HapticRunEffect(SDL_Haptic */*haptic*/, int /*effect*/, Uint32 /*iterations*/) {
	return -1;
}

int SDL_HapticStopEffect(SDL_Haptic */*haptic*/, int /*effect*/) {
	return -1;
}

// The clipboard of the stub is a string of the program (SDL_GetClipboardText() gives a copy that SDL_free() lets go).
static std::string s_clipboard;

char *SDL_GetClipboardText(void) {
	char *copy = static_cast<char *>(std::malloc(s_clipboard.size() + 1));

	std::memcpy(copy, s_clipboard.c_str(), s_clipboard.size() + 1);
	return copy;
}

int SDL_SetClipboardText(const char *text) {
	s_clipboard = text ? text : "";
	return 0;
}

void SDL_free(void *memory) {
	std::free(memory);
}

int SDL_PushEvent(SDL_Event */*event*/) {
	return 1;
}

void SDL_WarpMouseInWindow(SDL_Window */*window*/, int /*x*/, int /*y*/) {
}

void SDL_SetWindowTitle(SDL_Window */*window*/, const char */*title*/) {
}

float SDL_GetWindowBrightness(SDL_Window */*window*/) {
	return 1.0f;
}

int SDL_SetWindowBrightness(SDL_Window */*window*/, float /*brightness*/) {
	return 0;
}

int SDL_GL_SetAttribute(int /*attr*/, int /*value*/) {
	return 0;
}

int SDL_GL_GetAttribute(int /*attr*/, int *value) {
	*value = 0;
	return 0;
}

// The stub has no video: no window can be made.
SDL_Window *SDL_CreateWindow(const char */*title*/, int /*x*/, int /*y*/, int /*w*/, int /*h*/, Uint32 /*flags*/) {
	g_error = "The SDL stub has no video";
	return nullptr;
}

void SDL_DestroyWindow(SDL_Window */*window*/) {
}

SDL_GLContext SDL_GL_CreateContext(SDL_Window */*window*/) {
	return nullptr;
}

int SDL_GL_SetSwapInterval(int /*interval*/) {
	return 0;
}

void SDL_DisableScreenSaver(void) {
}

// A desktop of 1920 x 1080.
int SDL_GetDesktopDisplayMode(int /*displayIndex*/, SDL_DisplayMode *mode) {
	mode->format = SDL_PIXELFORMAT_RGB888;
	mode->w = 1920;
	mode->h = 1080;
	mode->refresh_rate = 60;
	mode->driverdata = nullptr;
	return 0;
}

int SDL_SetRelativeMouseMode(int /*enabled*/) {
	return 0;
}
