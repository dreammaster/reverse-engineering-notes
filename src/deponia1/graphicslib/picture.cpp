#include "graphicslib/picture.h"

#include <algorithm>

#include "baselib/file.h"
#include "Diagnostics.h"
#include "TPaintControl.h"
#include "graphicslib/graphics.h"
#include "graphicslib/preloadedPicManager.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/graphicslib/picture.cpp";

TPaintControl *TPictureIO::s_pPaintControl = nullptr;
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

// Confirmed (asm lines 785997-786253): "%s_%d_%d_%d_%d" with the full path, 0, the transparency mode,
// the transparent color (only in the color key mode, else 0) and _flagC0; with s_bTestCacheFileTime
// "%s_%d_%d_%d_%d_%d", where the last is the modification time of the file in seconds.
wxString TPictureIO::GetSpriteName() const {
	int color = (GetTransparency() == eTransparencyMode::kColorKey) ? static_cast<int>(GetTransparentColor()) : 0;
	std::wstring name = GetPath().GetFullPath().ToStdWstring() + L"_0_" + std::to_wstring(static_cast<int>(GetTransparency())) +
	                    L"_" + std::to_wstring(color) + L"_" + std::to_wstring(_flagC0 ? 1 : 0);

	if (s_bTestCacheFileTime)
		name += L"_" + std::to_wstring(static_cast<int>(TFile::GetFileTime(GetPath()) / 1000));

	return wxString(name);
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

// Confirmed (asm lines 788655-788735): looks at the bit for the 4 x 4 pixels the point is in, in the transparency bitmap of the
// sprite (a sprite without one is not transparent anywhere); a mirrored sprite is looked at from the other side.
bool TPictureIO::IsTransparent(const wxPoint &point) const {
	if (!_spriteHandle || !_spriteHandle->transparencyBitmap)
		return false;

	int x = point.x;
	int y = point.y;

	if (IsMirrored())
		x = _spriteHandle->width - x;

	x >>= 2;
	y >>= 2;

	unsigned char byte = _spriteHandle->transparencyBitmap[(static_cast<unsigned>(x) >> 3) + _spriteHandle->bitmapRowBytes * y];

	return (byte & (1 << (x & 7))) != 0;
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

void TPictureIO::PreparePaint(wxRect &sourceRect, FloatRect &destRect) {
	x_assert(s_pPaintControl != nullptr, "s_pPaintControl != NULL", kSourceFile, 0x392);

	const wxPoint &origin = s_pPaintControl->GetOrigin();
	const FloatPoint &scroll = s_pPaintControl->GetFloatScrollPos();
	wxPoint position = GetPosition();

	// where it is on the screen: the parallax moves it by that part (percent) of the scroll position
	float x = static_cast<float>(origin.x + position.x) - static_cast<float>(_parallaxX) * scroll.x / 100.0f;
	float y = static_cast<float>(origin.y + position.y) - static_cast<float>(_parallaxY) * scroll.y / 100.0f;
	float width = GetSizedWidth();
	float height = GetSizedHeight();

	_destRect.x = static_cast<int>(x);
	_destRect.y = static_cast<int>(y);
	_destRect.width = static_cast<int>(width);
	_destRect.height = static_cast<int>(height);

	sourceRect.x = 0;
	sourceRect.y = 0;
	sourceRect.width = GetWidth();
	sourceRect.height = GetHeight();

	// (the original moves the rectangle by the part of the position behind the point and takes it off at the right and
	// bottom, with a difference of two equal numbers each time: nothing changes)

	if (IsMirrored())
		sourceRect.x = GetWidth() - sourceRect.width - sourceRect.x;

	destRect.x = x - scroll.x;
	destRect.y = y - scroll.y;
	destRect.width = width;
	destRect.height = height;
}

// Confirmed (asm lines 789051-789078)
unsigned long TPictureIO::GetSpriteMemSize() const {
	if (!_spriteHandle)
		return 0;

	return static_cast<unsigned long>(graphics->GetSpriteMemSize(_spriteHandle));
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

// Confirmed (asm lines 790781-790882). Draws the picture in `destRect` (screen pixels), whole, not turned or scaled; through
// the matrices (matrix 1).
void TPictureIO::DrawWithDestRect(const wxRect &destRect, float alpha, unsigned int color) {
	if (!RefreshSprite(false))
		return;

	wxRect source{0, 0, GetWidth(), GetHeight()};
	FloatRect dest{static_cast<float>(destRect.x), static_cast<float>(destRect.y), static_cast<float>(destRect.width),
	               static_cast<float>(destRect.height)};
	wxPoint center{-1, -1};

	graphics->Draw(_spriteHandle, source, dest, alpha, IsMirrored(), color, -1, 0.0f, center, 1.0f, 1.0f, 1);
}

// Confirmed (asm lines 790882-790993). Draws the part `srcRect` of the picture where the picture is (at its size, as
// PreparePaint() has it for a picture of that size), not turned. NOTE: the original passes the scale (1, 0), which only
// does no harm because this is used in a batch (text), where scale and rotation are not looked at.
void TPictureIO::DrawWithSrcRect(const wxRect &srcRect, float alpha, unsigned int color) {
	if (!RefreshSprite(false))
		return;

	int savedWidth = GetWidth();
	int savedHeight = GetHeight();
	wxRect source = srcRect;
	FloatRect dest;
	wxPoint center{-1, -1};

	SetImageSize(srcRect.width, srcRect.height);
	PreparePaint(source, dest);
	SetImageSize(savedWidth, savedHeight);
	source.x += srcRect.x;
	source.y += srcRect.y;

	graphics->Draw(_spriteHandle, source, dest, alpha, IsMirrored(), color, -1, 0.0f, center, 1.0f, 0.0f, 1);
}

// Confirmed (asm lines 790993-791077). (The colour is not used.)
void TPictureIO::DrawWithLightMap(float alpha, unsigned int /*color*/, void *lightMap) {
	if (!RefreshSprite(false))
		return;

	wxRect source;
	FloatRect dest;

	PreparePaint(source, dest);
	graphics->DrawWithLightMap(_spriteHandle, source, dest, alpha, IsMirrored(), lightMap, wxPoint{0, 0});
}

// Confirmed (asm lines 791077-791178). Draws the picture with what it has been given: shader, rotation (and its centre),
// scale and matrix.
void TPictureIO::Draw(float alpha, unsigned int color) {
	if (!RefreshSprite(false))
		return;

	wxRect source;
	FloatRect dest;

	PreparePaint(source, dest);
	graphics->Draw(_spriteHandle, source, dest, alpha, IsMirrored(), color, _shader, _rotation, _rotationCenter, _scaleX,
	               _scaleY, _matrixId);
}
