// Not yet assert-confirmed to a specific file; stays at the top level.
//
// The sound-backend interface TSoundFFMPEG implements - confirmed distinct
// from TSoundFFMPEG itself (TGameControl::LoadAndInitGame passes
// TMasterControl's own TSoundFFMPEG* _soundManager to a parameter typed
// TSoundInterface*, Deponia_Linux.asm lines 468227-468230).
//
// The virtual surface here is the part of TSoundBase's vtable (Deponia_Linux.asm line 3437456:
// `_ZTV10TSoundBase`, whose entries carry their symbols) that the rest of the code calls; the
// sound engine itself (TSoundBase, TSoundFFMPEG) is not reconstructed yet, so each method does
// nothing and answers "no sound". The complete vtable, in order (the slot is its offset / 8):
//   0x00 Signal(TSignalData const &, TSignalData &) (TSignalSlot), 0x08/0x10 destructors,
//   0x18 Mute(bool), 0x20 Continue(TSoundTypeEnum), 0x28 Pause(TSoundTypeEnum), 0x30 ContinueAll(),
//   0x38 PauseAll(), 0x40 CleanUp(), 0x48 Stop(wxFileName const &), 0x50 Stop(int),
//   0x58 TogglePause(int) const, 0x60 GetExistingSoundID(wxFileName const &) const,
//   0x68 GetExistingSoundFromID(int) const,
//   0x70 SetStats(wxFileName const &, int, int, TSoundTypeEnum, bool, int),
//   0x78 SetStats(int, int, int, TSoundTypeEnum, bool, int), 0x80 IsPlaying(wxFileName const &) const,
//   0x88 IsPlaying(int) const, 0x90 IsPaused(int) const, 0x98 GetVolume(int) const,
//   0xA0 GetBalance(int) const, 0xA8 GetOffset(int) const, 0xB0 GetDuration(int) const,
//   0xB8 GetSampleAvg(int) const, 0xC0 IsLoop(int) const, 0xC8 BusActivate(TVList *),
//   0xD0 BusValuesUpdate(), 0xD8 FinishSoundFade(), 0xE0 StartSoundFade(TFadeEnum, int, bool),
//   0xE8 UpdateSoundFade(bool), 0xF0 PrintSounds(std::list<wxString> &) const,
//   0xF8 Keep(wxFileName const &), then three pure virtuals (the backend's own), 0x118
//   FadeOut(wxFileName const &) and two more pure virtuals.
#pragma once

#include "WxStub.h"

// The kinds of sound a sound can be played as; only value 0 (the background music of a
// scene, THScene), 1 (the sounds of the action parts and the frame sounds of TGAnimation) and
// 3 (the walking sound of TGCharacter) have been seen, so they are named by their raw values.
enum class TSoundTypeEnum {
	kValue0 = 0,
	kValue1 = 1,
	kValue3 = 3
};

// How a sound is faded (StartSoundFade()); only 4 (a new background music, THScene) has been seen.
enum class TFadeEnum {
	kValue4 = 4
};

class TSoundInterface {
public:
	virtual ~TSoundInterface() = default;

	// Confirmed virtual (a distinct vtable slot from the ones named below
	// - TLoadingControl::EndLoading, Deponia_Linux.asm lines 483759-483761)
	// - plays a single sound file with no other parameters; "Play" is a
	// guess from context (matching the confirmed free-standing
	// TSoundInterface::Play(wxFileName const&, int, int, bool,
	// TSoundTypeEnum, bool, int) overload seen in TLoadingControl::Init,
	// which isn't itself declared here since Init() remains unimplemented -
	// see its own comment), not a recovered identifier.
	virtual void Play(const wxFileName &file);
	/** Plays `file` with the volume and the balance (`loop`: again and again); the id of the
	 *  sound that plays (-1: none). The other arguments are not resolved: the kind of sound (see
	 *  TSoundTypeEnum), then a flag (true) and a value (0) wherever it is called. Not reversed
	 *  beyond that call shape. */
	int Play(const wxFileName &file, int volume, int balance, bool loop, TSoundTypeEnum type, bool flag, int value);

	virtual void ContinueAll();
	virtual void CleanUp();
	/** Stops the sound of `file`. */
	virtual void Stop(const wxFileName &file);
	/** Changes the volume and the balance of a sound that is playing, and its kind. */
	virtual void SetStats(const wxFileName &file, int volume, int balance, TSoundTypeEnum type, bool flag, int value);
	virtual void FinishSoundFade();
	virtual void StartSoundFade(TFadeEnum fade, int milliseconds, bool flag);
	virtual bool IsPlaying(const wxFileName &file) const;
	/** Keeps the sound of `file` loaded after it has played. */
	virtual void Keep(const wxFileName &file);
};
