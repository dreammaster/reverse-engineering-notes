#include "TGameClientSDK.h"

bool TSteamSDK::GetStatus() const {
	return false;
}

void TSteamSDK::Update() {
}

bool TGalaxySDK::IsActive() const {
	return false;
}

void TGalaxySDK::Update() {
}

TGameClientSDK::TGameClientSDK() : _steam(new TSteamSDK()), _galaxy(new TGalaxySDK()) {
}

TGameClientSDK::~TGameClientSDK() {
	delete _steam;
	delete _galaxy;
}
