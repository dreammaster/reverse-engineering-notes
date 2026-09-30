#include "graphicslib/picture.h"

#include <algorithm>

#include "graphicslib/graphics.h"
#include "graphicslib/preloadedPicManager.h"

std::vector<TPictureIO *> TPictureIO::_loadFailedPics;
wxCriticalSection TPictureIO::RetryFailedPicturesSection;
wxCriticalSection TPictureIO::_mutexStatus;
bool TPictureIO::s_bTestCacheFileTime = false;

void TPictureIO::RetryFailedPicturesLoad() {
	// TPictureIO::onRetryLoad's real body (TComposedFile::onRetryLoad in
	// main.cpp calls this indirectly). Not traced beyond confirming it's
	// the target - presumably iterates _loadFailedPics and retries
	// ReadPictureFile() for each.
}

void TPictureIO::TestCacheFileTime(bool enable) {
	s_bTestCacheFileTime = enable;
}

void TPictureIO::ReleaseSpriteHandle() {
	graphics->GetPreloadedPicManager()->StopPreloading(this);
	if (_spriteHandle) {
		_spriteHandle->Release();
		if (_spriteHandle->GetRefCount() == 0) {
			graphics->OnSpriteHandleReleased(_spriteHandle);
		}
		_spriteHandle = nullptr;
		wxCriticalSectionLocker lock(_mutexStatus);
		_preloadingStatus = ePreloadingStatus::NotPreloading;
	}
	_flag90 = false;
	_flagC0 = false;
}

TPictureIO::TPictureIO() {
	_scaleX = 1.0f;
	_scaleY = 1.0f;
}

TPictureIO::TPictureIO(const TSprite &sprite, bool ownsSprite) {
	_ownsSprite = ownsSprite;
	if (static_cast<const TSprite &>(*this) == sprite) {
		_field78 = 0;
		return;
	}
	ReleaseSpriteHandle();
	_width = 0;
	_height = 0;
	ClearMemData();
	TSprite::Set(sprite);
	_field78 = 0;
}

TPictureIO::~TPictureIO() {
	{
		wxCriticalSectionLocker lock(RetryFailedPicturesSection);
		auto it = std::find(_loadFailedPics.begin(), _loadFailedPics.end(), this);
		if (it != _loadFailedPics.end())
			_loadFailedPics.erase(it);
	}
	ReleaseSpriteHandle();
	_width = 0;
	_height = 0;
	ClearMemData();
}

TPictureIO &TPictureIO::operator=(const TPictureIO &other) {
	if (this != &other) {
		Set(other);
	}
	return *this;
}

void TPictureIO::Set(const TSprite &sprite) {
	if (static_cast<const TSprite &>(*this) == sprite)
		return;
	ReleaseSpriteHandle();
	_width = 0;
	_height = 0;
	ClearMemData();
	TSprite::Set(sprite);
}

void TPictureIO::Clear() {
	ReleaseSpriteHandle();
	_width = 0;
	_height = 0;
	ClearMemData();
}

void TPictureIO::RemoveSprite() {
	Clear();
}

wxString TPictureIO::GetSpriteName() const {
	// Builds a diagnostic name from the sprite's path plus GetTransparency(),
	// _flagC0, and a third value that's always 0 unless s_bTestCacheFileTime
	// is enabled, in which case it's the path's file modification time (via
	// TFile::GetFileTime()+wxDateTime::GetTicks()) instead. The exact wx
	// formatting call (wxString::privFormat with a bare "%" format string)
	// couldn't be pinned down at the byte level (see NOTES.md); this
	// reproduces the observable inputs, not the exact original string shape.
	wxString path = GetPath().GetFullPath();
	long thirdValue = 0;  // TFile::GetFileTime()-based value when s_bTestCacheFileTime - not modeled
	return wxString(path.ToStdWstring() + L" (" + std::to_wstring(static_cast<int>(GetTransparency())) + L"," +
	                std::to_wstring(_flagC0) + L"," + std::to_wstring(thirdValue) + L")");
}

void TPictureIO::SetParallax(int x, int y) {
	_parallaxX = x;
	_parallaxY = y;
}

