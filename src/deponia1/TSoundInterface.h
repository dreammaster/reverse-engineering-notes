// Confirmed (Deponia_Linux.asm lines 1105963-1107210, mmedialib/sound.cpp): the part of the sound
// system that the rest of the engine talks to: the volumes (of the music, the sounds, the speech,
// the movies and all together), whether sounds are turned off, the background music that plays
// (it is kept when the same music is asked for again, and faded out when another one is), and the
// messages of the speech (see Signal()). It sits on top of the sound engine (TSoundBase, and
// TSoundFFMPEG that plays with FFmpeg): what a sound really does is in the virtual methods at the
// bottom, which the engine implements. TMasterControl's own TSoundFFMPEG* _soundManager is passed
// as a TSoundInterface* (TGameControl::LoadAndInitGame, Deponia_Linux.asm lines 468227-468230).
//
// The sound engine is not reconstructed yet, so each of the engine's methods does nothing and
// answers "no sound". Their places in the vtable (TSoundBase's, Deponia_Linux.asm line 3437456,
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
//   0xF8 Keep(wxFileName const &), 0x100 (the engine's), 0x108 (whether the sound system works),
//   0x110 (plays a sound), 0x118 FadeOut(wxFileName const &), 0x120 (the volumes changed), 0x128.
//
// Original layout: +0x08 whether sounds are turned off, +0x09 a flag (answers kSignalSoundFlag),
// +0x0C an int (0), +0x10 a timer, +0x28 the background music's file, +0x30 the volume of the sounds,
// +0x34 of the music, +0x38 of the speech, +0x3C of the movies, +0x40 of all (percent).
#pragma once

#include "TSignalSlot.h"
#include "TTimer.h"
#include "WxStub.h"

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

// How a sound is faded (StartSoundFade()); only 4 (a new background music, THScene) has been seen.
enum class TFadeEnum {
	kValue4 = 4
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
	/** Changes the volume and the balance of a sound that is playing, and its kind. */
	virtual void SetStats(const wxFileName &file, int volume, int balance, TSoundTypeEnum type, bool flag, int value);
	virtual bool IsPlaying(const wxFileName &file) const;
	virtual void FinishSoundFade();
	virtual void StartSoundFade(TFadeEnum fade, int milliseconds, bool flag);
	virtual void UpdateSoundFade(bool update);
	/** Keeps the sound of `file` loaded after it has played. */
	virtual void Keep(const wxFileName &file);
	/** Whether the sound system works (a game without one has no speech). */
	virtual bool IsSoundSystemReady() const;
	/** The sound engine plays `file`; the id (-1: none). See Play() for the arguments. */
	virtual int PlaySound(const wxFileName &file, int volume, int balance, bool loop, bool walking, int type,
	                      bool flag, int value);
	/** The sound of `file` fades out. */
	virtual void FadeOut(const wxFileName &file);
	/** The volumes have changed. */
	virtual void VolumesChanged();

	// Confirmed virtual (a distinct vtable slot from the ones named above
	// - TLoadingControl::EndLoading, Deponia_Linux.asm lines 483759-483761)
	// - plays a single sound file with no other parameters; "Play" is a
	// guess from context, not a recovered identifier.
	virtual void Play(const wxFileName &file);

protected:
	bool _disabled;               // +0x08
	bool _flag;                   // +0x09
	int _field0C;                 // +0x0C
	TTimer _timer;                // +0x10
	wxFileName _backgroundMusic;  // +0x28
	int _soundVolume;             // +0x30
	int _musicVolume;             // +0x34
	int _speechVolume;            // +0x38
	int _movieVolume;             // +0x3C
	int _globalVolume;            // +0x40
};
