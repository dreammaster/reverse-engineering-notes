#include "graphicslib/graphics.h"

#include <algorithm>
#include <cmath>

#include "AppGlobals.h"
#include "Diagnostics.h"
#include "Easing.h"
#include "TSignalData.h"
#include "TSignalSlot.h"
#include "TSpriteHandle.h"
#include "TTimer.h"
#include "Tween.h"
#include "SdlStub.h"
#include "TPictureMEM.h"
#include "graphicslib/picture.h"
#include "graphicslib/preloadedPicManager.h"

TGraphicsInterface *graphics = new TGraphicsInterface();

int defaultShader = 0;

TGraphicsInterface::TGraphicsInterface() {
	_matrix.Identity();
}

TGraphicsInterface::~TGraphicsInterface() {
	delete _snapshot;
}

void TGraphicsInterface::MultiplyByMatrix1() {
	if (!matricesActive || matrix1.size() != 9)
		return;

	const std::vector<float> &m = matrix1;
	TMatrix4 other;

	other._m[0] = m[0];
	other._m[1] = m[1];
	other._m[2] = 0.0f;
	other._m[3] = m[2];
	other._m[4] = m[3];
	other._m[5] = m[4];
	other._m[6] = 0.0f;
	other._m[7] = m[5];
	other._m[8] = 0.0f;
	other._m[9] = 0.0f;
	other._m[10] = 1.0f;
	other._m[11] = 0.0f;
	other._m[12] = m[6];
	other._m[13] = m[7];
	other._m[14] = 0.0f;
	other._m[15] = m[8];

	_matrix = other * _matrix;
}

void TGraphicsInterface::SetOffset(float /*x*/, float /*y*/) {
}

void TGraphicsInterface::Draw(TParticleSystem &/*system*/, bool /*first*/, bool /*second*/) {
}

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

TPictureMEM *TGraphicsInterface::GetSnapshotForSavegame() {
	return _snapshotMade ? _snapshot : nullptr;
}

TPictureMemBlock *TGraphicsInterface::GetLightMapMemBlock() {
	return nullptr;
}

TPreloadedPicManager *TGraphicsInterface::GetPreloadedPicManager() {
	if (!_preloadedPicManager)
		_preloadedPicManager = new TPreloadedPicManager();
	return _preloadedPicManager;
}

bool TGraphicsInterface::ReadScreen(std::vector<unsigned char> &/*rgb*/, int &/*width*/, int &/*height*/) {
	return false;
}

// The pixels are given to a picture (a buffer of its own: the original's go to its main memory block): 3 bytes to a pixel, red first.
static bool fillPicture(TPictureMEM &picture, const std::vector<unsigned char> &rgb, int width, int height) {
	if (width <= 0 || height <= 0 || rgb.size() < static_cast<size_t>(width) * height * 3)
		return false;

	picture.ClearMemData();

	char *copy = new char[rgb.size()];

	std::copy(rgb.begin(), rgb.end(), copy);
	picture.SetMemoryData(copy, width, height, false, true, true);
	return true;
}

// Slot 0xC8 (CmdCreateScreenshot::Redo): the picture of the game as it is shown.
bool TGraphicsInterface::CaptureScreen(TPictureIO &picture) {
	std::vector<unsigned char> rgb;
	int width = 0;
	int height = 0;

	if (!ReadScreen(rgb, width, height))
		return false;

	return fillPicture(picture, rgb, width, height);
}

// Confirmed (asm 710421-710610): a kept snapshot is not replaced by one that is not to be kept; else the picture of the game is read and
// put in the picture of the snapshot, which is kept when asked.
void TGraphicsInterface::MakeSnapshotForSavegame(bool keep) {
	if (_snapshotKept && !keep)
		return;

	std::vector<unsigned char> rgb;
	int width = 0;
	int height = 0;

	if (ReadScreen(rgb, width, height)) {
		if (!_snapshot)
			_snapshot = new TPictureMEM();

		_snapshotMade = fillPicture(*_snapshot, rgb, width, height);
	}

	if (keep)
		_snapshotKept = true;
}

// Confirmed (asm 708254-708300): the picture is let go, unless it is kept and `keep` is false; `keep` ends the keeping.
void TGraphicsInterface::ClearSnapshotForSavegame(bool keep) {
	if (_snapshotKept && !keep)
		return;

	if (_snapshot)
		_snapshot->ClearMemData();

	_snapshotMade = false;

	if (keep)
		_snapshotKept = false;
}

void TGraphicsInterface::CalculateDisplayedArea(const wxSize &windowSize, const wxSize &/*renderSize*/,
        wxRect *displayedArea) {
	// TODO: the GL backend (not reconstructed) keeps the aspect ratio; here the whole window is used.
	displayedArea->x = 0;
	displayedArea->y = 0;
	displayedArea->width = windowSize.width;
	displayedArea->height = windowSize.height;
}

