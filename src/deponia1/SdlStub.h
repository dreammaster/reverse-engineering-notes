// Minimal SDL2-API-compatible declarations, standing in for real SDL2 headers
// until the third-party dependency is wired up. Names/signatures match SDL2
// exactly so this file can later be deleted in favor of <SDL2/SDL.h>.
#pragma once

#include <cstdint>

using Uint8 = std::uint8_t;
using Uint16 = std::uint16_t;
using Sint16 = std::int16_t;
using Uint32 = std::uint32_t;
using Sint32 = std::int32_t;

struct SDL_Window;
using SDL_GLContext = void*;
struct SDL_RWops;
struct SDL_Joystick;
struct SDL_GameController;
struct SDL_Haptic;

constexpr Uint32 SDL_INIT_TIMER = 0x00000001u;
constexpr Uint32 SDL_INIT_VIDEO = 0x00000020u;

// Event type constant used by main(): SDL_TEXTINPUT == 0x303 in real SDL2.
constexpr Uint32 SDL_TEXTINPUT = 0x303u;

constexpr int SDL_ENABLE = 1;
constexpr int SDL_DISABLE = 0;

// These are stub *implementations* (see SdlStub.cpp), not just
// declarations, so the reconstructed main() links and runs standalone
// before real SDL2 is wired in as a dependency.
extern "C" {
int SDL_Init(Uint32 flags);
void SDL_Quit(void);
const char* SDL_GetError(void);
Uint32 SDL_GetTicks(void);

int SDL_ShowCursor(int toggle);
Uint8 SDL_EventState(Uint32 type, int state);

int SDL_NumJoysticks(void);

SDL_RWops* SDL_RWFromFile(const char* file, const char* mode);
int SDL_GameControllerAddMappingsFromRW(SDL_RWops* rw, int freesrc);

// --- Game controller / joystick ---
int SDL_IsGameController(int joystickIndex);
SDL_GameController* SDL_GameControllerOpen(int joystickIndex);
void SDL_GameControllerClose(SDL_GameController* controller);
SDL_Joystick* SDL_GameControllerGetJoystick(SDL_GameController* controller);
const char* SDL_GameControllerName(SDL_GameController* controller);
int SDL_JoystickInstanceID(SDL_Joystick* joystick);
int SDL_JoystickIsHaptic(SDL_Joystick* joystick);

// --- Haptics ---
SDL_Haptic* SDL_HapticOpenFromJoystick(SDL_Joystick* joystick);
void SDL_HapticClose(SDL_Haptic* haptic);
int SDL_HapticQuery(SDL_Haptic* haptic);
int SDL_HapticNumAxes(SDL_Haptic* haptic);
int SDL_HapticNumEffects(SDL_Haptic* haptic);
int SDL_HapticRumbleSupported(SDL_Haptic* haptic);
int SDL_HapticRumbleInit(SDL_Haptic* haptic);
int SDL_HapticRumblePlay(SDL_Haptic* haptic, float strength, Uint32 lengthMs);
int SDL_HapticRumbleStop(SDL_Haptic* haptic);
int SDL_HapticStopAll(SDL_Haptic* haptic);
}

struct SDL_HapticDirection {
    Uint8 type;
    Sint32 dir[3];
};

struct SDL_HapticConstant {
    Uint16 type;
    SDL_HapticDirection direction;
    Uint32 length;
    Uint16 delay;
    Uint16 button;
    Uint16 interval;
    Sint16 level;
    Uint16 attack_length;
    Uint16 attack_level;
    Uint16 fade_length;
    Uint16 fade_level;
};

struct SDL_HapticPeriodic {
    Uint16 type;
    SDL_HapticDirection direction;
    Uint32 length;
    Uint16 delay;
    Uint16 button;
    Uint16 interval;
    Uint16 period;
    Sint16 magnitude;
    Sint16 offset;
    Uint16 phase;
    Uint16 attack_length;
    Uint16 attack_level;
    Uint16 fade_length;
    Uint16 fade_level;
};

struct SDL_HapticCondition {
    Uint16 type;
    SDL_HapticDirection direction;
    Uint32 length;
    Uint16 delay;
    Uint16 button;
    Uint16 interval;
    Uint16 right_sat[3];
    Uint16 left_sat[3];
    Sint16 right_coeff[3];
    Sint16 left_coeff[3];
    Uint16 deadband[3];
    Sint16 center[3];
};

struct SDL_HapticRamp {
    Uint16 type;
    SDL_HapticDirection direction;
    Uint32 length;
    Uint16 delay;
    Uint16 button;
    Uint16 interval;
    Sint16 start;
    Sint16 end;
    Uint16 attack_length;
    Uint16 attack_level;
    Uint16 fade_length;
    Uint16 fade_level;
};

struct SDL_HapticLeftRight {
    Uint16 type;
    Uint32 length;
    Uint16 large_magnitude;
    Uint16 small_magnitude;
};

union SDL_HapticEffect {
    Uint16 type;
    SDL_HapticConstant constant;
    SDL_HapticPeriodic periodic;
    SDL_HapticCondition condition;
    SDL_HapticRamp ramp;
    SDL_HapticLeftRight leftright;
};

extern "C" {
int SDL_HapticNewEffect(SDL_Haptic* haptic, SDL_HapticEffect* effect);
int SDL_HapticRunEffect(SDL_Haptic* haptic, int effect, Uint32 iterations);
int SDL_HapticStopEffect(SDL_Haptic* haptic, int effect);
}

// Real SDL2 enum/struct, needed for TGameControl's controller input
// handlers (ConvertControllerButtonToSymKey, HandleControllerButtonHit,
// etc.) - not reversed, just declared so those signatures compile.
enum SDL_GameControllerAxis {
    SDL_CONTROLLER_AXIS_INVALID = -1,
    SDL_CONTROLLER_AXIS_LEFTX,
    SDL_CONTROLLER_AXIS_LEFTY,
    SDL_CONTROLLER_AXIS_RIGHTX,
    SDL_CONTROLLER_AXIS_RIGHTY,
    SDL_CONTROLLER_AXIS_TRIGGERLEFT,
    SDL_CONTROLLER_AXIS_TRIGGERRIGHT,
    SDL_CONTROLLER_AXIS_MAX
};

struct SDL_ControllerButtonEvent {
    Uint32 type;
    Uint32 timestamp;
    Sint32 which;
    Uint8 button;
    Uint8 state;
    Uint8 padding1;
    Uint8 padding2;
};
