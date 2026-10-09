#include "graphicslib/picture.h"

#include <algorithm>

#include "baselib/file.h"
#include "Diagnostics.h"
#include "TPaintControl.h"
#include "TComposedFileManager.h"
#include "TFramebuffer.h"
#include "TTimer.h"
#include "graphicslib/graphics.h"
#include "graphicslib/preloadedPicManager.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/graphicslib/picture.cpp";

TPaintControl *TPictureIO::s_pPaintControl = nullptr;
std::vector<TPictureIO *> TPictureIO::_loadFailedPics;
wxCriticalSection TPictureIO::RetryFailedPicturesSection;
wxCriticalSection TPictureIO::_mutexStatus;
bool TPictureIO::s_bTestCacheFileTime = false;

// Confirmed (asm lines 785867-785961): every picture that was noted gets another try (its failed flag is cleared); one that
// failed while it was preloaded (status 5) is set back to "not preloading" and gets no memory block; then the list is emptied.
void TPictureIO::RetryFailedPicturesLoad() {
	wxCriticalSectionLocker lock(RetryFailedPicturesSection);

	for (size_t i = 0; i < _loadFailedPics.size(); i++) {
		TPictureIO *picture = _loadFailedPics[i];

		picture->_flag90 = false;

		if (graphics && !graphics->GetPreloadedPicManager()->HasQueuedPictures())
			continue;

		int status;

		{
			wxCriticalSectionLocker statusLock(_mutexStatus);
			status = static_cast<int>(picture->_preloadingStatus);
		}

		if (status == 5) {
			{
				wxCriticalSectionLocker statusLock(_mutexStatus);
				picture->_preloadingStatus = ePreloadingStatus::NotPreloading;
			}

			picture->SetMemoryBlock(nullptr);
		}
	}

	_loadFailedPics.clear();
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
		_preloadFile = nullptr;
		return;
	}
	ReleaseSpriteHandle();
	_width = 0;
	_height = 0;
	ClearMemData();
	TSprite::Set(sprite);
	_preloadFile = nullptr;
	_preloadFormat = nullptr;
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

// Confirmed (asm lines 788598-788655): lets go of the sprite (the backend frees it at 0 references); not the pixels.
void TPictureIO::RemoveSprite() {
	graphics->GetPreloadedPicManager()->StopPreloading(this);

	if (_spriteHandle) {
		_spriteHandle->Release();

		if (_spriteHandle->GetRefCount() == 0)
			graphics->OnSpriteHandleReleased(_spriteHandle);

		_spriteHandle = nullptr;

		wxCriticalSectionLocker lock(_mutexStatus);
		_preloadingStatus = ePreloadingStatus::NotPreloading;
	}
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
	DropMemData();
	SetImageSize(handle->width, handle->height);
	return true;
}

// Confirmed (asm lines 786442-786901). The format is told from the first four bytes of the file: "RIFF" is a WebP, and "PNG" in
// the last three bytes is a PNG (whatever the first byte is); else from the first three letters of the extension that `name` has.
bool TPictureIO::GetFormat(TFile &file, TPictureFormat **outFormat, const wxString &name) const {
	wxString extension = name.Mid(0, 3);

	*outFormat = nullptr;
	file.Seek(0, 0);

	if (file.GetFileLength() > 4) {
		char magic[4];

		file.ReadByte(magic[0]);
		file.ReadByte(magic[1]);
		file.ReadByte(magic[2]);
		file.ReadByte(magic[3]);

		bool riff = (magic[0] == 'R');

		if (riff && magic[1] == 'I') {
			if (magic[2] == 'F' && magic[3] == 'F') {
				*outFormat = new TPictureWebP();
				return true;
			}
		} else if (magic[1] == 'P') {
			if (magic[2] == 'N' && magic[3] == 'G') {
				*outFormat = new TPicturePNG();
				return true;
			}
		}
	}

	if (extension.CmpNoCase(wxString(L"PNG")) == 0)
		*outFormat = new TPicturePNG();
	else if (extension.CmpNoCase(wxString(L"WEB")) == 0)
		*outFormat = new TPictureWebP();
	else if (extension.CmpNoCase(wxString(L"JPG")) == 0)
		*outFormat = new TPictureJPG();
	else if (extension.CmpNoCase(wxString(L"GIF")) == 0)
		*outFormat = new TPictureGIF();
	else if (extension.CmpNoCase(wxString(L"PCX")) == 0)
		*outFormat = new TPicturePCX();

	return *outFormat != nullptr;
}

