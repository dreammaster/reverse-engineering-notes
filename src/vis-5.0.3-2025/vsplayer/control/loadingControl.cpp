#include "vsplayer/control/loadingControl.h"

#include "vsplayer/control/masterControl.h"

// Confirmed (asm lines 481265-481687): the picture of the screen is loaded and put in the middle of the window, the sound starts, the bar is loaded and
// put in the screen's place for it, and the part of it that is drawn is the whole (the bar that fills starts with none of it).
void TLoadingControl::Init(SLoadingScreen &screen, TSoundInterface *soundManager) {
	_backgroundPic.LoadPicture(screen.background.GetPath(), TPictureIO::eLoadSetting::Normal);

	int offsetX = (screen.windowWidth - _backgroundPic.GetWidth()) / 2;
	int offsetY = (screen.windowHeight - _backgroundPic.GetHeight()) / 2;

	screen.background.SetPosition(wxPoint{offsetX, offsetY}, -1.0f);

	if (soundManager) {
		_soundPath = wxFileName(screen.music);

		if (_soundPath.IsOk())
			soundManager->Play(_soundPath, TVisObjRef(), 100, 0, true, TSoundTypeEnum::kMusic, 0, 0);
	}

	_progressFillsForward = screen.fillForward;
	_progressBarPic.LoadPicture(screen.status.GetPath(), TPictureIO::eLoadSetting::Normal);

	_totalRect.x = screen.statusPosition.x;
	_totalRect.y = screen.statusPosition.y;
	_totalRect.width = _progressBarPic.GetWidth();
	_totalRect.height = _progressBarPic.GetHeight();
	_fillOrigin = wxPoint{offsetX + _totalRect.GetLeft(), offsetY + _totalRect.GetTop()};
	_progressBarPic.SetPosition(_fillOrigin, -1.0f);
	_fillRect.SetLeft(0);
	_fillRect.SetTop(0);
	_fillRect.SetWidth(_progressFillsForward ? 0 : _progressBarPic.GetWidth());
	_fillRect.SetHeight(_progressBarPic.GetHeight());
}

void TLoadingControl::UpdateStatus(int current, int total) {
	// Confirmed (Deponia_Linux.asm lines 483637-483725).
	int fillWidth;
	if (_progressFillsForward) {
		fillWidth = static_cast<int>(_totalRect.GetWidth() * static_cast<float>(current) / static_cast<float>(total));
	} else {
		int barWidth = _totalRect.GetWidth();
		fillWidth = static_cast<int>(barWidth * static_cast<float>(total - current) / static_cast<float>(total));
		int newX = _fillOrigin.x + current;
		_progressBarPic.SetPosition(wxPoint{newX, _fillOrigin.y}, 1.0f);
	}
	_fillRect.SetWidth(fillWidth);
}

void TLoadingControl::EndLoading(TSoundInterface *soundManager) {
	// Confirmed (Deponia_Linux.asm lines 483735-483771): only plays the
	// loading sound here if Init() never got a chance to (soundManager was
	// null there, or the path wasn't valid) - matches EndLoading()'s own
	// "already played" check via the same wxFileName::IsOk() test.
	if (soundManager == nullptr)
		return;
	if (_soundPath.IsOk())
		return;
	soundManager->Play(_soundPath);
}

void TLoadingControl::Draw() {
	// Confirmed (Deponia_Linux.asm lines 483185-483201).
	SetCurrent();
	_backgroundPic.Draw(1.0f, 0xFFFFFFFF);
	_progressBarPic.DrawWithSrcRect(_fillRect, 1.0f, 0xFFFFFFFF);
}
