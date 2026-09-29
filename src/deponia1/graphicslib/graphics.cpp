#include "graphicslib/graphics.h"

#include "graphicslib/preloadedPicManager.h"

TGraphicsInterface *graphics = new TGraphicsInterface();

TPreloadedPicManager *TGraphicsInterface::GetPreloadedPicManager() {
	if (!_preloadedPicManager)
		_preloadedPicManager = new TPreloadedPicManager();
	return _preloadedPicManager;
}

TSpriteHandle *TGraphicsInterface::GetSpriteFromCache(const wxString &/*name*/) {
	return nullptr;
}

void TGraphicsInterface::OnSpriteHandleReleased(TSpriteHandle */*handle*/) {
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

void TGraphicsInterface::SetCacheSize(int /*size*/) {
}
