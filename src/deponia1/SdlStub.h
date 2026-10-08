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
	const char *SDL_GetError(void);
	Uint32 SDL_GetTicks(void);
	// Confirmed call shape only (TGameControl::HandleKeyEvent,
	// Deponia_Linux.asm line 471295) - not reversed beyond that.
	const char *SDL_GetKeyName(int key);

	int SDL_ShowCursor(int toggle);
	Uint8 SDL_EventState(Uint32 type, int state);

	int SDL_NumJoysticks(void);

	SDL_RWops *SDL_RWFromFile(const char *file, const char *mode);
	int SDL_GameControllerAddMappingsFromRW(SDL_RWops *rw, int freesrc);

// --- Game controller / joystick ---
	int SDL_IsGameController(int joystickIndex);
	SDL_GameController *SDL_GameControllerOpen(int joystickIndex);
	void SDL_GameControllerClose(SDL_GameController *controller);
	SDL_Joystick *SDL_GameControllerGetJoystick(SDL_GameController *controller);
	const char *SDL_GameControllerName(SDL_GameController *controller);
	int SDL_JoystickInstanceID(SDL_Joystick *joystick);
	int SDL_JoystickIsHaptic(SDL_Joystick *joystick);

// --- Haptics ---
	SDL_Haptic *SDL_HapticOpenFromJoystick(SDL_Joystick *joystick);
	void SDL_HapticClose(SDL_Haptic *haptic);
	int SDL_HapticQuery(SDL_Haptic *haptic);
	int SDL_HapticNumAxes(SDL_Haptic *haptic);
	int SDL_HapticNumEffects(SDL_Haptic *haptic);
	int SDL_HapticRumbleSupported(SDL_Haptic *haptic);
	int SDL_HapticRumbleInit(SDL_Haptic *haptic);
	int SDL_HapticRumblePlay(SDL_Haptic *haptic, float strength, Uint32 lengthMs);
	int SDL_HapticRumbleStop(SDL_Haptic *haptic);
	int SDL_HapticStopAll(SDL_Haptic *haptic);
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
	int SDL_HapticNewEffect(SDL_Haptic *haptic, SDL_HapticEffect *effect);
	int SDL_HapticRunEffect(SDL_Haptic *haptic, int effect, Uint32 iterations);
	int SDL_HapticStopEffect(SDL_Haptic *haptic, int effect);
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

// Real SDL2 keycode values, needed for TGameControl::InitGameActions' fixed
// action-key table (Deponia_Linux.asm data at address 0xD6D240 onward): a
// non-printable key's SDL_Keycode is its SDL_Scancode OR'd with
// SDLK_SCANCODE_MASK - see SDL2's SDL_keycode.h/SDL_scancode.h. Only the
// scancodes actually seen in that table are given names here.
constexpr Sint32 SDLK_SCANCODE_MASK = 1 << 30;
constexpr Sint32 SDLK_F1 = 58 | SDLK_SCANCODE_MASK;
constexpr Sint32 SDLK_F2 = 59 | SDLK_SCANCODE_MASK;
constexpr Sint32 SDLK_F3 = 60 | SDLK_SCANCODE_MASK;
constexpr Sint32 SDLK_F4 = 61 | SDLK_SCANCODE_MASK;
constexpr Sint32 SDLK_F5 = 62 | SDLK_SCANCODE_MASK;
constexpr Sint32 SDLK_F6 = 63 | SDLK_SCANCODE_MASK;
constexpr Sint32 SDLK_F7 = 64 | SDLK_SCANCODE_MASK;
constexpr Sint32 SDLK_F8 = 65 | SDLK_SCANCODE_MASK;
constexpr Sint32 SDLK_F9 = 66 | SDLK_SCANCODE_MASK;
constexpr Sint32 SDLK_F10 = 67 | SDLK_SCANCODE_MASK;
constexpr Sint32 SDLK_F11 = 68 | SDLK_SCANCODE_MASK;
constexpr Sint32 SDLK_F12 = 69 | SDLK_SCANCODE_MASK;
constexpr Sint32 SDLK_RIGHT = 79 | SDLK_SCANCODE_MASK;
constexpr Sint32 SDLK_LEFT = 80 | SDLK_SCANCODE_MASK;
constexpr Sint32 SDLK_DOWN = 81 | SDLK_SCANCODE_MASK;
constexpr Sint32 SDLK_UP = 82 | SDLK_SCANCODE_MASK;

// The events that the commands of the scripts make (real SDL2 values and layouts).
constexpr Uint32 SDL_KEYDOWN = 0x300u;
constexpr Uint32 SDL_KEYUP = 0x301u;
constexpr Uint32 SDL_MOUSEMOTION = 0x400u;
constexpr Uint32 SDL_MOUSEBUTTONDOWN = 0x401u;
constexpr Uint32 SDL_MOUSEBUTTONUP = 0x402u;
constexpr Uint32 SDL_MOUSEWHEEL = 0x403u;
constexpr Uint32 SDL_CONTROLLERAXISMOTION = 0x650u;
constexpr Uint32 SDL_CONTROLLERBUTTONDOWN = 0x651u;
constexpr Uint32 SDL_CONTROLLERBUTTONUP = 0x652u;

struct SDL_MouseMotionEvent {
	Uint32 type;
	Uint32 timestamp;
	Uint32 windowID;
	Uint32 which;
	Uint32 state;
	Sint32 x;
	Sint32 y;
	Sint32 xrel;
	Sint32 yrel;
};

struct SDL_MouseButtonEvent {
	Uint32 type;
	Uint32 timestamp;
	Uint32 windowID;
	Uint32 which;
	Uint8 button;
	Uint8 state;
	Uint8 clicks;
	Uint8 padding1;
	Sint32 x;
	Sint32 y;
};

struct SDL_MouseWheelEvent {
	Uint32 type;
	Uint32 timestamp;
	Uint32 windowID;
	Uint32 which;
	Sint32 x;
	Sint32 y;
	Uint32 direction;
};

struct SDL_Keysym {
	Sint32 scancode;
	Sint32 sym;
	Uint16 mod;
	Uint32 unused;
};

struct SDL_KeyboardEvent {
	Uint32 type;
	Uint32 timestamp;
	Uint32 windowID;
	Uint8 state;
	Uint8 repeat;
	Uint8 padding2;
	Uint8 padding3;
	SDL_Keysym keysym;
};

struct SDL_ControllerAxisEvent {
	Uint32 type;
	Uint32 timestamp;
	Sint32 which;
	Uint8 axis;
	Uint8 padding1;
	Uint8 padding2;
	Uint8 padding3;
	Sint16 value;
	Uint16 padding4;
};

union SDL_Event {
	Uint32 type;
	SDL_MouseMotionEvent motion;
	SDL_MouseButtonEvent button;
	SDL_MouseWheelEvent wheel;
	SDL_KeyboardEvent key;
	SDL_ControllerAxisEvent caxis;
	SDL_ControllerButtonEvent cbutton;
	Uint8 padding[56];
};

extern "C" {
	void SDL_SetWindowTitle(SDL_Window *window, const char *title);
	float SDL_GetWindowBrightness(SDL_Window *window);
	int SDL_SetWindowBrightness(SDL_Window *window, float brightness);
	int SDL_PushEvent(SDL_Event *event);
	void SDL_WarpMouseInWindow(SDL_Window *window, int x, int y);
}