// Confirmed (asm lines 787554-787778). Opens the file of the picture (through the composed files), makes the format and reads
// the size. The picture is then a loaded image; a transparency of "any" becomes alpha.
bool TPictureIO::LoadHeader(TFile &file, TPictureFormat **outFormat) {
	if (!TComposedFileManager::Open(file, GetPath()))
		return false;

	if (!GetFormat(file, outFormat, GetPath().GetExt()))
		return false;

	int width;
	int height;

	if (!(*outFormat)->ReadHeader(file, &width, &height))
		return false;

	SetLoadedImage();

	if (GetTransparency() == eTransparencyMode::kAny)
		SetTransparency(eTransparencyMode::kAlpha, 0);

	SetImageSize(width, height);
	return true;
}

// Confirmed (asm lines 787778-788232). Reads the header, makes the memory (RGBA) and reads the pixels.
bool TPictureIO::ReadPictureFile() {
	TFile file;
	TPictureFormat *format = nullptr;
	bool result = false;

	if (GetPath().IsOk() && wxLog::loglevel > 2)
		wxLog::logexpanded(L"loading image '%s'", GetPath().GetFullPath().wc_str());

	if (!LoadHeader(file, &format)) {
		if (wxLog::loglevel > 0)
			wxLog::logexpanded(L"Error reading header from image %s", GetPath().GetFullName().wc_str());
	} else {
		x_assert(format != nullptr, "format != NULL", kSourceFile, 0x28E);

		if (InitMemory(true)) {
			if (format->ReadData(file, *this)) {
				result = true;
			} else if (wxLog::loglevel > 0) {
				wxLog::logexpanded(L"Error reading data from image %s", GetPath().GetFullName().wc_str());
			}
		}
	}

	delete format;
	return result;
}

// Confirmed (asm lines 788232-788452). A picture whose size is not known yet (and that has a path, and has not failed) has its
// header read; it has failed if the size is still not there. Always true.
bool TPictureIO::EnsureSizeValid() {
	if (GetWidth() != 0 || GetPathNonConst().size() == 0 || _flag90)
		return true;

	TTimer timer;
	TFile file;
	TPictureFormat *format = nullptr;

	LoadHeader(file, &format);
	delete format;

	if (GetWidth() == 0)
		_flag90 = true;

	if (wxLog::loglevel > 2)
		wxLog::logexpanded(L"size valid '%s', %dms", GetPath().GetFullPath().wc_str(), static_cast<int>(timer.GetTime()));

	return true;
}

// Confirmed (asm lines 788452-788598). The size of the picture at the path of `sprite` goes to `sprite` (the picture takes the
// path of `sprite` first).
bool TPictureIO::LoadRect(TSprite &sprite) {
	TFile file;
	TPictureFormat *format = nullptr;

	SetPath(TCharHolder(sprite.GetPath()));

	bool result = LoadHeader(file, &format);

	if (result) {
		// (the original sets the transparency of the picture to its own here, not of `sprite`: nothing changes)
		if (sprite.GetTransparency() == eTransparencyMode::kAny)
			SetTransparency(GetTransparency(), 0);

		sprite.SetImageSize(GetWidth(), GetHeight());
	}

	file.Close();
	delete format;
	return result;
}

// Confirmed (asm lines 786901-786983): the picture is the frame buffer (a sprite of one part that is the whole of it).
bool TPictureIO::CreateFromFramebuffer(TFramebuffer *framebuffer) {
	TSpriteHandle *handle = new TSpriteHandle();
	TSpritePartHandle *part = new TSpritePartHandle();

	part->left = 0;
	part->top = 0;
	part->right = framebuffer->width;
	part->bottom = framebuffer->height;
	handle->parts.push_back(part);
	_spriteHandle = handle;
	TakePixels(nullptr, framebuffer->width, framebuffer->height, 4);
	SetImageSize(framebuffer->width, framebuffer->height);
	return true;
}

