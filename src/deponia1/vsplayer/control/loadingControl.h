// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/vsplayer/control/loadingControl.cpp - see manifest/source_layout.tsv.
#pragma once

#include "TPaintControl.h"
#include "TSoundInterface.h"

class TLoadingControl : public TPaintControl {
public:
	void UpdateStatus(int current, int total);
	// Confirmed call shape only (TGameControl::LoadAndInitGame,
	// Deponia_Linux.asm lines 468227-468230) - not reversed beyond that.
	void EndLoading(TSoundInterface *soundManager);
};
