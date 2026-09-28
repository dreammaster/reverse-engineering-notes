#include "TGameClientSDK.h"

TGameClientSDK::TGameClientSDK() : _steam(new TSteamSDK()), _galaxy(new TGalaxySDK()) {
}

TGameClientSDK::~TGameClientSDK() {
	delete _steam;
	delete _galaxy;
}