// Confirmed (asm lines 786983-787250). The pixels in memory become a sprite of the backend: if `preload` is not set the cache
// is looked in first (the pixels are not needed then); a new sprite gets the transparency bitmap (if `_ownsSprite` is set and the
// transparency is a colour key or alpha) and is put in the cache. Afterwards, if `preload` is not set, the pixels are let go of
// (the memory block of the picture is let go of in any case). Then, if the pictures are being preloaded, the picture is told
// to the manager. False if there are no pixels or the backend cannot make the sprite.
bool TPictureIO::CreateSprite(bool preload) {
	if (!GetMemoryData())
		return false;

	bool result = false;
	bool found = false;

	if (!preload) {
		TSpriteHandle *cached = graphics->GetSpriteFromCache(GetSpriteName());

		if (cached) {
			cached->AddRef();
			_spriteHandle = cached;
			_flag90 = false;
			DropMemData();
			SetImageSize(cached->width, cached->height);
			result = true;
			found = true;
		}
	}

	if (!found) {
		result = graphics->CreateSprite(&_spriteHandle, GetMemoryData(), _width, _height, GetBytesPerPixel(), GetPitch(),
		                                _flagC0);

		if (result) {
			x_assert(_spriteHandle != nullptr, "m_pSprite != NULL", kSourceFile, 0x1CC);

			eTransparencyMode mode = GetTransparency();

			if (_ownsSprite && (mode == eTransparencyMode::kColorKey || mode == eTransparencyMode::kAlpha) && _spriteHandle) {
				unsigned char *bitmap = nullptr;
				int rowBytes = 0;

				if (CreateTransparencyBitmap(GetTransparentColor(), GetTransparency(), &bitmap, rowBytes)) {
					_spriteHandle->transparencyBitmap = bitmap;
					_spriteHandle->bitmapRowBytes = rowBytes;
				}
			}

			graphics->AddToCache(GetSpriteName(), _spriteHandle);
		}

		if (!preload) {
			ReleasePixels();
		}
	} else {
		SetMemoryBlock(nullptr);
	}

	if (!graphics->GetPreloadedPicManager()->HasQueuedPictures())
		return result;

	ePreloadingStatus status = GetPreloadingStatus();

	if (status != ePreloadingStatus::NotPreloading)
		graphics->GetPreloadedPicManager()->ReleasePicture(this);

	return result;
}

// Confirmed (asm lines 787250-787284): the picture is these pixels, made a sprite at once; the pixels are the caller's.
bool TPictureIO::CreateSprite(void *pixels, int width, int height, int bpp) {
	TakePixels(static_cast<char *>(pixels), width, height, bpp);
	SetImageSize(width, height);
	CreateSprite(true);
	DropMemData();
	return true;
}

// Confirmed (asm lines 789210-789308): an empty sprite of that size, made by the backend. (The original returns -1 whatever
// happens; nothing calls it.)
bool TPictureIO::CreateEmptySprite(int width, int height, int bpp, bool /*preload*/) {
	graphics->CreateSprite(&_spriteHandle, nullptr, width, height, bpp, bpp * width, true);
	x_assert(_spriteHandle != nullptr, "m_pSprite != NULL", kSourceFile, 0x453);
	SetImageSize(width, height);

	TSpriteHandle *handle = _spriteHandle;
	eTransparencyMode mode = GetTransparency();

	if (_ownsSprite && (mode == eTransparencyMode::kColorKey || mode == eTransparencyMode::kAlpha) && handle) {
		unsigned char *bitmap = nullptr;
		int rowBytes = 0;

		if (CreateTransparencyBitmap(GetTransparentColor(), GetTransparency(), &bitmap, rowBytes)) {
			handle->transparencyBitmap = bitmap;
			handle->bitmapRowBytes = rowBytes;
		}
	}

	return true;
}

// Confirmed (asm lines 791178-791259): marks the picture (the transparency bitmap is not made for it... `_flagC0`) and loads it.
bool TPictureIO::CreateSpriteTexture(const wxFileName &file) {
	_flagC0 = true;

	wxFileName path = file;

	path.NormalizePath();
	return LoadPicture(path, eLoadSetting::Normal);
}

// Confirmed (asm lines 787284-787512). The data of a picture whose header PreparePreloaderLoad() read: made into memory and
// read from the file; the file and the format are let go of in any case.
bool TPictureIO::FinishPreloaderRead(int result) {
	TTimer unused;
	bool ok = false;

	(void)unused;
	(void)result;

	if (_preloadFormat && InitMemory(true)) {
		if (_preloadFormat->ReadData(*_preloadFile, *this)) {
			ok = true;
		} else if (wxLog::loglevel > 0) {
			wxLog::logexpanded(L"Error reading data from image %s", GetPath().GetFullName().wc_str());
		}
	}

	CleanupPreloader();
	return ok;
}

// Confirmed (asm lines 787512-787554)
void TPictureIO::CleanupPreloader() {
	delete _preloadFormat;
	_preloadFormat = nullptr;
	delete _preloadFile;
	_preloadFile = nullptr;
}

