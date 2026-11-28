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
//   0xD0 BusValuesUpdate(), 0xD8 LoadMixers(TVisionaire *), 0xE0 FadeSound(int, int, int, bool), 0xE8 FadeAll(int, int),
//   0xF0 PrintSounds(std::list<wxString> &) const, 0xF8 Update() (TSoundFFMPEG's), 0x100 IsSoundInitialized() (the sound
//   system works), 0x108 Play() (plays a sound, TSoundFFMPEG::Play), 0x110 AdjustVolume(music, sound, speech, movie,
//   global) (the volumes changed), 0x118 SetStats(TSoundItem *, ...). (Night of the Rabbit's; Deponia 1's has FinishSoundFade,
//   StartSoundFade, UpdateSoundFade, Keep and FadeOut in their places, which moves the ones after them by 0x10.)
//
// Layout of Night of the Rabbit's: +0x08 whether sounds are turned off, +0x09 whether the sounds are muted (answers
// kSignalSoundFlag), +0x10 the timer (since the last Update()), +0x20 the id of the background music (-1: none), +0x28 the
// background music's file, +0x30 the volume of the sounds, +0x34 of the music, +0x38 of the speech, +0x3C of the movies,
// +0x40 of all (percent: the general volumes). Every volume that is given to a sound is in decibels (-100 to 0).
#pragma once

#include <list>

#include "TSignalSlot.h"
#include "TTimer.h"
#include "WxStub.h"
#include "datastruct/vlist.h"
#include "datastruct/visobjref.h"

// The kinds of sound (the general volume of the bus they play on). Named by the volume
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

// The kinds of picture fade (TGraphicsInterface::FadeIn takes one as a number). Deponia 1's sound engine also used it for the
// fade of its sounds; Night of the Rabbit's does not (a sound has its own fade, see FadeSound() and FadeAll()).
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

	/** Plays `file` (Night of the Rabbit's, TSoundInterface::Play(wxFileName const &, TVisObjRef const &, int, int, bool, TSoundTypeEnum, int,
	 *  int)) on the audio bus `bus` (empty: the one of its kind) with the volume (decibels, -100 to 0) and the balance (-100 to 100)
	 *  (`loop`: again and again); `fadeMs` is how long it takes to fade in; the id of the sound that plays (-1: none). The background
	 *  music (the kind 0) is told by the name before a '#' in it: the one that plays goes on (-1), another fades the old one out
	 *  (`fadeMs`, to -100 dB) and plays, looped, whatever `loop` says. */
	int Play(const wxFileName &file, const TVisObjRef &bus, int volume, int balance, bool loop, TSoundTypeEnum type, int fadeMs,
	         int offset);
	/** The background music stops (fades out in a second, with `fade`). */
	void StopBackgroundMusic(bool fade);

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
	/** Slot 0xE0: the sound with the id fades in `milliseconds` to `decibels` (-100: silent, then it is let go); `keep`: the volume it has
	 *  then is the new volume of the sound. */
	virtual void FadeSound(int id, int milliseconds, int decibels, bool keep);
	/** Slot 0xE8: every sound that is not fading fades in `milliseconds` to `percent` of its volume. */
	virtual void FadeAll(int milliseconds, int percent);
	/** Slot 0x100: the engine goes on (the sounds that have ended are let go). */
	virtual void Update();
	/** Whether the sound system works (a game without one has no speech). */
	virtual bool IsSoundSystemReady() const;
	/** Slot 0x108: the sound engine plays `file`; the id (-1: none). `streamed`: the sound is kept when it is over and
	 *  used again for the same file (the walking sound); `fadeInMs`: it starts silently and fades in. See Play() for the others. */
	virtual int PlaySound(const wxFileName &file, const TVisObjRef &bus, int volume, int balance, bool loop, bool streamed, int fadeInMs,
	                      TSoundTypeEnum type, int offset);
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
	TTimer _timer;                // +0x10, since the last Update()
	int _musicId;                 // +0x20, the sound of the background music (-1: none)
	wxFileName _backgroundMusic;  // +0x28
	int _soundVolume;             // +0x30
	int _musicVolume;             // +0x34
	int _speechVolume;            // +0x38
	int _movieVolume;             // +0x3C
	int _globalVolume;            // +0x40
};
