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
	// The stats and achievements of the player in Steam, and a web page in the overlay (the Steam SDK is the
	// vendor's; asked by the script commands through TGameClientSDK, asm 426024-426530). Nothing here is
	// reconstructed: the answers are those of a game that Steam does not run.
	int GetStat(const wxString &name);
	bool SetStat(const wxString &name, int value);
	bool ResetStats(bool achievementsToo);
	bool GetAchievement(const wxString &name);
	bool SetAchievement(const wxString &name);
	bool ClearAchievement(const wxString &name);
	bool ActivateGameOverlayToWebPage(const wxString &url);
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
	// The same for GOG Galaxy (also the vendor's, not reconstructed).
	int GetStat(const wxString &name);
	bool SetStat(const wxString &name, int value);
	bool ResetStatsAndAchievements();
	bool GetAchievement(const wxString &name);
	bool SetAchievement(const wxString &name);
	bool ClearAchievement(const wxString &name);
	/** Whether Galaxy is initialised (`ready` false) or ready for requests (`ready` true). */
	bool GetStatus(bool ready) const;
	bool ActivateGameOverlayToWebPage(const wxString &url);
	/** Starts Galaxy with the client id and secret of the game. */
	bool InitSDK(const wxString &clientId, const wxString &clientSecret);
};

/** What TGameClientSDK::GetStatus() asks about. */
enum TGameClientType {
	kGameClientNone = 0,
	kGameClientSteamInitialized = 1,
	kGameClientGalaxyInitialized = 2,
	kGameClientGalaxyReady = 3
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

	// Confirmed (asm lines 426024-426530): each asks Steam (when it is active) and then Galaxy (when it is
	// active); an active Galaxy has the last word.
	int GetStat(const wxString &name);
	bool SetStat(const wxString &name, int value);
	bool ResetStats(bool achievementsToo);
	bool GetAchievement(const wxString &name);
	bool SetAchievement(const wxString &name);
	bool ClearAchievement(const wxString &name);
	bool GetStatus(TGameClientType type);
	bool ActivateGameOverlayToWebPage(const wxString &url);

private:
	TSteamSDK *_steam;
	TGalaxySDK *_galaxy;
};