void TGraphicsInterface::DrawBox(const wxRect &/*rect*/, unsigned int /*color*/, float /*alpha*/) {
}

void TGraphicsInterface::DrawLine(const wxPoint &/*from*/, const wxPoint &/*to*/, unsigned int /*color*/, float /*alpha*/) {
}

bool TGraphicsInterface::InitGraphics(const wxSize &windowSize, wxSize &renderSize, wxRect *displayedArea) {
	CalculateDisplayedArea(windowSize, renderSize, displayedArea);
	return true;
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

void TGraphicsInterface::BeforeDrawScene(bool /*a*/, int /*b*/) {
}

void TGraphicsInterface::AfterDrawScene(bool /*a*/, bool /*b*/) {
}

void TGraphicsInterface::Swap() {
}

void TGraphicsInterface::SetDirectToScreen() {
}

// Confirmed (asm 763382-764646). The frames of the transitions are made in this loop and nowhere else: while it runs, the game
// is updated (kSignalGameEvent) and drawn (kSignalDraw) through `game` - every turn the backend is asked to begin a frame
// (BeforeDrawScene), what it draws for the effect is up to it, and a frame that it did not draw for the effect is shown with
// AfterDrawScene(true, true). g_loadingScreenLock is held for the time of a frame (all of the effects 4 and 11).
// (The original also takes the events of SDL out of the queue every turn of 4, 5, 6 and 11.)
void TGraphicsInterface::FadeIn(int effect, int time, TSpriteHandle *first, TSpriteHandle *second, TSignalSlot *sound,
                                TSignalSlot *game) {
	TTimer timer;
	TSignalData signal;
	TSignalData result;
	const float duration = (float)time;
	const wxRect screen{0, 0, _surfaceWidth, _surfaceHeight};

	signal.type = kSignalSoundFade;
	signal.value = 0;

	// the game goes on and is drawn
	auto step = [&]() {
		TSignalData update;
		TSignalData draw;

		update.type = kSignalGameEvent;
		game->Signal(update, result);
		draw.type = kSignalDraw;
		game->Signal(draw, result);
	};
	// the frame is shown (the backend says whether it drew something for the effect)
	auto show = [&]() {
		AfterDrawScene(true, !_fadeDrawn);
	};
	// the sounds go on
	auto sounds = [&]() {
		if (sound) {
			signal.type = kSignalSoundFade;
			sound->Signal(signal, result);
		}
	};

	switch (effect) {
	case 1:
	case 2:
		if (effect == 1) {
			// the scene is drawn once, and covered
			g_loadingScreenLock.Enter();
			BeforeDrawScene(true, 0);
			AfterDrawScene(true, true);
			BeforeDrawScene(true, 0);
			step();
			FinishDraw();
			DrawBox(screen, 0, 1.0f);
			show();
			g_loadingScreenLock.Leave();
		}

		timer.SetTime();

		do {
			float alpha = (float)timer.GetTime() / duration;

			alpha = (effect == 1) ? std::max(0.0f, 1.0f - alpha) : std::min(1.0f, alpha);

			g_loadingScreenLock.Enter();
			BeforeDrawScene(true, 0);
			step();
			FinishDraw();
			DrawBox(screen, 0, alpha);
			show();
			lastFrameEnd = SDL_GetTicks();
			g_loadingScreenLock.Leave();
		} while (timer.GetTime() < time);

		if (effect == 2) {
			g_loadingScreenLock.Enter();
			BeforeDrawScene(true, 0);
			DrawBox(screen, 0, 1.0f);
			AfterDrawScene(true, true);
			g_loadingScreenLock.Leave();
		}
		break;
	case 4:
	case 11:
		g_loadingScreenLock.Enter();
		timer.SetTime();

		do {
			float progress = (float)timer.GetTime() / duration;

			if (progress < 0.0f)
				progress = 0.0f;

			_fadeProgress = progress;
			_fadeAux = 0;
			_fadeEffect = effect;
			BeforeDrawScene(true, 0);
			step();
			sounds();
			show();
			lastFrameEnd = SDL_GetTicks();
		} while (timer.GetTime() < time);

		g_loadingScreenLock.Leave();
		break;
	case 5:
	case 6: {
		// the same, the progress eased in and out
		Tween tween(0.0, 1.0, (double)duration, Easing::QuadInOut, false, false);
		double last = 0.0;

		timer.SetTime();

		do {
			g_loadingScreenLock.Enter();
			BeforeDrawScene(true, 0);
			tween.Update((float)((double)timer.GetTime() - last));
			_fadeAux = 0;
			_fadeEffect = effect;
			_fadeProgress = (float)tween.GetValue();
			last = (double)timer.GetTime();

			TSignalData update;
			TSignalData draw;
			TSignalData interfaces;

			update.type = kSignalGameEvent;
			game->Signal(update, result);
			draw.type = kSignalDraw;
			game->Signal(draw, result);
			interfaces.type = kSignalDrawInterfaces;
			game->Signal(interfaces, result);
			sounds();
			show();
			lastFrameEnd = SDL_GetTicks();
			g_loadingScreenLock.Leave();
		} while (timer.GetTime() < time);
		break;
	}
	case 8:
	case 9: {
		// an iris: a black picture with a round hole in the middle (made here) is drawn bigger or smaller over the scene
		const int maskSize = 256;
		std::vector<char> mask((size_t)maskSize * maskSize * 4);
		size_t at = 0;

		for (int y = 127; y >= -128; y--) {
			for (int x = 127; x >= -128; x--) {
				const bool outside = (x * x + y * y > 0x3F00);

				mask[at++] = 0;
				mask[at++] = 0;
				mask[at++] = 0;
				mask[at++] = outside ? (char)0xFF : 0;
			}
		}

		TSpriteHandle *sprite = nullptr;

		g_loadingScreenLock.Enter();

		if (CreateSprite(&sprite, mask.data(), maskSize, maskSize, 4, maskSize * 4, true)) {
			g_loadingScreenLock.Leave();
			timer.SetTime();

			const int centreX = _surfaceWidth / 2;
			const int centreY = _surfaceHeight / 2;
			const float biggest = (float)_surfaceWidth * 1.2f;
			const int biggestHalf = (int)biggest / 2;
			const wxRect source{0, 0, maskSize, maskSize};

			do {
				g_loadingScreenLock.Enter();

				int size;

				if (effect == 8)
					size = (int)(biggest - (float)timer.GetTime() / duration * biggest);
				else
					size = (int)((float)timer.GetTime() / duration * biggest);

				int half = 0;

				if (size >= 0) {
					half = size >> 1;

					if ((float)size > biggest)
						half = biggestHalf;
				}

				wxRect iris;

				iris.x = centreX - half;
				iris.y = centreY - half;
				iris.width = half * 2;
				iris.height = half * 2;

				BeforeDrawScene(true, 0);
				step();

				FloatRect target;
				const unsigned int white = 0xFFFFFFFF;
				const wxPoint noCentre{-1, -1};

				target.x = (float)iris.x;
				target.y = (float)iris.y;
				target.width = (float)iris.width;
				target.height = (float)iris.height;
				Draw(sprite, source, target, 1.0f, false, white, -1, 0.0f, noCentre, 1.0f, 1.0f, 1);

				// the rest of the screen is black
				if (iris.GetTop() > 0)
					DrawBox(wxRect{0, 0, _surfaceWidth, iris.GetTop()}, 0, 1.0f);

				if (iris.GetBottom() < _surfaceHeight)
					DrawBox(wxRect{0, iris.GetBottom(), _surfaceWidth, _surfaceHeight - iris.GetBottom()}, 0, 1.0f);

				if (iris.GetLeft() > 0)
					DrawBox(wxRect{0, 0, iris.GetLeft(), _surfaceHeight}, 0, 1.0f);

				if (iris.GetRight() < _surfaceWidth)
					DrawBox(wxRect{iris.GetRight(), 0, _surfaceWidth - iris.GetRight(), _surfaceHeight}, 0, 1.0f);

				show();
				lastFrameEnd = SDL_GetTicks();
				g_loadingScreenLock.Leave();
			} while (timer.GetTime() < time);

			if (effect == 8) {
				g_loadingScreenLock.Enter();
				BeforeDrawScene(true, 0);
				AfterDrawScene(true, true);
				g_loadingScreenLock.Leave();
			}

			g_loadingScreenLock.Enter();
			OnSpriteHandleReleased(sprite);
			g_loadingScreenLock.Leave();
		} else {
			g_loadingScreenLock.Leave();
		}
		break;
	}
	case 3:
		x_assert(false, "false", "/home/simon/Documents/jenkins/branchPillars/src/graphicslib/graphics.cpp", 0x208);
		break;
	case 7:
		x_assert(false, "false", "/home/simon/Documents/jenkins/branchPillars/src/graphicslib/graphics.cpp", 0x2A9);
		break;
	default:
		break;
	}

	if (sound) {
		signal.type = kSignalSoundFade;
		signal.value = 1;
		sound->Signal(signal, result);
	}

	if (first)
		OnSpriteHandleReleased(first);

	if (second)
		OnSpriteHandleReleased(second);
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