bool TPictureIO::LoadSpriteFromCache() {
	wxString name = GetSpriteName();
	TSpriteHandle *handle = graphics->GetSpriteFromCache(name);
	if (!handle)
		return false;
	handle->AddRef();
	_spriteHandle = handle;
	_flag90 = false;
	SetImageSize(handle->width, handle->height);
	return true;
}

bool TPictureIO::GetFormat(TFile &file, TPictureFormat **outFormat, const wxString &formatHint) const {
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

bool TPictureIO::LoadHeader(TFile &/*file*/, TPictureFormat **outFormat) {
	*outFormat = nullptr;
	return false;
}

bool TPictureIO::ReadPictureFile() {
	return false;
}

void TPictureIO::EnsureSizeValid() {
	if (_width < 0)
		_width = 0;
	if (_height < 0)
		_height = 0;
}

bool TPictureIO::LoadRect(TSprite &/*sprite*/) {
	return false;
}

bool TPictureIO::CreateFromFramebuffer(TFramebuffer */*framebuffer*/) {
	return false;
}

bool TPictureIO::CreateSprite(bool /*preload*/) {
	return false;
}

bool TPictureIO::CreateSprite(void */*pixels*/, int /*width*/, int /*height*/, int /*bpp*/) {
	return false;
}

bool TPictureIO::CreateEmptySprite(int width, int height, int /*bpp*/, bool /*preload*/) {
	_width = width;
	_height = height;
	return true;
}

bool TPictureIO::CreateSpriteTexture(const wxFileName &/*file*/) {
	return false;
}

bool TPictureIO::FinishPreloaderRead(int /*result*/) {
	return false;
}

void TPictureIO::CleanupPreloader() {
	delete _preloader;
	_preloader = nullptr;
}

void TPictureIO::PreparePreloaderLoad(wxFileName /*file*/) {
}

bool TPictureIO::LoadPicture(wxFileName /*file*/, eLoadSetting /*loadSetting*/) {
	return false;
}

bool TPictureIO::RefreshSprite(bool /*force*/) {
	return false;
}

bool TPictureIO::IsTransparent(const wxPoint &/*point*/) const {
	return false;
}

bool TPictureIO::SavePicture(const wxFileName &/*file*/, bool /*overwrite*/) const {
	return false;
}

bool TPictureIO::WritePicture(TPictureFormat &/*format*/, const wxFileName &/*file*/) const {
	return false;
}

wxRect TPictureIO::GetDestRect() const {
	return _destRect;
}

void TPictureIO::PreparePaint(wxRect &destRect, FloatRect &srcRect) {
	destRect = _destRect;
	srcRect = FloatRect{0.0f, 0.0f, static_cast<float>(_width), static_cast<float>(_height)};
}

unsigned long TPictureIO::GetSpriteMemSize() const {
	return static_cast<unsigned long>(_width) * static_cast<unsigned long>(_height) * 4;
}

TSpriteHandle *TPictureIO::GetSpriteHandle() const {
	return _spriteHandle;
}

void TPictureIO::SetPreloadingStatus(ePreloadingStatus status) {
	wxCriticalSectionLocker lock(_mutexStatus);
	_preloadingStatus = status;
}

TPictureIO::ePreloadingStatus TPictureIO::GetPreloadingStatus() {
	wxCriticalSectionLocker lock(_mutexStatus);
	return _preloadingStatus;
}

void TPictureIO::SetPreloader(TPicturePreloader *preloader) {
	_preloader = preloader;
}

TPicturePreloader *TPictureIO::GetPreloader() {
	return _preloader;
}

void TPictureIO::DrawWithDestRect(const wxRect &/*destRect*/, float /*alpha*/, unsigned int /*color*/) {
}

void TPictureIO::DrawWithSrcRect(const wxRect &/*srcRect*/, float /*alpha*/, unsigned int /*color*/) {
}

void TPictureIO::DrawWithLightMap(float /*alpha*/, unsigned int /*color*/, void */*lightMap*/) {
}

void TPictureIO::Draw(float /*alpha*/, unsigned int /*color*/) {
}
