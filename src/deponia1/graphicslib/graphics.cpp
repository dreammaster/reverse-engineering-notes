#include "graphicslib/graphics.h"

#include "graphicslib/preloadedPicManager.h"

TGraphicsInterface *graphics = new TGraphicsInterface();

int defaultShader = 0;

void ShaderCallback(int /*shader*/, TVisObjRef */*ref*/) {
}

void TGraphicsInterface::RemoveFromCache(const wxString &/*name*/) {
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

int TGraphicsInterface::GetCacheSpriteCount() const {
	return 0;
}

void TGraphicsInterface::UpdateCache() {
}
