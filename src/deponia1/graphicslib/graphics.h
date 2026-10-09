// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/graphicslib/graphics.cpp - see manifest/source_layout.tsv.
//
// The global rendering-backend singleton (`cs:graphics` in the
// disassembly). Only the surface TPictureIO/TMasterControl touch is
// stubbed: GetPreloadedPicManager(), GetSpriteFromCache(), and a virtual
// method at vtable slot 0x90 (called whenever a TSpriteHandle's refcount
// hits zero, to let the backend release the GPU resource - real name not
// recovered).
#pragma once

#include "TSpriteHandle.h"
#include "WxStub.h"
#include "datastruct/visobjref.h"
#include "graphicslib/spriteCache.h"

class TPreloadedPicManager;
struct TPictureMemBlock;
class TPictureMEM;

// Confirmed 2 raw-int parameters read straight from game data (field ids
// 0x224/0x225), never a named constant - real values/meaning not resolved
// (TGameControl::LoadAndInitGame, Deponia_Linux.asm lines 467710-467720).
enum class TInterpolationEnum : int {};

class TGraphicsInterface {
public:
	virtual ~TGraphicsInterface() = default;

	TPreloadedPicManager *GetPreloadedPicManager();
	/** The sprite cache (see TSpriteCache; the base class keeps it at +0x08 and passes the calls below on to it). */
	TSpriteCache *GetSpriteCache() {
		return &_spriteCache;
	}
	TSpriteHandle *GetSpriteFromCache(const wxString &name);
	void AddToCache(const wxString &name, TSpriteHandle *sprite);
	void PrintCacheContents(std::list<wxString> &lines);
	void ClearCache();
	// Confirmed call shape only (TGScene::SetCurrentLightmap(), Deponia_
	// Linux.asm line 168383+) - the shared memory block a scene's lightmap
	// picture decodes into; not reversed beyond that call shape.
	TPictureMemBlock *GetLightMapMemBlock();
	// Confirmed call shape only (TMSavegame::~TMSavegame()/SetActive()/
	// SaveGame(), Deponia_Linux.asm lines 159388-163700) - evicts a named
	// sprite from the sprite cache; not reversed beyond that call shape.
	bool RemoveFromCache(const wxString &name);
	// Confirmed call shapes only (TMSavegame::SaveSnapShot(), Deponia_Linux.
	// asm lines 162236-162327): the shared memory block frame captures decode
	// into, and the current frame's captured picture (a virtual slot, +0x128
	// of the unmodeled GL backend's vtable - null when nothing was captured);
	// not reversed beyond those call shapes.
	TPictureMemBlock *GetMainMemBlock();
	/** Confirmed (asm 762363-762383, bytes +0x270 and +0x272 of the backend): how the pixels of a picture that is made in
	 *  memory are laid out for the backend: `rgbOrder` (red, green, blue; else the other way round) and `flipped` (the lines
	 *  begin at the bottom). The OpenGL backend sets them; here they are what a TPictureMEM starts with. */
	void GetPicsMemSettings(bool &rgbOrder, bool &flipped);
	virtual TPictureMEM *GetCapturedFrame();
	// Confirmed call shapes only (CmdCreateScreenshot::Redo(), Deponia_Linux.asm lines 392054-392180): three more
	// slots of the unmodeled GL backend. Slot 0xC8 captures the screen into a picture (false: it failed); slot
	// 0x120 makes the screenshot that the next savegames use, and slot 0x130 clears it (the engine makes a new one
	// by itself when the scene changes from a playable one to a menu); both are called with 1.
	virtual bool CaptureScreen(class TPictureIO &picture);
	virtual void CreateSavegameScreenshot(bool flag);
	virtual void ClearSavegameScreenshot(bool flag);
	// Confirmed call shapes only (CmdToggleWindowMode/CmdSetWindowSize/CmdGetWindowMode, asm lines 388874-388967):
	// slots 0x158 (changes between the window and the full screen), 0x160 (the size of the window, in windowed mode)
	// and 0x168 (whether the window is a full screen one) of the unmodeled GL backend.
	/** Slot 0x28 (a window event of ShowFrame): works out the part of the window where the game is drawn (the window
	 *  less the bars of the aspect ratio), from the size of the window and the resolution of the game. */
	virtual void CalculateDisplayedArea(const wxSize &windowSize, const wxSize &renderSize, wxRect *displayedArea);
	/** Slot 0x18 (CreateWindowGL, asm 492404): the backend takes the window that was just made (its OpenGL state is set up
	 *  here) and works out the part of it where the game is drawn (into `displayedArea`). False when it cannot. Here only
	 *  the area is worked out. */
	virtual bool InitGraphics(const wxSize &windowSize, wxSize &renderSize, wxRect *displayedArea);
	virtual void ToggleWindowMode();
	virtual bool SetWindowSize(int width, int height);
	virtual bool IsFullscreen();

	// Confirmed call shapes only (TGameControl::LoadAndInitGame, asm lines
	// 467717-467726, 467884-467886) - not reversed beyond that.
	void SetFilters(TInterpolationEnum a, TInterpolationEnum b);
	void PreallocateTextures(int count);
	void SetCacheSize(int size);

