// Not yet assert-confirmed to a specific file; stays at the top level.
//
// The sound-backend interface TSoundFFMPEG implements - confirmed distinct
// from TSoundFFMPEG itself (TGameControl::LoadAndInitGame passes
// TMasterControl's own TSoundFFMPEG* _soundManager to a parameter typed
// TSoundInterface*, Deponia_Linux.asm lines 468227-468230). Nothing about
// its own virtual surface has been reversed.
#pragma once

class TSoundInterface {
public:
	virtual ~TSoundInterface() = default;
};
