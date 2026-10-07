// Not yet assert-confirmed to a specific file; stays at the top level.
// TMasterControl::GetSoundManager() returns this directly, so whatever the
// "sound manager" interface is, TSoundFFMPEG implements it (its own methods are the
// TSoundBase/TSoundFFMPEG sound engine, which is not reconstructed: see TSoundInterface.h
// for the virtuals the rest of the code calls, and the names of the vtable slots the guessed
// ones here were - slot 6 ContinueAll(), 8 CleanUp(), 14 SetStats(), 0x48 Stop(), 0xD8
// FinishSoundFade() and 0xE0 StartSoundFade()).
#pragma once

#include "WxStub.h"
#include "TSignalSlot.h"
#include "TSoundInterface.h"

class TSoundFFMPEG : public TSoundInterface, public TSignalSlot {
public:
	virtual ~TSoundFFMPEG() = default;
};
