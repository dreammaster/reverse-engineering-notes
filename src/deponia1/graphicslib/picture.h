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
// Depth varies a lot by method, same as TMasterControl/TComposedFile:
//   - Fully traced: ctor x2, dtor, Set, Clear, SetParallax,
//     TestCacheFileTime, LoadSpriteFromCache, RetryFailedPicturesLoad, and
//     GetFormat's format-detection logic (magic-byte sniffing for
//     RIFF/WEBP and the PNG signature, falling back to a single-letter
//     format-hint code: 'P'->PNG, 'W'->WebP, 'J'->JPG, 'G'->GIF, and a
//     final 'P' check for PCX that - per the disassembly - can only be
//     reached after the *first* 'P' check already failed, so it appears
//     unreachable; reproduced as observed, not "fixed").
//   - The five common "release the current sprite handle, notify the
//     graphics backend, clear the loaded pixel data" sequences repeated
//     nearly verbatim across the dtor/ctor/Set/Clear were factored into one
//     private ReleaseSpriteHandle() helper rather than duplicated.
//   - Everything else (the actual PNG/WebP/JPG/GIF/PCX decode pipeline -
//     ReadPictureFile/LoadHeader, preloading - FinishPreloaderRead/
//     CleanupPreloader/PreparePreloaderLoad, and all OpenGL-facing drawing -
//     Draw*/CreateSpriteTexture/PreparePaint) has correct signatures and
//     member layout to compile against, but stub bodies - each of those is
//     its own substantial reversing effort (real vendored libpng/libwebp/
//     jpgd decoders, real OpenGL call sequences) that wasn't done this
//     pass.
#pragma once

#include <vector>

#include "TPictureFormat.h"
#include "TPictureMEM.h"
#include "TPicturePreloader.h"
#include "TSpriteHandle.h"
#include "WxStub.h"
#include "baselib/file.h"

class TFramebuffer;

class TPictureIO : public TPictureMEM {
public:
    enum class ePreloadingStatus { NotPreloading, Preloading, Preloaded };
    enum class eLoadSetting { Normal, ForceReload };

    TPictureIO();
    TPictureIO(const TSprite& sprite, bool ownsSprite);
    virtual ~TPictureIO();

    TPictureIO(const TPictureIO&) = delete;
    TPictureIO& operator=(const TPictureIO& other);

    static void RetryFailedPicturesLoad();
    static void TestCacheFileTime(bool enable);

    void Set(const TSprite& sprite);
    void Clear();
    void RemoveSprite();

    wxString GetSpriteName() const;
    void SetParallax(int x, int y);
    bool LoadSpriteFromCache();

    bool GetFormat(TFile& file, TPictureFormat** outFormat, const wxString& formatHint) const;
    bool LoadHeader(TFile& file, TPictureFormat** outFormat);
    bool ReadPictureFile();
    void EnsureSizeValid();
    bool LoadRect(TSprite& sprite);

    bool CreateFromFramebuffer(TFramebuffer* framebuffer);
    bool CreateSprite(bool preload);
    bool CreateSprite(void* pixels, int width, int height, int bpp);
    bool CreateEmptySprite(int width, int height, int bpp, bool preload);
    bool CreateSpriteTexture(const wxFileName& file);

    bool FinishPreloaderRead(int result);
    void CleanupPreloader();
    void PreparePreloaderLoad(wxFileName file);
    bool LoadPicture(wxFileName file, eLoadSetting loadSetting);
    bool RefreshSprite(bool force);

    bool IsTransparent(const wxPoint& point) const;
    bool SavePicture(const wxFileName& file, bool overwrite) const;
    bool WritePicture(TPictureFormat& format, const wxFileName& file) const;

    wxRect GetDestRect() const;
    void PreparePaint(wxRect& destRect, FloatRect& srcRect);
    unsigned long GetSpriteMemSize() const;
    TSpriteHandle* GetSpriteHandle() const;

    void SetPreloadingStatus(ePreloadingStatus status);
    ePreloadingStatus GetPreloadingStatus();
    void SetPreloader(TPicturePreloader* preloader);
    TPicturePreloader* GetPreloader();

    void DrawWithDestRect(const wxRect& destRect, float alpha, unsigned int color);
    void DrawWithSrcRect(const wxRect& srcRect, float alpha, unsigned int color);
    void DrawWithLightMap(float alpha, unsigned int color, void* lightMap);
    void Draw(float alpha, unsigned int color);

private:
    // Common "release current sprite handle, tell the graphics backend,
    // reset the transient flags" sequence shared (near-verbatim in the
    // original) by the destructor, both constructors, Set(), and Clear().
    void ReleaseSpriteHandle();

    static std::vector<TPictureIO*> m_loadFailedPics;
    static wxCriticalSection RetryFailedPicturesSection;
    static wxCriticalSection m_mutexStatus;
    static bool s_bTestCacheFileTime;

    TSpriteHandle* m_spriteHandle = nullptr;
    bool m_flag90 = false;
    wxRect m_destRect;      // +0x94-0xA0 in the original - guessed from GetDestRect()
    bool m_ownsSprite = false;  // +0xA4, set from the (TSprite,bool) ctor's bool param
    int m_parallaxX = 0;
    int m_parallaxY = 0;
    int m_field78 = 0;      // reset (only) when Set() is called with an unchanged sprite
    ePreloadingStatus m_preloadingStatus = ePreloadingStatus::NotPreloading;  // +0xB4, guarded by m_mutexStatus
    TPicturePreloader* m_preloader = nullptr;  // +0xB8
    bool m_flagC0 = false;
    int m_loadRectX = -1;   // +0xC8/+0xCC/+0xD0 - guessed to relate to LoadRect()
    int m_loadRectY = -1;
    int m_loadRectW = -1;
    int m_fieldD4 = 1;
    float m_scaleX = 1.0f;  // +0xD8
    float m_scaleY = 1.0f;  // +0xDC
    bool m_flagE0 = false;
};
