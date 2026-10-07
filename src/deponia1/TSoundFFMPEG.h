// Not yet assert-confirmed to a specific file; stays at the top level.
// TMasterControl::GetSoundManager() returns this directly: the sound manager is TSoundFFMPEG, the
// sound engine that plays with FFmpeg (a TSoundBase, which is a TSoundInterface). None of its
// own methods is reconstructed: see TSoundInterface.h for what the rest of the code calls, and for
// the names of the vtable slots that this class's earlier, guessed methods were (slot 6
// ContinueAll(), 8 CleanUp(), 14 SetStats(), 0x48 Stop(), 0xD8 FinishSoundFade() and 0xE0
// StartSoundFade()).
#pragma once

#include "TSoundInterface.h"
#include "WxStub.h"

class TSoundFFMPEG : public TSoundInterface {
public:
	~TSoundFFMPEG() override = default;
};
