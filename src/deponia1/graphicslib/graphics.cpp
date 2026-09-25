#include "graphicslib/graphics.h"

#include "graphicslib/preloadedPicManager.h"

TGraphicsInterface* graphics = new TGraphicsInterface();

TPreloadedPicManager* TGraphicsInterface::GetPreloadedPicManager() {
    if (!m_preloadedPicManager)
        m_preloadedPicManager = new TPreloadedPicManager();
    return m_preloadedPicManager;
}

TSpriteHandle* TGraphicsInterface::GetSpriteFromCache(const wxString& /*name*/) {
    return nullptr;
}

void TGraphicsInterface::OnSpriteHandleReleased(TSpriteHandle* /*handle*/) {
}

void TGraphicsInterface::SetMatrixMode(bool /*a*/, bool /*b*/) {
}

void TGraphicsInterface::ResetMatrix(bool /*a*/, bool /*b*/) {
}

void TGraphicsInterface::Flip() {
}
