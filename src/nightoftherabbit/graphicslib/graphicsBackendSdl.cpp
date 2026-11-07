#ifdef USE_REAL_SDL

#include "graphicslib/graphicsBackend.h"
#include "graphicslib/graphics.h"
#include "graphicslib/sdlGraphics.h"

// The standalone player draws with SDL's renderer (TSdlGraphics): there is no OpenGL backend and no sub system for it (the scripts see no shaders).
bool CreateGraphicsBackend(const TGraphicsStartup &startup) {
	graphics = new TSdlGraphics(startup);
	return true;
}

#endif // USE_REAL_SDL
