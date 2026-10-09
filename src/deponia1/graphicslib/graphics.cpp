#include "graphicslib/graphics.h"

#include "TSpriteHandle.h"
#include "graphicslib/preloadedPicManager.h"

TGraphicsInterface *graphics = new TGraphicsInterface();

int defaultShader = 0;

void ShaderCallback(int /*shader*/, TVisObjRef */*ref*/) {
}

// Confirmed (asm lines 762206-762223): the calls of the sprite cache, passed on.
bool TGraphicsInterface::RemoveFromCache(const wxString &name) {
	return _spriteCache.RemoveFromCache(name);
}

void TGraphicsInterface::AddToCache(const wxString &name, TSpriteHandle *sprite) {
	_spriteCache.AddToCache(name, sprite);
}

void TGraphicsInterface::PrintCacheContents(std::list<wxString> &lines) {
	_spriteCache.PrintCacheContents(lines);
}

void TGraphicsInterface::ClearCache() {
	_spriteCache.Clear();
}

void TGraphicsInterface::GetPicsMemSettings(bool &rgbOrder, bool &flipped) {
	rgbOrder = true;
	flipped = false;
}

TPictureMemBlock *TGraphicsInterface::GetMainMemBlock() {
	return nullptr;
}

TPictureMEM *TGraphicsInterface::GetCapturedFrame() {
	return nullptr;
}

TPictureMemBlock *TGraphicsInterface::GetLightMapMemBlock() {
	return nullptr;
}

TPreloadedPicManager *TGraphicsInterface::GetPreloadedPicManager() {
	if (!_preloadedPicManager)
		_preloadedPicManager = new TPreloadedPicManager();
	return _preloadedPicManager;
}

bool TGraphicsInterface::CaptureScreen(TPictureIO &/*picture*/) {
	return false;
}

void TGraphicsInterface::CreateSavegameScreenshot(bool /*flag*/) {
}

void TGraphicsInterface::ClearSavegameScreenshot(bool /*flag*/) {
}

void TGraphicsInterface::CalculateDisplayedArea(const wxSize &windowSize, const wxSize &/*renderSize*/,
        wxRect *displayedArea) {
	// TODO: the GL backend (not reconstructed) keeps the aspect ratio; here the whole window is used.
	displayedArea->x = 0;
	displayedArea->y = 0;
	displayedArea->width = windowSize.width;
	displayedArea->height = windowSize.height;
}

void TGraphicsInterface::ToggleWindowMode() {
}

bool TGraphicsInterface::SetWindowSize(int /*width*/, int /*height*/) {
	return true;
}

bool TGraphicsInterface::IsFullscreen() {
	return false;
}

TSpriteHandle *TGraphicsInterface::GetSpriteFromCache(const wxString &name) {
	return _spriteCache.GetSprite(name);
}

// Confirmed (asm lines 776655-776718, TGraphicsInterface::FreeSprite()): the handle is deleted (with its parts; the texture
// of the backend is the backend's to free before).
void TGraphicsInterface::OnSpriteHandleReleased(TSpriteHandle *handle) {
	delete handle;
}

void TGraphicsInterface::FinishDraw() {
}

bool TGraphicsInterface::CreateSprite(TSpriteHandle **sprite, const char */*data*/, int width, int height, int bytesPerPixel,
                                      int /*pitch*/, bool /*flag*/) {
	TSpriteHandle *handle = new TSpriteHandle();

	handle->width = width;
	handle->height = height;
	handle->SetMemorySize(width * height * bytesPerPixel);
	*sprite = handle;
	return true;
}

void TGraphicsInterface::Draw(TSpriteHandle */*sprite*/, const wxRect &/*sourceRect*/, const FloatRect &/*destRect*/,
                              float /*alpha*/, bool /*mirrored*/, const unsigned int &/*color*/, int /*shader*/,
                              float /*rotation*/, const wxPoint &/*rotationCenter*/, float /*scaleX*/, float /*scaleY*/,
                              int /*matrixId*/) {
}

