// Reconstructed from Deponia_Linux.asm, TPictureIO methods at asm lines
// 785258-791178+ (address range 0x6F7540-0x6FA000+). See NOTES.md.
//
// Original path confirmed via x_assert() calls inside several TPictureIO
// methods (not yet reconstructed), e.g. TPictureIO::ReadPictureFile():
// src/graphicslib/picture.cpp - see manifest/source_layout.tsv.
//
// TPictureIO manages one loaded/loadable image (a "sprite"): decoding,
// caching, preloading, and drawing it. Confirmed: it derives from
// TPictureMEM, which itself derives from TSprite (TPictureIO's ctor/dtor
// call TPictureMEM's ctor/dtor directly on `this` with no offset
// adjustment, and TPictureIO also calls TSprite::Set/operator==/
// SetImageSize the same way - only explained by that inheritance chain).
//
// What is reconstructed: the loading of a picture (GetFormat(), LoadHeader(), ReadPictureFile(), LoadPicture(), LoadRect(),
// the preloader calls PreparePreloaderLoad()/FinishPreloaderRead()/CleanupPreloader(), RetryFailedPicturesLoad()), the sprite
// (CreateSprite(), the cache, the transparency bitmap, IsTransparent()), the drawing calls (PreparePaint(), Draw*() - which hand
// the work to the backend, TGraphicsInterface::Draw()) and saving (SavePicture()). Not reconstructed: the decoders of the
// formats (TPictureFormat), the thread that preloads pictures (TPreloadedPicManager: nothing is ever queued) and so
// RefreshSprite(), which waits for it, and the backend (graphics) that makes the textures.
#pragma once

#include <vector>

#include "TPictureFormat.h"
#include "TPictureMEM.h"
#include "TPicturePreloader.h"
#include "TSpriteHandle.h"
#include "WxStub.h"
#include "baselib/file.h"

class TFramebuffer;
class TPaintControl;

class TPictureIO : public TPictureMEM {
public:
	enum class ePreloadingStatus { NotPreloading, Preloading, Preloaded };
	/** LoadPicture(): Normal looks in the sprite cache first, ForceReload reads the file again; MemoryOnly (the original passes 2)
	 *  reads the pixels and makes no sprite. */
	enum class eLoadSetting { Normal, ForceReload, MemoryOnly };

	TPictureIO();
	TPictureIO(const TSprite &sprite, bool ownsSprite);
	virtual ~TPictureIO();

	TPictureIO(const TPictureIO &) = delete;
	TPictureIO &operator=(const TPictureIO &other);

	// Confirmed a real static (TPaintControl::SetCurrent()/GetCurrent()/~TPaintControl(),
	// Deponia_Linux.asm lines 748595-748654): the paint control pictures
	// currently draw relative to.
	static TPaintControl *s_pPaintControl;

	static void RetryFailedPicturesLoad();
	static void TestCacheFileTime(bool enable);

	void Set(const TSprite &sprite);
	void Clear();
	void RemoveSprite();

	wxString GetSpriteName() const;
	void SetParallax(int x, int y);
	bool LoadSpriteFromCache();

	bool GetFormat(TFile &file, TPictureFormat **outFormat, const wxString &formatHint) const;
	bool LoadHeader(TFile &file, TPictureFormat **outFormat);
	bool ReadPictureFile();
	/** Reads the size of the picture from its file if it is not known yet (a picture that has none is marked as failed). */
	bool EnsureSizeValid();
	bool LoadRect(TSprite &sprite);

	bool CreateFromFramebuffer(TFramebuffer *framebuffer);
	bool CreateSprite(bool preload);
	/** The picture is these pixels (the picture takes them over). */
	bool CreateSprite(void *pixels, int width, int height, int bpp);
	bool CreateEmptySprite(int width, int height, int bpp, bool preload);
	bool CreateSpriteTexture(const wxFileName &file);

	bool FinishPreloaderRead(int result);
	void CleanupPreloader();
	/** Opens the file and reads the header on the thread of the game, the data is read by FinishPreloaderRead(): the size
	 *  (height << 32 | width), or -1 if the file cannot be read. */
	unsigned long long PreparePreloaderLoad(wxFileName file);
	bool LoadPicture(wxFileName file, eLoadSetting loadSetting);
	bool RefreshSprite(bool force);

	bool IsTransparent(const wxPoint &point) const;
	/** Writes the picture as a PNG (`webp`: as a WebP). */
	bool SavePicture(const wxFileName &file, bool webp) const;
	bool WritePicture(TPictureFormat &format, const wxFileName &file) const;

