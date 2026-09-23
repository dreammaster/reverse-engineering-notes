// Minimal SDL2-API-compatible declarations, standing in for real SDL2 headers
// until the third-party dependency is wired up. Names/signatures match SDL2
// exactly so this file can later be deleted in favor of <SDL2/SDL.h>.
#pragma once

#include <cstdint>

using Uint8 = std::uint8_t;
using Uint32 = std::uint32_t;

struct SDL_Window;
using SDL_GLContext = void*;
struct SDL_RWops;

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

int SDL_ShowCursor(int toggle);
Uint8 SDL_EventState(Uint32 type, int state);

int SDL_NumJoysticks(void);

SDL_RWops* SDL_RWFromFile(const char* file, const char* mode);
int SDL_GameControllerAddMappingsFromRW(SDL_RWops* rw, int freesrc);
}
