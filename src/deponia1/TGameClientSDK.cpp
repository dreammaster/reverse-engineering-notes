#include "TGameClientSDK.h"

TGameClientSDK::TGameClientSDK() : m_steam(new TSteamSDK()), m_galaxy(new TGalaxySDK()) {
}

TGameClientSDK::~TGameClientSDK() {
    delete m_steam;
    delete m_galaxy;
}
