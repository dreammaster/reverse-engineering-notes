// The OpenGL backend of the shaders (Deponia 4), made on top of SDL's OpenGL renderer: the stand-alone player draws the game into a
// texture of SDL's renderer, and this runs the passes of the render configuration (`shaderSetOptions`) over that picture with the
// shaders of the scripts and the ones built into the original (`builtinShaders.cpp`).
//
// What the original does is `TGraphicsOGL::AfterDrawScene()` (asm 1222775-1224700 of the Deponia 4 listing: the transition, then the
// passes) over `TGraphicsSubSystemGL` and `GLShader`. Not the same: the original draws the whole game through its own shaders (the
// sprites, the light maps, the text, the particles are `textured.bin`, `lightmap.bin`, `text.bin` ...), here SDL draws them and only the
// picture of the whole frame is given to the passes. Not done: the transition of two scenes, the textures of the uniforms (`_t_`),
// the framebuffers of the scripts.
#pragma once

#ifdef USE_REAL_SDL

#include "WxStub.h"
#include "graphicslib/subsys.h"

struct SDL_Renderer;
struct SDL_Texture;

class TSubSysGL : public TSubSys {
public:
	explicit TSubSysGL(SDL_Renderer *renderer);
	~TSubSysGL() override;

	/** Whether the functions of OpenGL that are used were found (else nothing here works). */
	bool IsReady() const {
		return _ready;
	}

	/** A texture of the passes with the framebuffer that draws into it. */
	struct Buffer {
		unsigned texture = 0;
		unsigned framebuffer = 0;
		int width = 0;
		int height = 0;
		float scale = 1.0f;   ///< the part of the texture that the last pass wrote (1 / downsize)
	};

	TShader *CreateShader() override;
	void WindowResized(int width, int height, bool fullscreen) override;
	std::string GetGPUName() override;

	/** Runs the passes of the first render configuration over the picture `scene` (the game's picture, `sceneWidth` x `sceneHeight`); what the
	 *  passes that have the target 0 (the screen) draw is kept for DrawOutput(). False when there is nothing to do (the caller draws the
	 *  picture as always) or it could not be done. */
	bool RunPasses(SDL_Texture *scene, int sceneWidth, int sceneHeight);
	/** Draws what the last RunPasses() made into the window, in `area` (a window of the height `windowHeight`). */
	void DrawOutput(const wxRect &area, int windowHeight);


private:
	bool ensureBuffer(unsigned index, int width, int height);
	void freeBuffers();

	SDL_Renderer *_renderer;
	bool _ready = false;
	std::string _gpu;
	std::vector<Buffer> _buffers;
	Buffer _output;   ///< the screen of the passes (target 0)
	class GLShader *_copyShader = nullptr;
	int _pass = 0;
};

#endif
