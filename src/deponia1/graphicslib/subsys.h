// Confirmed call shapes only (Deponia_Linux.asm: `g_subSys`, the global the video functions of the GL backend are
// called through; 299 uses): the two virtual functions that the script commands use. The rest of the backend is not
// reconstructed - nothing sets g_subSys, so the scripts see no video card name and no shaders.
#pragma once

#include <string>

#include "graphicslib/shader.h"

/** A texture of the backend (the particle container keeps one for all of its pictures; only its destructor is known). */
class TSubSysTexture {
public:
	virtual ~TSubSysTexture() {}
};

class TSubSys {
public:
	virtual ~TSubSys() {}

	/** Slot 0x20 (shaderCompile): makes a shader of the backend. */
	virtual TShader *CreateShader() = 0;
	/** Slot 0xD8 (the window changed its size): the new size and whether the window is a full screen one. */
	virtual void WindowResized(int width, int height, bool fullscreen) = 0;
	/** Slot 0x100 (`system.systemInfo().gpu`): the name of the video card. */
	virtual std::string GetGPUName() = 0;
};

extern TSubSys *g_subSys;
