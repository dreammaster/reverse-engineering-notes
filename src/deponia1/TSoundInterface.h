// Confirmed (Deponia_Linux.asm lines 1105963-1107210, mmedialib/sound.cpp): the part of the sound
// system that the rest of the engine talks to: the volumes (of the music, the sounds, the speech,
// the movies and all together), whether sounds are turned off, the background music that plays
// (it is kept when the same music is asked for again, and faded out when another one is), and the
// messages of the speech (see Signal()). It sits on top of the sound engine (TSoundBase, and
// TSoundFFMPEG that plays with FFmpeg): what a sound really does is in the virtual methods at the
// bottom, which the engine implements. TMasterControl's own TSoundFFMPEG* _soundManager is passed
// as a TSoundInterface* (TGameControl::LoadAndInitGame, Deponia_Linux.asm lines 468227-468230).
//
// TSoundBase (TSoundBase.h) is the engine that keeps the list of the sounds; here the methods of the engine do
// nothing and answer "no sound". Their places in the vtable (TSoundBase's, Deponia_Linux.asm line 3437456,
// whose entries carry their symbols; the slot is the offset / 8):
//   0x00 Signal(TSignalData const &, TSignalData &), 0x08/0x10 destructors,
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
//   0xF8 Keep(wxFileName const &), 0x100 Update() (TSoundFFMPEG's), 0x108 IsSoundInitialized() (the sound system
//   works), 0x110 Play() (plays a sound, TSoundFFMPEG::Play), 0x118 FadeOut(wxFileName const &),
//   0x120 AdjustVolume(music, sound, speech, movie, global) (the volumes changed), 0x128 SetStats(TSoundItem *, ...).
//
// Original layout: +0x08 whether sounds are turned off, +0x09 whether the sounds are muted (answers
// kSignalSoundFlag), +0x0C the fade that is running (TFadeEnum, 0: none), +0x10 the timer of the fade, +0x20 the time
// of the fade in milliseconds (float), +0x28 the background music's file, +0x30 the volume of the sounds,
// +0x34 of the music, +0x38 of the speech, +0x3C of the movies, +0x40 of all (percent).
#pragma once

#include <list>

#include "TSignalSlot.h"
#include "TTimer.h"
#include "WxStub.h"
#include "datastruct/vlist.h"

// The kinds of sound (what volume they play with: AdjustToGeneralVolume()). Named by the volume
// they are adjusted with: 0 the music (the background music of a scene, THScene), 1 the sounds (of the
// action parts and the frame sounds of TGAnimation), 2 the speech, 3 the sounds again (the
// walking sound of TGCharacter), 4 the movies, 5 the global volume.
enum class TSoundTypeEnum {
	kMusic = 0,
	kSound = 1,
	kSpeech = 2,
	kSound2 = 3,
	kMovie = 4,
	kGlobal = 5
};

// How a sound is faded (StartSoundFade(); TSoundBase::GetFadeTypeAsString() has the names): the new sounds
// fade in, the old ones out, or both one after the other, or the old ones out while the new ones come in (a new
// background music, THScene). For a sound itself the value says what it is doing in the fade (kNone, kIn: it is
// new, kOut: it goes) or kKeep: it is kept (Keep()).
enum class TFadeEnum {
	kNone = 0,
	kIn = 1,
	kOut = 2,
	kInAndOut = 3,
	kToNew = 4,
	kKeep = 10
};

class TSoundInterface : public TSignalSlot {
public:
	TSoundInterface();
	~TSoundInterface() override;

	/** Sounds are turned off (and Play() plays nothing), or on. */
	void DisableSounds(bool disable);

	/** Answers the sound messages (see TSignalData.h): whether a file is playing, the volumes,
	 *  and plays and stops a file. */
	void Signal(const TSignalData &signal, TSignalData &result) override;

	/** Plays `file` with the volume and the balance (`loop`: again and again); the id of the sound
	 *  that plays (-1: none). `flag`: for the music, the same music goes on (and is kept) and
	 *  another one fades the old one out; `value`: given on to the engine, 0 wherever it is called.
	 *  The background music is told by the name before a '#' in it. */
	int Play(const wxFileName &file, int volume, int balance, bool loop, TSoundTypeEnum type, bool flag, int value);
	/** The background music stops (fades out, with `fade`). */
	void StopBackgroundMusic(bool fade);
	void KeepCurrentBackgroundMusic();
	void KeepCurrentBackgroundMusicIfSameAs(const wxFileName &file);

	/** Sets the volumes (percent; a volume above 100 is not changed). */
	void SetVolume(int music, int sound, int speech, int movie, int global);
	int GetMusicVolume() const {
		return _musicVolume;
	}
	int GetSoundVolume() const {
		return _soundVolume;
	}
	int GetSpeechVolume() const {
		return _speechVolume;
	}
	int GetMovieVolume() const {
		return _movieVolume;
	}
	int GetGlobalVolume() const {
		return _globalVolume;
	}
	/** `volume` (percent) of a sound of `type` with the volume of its kind. */
	int AdjustToGeneralVolume(int volume, TSoundTypeEnum type) const;

