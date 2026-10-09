#include "graphicslib/graphicsBackend.h"

// Not reconstructed: the OpenGL backend (TGraphicsSubSystemGL, TGraphicsOGL). Without it the player cannot start: this
// says so and Init() fails.
bool CreateGraphicsBackend(const TGraphicsStartup &/*startup*/) {
	if (wxLog::loglevel >= 0)
		wxLog::logexpanded(L"There is no graphics backend (the OpenGL backend is not reconstructed yet).");

	return false;
}
