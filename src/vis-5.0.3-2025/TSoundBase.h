// Confirmed (Deponia_Linux.asm lines 1050713-1105960, "src/mmedialib/soundBase.cpp"): the part of the sound engine that
// does not depend on how a sound is played: the list of the sounds (TSoundItem), pausing, muting, stopping and asking
// them by id or by file, and the fades. A sound is found by the id the scripts know (it has to be neither dead nor
// stopped) or by the name of its file (without what follows a '#', in any case). The functions that make or adjust
// a sound are the engine's (TSoundFFMPEG).
//
// The fades (Night of the Rabbit's; Deponia 1's were a state of the whole engine, with StartSoundFade() and the others): each sound
// has a fade of its own (TSoundItem::_fading and the numbers with it), that FadeSound() (one sound) and FadeAll() (every sound that
// is not music) start and TSoundFFMPEG::Update() goes through. See AUDIO.md.
//
// Original layout: TSoundInterface ends at +0x48; +0x48 the list of the sounds, +0x58 the critical section.
#pragma once

#include <list>

#include "TSoundInterface.h"
#include "TSoundItem.h"
#include "WxStub.h"

class TSoundBase : public TSoundInterface {
public:
	TSoundBase();
	~TSoundBase() override;

	void Mute(bool mute) override;
	void Continue(TSoundTypeEnum type) override;
	void Pause(TSoundTypeEnum type) override;
	void ContinueAll() override;
	void PauseAll() override;
	void CleanUp() override;
	void Stop(const wxFileName &file) override;
	bool Stop(int id) override;
	bool TogglePause(int id) override;
	int GetExistingSoundID(const wxFileName &file) const override;
	wxString GetExistingSoundFromID(int id) const override;
	void SetStats(const wxFileName &file, int volume, int balance, TSoundTypeEnum type, bool loop,
	              int offset) override;
	bool SetStats(int id, int volume, int balance, TSoundTypeEnum type, bool loop, int offset) override;
	bool IsPlaying(const wxFileName &file) const override;
	bool IsPlaying(int id) const override;
	bool IsPaused(int id) const override;
	int GetVolume(int id) const override;
	int GetBalance(int id) const override;
	int GetOffset(int id) const override;
	int GetDuration(int id) const override;
	float GetSampleAvg(int id) const override;
	bool IsLoop(int id) const override;
	/** TODO: the audio busses (soundengine::AudioBus) are not reconstructed. */
	void BusActivate(TVList *busses) override;
	void BusValuesUpdate() override;
	/** Slot 0xE8 (Night of the Rabbit): every sound that is not fading and is not the music fades from its volume to `percent` of it. */
	void FadeAll(int milliseconds, int percent) override;
	void PrintSounds(std::list<wxString> &lines) const override;

	/** Slot 0x128: sets the volume, the balance, the kind and the loop of a sound (null: no such sound; false). */
	virtual bool SetStats(TSoundItem *item, int volume, int balance, TSoundTypeEnum type, bool loop, int offset) = 0;

	/** The sounds. */
	const std::list<TSoundItem *> &GetList() const {
		return _items;
	}

protected:
	/** The sound of the file (without what follows a '#'; any case), null when there is none that is not dead or stopped. */
	TSoundItem *GetSoundItem(const wxString &file) const;
	/** The sound with the id, null when there is none that is not dead or stopped. */
	TSoundItem *GetSoundItem(int id) const;
	/** A sound that is stopped and kept (streamed) for the same file. */
	TSoundItem *GetFreeSoundItem(const wxString &file) const;
	/** Stops the sound and lets it go. */
	void DeleteSoundItem(TSoundItem *item);

	std::list<TSoundItem *> _items;  // +0x48
	mutable wxCriticalSection _lock;  // +0x58
};
