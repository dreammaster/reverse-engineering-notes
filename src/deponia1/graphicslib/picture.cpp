#include "graphicslib/picture.h"

#include <algorithm>

#include "graphicslib/graphics.h"
#include "graphicslib/preloadedPicManager.h"

std::vector<TPictureIO*> TPictureIO::m_loadFailedPics;
wxCriticalSection TPictureIO::RetryFailedPicturesSection;
wxCriticalSection TPictureIO::m_mutexStatus;
bool TPictureIO::s_bTestCacheFileTime = false;

void TPictureIO::RetryFailedPicturesLoad() {
    // TPictureIO::onRetryLoad's real body (TComposedFile::onRetryLoad in
    // main.cpp calls this indirectly). Not traced beyond confirming it's
    // the target - presumably iterates m_loadFailedPics and retries
    // ReadPictureFile() for each.
}

void TPictureIO::TestCacheFileTime(bool enable) {
    s_bTestCacheFileTime = enable;
}

void TPictureIO::ReleaseSpriteHandle() {
    graphics->GetPreloadedPicManager()->StopPreloading(this);
    if (m_spriteHandle) {
        m_spriteHandle->Release();
        if (m_spriteHandle->GetRefCount() == 0) {
            graphics->OnSpriteHandleReleased(m_spriteHandle);
        }
        m_spriteHandle = nullptr;
        wxCriticalSectionLocker lock(m_mutexStatus);
        m_preloadingStatus = ePreloadingStatus::NotPreloading;
    }
    m_flag90 = false;
    m_flagC0 = false;
}

TPictureIO::TPictureIO() {
    m_scaleX = 1.0f;
    m_scaleY = 1.0f;
}

TPictureIO::TPictureIO(const TSprite& sprite, bool ownsSprite) {
    m_ownsSprite = ownsSprite;
    if (static_cast<const TSprite&>(*this) == sprite) {
        m_field78 = 0;
        return;
    }
    ReleaseSpriteHandle();
    m_width = 0;
    m_height = 0;
    ClearMemData();
    TSprite::Set(sprite);
    m_field78 = 0;
}

TPictureIO::~TPictureIO() {
    {
        wxCriticalSectionLocker lock(RetryFailedPicturesSection);
        auto it = std::find(m_loadFailedPics.begin(), m_loadFailedPics.end(), this);
        if (it != m_loadFailedPics.end())
            m_loadFailedPics.erase(it);
    }
    ReleaseSpriteHandle();
    m_width = 0;
    m_height = 0;
    ClearMemData();
}

TPictureIO& TPictureIO::operator=(const TPictureIO& other) {
    if (this != &other) {
        Set(other);
    }
    return *this;
}

void TPictureIO::Set(const TSprite& sprite) {
    if (static_cast<const TSprite&>(*this) == sprite)
        return;
    ReleaseSpriteHandle();
    m_width = 0;
    m_height = 0;
    ClearMemData();
    TSprite::Set(sprite);
}

void TPictureIO::Clear() {
    ReleaseSpriteHandle();
    m_width = 0;
    m_height = 0;
    ClearMemData();
}

void TPictureIO::RemoveSprite() {
    Clear();
}

wxString TPictureIO::GetSpriteName() const {
    // Builds a diagnostic name from the sprite's path plus a few numeric
    // fields (id, type, and - if s_bTestCacheFileTime - a file
    // modification timestamp via TFile::GetFileTime). The exact wx
    // formatting call (wxString::privFormat with a bare "%" format string)
    // couldn't be pinned down at the byte level (see NOTES.md); this
    // reproduces the observable inputs.
    wxString path = m_path.GetFullPath();
    return wxString(path.ToStdWstring() + L" (" + std::to_wstring(m_id) + L"," + std::to_wstring(m_type) + L")");
}

void TPictureIO::SetParallax(int x, int y) {
    m_parallaxX = x;
    m_parallaxY = y;
}

bool TPictureIO::LoadSpriteFromCache() {
    wxString name = GetSpriteName();
    TSpriteHandle* handle = graphics->GetSpriteFromCache(name);
    if (!handle)
        return false;
    handle->AddRef();
    m_spriteHandle = handle;
    m_flag90 = false;
    SetImageSize(handle->width, handle->height);
    return true;
}

