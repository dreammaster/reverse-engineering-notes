// Not yet assert-confirmed to a specific file; stays at the top level.
// TMasterControl::GetSoundManager() returns this directly, so whatever the
// "sound manager" interface is, TSoundFFMPEG implements it. Only the one
// virtual call TMasterControl::VideoFrame makes (through TSoundFFMPEG's
// vtable slot 6; the other slots' names aren't identified, so this doesn't
// declare them - just the one method actually exercised) is stubbed.
#pragma once

#include "WxStub.h"
#include "TSignalSlot.h"
#include "TSoundInterface.h"

class TSoundFFMPEG : public TSoundInterface, public TSignalSlot {
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
	// Confirmed virtual (vtable slot 14, TGameControl::UpdateWalkingSounds,
	// Deponia_Linux.asm lines 463592-463607) - called with a sound file, a
	// volume clamped to [0,100], and a stereo pan in [-100,100] (both
	// confirmed by their derivation, not just guessed ranges), plus two
	// further int args (confirmed 3 and 0 at this one call site, meaning
	// unresolved). "PlaySound" is a guess from the call shape, not a
	// recovered identifier.
	virtual void PlaySound(const wxFileName &file, int volume, int pan, int a, int b);
	// Confirmed call shape only (TGCharacter::StopWalkingSound(), Deponia_Linux.asm lines
	// 177644-177654): the virtual at slot 0x48 of the sound manager stops the sound of
	// a file (the walking sound it started with TSoundInterface::Play()). Not reversed
	// beyond that call shape, so it is a plain method here.
	void StopSound(const wxFileName &file);
	// Confirmed call shapes only (THScene::OnEvent(), Deponia_Linux.asm lines 219546-219588):
	// the virtuals at slots 0xD8 and 0xE0 of the sound manager that are called around playing
	// a scene's new background music - the first with nothing, the second with (4, 3000, 0).
	// Not reversed (the sound base class is still `todo`), named by their slots.
	virtual void Slot0xD8();
	virtual void Slot0xE0(int kind, int milliseconds, bool flag);
};
