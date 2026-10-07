// Not yet assert-confirmed to a specific file; stays at the top level.
//
// The sound-backend interface TSoundFFMPEG implements - confirmed distinct
// from TSoundFFMPEG itself (TGameControl::LoadAndInitGame passes
// TMasterControl's own TSoundFFMPEG* _soundManager to a parameter typed
// TSoundInterface*, Deponia_Linux.asm lines 468227-468230). Nothing about
// its own virtual surface has been reversed.
#pragma once

#include "WxStub.h"

// The kinds of sound a sound can be played as; only value 1 (the frame sounds of
// TGAnimation) and value 3 (the walking sound of TGCharacter) have been seen, so they
// are named by their raw values.
enum class TSoundTypeEnum {
	kValue1 = 1,
	kValue3 = 3
};

class TSoundInterface {
public:
	virtual ~TSoundInterface() = default;

	// Confirmed virtual (a distinct vtable slot from TSoundFFMPEG::PlaySound
	// - TLoadingControl::EndLoading, Deponia_Linux.asm lines 483759-483761)
	// - plays a single sound file with no other parameters; "Play" is a
	// guess from context (matching the confirmed free-standing
	// TSoundInterface::Play(wxFileName const&, int, int, bool,
	// TSoundTypeEnum, bool, int) overload seen in TLoadingControl::Init,
	// which isn't itself declared here since Init() remains unimplemented -
	// see its own comment), not a recovered identifier.
	virtual void Play(const wxFileName &file);
	// Confirmed call shape only (TGAnimation::NextSpriteSelected(), Deponia_Linux.asm
	// lines 149021): the non-virtual overload that plays a frame's sound with its
	// volume and balance; the other arguments are always (false, kValue1, true, 0)
	// there and are not resolved. Not reversed beyond that call shape.
	void Play(const wxFileName &file, int volume, int balance, bool flagA, TSoundTypeEnum type, bool flagB,
	          int value);
};