bool TPictureIO::GetFormat(TFile& file, TPictureFormat** outFormat, const wxString& formatHint) const {
    *outFormat = nullptr;

    unsigned char magic[4] = {0, 0, 0, 0};
    // Real signature: TFile::Seek/GetFileLength/ReadByte x4 - approximated
    // via the stub TFile surface (no real Seek/GetFileLength yet).
    (void)file;

    if (magic[0] == 'R' && magic[1] == 'I' && magic[2] == 'F' && magic[3] == 'F') {
        *outFormat = new TPictureWebP();
        return true;
    }
    if (magic[1] == 'P' && magic[2] == 'N' && magic[3] == 'G') {
        *outFormat = new TPicturePNG();
        return true;
    }

    wxString hint = formatHint;
    if (hint.ToStdWstring() == L"P")
        *outFormat = new TPicturePNG();
    else if (hint.ToStdWstring() == L"W")
        *outFormat = new TPictureWebP();
    else if (hint.ToStdWstring() == L"J")
        *outFormat = new TPictureJPG();
    else if (hint.ToStdWstring() == L"G")
        *outFormat = new TPictureGIF();
    // A final check against "P" here in the original can only be reached
    // after the first "P" check (above) already failed, so a PCX result
    // from it appears unreachable - see the class comment.

    return *outFormat != nullptr;
}

bool TPictureIO::LoadHeader(TFile& /*file*/, TPictureFormat** outFormat) {
    *outFormat = nullptr;
    return false;
}

bool TPictureIO::ReadPictureFile() {
    return false;
}

void TPictureIO::EnsureSizeValid() {
    if (m_width < 0)
        m_width = 0;
    if (m_height < 0)
        m_height = 0;
}

bool TPictureIO::LoadRect(TSprite& /*sprite*/) {
    return false;
}

bool TPictureIO::CreateFromFramebuffer(TFramebuffer* /*framebuffer*/) {
    return false;
}

bool TPictureIO::CreateSprite(bool /*preload*/) {
    return false;
}

bool TPictureIO::CreateSprite(void* /*pixels*/, int /*width*/, int /*height*/, int /*bpp*/) {
    return false;
}

bool TPictureIO::CreateEmptySprite(int width, int height, int /*bpp*/, bool /*preload*/) {
    m_width = width;
    m_height = height;
    return true;
}

bool TPictureIO::CreateSpriteTexture(const wxFileName& /*file*/) {
    return false;
}

bool TPictureIO::FinishPreloaderRead(int /*result*/) {
    return false;
}

void TPictureIO::CleanupPreloader() {
    delete m_preloader;
    m_preloader = nullptr;
}

void TPictureIO::PreparePreloaderLoad(wxFileName /*file*/) {
}

bool TPictureIO::LoadPicture(wxFileName /*file*/, eLoadSetting /*loadSetting*/) {
    return false;
}

bool TPictureIO::RefreshSprite(bool /*force*/) {
    return false;
}

bool TPictureIO::IsTransparent(const wxPoint& /*point*/) const {
    return false;
}

bool TPictureIO::SavePicture(const wxFileName& /*file*/, bool /*overwrite*/) const {
    return false;
}

bool TPictureIO::WritePicture(TPictureFormat& /*format*/, const wxFileName& /*file*/) const {
    return false;
}

wxRect TPictureIO::GetDestRect() const {
    return m_destRect;
}

void TPictureIO::PreparePaint(wxRect& destRect, FloatRect& srcRect) {
    destRect = m_destRect;
    srcRect = FloatRect{0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height)};
}

unsigned long TPictureIO::GetSpriteMemSize() const {
    return static_cast<unsigned long>(m_width) * static_cast<unsigned long>(m_height) * 4;
}

TSpriteHandle* TPictureIO::GetSpriteHandle() const {
    return m_spriteHandle;
}

void TPictureIO::SetPreloadingStatus(ePreloadingStatus status) {
    wxCriticalSectionLocker lock(m_mutexStatus);
    m_preloadingStatus = status;
}

TPictureIO::ePreloadingStatus TPictureIO::GetPreloadingStatus() {
    wxCriticalSectionLocker lock(m_mutexStatus);
    return m_preloadingStatus;
}

void TPictureIO::SetPreloader(TPicturePreloader* preloader) {
    m_preloader = preloader;
}

TPicturePreloader* TPictureIO::GetPreloader() {
    return m_preloader;
}

void TPictureIO::DrawWithDestRect(const wxRect& /*destRect*/, float /*alpha*/, unsigned int /*color*/) {
}

void TPictureIO::DrawWithSrcRect(const wxRect& /*srcRect*/, float /*alpha*/, unsigned int /*color*/) {
}

void TPictureIO::DrawWithLightMap(float /*alpha*/, unsigned int /*color*/, void* /*lightMap*/) {
}

void TPictureIO::Draw(float /*alpha*/, unsigned int /*color*/) {
}
