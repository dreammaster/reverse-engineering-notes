// Confirmed (Deponia_Linux.asm: TSoundFFMPEG, TFFMPEGSoundItem; lines 1053026-1060440): the sound engine of the
// player. It plays with OpenAL and decodes with FFmpeg (soundengine::Stream, AudioDataSourceFFMPEG ...), and has its own
// thread that calls Update(). Those parts - the playing of a stream - are not reconstructed: TSoundStream is what the
// engine needs of a stream (the names are those of the operations that the original does on its OpenAL source), and a
// backend gives the engine its streams through CreateStream(). Without one no sound plays, but the list of the sounds,
// the volumes and the fades work (see TSoundBase.h).
//
// Original layout of TSoundFFMPEG: TSoundBase ends at +0x70; +0x60 the volume of the movies (percent), +0x64 the same
// adjusted to the general volumes, +0x68 and +0x6C the same for the global volume, +0x70 the data of the thread of the
// engine, +0x78 the thread, +0x80 the streams that have ended (a vector).
// Of a TFFMPEGSoundItem: TSoundItem ends at +0x20; +0x20 the stream, +0x28 its volume (percent), +0x2C its volume for the
// engine (float), +0x30 its balance (-1 to 1), +0x34 the place where it begins, +0x38 the data of the file (TMemoryFile).
#pragma once

#include <vector>

#include "TSoundBase.h"
#include "WxStub.h"

/** What the engine needs of the stream of a sound (the OpenAL source and its buffers in the original). */
class TSoundStream {
public:
	virtual ~TSoundStream() {}

	/** The stream plays (from the place it is at). */
	virtual void Play() = 0;
	virtual void Pause() = 0;
	virtual void Stop() = 0;
	/** Whether it plays now (not paused or stopped). */
	virtual bool IsPlaying() const = 0;
	/** Whether the sound is over: it has been played to its end (and does not loop). */
	virtual bool HasEnded() const = 0;
	/** The volume (0 to 1) and the place of the sound between the speakers (-1 to 1). */
	virtual void SetGain(float gain) = 0;
	virtual void SetPan(float pan) = 0;
	virtual bool IsLoop() const = 0;
	virtual void SetLoop(bool loop) = 0;
	/** Milliseconds. */
	virtual int GetOffset() const = 0;
	virtual void SetOffset(int milliseconds) = 0;
	virtual int GetDuration() const = 0;
	/** The average volume of the samples that play now. */
	virtual float GetSampleAverage() const = 0;
};

class TFFMPEGSoundItem : public TSoundItem {
public:
	TFFMPEGSoundItem() = default;
	~TFFMPEGSoundItem() override;

	void Play() override;
	void Stop() override;
	void UpdateVolume() override;
	void ResetVolume() override;
	void ScaleVolume(float factor) override;
	int GetVolume() const override;
	int GetPan() const override;
	bool IsPlaying() const override;
	int GetOffset() const override;
	int GetDuration() const override;
	float GetSampleAvg() const override;
	bool IsLoop() const override;
	void Pause() override;

	TSoundStream *_stream = nullptr;  // +0x20
	int _baseVolume = 0;              // +0x28, the volume the sound was given (percent)
	float _volume = 0.0f;             // +0x2C, that volume with the general ones (SetStats, AdjustVolume)
	float _pan = 0.0f;                // +0x30
	int _offset = 0;                  // +0x34, where the sound begins (the milliseconds Play() was given)
};

class TSoundFFMPEG : public TSoundBase {
public:
	TSoundFFMPEG();
	~TSoundFFMPEG() override;

	/** Slot 0x100: the sounds that have ended are let go. */
	void Update() override;
	/** Slot 0x108: whether there is a sound device. */
	bool IsSoundSystemReady() const override;
	/** Slot 0x110: plays the file (the id of the sound; -1: it cannot). `streamed`: the sound is kept when it is over and used
	 *  again for the same file; `fadeIn`: it begins silent and is faded in. */
	int PlaySound(const wxFileName &file, int volume, int balance, bool loop, bool streamed, bool fadeIn,
	              TSoundTypeEnum type, int offset) override;
	/** Slot 0x120 */
	void AdjustVolume(int music, int sound, int speech, int movie, int global) override;
	/** Slot 0x128 */
	bool SetStats(TSoundItem *item, int volume, int balance, TSoundTypeEnum type, bool loop, int offset) override;

	using TSoundBase::SetStats;

	/** The volume (0 to 1) of the whole engine, from the global volume (and the movies). */
	float GetListenerVolume() const {
		return _listenerVolume;
	}
	/** The next id of a sound. */
	static int GetNextUniqueId();
	/** The volume (percent) as the engine takes it, and the balance (-100 to 100) as -1 to 1. */
	float ConvertVolume(int volume) const;
	float ConvertBalance(int balance) const;

protected:
	/** What the backend makes of a file: the stream of its sound (null: the file cannot be played). */
	virtual TSoundStream *CreateStream(const wxFileName &file, bool loop);
	/** The backend changes the volume of the whole engine. */
	virtual void ApplyListenerVolume(float volume);

private:
	void updateFinishedStreams();
	void updateStreamEnded();

	int _movieBase;                     // +0x60 (percent)
	float _adjustedMovieVolume;         // +0x64
	int _globalBase;                    // +0x68 (percent)
	float _adjustedGlobalVolume;        // +0x6C
	float _listenerVolume = 0.0f;
	std::vector<TSoundStream *> _finishedStreams;  // +0x80
};