void TGraphicsInterface::DrawWithLightMap(TSpriteHandle */*sprite*/, const wxRect &/*sourceRect*/,
                                          const FloatRect &/*destRect*/, float /*alpha*/, bool /*mirrored*/,
                                          void */*lightMap*/, const wxPoint &/*offset*/) {
}

void TGraphicsInterface::BeginBatch() {
}

void TGraphicsInterface::EndBatch() {
}

int TGraphicsInterface::GetSpriteMemSize(TSpriteHandle *handle) const {
	return handle->GetMemorySize();
}

// Confirmed (asm lines 772807-773173)
void TGraphicsInterface::RemoveTransparentEdges(int &left, int &top, int &width, int &height, int pitch, int originY,
                                                int originX, const char *rgba) {
	auto opaque = [&](int row, int column) {
		return rgba[((originY + row) * pitch + originX + column) * 4 + 3] != 0;
	};

	// (the first pixel of the picture decides if the top and the left are looked at)
	if (rgba[3] == 0) {
		// the top: the first line that has something in it (none: all of them are taken off)
		for (int row = 0; row < height; row++) {
			bool found = false;

			for (int column = 0; column < width && !found; column++)
				found = opaque(row, column);

			if (found) {
				top = row;
				break;
			}

			top = row;

			if (row == height - 1)
				top = height;
		}

		height -= top;

		if (height <= 0)
			return;

		// the left: the first column that has something in it
		if (width > 0) {
			for (int column = 0; column < width; column++) {
				bool found = false;

				for (int row = top; row < top + height && !found; row++)
					found = opaque(row, column);

				if (found) {
					left = column;
					break;
				}

				left = column;

				if (column == width - 1)
					left = width;
			}
		}

		width -= left;
	}

	if (height <= 0 || width <= 0)
		return;

	// the bottom
	int lastRow = height + top - 1;
	int cutBottom = 0;

	if (lastRow > 0) {
		for (;;) {
			bool found = false;

			for (int column = left; column < left + width && !found; column++)
				found = opaque(lastRow - cutBottom, column);

			if (found)
				break;

			// (the original's mistake, see the header)
			if (left == 1)
				break;

			cutBottom++;

			if (cutBottom == lastRow)
				break;
		}
	}

	int oldHeight = height;

	height = oldHeight - cutBottom;

	// the right
	int lastColumn = left + width - 1;
	int cutRight = 0;

	if (lastColumn > 0) {
		for (;;) {
			bool found = false;

			for (int row = top; row < top + height && !found; row++)
				found = opaque(row, lastColumn - cutRight);

			if (found)
				break;

			cutRight++;

			if (cutRight == lastColumn)
				break;
		}
	}

	width -= cutRight;
}

// Confirmed (asm lines 773175-773531): the same for a whole picture of the width `width`.
void TGraphicsInterface::RemoveTransparentEdges(int &left, int &top, int &width, int &height, const char *rgba) {
	RemoveTransparentEdges(left, top, width, height, width, 0, 0, rgba);
}

void TGraphicsInterface::SetMatrixMode(bool /*a*/, bool /*b*/) {
}

void TGraphicsInterface::ResetMatrix(bool /*a*/, bool /*b*/) {
}

void TGraphicsInterface::Flip() {
}

void TGraphicsInterface::SetFilters(TInterpolationEnum /*a*/, TInterpolationEnum /*b*/) {
}

void TGraphicsInterface::PreallocateTextures(int /*count*/) {
}

void TGraphicsInterface::SetCacheSize(int size) {
	_spriteCache.SetCacheSize(size);
}

int TGraphicsInterface::GetCacheSpriteCount() const {
	return static_cast<int>(_spriteCache.GetSpriteCount());
}

bool TGraphicsInterface::UpdateCache() {
	return _spriteCache.UpdateCache();
}
