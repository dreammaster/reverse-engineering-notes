#include "TTimer.h"

#include "SdlStub.h"

// GetTime()'s callers (TMasterControl::Draw/ScrollUpdate) all treat it as
// "milliseconds elapsed since the last SetTime()", so that's what this
// implements; SDL_GetTicks (already stubbed for main()) is a reasonable
// backing clock.
void TTimer::SetTime() {
    m_setAt = SDL_GetTicks();
}

std::int64_t TTimer::GetTime() const {
    return static_cast<std::int64_t>(SDL_GetTicks()) - m_setAt;
}
