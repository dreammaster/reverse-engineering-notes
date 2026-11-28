// Confirmed (Deponia_Linux.asm lines 1050713-1105960, "src/mmedialib/soundBase.cpp"): the part of the sound engine that
// does not depend on how a sound is played: the list of the sounds (TSoundItem), pausing, muting, stopping and asking
// them by id or by file, and the fades. A sound is found by the id the scripts know (it has to be neither dead nor
// stopped) or by the name of its file (without what follows a '#', in any case). The functions that make or adjust
// a sound are the engine's (TSoundFFMPEG).
//
// A fade (StartSoundFade()) works with the state every sound has in it (TSoundItem::_fade): the sounds that were there
// are marked kOut to go, the new ones are kIn (their volume is scaled from 0 to 100% while the fade lasts); when the
// time is over (UpdateSoundFade()) the ones that go are stopped and let go and the new ones are at their own volume.
// A fade of the kind kInAndOut takes the half of the time for the old ones, then the new ones begin. kKeep sounds stay
// as they are (Keep()).
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
	void FinishSoundFade() override;
	void StartSoundFade(TFadeEnum fade, int milliseconds, bool markExisting) override;
	void UpdateSoundFade(bool finish) override;
	void PrintSounds(std::list<wxString> &lines) const override;
	void Keep(const wxFileName &file) override;
	void FadeOut(const wxFileName &file) override;

	/** Slot 0x128: sets the volume, the balance, the kind and the loop of a sound (null: no such sound; false). */
	virtual bool SetStats(TSoundItem *item, int volume, int balance, TSoundTypeEnum type, bool loop, int offset) = 0;

	/** The sounds. */
	const std::list<TSoundItem *> &GetList() const {
		return _items;
	}
	/** The name of a kind of fade ("Fade In" ...). */
	wxString GetFadeTypeAsString(TFadeEnum fade) const;

protected:
	/** The sound of the file (without what follows a '#'; any case), null when there is none that is not dead or stopped. */
	TSoundItem *GetSoundItem(const wxString &file) const;
	/** The sound with the id, null when there is none that is not dead or stopped. */
	TSoundItem *GetSoundItem(int id) const;
	/** A sound that is stopped and kept (streamed) for the same file. */
	TSoundItem *GetFreeSoundItem(const wxString &file) const;
	/** Stops the sound and lets it go. */
	void DeleteSoundItem(TSoundItem *item);
	/** The new sounds of a fade begin to play. */
	void StartFadeInSounds();
	/** The sounds that go with a fade are stopped and let go. */
	void StopFadeOutSounds();
	/** The fade is over for the new sounds: at their own volume. */
	void FinishFadeIn();

	std::list<TSoundItem *> _items;  // +0x48
	mutable wxCriticalSection _lock;  // +0x58

private:
	/** The end of a fade (the sounds are not locked here): the ones that go are let go, the new ones play, and the
	 *  states are cleared. */
	void finishFade(bool startNew);
	void startNewSounds();
	void clearFadeStates();
	void removeFadeOutSounds();
};