	// Confirmed call shapes only (TGameControl::Update, Deponia_Linux.asm
	// lines 469849, 469854) - checked once per frame; when the count has
	// changed, UpdateCache() is called to react to it. Not reversed beyond
	// that call shape.
	int GetCacheSpriteCount() const;
	bool UpdateCache();

	// vtable slot 0x90 in the original; called with a TSpriteHandle* whose
	// refcount just reached zero.
	virtual void OnSpriteHandleReleased(TSpriteHandle *handle);
	/** Slot 0x40 (TGraphicsOGL::FinishDraw() is empty; the sprite cache calls it before it lets go of the sprites). */
	virtual void FinishDraw();
	/** Makes the sprite (the backend's texture) of `width` x `height` pixels of `bytesPerPixel` bytes from `data` and puts the
	 *  handle in `sprite`; false if it cannot be made. Here only the handle is made, with the memory the pixels take. */
	virtual bool CreateSprite(TSpriteHandle **sprite, const char *data, int width, int height, int bytesPerPixel, int pitch,
	                          bool flag);

	// Called from TMasterControl::Draw/Signal/PlayAVI (vtable slots
	// 0x30/0x38/0x178 there); real parameter meaning not recovered.
	virtual void SetMatrixMode(bool a, bool b);
	virtual void ResetMatrix(bool a, bool b);
	virtual void Flip();
	/** Slots 0x110 and 0x118 (TGraphicsOGL::BeginBatch()/EndBatch()): the drawing calls in between are put together to
	 *  one (a text is drawn letter by letter between the two). */
	virtual void BeginBatch();
	virtual void EndBatch();

	/** The size of the picture that the game is drawn on (+0x1F4, +0x1F8; the backend sets them). */
	int GetWidth() const {
		return _width;
	}
	int GetHeight() const {
		return _height;
	}
	void SetSize(int width, int height) {
		_width = width;
		_height = height;
	}

	/** Slot 0x70 (TGraphicsOGL::Draw(void *, ...), asm 715317-716895): draws the part `sourceRect` (pixels of the sprite) of the
	 *  sprite on the screen at `destRect`, `alpha` opaque, `mirrored`, with `color` (0xAABBGGRR; the picture is multiplied
	 *  by it, premultiplied with the alpha), with the shader number `shader` (-1: the default), turned by `rotation` and
	 *  scaled by `scaleX`/`scaleY` about `rotationCenter` (-1 for a coordinate: the middle of the rectangle), through the
	 *  matrix `matrixId` (1 or more: through the matrices that the scripts set). While a batch is open (BeginBatch()) the
	 *  quads are collected and rotation, scale and matrix are not used. The backend's.  */
	virtual void Draw(TSpriteHandle *sprite, const wxRect &sourceRect, const FloatRect &destRect, float alpha, bool mirrored,
	                  const unsigned int &color, int shader, float rotation, const wxPoint &rotationCenter, float scaleX,
	                  float scaleY, int matrixId);
	/** Slot 0x78 (TGraphicsOGL::DrawWithLightMap): the same, the picture multiplied by `lightMap` (the light map's pixels,
	 *  moved by `offset`). */
	virtual void DrawWithLightMap(TSpriteHandle *sprite, const wxRect &sourceRect, const FloatRect &destRect, float alpha,
	                              bool mirrored, void *lightMap, const wxPoint &offset);

	/** Confirmed (asm 772807-773531, two overloads): narrows the rectangle `left`, `top`, `width`, `height` of a picture of
	 *  4-byte pixels (`rgba`, alpha the last byte) to what has something in it - the lines at the top and the bottom and
	 *  the columns at the left and the right that are transparent are taken off. On return `left` and `top` are how much
	 *  was taken off at those sides, `width` and `height` the new size. The first form looks at the rectangle at
	 *  (`originX`, `originY`) of a picture that is `pitch` pixels wide (everything is left as it is if the very first
	 *  pixel of the picture, not of the rectangle, has alpha: the original looks there); the second at a picture
	 *  that is `width` wide.
	 *  NOTE: the original does not take off the transparent lines at the bottom if the rectangle was narrowed by exactly
	 *  one column at the left (it tests the end of the scan with the wrong number); this is kept. */
	void RemoveTransparentEdges(int &left, int &top, int &width, int &height, int pitch, int originY, int originX,
	                            const char *rgba);
	void RemoveTransparentEdges(int &left, int &top, int &width, int &height, const char *rgba);
	/** The memory a sprite handle takes (the handle's own number). */
	int GetSpriteMemSize(TSpriteHandle *handle) const;

private:
	int _width = 0;
	int _height = 0;
	TSpriteCache _spriteCache;
	TPreloadedPicManager *_preloadedPicManager = nullptr;
};

extern TGraphicsInterface *graphics;

// Confirmed real globals (TGScene::Draw(), Deponia_Linux.asm lines 166568-
// 166997): the shader every picture draws with when its data record names
// none (id -1), and the callback that activates a given shader for the
// picture about to be drawn. Neither is reversed - the real shader system
// belongs with the (unmodeled) GL backend behind `graphics`.
extern int defaultShader;
void ShaderCallback(int shader, TVisObjRef *ref);
