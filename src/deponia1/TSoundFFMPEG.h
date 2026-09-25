// Not yet assert-confirmed to a specific file; stays at the top level.
// TMasterControl::GetSoundManager() returns this directly, so whatever the
// "sound manager" interface is, TSoundFFMPEG implements it. Only the one
// virtual call TMasterControl::VideoFrame makes (through TSoundFFMPEG's
// vtable slot 6; the other slots' names aren't identified, so this doesn't
// declare them - just the one method actually exercised) is stubbed.
#pragma once

class TSoundFFMPEG {
public:
    virtual ~TSoundFFMPEG() = default;
    virtual void OnVideoFrameFinished();
};