	// What the sound engine does (see the list above).
	virtual void Mute(bool mute);
	/** Slots 0x20 and 0x28: lets the sounds of a kind go on / pauses them (`system.pauseAllSounds` ...). */
	virtual void Continue(TSoundTypeEnum type);
	virtual void Pause(TSoundTypeEnum type);
	/** Slots 0x38 and 0xD0: pauses all sounds (the window lost the focus); updates the values of the audio busses (each frame). */
	virtual void PauseAll();
	virtual void BusValuesUpdate();
	/** Slot 0xC8: the audio busses of the game (a list of bus objects) are the active ones. */
	virtual void BusActivate(TVList *busses);
	virtual void ContinueAll();
	virtual void CleanUp();
	/** Stops the sound of `file`. */
	virtual void Stop(const wxFileName &file);
	// The sounds by their id (the id Play() gives); asked by the script commands. Slots of TSoundBase:
	// 0x50 Stop(int), 0x58 TogglePause(int), 0x60 GetExistingSoundID(file), 0x78 SetStats(int ...), 0x88
	// IsPlaying(int), 0x90 IsPaused(int), 0x98 GetVolume(int), 0xA0 GetBalance(int), 0xA8 GetOffset(int),
	// 0xB0 GetDuration(int), 0xC0 IsLoop(int). All say "no sound" (-1, false) until the engine is there.
	virtual bool Stop(int id);
	virtual bool TogglePause(int id);
	/** The id of the sound of `file` that plays (-1: none). */
	virtual int GetExistingSoundID(const wxFileName &file) const;
	virtual bool SetStats(int id, int volume, int balance, TSoundTypeEnum type, bool loop, int offset);
	virtual bool IsPlaying(int id) const;
	virtual bool IsPaused(int id) const;
	virtual int GetVolume(int id) const;
	virtual int GetBalance(int id) const;
	virtual int GetOffset(int id) const;
	virtual int GetDuration(int id) const;
	virtual bool IsLoop(int id) const;
	/** Slots 0x68 and 0xB8: the name of the file of a sound that plays or is paused (empty: none); the average
	 *  volume of the samples that play now. */
	virtual wxString GetExistingSoundFromID(int id) const;
	virtual float GetSampleAvg(int id) const;
	/** Slot 0xF0: the lines of text that tell what sounds there are (for the console). */
	virtual void PrintSounds(std::list<wxString> &lines) const;
	/** Changes the volume and the balance of a sound that is playing, and its kind. */
	virtual void SetStats(const wxFileName &file, int volume, int balance, TSoundTypeEnum type, bool flag, int value);
	virtual bool IsPlaying(const wxFileName &file) const;
	virtual void FinishSoundFade();
	virtual void StartSoundFade(TFadeEnum fade, int milliseconds, bool flag);
	virtual void UpdateSoundFade(bool update);
	/** Keeps the sound of `file` loaded after it has played. */
	virtual void Keep(const wxFileName &file);
	/** Slot 0x100: the engine goes on (the sounds that have ended are let go). */
	virtual void Update();
	/** Whether the sound system works (a game without one has no speech). */
	virtual bool IsSoundSystemReady() const;
	/** Slot 0x110: the sound engine plays `file`; the id (-1: none). `streamed`: the sound is kept when it is over and
	 *  used again for the same file (the walking sound); `fadeIn`: it starts silently, to be faded in. See Play() for
	 *  the others. */
	virtual int PlaySound(const wxFileName &file, int volume, int balance, bool loop, bool streamed, bool fadeIn,
	                      TSoundTypeEnum type, int offset);
	/** The sound of `file` fades out. */
	virtual void FadeOut(const wxFileName &file);
	/** Slot 0x120: the volumes have changed (-1: that one has not). */
	virtual void AdjustVolume(int music, int sound, int speech, int movie, int global);

	// Confirmed virtual (a distinct vtable slot from the ones named above
	// - TLoadingControl::EndLoading, Deponia_Linux.asm lines 483759-483761)
	// - plays a single sound file with no other parameters; "Play" is a
	// guess from context, not a recovered identifier.
	virtual void Play(const wxFileName &file);

protected:
	bool _disabled;               // +0x08
	bool _muted;                  // +0x09
	int _soundFade;               // +0x0C, the fade that runs (TFadeEnum)
	TTimer _timer;                // +0x10, since the fade began
	float _fadeDuration;          // +0x20, milliseconds
	wxFileName _backgroundMusic;  // +0x28
	int _soundVolume;             // +0x30
	int _musicVolume;             // +0x34
	int _speechVolume;            // +0x38
	int _movieVolume;             // +0x3C
	int _globalVolume;            // +0x40
};
