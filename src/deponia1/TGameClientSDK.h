// Not yet assert-confirmed to a specific file; stays at the top level.
// Confirmed from TMasterControl's constructor: a small polymorphic wrapper
// (own vtable has only destructors) holding pointers to platform-specific
// backends.
#pragma once

#include "WxStub.h"

class TSteamSDK {
public:
	TSteamSDK() = default;

	// Confirmed call shapes only (TGameControl::Update, Deponia_Linux.asm
	// lines 470456, 470579) - checked once per frame; Update() only runs
	// when GetStatus() reports active. Not reversed beyond that call shape.
	bool GetStatus() const;
	// Confirmed call shape only (TMSavegame::Delete, Deponia_Linux.asm line
	// 163096) - not reversed beyond that.
	void DeleteCloudSavegame(const wxString &fileName);
	void Update();
};

class TGalaxySDK {
public:
	TGalaxySDK() = default;

	// Confirmed call shapes only (TGameControl::Update, Deponia_Linux.asm
	// lines 470462, 470573) - same "checked, then updated if active" shape
	// as TSteamSDK above; not reversed beyond that call shape.
	bool IsActive() const;
	void Update();
};

class TGameClientSDK {
public:
	TGameClientSDK();
	virtual ~TGameClientSDK();

	// Confirmed direct field reads at fixed offsets (TGameControl::Update,
	// Deponia_Linux.asm lines 470454, 470461) - no accessor in the original;
	// added since TGameControl needs one.
	TSteamSDK *GetSteam() const {
		return _steam;
	}
	TGalaxySDK *GetGalaxy() const {
		return _galaxy;
	}

private:
	TSteamSDK *_steam;
	TGalaxySDK *_galaxy;
};
