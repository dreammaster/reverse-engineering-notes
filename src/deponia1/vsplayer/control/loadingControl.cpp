#include "vsplayer/control/loadingControl.h"

#include "vsplayer/control/masterControl.h"

void TLoadingControl::Init(SLoadingScreen &/*screen*/, TSoundInterface */*soundManager*/) {
	// Not reversed - see this class's own header comment for why.
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
