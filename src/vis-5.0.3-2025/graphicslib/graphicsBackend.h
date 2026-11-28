// What Init() (AppFunctions.cpp) needs from the graphics backend. The original makes the OpenGL backend there:
// `g_subSys = new TGraphicsSubSystemGL` and `graphics = new TGraphicsOGL(...)` (asm 494588-494625). The backend is not
// reconstructed (TODO.md), so the construction is behind this one function; the ScummVM engine will provide its own.
#pragma once

#include "WxStub.h"

/** The arguments of the constructor of TGraphicsOGL (asm 494625), the values the game and the command line / config.ini ask for. */
struct TGraphicsStartup {
	wxSize gameResolution;          ///< kGameWindowResolution (0x7E): the size of the game's own picture
	wxSize windowSize;              ///< the size of the window (`-r WxH`, else the game's size; full screen: the desktop)
	bool textureForWidescreen = true;   ///< `-utw` / UseTextureForWidescreen
	bool nearestNeighbor = false;       ///< kGameNearestNeighborInterpolation (0x2E9)
	bool fullscreen = true;
	bool resizeable = false;
	bool textureCompression = false;    ///< `-tc` / UseTextureCompression
	int picBufferSize = 0;              ///< kGamePicBufferSize (0x2DD)
	int preloadPicThreads = 0;          ///< kGamePreloadPicThreads (0x2DB)
	int preloadedPicBufferSize = 0;     ///< kGamePreloadedPicBufferSize (0x2DC)
};

/** Makes `g_subSys` and `graphics` for the settings; false when there is no backend (the cause is logged). */
bool CreateGraphicsBackend(const TGraphicsStartup &startup);