// Confirmed (asm lines 789308-789745). The size of the picture (height << 32 | width) once its header is read, -1 if it
// is not readable (the picture is marked as failed and noted for RetryFailedPicturesLoad()).
unsigned long long TPictureIO::PreparePreloaderLoad(wxFileName file) {
	SetPath(TCharHolder(file.GetFullPath()));

	if (_flag90 || !GetPathNonConst().IsOk())
		return ~0ULL;

	if (!_preloadFile) {
		_preloadFile = new TFile();
		x_assert(_preloadFile != nullptr, "m_preloadFile != NULL", kSourceFile, 0x21E);

		if (GetPath().IsOk() && wxLog::loglevel > 2)
			wxLog::logexpanded(L"loading image '%s'", GetPath().GetFullPath().wc_str());

		if (!LoadHeader(*_preloadFile, &_preloadFormat)) {
			CleanupPreloader();
			_flag90 = true;
			noteFailedLoad();
			return ~0ULL;
		}

		x_assert(_preloadFormat != nullptr, "m_preloadFormat != NULL", kSourceFile, 0x226);
	}

	return static_cast<unsigned long long>(static_cast<unsigned>(GetWidth())) |
	       (static_cast<unsigned long long>(static_cast<unsigned>(GetHeight())) << 32);
}

/** The picture is put in the list of the ones that RetryFailedPicturesLoad() looks at (once). */
void TPictureIO::noteFailedLoad() {
	wxCriticalSectionLocker lock(RetryFailedPicturesSection);

	if (std::find(_loadFailedPics.begin(), _loadFailedPics.end(), this) == _loadFailedPics.end())
		_loadFailedPics.push_back(this);
}

// Confirmed (asm lines 789745-790163). Loads the picture of the file: the sprite cache is looked in first (Normal), else the file is
// read (and made a sprite unless MemoryOnly). A picture that has failed before fails at once (until RetryFailedPicturesLoad()).
// The picture is always put in the list of the retry (once). The memory block for the pixels is the main one.
bool TPictureIO::LoadPicture(wxFileName file, eLoadSetting loadSetting) {
	TTimer timer;

	timer.SetTime();
	SetPath(TCharHolder(file.GetFullPath()));

	if (_flag90)
		return false;

	if (!GetPathNonConst().IsOk())
		return false;

	if (!GetMemoryBlock())
		SetMemoryBlock(graphics->GetMainMemBlock());

	if (loadSetting == eLoadSetting::Normal) {
		TSpriteHandle *cached = graphics->GetSpriteFromCache(GetSpriteName());

		if (cached) {
			cached->AddRef();
			_spriteHandle = cached;
			_flag90 = false;
			DropMemData();
			SetImageSize(cached->width, cached->height);
			return true;
		}
	}

	bool result = ReadPictureFile();
	long spriteTime = 0;

	if (!result) {
		DropMemData();
	} else if (loadSetting != eLoadSetting::MemoryOnly) {
		timer.GetTime();
		timer.SetTime();
		result = CreateSprite(loadSetting != eLoadSetting::Normal);
		spriteTime = timer.GetTime();
	}

	if (wxLog::loglevel > 2)
		wxLog::logexpanded(L"%dms for %s %dms to read", static_cast<int>(timer.GetTime()), GetPath().GetFullPath().wc_str(),
		                   static_cast<int>(spriteTime));

	_flag90 = !result;
	noteFailedLoad();
	return result;
}

// Confirmed for the case that nothing is being preloaded (asm lines 790163-790781; the manager thread is not reconstructed,
// TPreloadedPicManager keeps no queue, so this is always the case): a picture with no sprite that has not failed and has a path
// is loaded now (`force`: read the file again); true if it has a sprite then. Not reconstructed: when pictures are queued, a
// picture that is being preloaded is waited for (wxMilliSleep(5) in a loop) and made a sprite from the data the thread read.
bool TPictureIO::RefreshSprite(bool force) {
	if (!_spriteHandle && !_flag90 && GetPathNonConst().IsOk()) {
		{
			wxCriticalSectionLocker lock(_mutexStatus);
			_preloadingStatus = ePreloadingStatus::NotPreloading;
		}

		LoadPicture(GetPath(), force ? eLoadSetting::ForceReload : eLoadSetting::Normal);
	}

	return _spriteHandle != nullptr;
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

// Confirmed (asm lines 788735-788822)
bool TPictureIO::SavePicture(const wxFileName &file, bool webp) const {
	TPictureFormat *format = webp ? static_cast<TPictureFormat *>(new TPictureWebP()) : new TPicturePNG();
	bool result = WritePicture(*format, file);

	delete format;
	return result;
}

// Confirmed (asm lines 788822-788854)
bool TPictureIO::WritePicture(TPictureFormat &format, const wxFileName &file) const {
	return format.Write(file, const_cast<TPictureIO &>(*this), _width, _height, GetBytesPerPixel(), IsLoadedImage());
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
