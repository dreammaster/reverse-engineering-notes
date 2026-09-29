// Not yet assert-confirmed to a specific file; stays at the top level.
// TMasterControl::GetSoundManager() returns this directly, so whatever the
// "sound manager" interface is, TSoundFFMPEG implements it. Only the one
// virtual call TMasterControl::VideoFrame makes (through TSoundFFMPEG's
// vtable slot 6; the other slots' names aren't identified, so this doesn't
// declare them - just the one method actually exercised) is stubbed.
#pragma once

#include "TSoundInterface.h"

class TSoundFFMPEG : public TSoundInterface {
public:
	virtual ~TSoundFFMPEG() = default;
	virtual void OnVideoFrameFinished();
	// Confirmed virtual (vtable slot 8, TGameControl::InitAfterLoadingScreen,
	// Deponia_Linux.asm lines 457475-457479) - called once, right after a
	// loading screen finishes and before the scene is shown. Real name/
	// purpose not recovered; "Resume" is a guess from context (a sound
	// engine lifecycle hook makes sense at that point), not a recovered
	// identifier.
	virtual void Resume();
};
