#include "SdlStub.h"

#include <chrono>
#include <cstdio>

int SDL_Init(Uint32 /*flags*/) {
    return 0;
}

void SDL_Quit(void) {
}

const char* SDL_GetError(void) {
    return "";
}

Uint32 SDL_GetTicks(void) {
    using namespace std::chrono;
    return static_cast<Uint32>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
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

SDL_RWops* SDL_RWFromFile(const char* /*file*/, const char* /*mode*/) {
    return nullptr;
}

int SDL_GameControllerAddMappingsFromRW(SDL_RWops* /*rw*/, int /*freesrc*/) {
    return 0;
}

int SDL_IsGameController(int /*joystickIndex*/) {
    return 0;
}

SDL_GameController* SDL_GameControllerOpen(int /*joystickIndex*/) {
    return nullptr;
}

void SDL_GameControllerClose(SDL_GameController* /*controller*/) {
}

SDL_Joystick* SDL_GameControllerGetJoystick(SDL_GameController* /*controller*/) {
    return nullptr;
}

const char* SDL_GameControllerName(SDL_GameController* /*controller*/) {
    return "";
}

int SDL_JoystickInstanceID(SDL_Joystick* /*joystick*/) {
    return -1;
}

int SDL_JoystickIsHaptic(SDL_Joystick* /*joystick*/) {
    return 0;
}

SDL_Haptic* SDL_HapticOpenFromJoystick(SDL_Joystick* /*joystick*/) {
    return nullptr;
}

void SDL_HapticClose(SDL_Haptic* /*haptic*/) {
}

int SDL_HapticQuery(SDL_Haptic* /*haptic*/) {
    return 0;
}

int SDL_HapticNumAxes(SDL_Haptic* /*haptic*/) {
    return 0;
}

int SDL_HapticNumEffects(SDL_Haptic* /*haptic*/) {
    return 0;
}

int SDL_HapticRumbleSupported(SDL_Haptic* /*haptic*/) {
    return 0;
}

int SDL_HapticRumbleInit(SDL_Haptic* /*haptic*/) {
    return 0;
}

int SDL_HapticRumblePlay(SDL_Haptic* /*haptic*/, float /*strength*/, Uint32 /*lengthMs*/) {
    return 0;
}

int SDL_HapticRumbleStop(SDL_Haptic* /*haptic*/) {
    return 0;
}

int SDL_HapticStopAll(SDL_Haptic* /*haptic*/) {
    return 0;
}

int SDL_HapticNewEffect(SDL_Haptic* /*haptic*/, SDL_HapticEffect* /*effect*/) {
    return -1;
}

int SDL_HapticRunEffect(SDL_Haptic* /*haptic*/, int /*effect*/, Uint32 /*iterations*/) {
    return -1;
}

int SDL_HapticStopEffect(SDL_Haptic* /*haptic*/, int /*effect*/) {
    return -1;
}
