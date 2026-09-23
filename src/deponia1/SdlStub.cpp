#include "SdlStub.h"

#include <cstdio>

int SDL_Init(Uint32 /*flags*/) {
    return 0;
}

void SDL_Quit(void) {
}

const char* SDL_GetError(void) {
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

SDL_RWops* SDL_RWFromFile(const char* /*file*/, const char* /*mode*/) {
    return nullptr;
}

int SDL_GameControllerAddMappingsFromRW(SDL_RWops* /*rw*/, int /*freesrc*/) {
    return 0;
}