	wxRect GetDestRect() const;
	/** Confirmed (asm 788872-789051): works out where the picture is drawn. `sourceRect` is the part of the sprite (all of
	 *  it: 0, 0, the image size; mirrored: moved to the other side), `destRect` where it is on the screen (the position of
	 *  the picture plus the origin of the paint control, less its scroll position times the parallax and its scroll
	 *  position, in floats; the size is the image size times the scale); the rectangle in screen pixels is kept in
	 *  GetDestRect(). Needs the current paint control. */
	void PreparePaint(wxRect &sourceRect, FloatRect &destRect);
	unsigned long GetSpriteMemSize() const;
	TSpriteHandle *GetSpriteHandle() const;

	void SetPreloadingStatus(ePreloadingStatus status);
	ePreloadingStatus GetPreloadingStatus();
	void SetPreloader(TPicturePreloader *preloader);
	TPicturePreloader *GetPreloader();

	void DrawWithDestRect(const wxRect &destRect, float alpha, unsigned int color);
	void DrawWithSrcRect(const wxRect &srcRect, float alpha, unsigned int color);
	void DrawWithLightMap(float alpha, unsigned int color, void *lightMap);
	void Draw(float alpha, unsigned int color);
	// Confirmed (TGScene::Draw(), Deponia_Linux.asm line 166568+, writes
	// +0xD0 of the embedded background picture right before Draw()).
	void SetShader(int shader) {
		_shader = shader;
	}
	// Confirmed (TCAnimation::PreloadSprites, asm lines 1381578-1381644): written
	// (+0xB0) with the order a picture is to be preloaded in before it is handed
	// to the preloader.
	void SetPreloadPriority(int priority) {
		_preloadPriority = priority;
	}
	// Confirmed (TGAnimation::Draw(), asm lines 149954-150106): written (+0xC4,
	// +0xC8, +0xD8/+0xDC, +0xD4) from the rotation, rotation centre, scale and
	// matrix id fields of the object an animation is drawn for, just before Draw().
	void SetRotation(float rotation) {
		_rotation = rotation;
	}
	void SetRotationCenter(const wxPoint &center) {
		_rotationCenter = center;
	}
	void SetScale(float scaleX, float scaleY) {
		_scaleX = scaleX;
		_scaleY = scaleY;
	}
	void SetScaleX(float scaleX) {
		_scaleX = scaleX;
	}
	void SetScaleY(float scaleY) {
		_scaleY = scaleY;
	}
	float GetRotation() const {
		return _rotation;
	}
	const wxPoint &GetRotationCenter() const {
		return _rotationCenter;
	}
	int GetShader() const {
		return _shader;
	}
	float GetScaleX() const {
		return _scaleX;
	}
	float GetScaleY() const {
		return _scaleY;
	}
	void SetMatrixId(int matrixId) {
		_matrixId = matrixId;
	}
	int GetMatrixId() const {
		return _matrixId;
	}

private:
	// Common "release current sprite handle, tell the graphics backend,
	// reset the transient flags" sequence shared (near-verbatim in the
	// original) by the destructor, both constructors, Set(), and Clear().
	void ReleaseSpriteHandle();
	/** Notes the picture for RetryFailedPicturesLoad() (once). */
	void noteFailedLoad();

	static std::vector<TPictureIO *> _loadFailedPics;
	static wxCriticalSection RetryFailedPicturesSection;
	static wxCriticalSection _mutexStatus;
	static bool s_bTestCacheFileTime;

	TSpriteHandle *_spriteHandle = nullptr;
	bool _flag90 = false;
	wxRect _destRect;      // +0x94-0xA0 in the original - guessed from GetDestRect()
	bool _ownsSprite = false;  // +0xA4, set from the (TSprite,bool) ctor's bool param (CreateSprite() makes the transparency bitmap when it is set)
	int _parallaxX = 0;
	int _parallaxY = 0;
	TFile *_preloadFile = nullptr;           // +0x78, the file a preload reads from
	TPictureFormat *_preloadFormat = nullptr;  // +0x80, the format of it
	int _preloadPriority = -1;  // +0xB0
	ePreloadingStatus _preloadingStatus = ePreloadingStatus::NotPreloading;  // +0xB4, guarded by _mutexStatus
	TPicturePreloader *_preloader = nullptr;  // +0xB8
	bool _flagC0 = false;
	float _rotation = 0.0f;  // +0xC4, from the rotation field of the object the picture is drawn for
	wxPoint _rotationCenter{-1, -1};  // +0xC8/+0xCC (-1, -1: none)
	int _shader = -1;      // +0xD0, confirmed by TGScene::Draw(): written with the shader id just before Draw()
	int _matrixId = 1;     // +0xD4, from the matrix id field of the object the picture is drawn for
	float _scaleX = 1.0f;  // +0xD8
	float _scaleY = 1.0f;  // +0xDC
	bool _flagE0 = false;
};
