#include "TGameClientSDK.h"

void TSteamSDK::DeleteCloudSavegame(const wxString &/*fileName*/) {
}

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

int TSteamSDK::GetStat(const wxString &/*name*/) {
	return 0;
}

bool TSteamSDK::SetStat(const wxString &/*name*/, int /*value*/) {
	return false;
}

bool TSteamSDK::ResetStats(bool /*achievementsToo*/) {
	return false;
}

bool TSteamSDK::GetAchievement(const wxString &/*name*/) {
	return false;
}

bool TSteamSDK::SetAchievement(const wxString &/*name*/) {
	return false;
}

bool TSteamSDK::ClearAchievement(const wxString &/*name*/) {
	return false;
}

bool TSteamSDK::ActivateGameOverlayToWebPage(const wxString &/*url*/) {
	return false;
}

int TGalaxySDK::GetStat(const wxString &/*name*/) {
	return 0;
}

bool TGalaxySDK::SetStat(const wxString &/*name*/, int /*value*/) {
	return false;
}

bool TGalaxySDK::ResetStatsAndAchievements() {
	return false;
}

bool TGalaxySDK::GetAchievement(const wxString &/*name*/) {
	return false;
}

bool TGalaxySDK::SetAchievement(const wxString &/*name*/) {
	return false;
}

bool TGalaxySDK::ClearAchievement(const wxString &/*name*/) {
	return false;
}

bool TGalaxySDK::GetStatus(bool /*ready*/) const {
	return false;
}

bool TGalaxySDK::InitSDK(const wxString &/*clientId*/, const wxString &/*clientSecret*/) {
	return false;
}

bool TGalaxySDK::ActivateGameOverlayToWebPage(const wxString &/*url*/) {
	return false;
}

TGameClientSDK::TGameClientSDK() : _steam(new TSteamSDK()), _galaxy(new TGalaxySDK()) {
}

TGameClientSDK::~TGameClientSDK() {
	delete _steam;
	delete _galaxy;
}

// Confirmed (asm lines 426024-426075)
int TGameClientSDK::GetStat(const wxString &name) {
	int result = 0;

	if (_steam->GetStatus())
		result = _steam->GetStat(name);

	if (_galaxy->IsActive())
		return _galaxy->GetStat(name);

	return result;
}

// Confirmed (asm lines 426085-426143)
bool TGameClientSDK::SetStat(const wxString &name, int value) {
	bool result = false;

	if (_steam->GetStatus())
		result = _steam->SetStat(name, value);

	if (_galaxy->IsActive())
		return _galaxy->SetStat(name, value);

	return result;
}

// Confirmed (asm lines 426155-426205)
bool TGameClientSDK::ResetStats(bool achievementsToo) {
	bool result = false;

	if (_steam->GetStatus())
		result = _steam->ResetStats(achievementsToo);

	if (_galaxy->IsActive())
		return _galaxy->ResetStatsAndAchievements();

	return result;
}

// Confirmed (asm lines 426217-426268)
bool TGameClientSDK::GetAchievement(const wxString &name) {
	bool result = false;

	if (_steam->GetStatus())
		result = _steam->GetAchievement(name);

	if (_galaxy->IsActive())
		return _galaxy->GetAchievement(name);

	return result;
}

// Confirmed (asm lines 426278-426329)
bool TGameClientSDK::SetAchievement(const wxString &name) {
	bool result = false;

	if (_steam->GetStatus())
		result = _steam->SetAchievement(name);

	if (_galaxy->IsActive())
		return _galaxy->SetAchievement(name);

	return result;
}

// Confirmed (asm lines 426339-426390)
bool TGameClientSDK::ClearAchievement(const wxString &name) {
	bool result = false;

	if (_steam->GetStatus())
		result = _steam->ClearAchievement(name);

	if (_galaxy->IsActive())
		return _galaxy->ClearAchievement(name);

	return result;
}

// Confirmed (asm lines 426400-426464)
bool TGameClientSDK::GetStatus(TGameClientType type) {
	switch (type) {
	case kGameClientSteamInitialized:
		return _steam->GetStatus();
	case kGameClientGalaxyInitialized:
		return _galaxy->IsActive() && _galaxy->GetStatus(false);
	case kGameClientGalaxyReady:
		return _galaxy->IsActive() && _galaxy->GetStatus(true);
	default:
		return false;
	}
}

// Confirmed (asm lines 426464-426525)
bool TGameClientSDK::ActivateGameOverlayToWebPage(const wxString &url) {
	bool result = false;

	if (_steam->GetStatus())
		result = _steam->ActivateGameOverlayToWebPage(url);

	if (_galaxy->IsActive())
		return _galaxy->ActivateGameOverlayToWebPage(url);

	return result;
}
