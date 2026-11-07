// Confirmed (Deponia_Linux.asm: the vtable `TSoundItem`/`TFFMPEGSoundItem` at line 3404146, and the way TSoundBase
// (soundBase.cpp) uses the objects): one sound of the sound engine. It has a name (the file, without what follows a '#'),
// an id that the scripts use, and the state the engine keeps of it; what it does to the sound itself is what the
// virtual functions say (the slot is the offset in the vtable / 8).
#pragma once

#include "TSoundInterface.h"
#include "WxStub.h"

class TSoundItem {
public:
	TSoundItem() = default;
	virtual ~TSoundItem() {
	}

	/** Slot 0x10: the sound plays (goes on after a pause). */
	virtual void Play() = 0;
	/** Slot 0x18 */
	virtual void Stop() = 0;
	/** Slot 0x20: the volume of the sound is set from its own volume. */
	virtual void UpdateVolume() = 0;
	/** Slot 0x28: the volume of the sound is 0 (the sounds are muted). */
	virtual void ResetVolume() = 0;
	/** Slot 0x30: the volume of the sound is its own volume times `factor` (a fade). */
	virtual void ScaleVolume(float factor) = 0;
	/** Slots 0x38 and 0x40: the volume (percent) and the balance (-100 to 100). */
	virtual int GetVolume() const = 0;
	virtual int GetPan() const = 0;
	/** Slot 0x48 */
	virtual bool IsPlaying() const = 0;
	/** Slots 0x50 and 0x58: where the sound is and how long it is, in milliseconds (-1: not known). */
	virtual int GetOffset() const = 0;
	virtual int GetDuration() const = 0;
	/** Slot 0x60: the average volume of the samples that play now. */
	virtual float GetSampleAvg() const = 0;
	/** Slot 0x68 */
	virtual bool IsLoop() const = 0;
	/** Slot 0x70 */
	virtual void Pause() = 0;

	wxString _name;                  // +0x08, the file (up to a '#')
	int _id = 0;                     // +0x10
	bool _paused = false;            // +0x14
	bool _stopped = false;           // +0x15, the sound is over, but kept for the next use (_streamed)
	bool _streamed = false;          // +0x16, a sound that is kept when it is over, for the same file
	bool _dead = false;              // +0x17, the sound has ended and is let go (the engine does that)
	TFadeEnum _fade = TFadeEnum::kNone;  // +0x18, what it does in the fade of the sounds
	TSoundTypeEnum _type = TSoundTypeEnum::kMusic;  // +0x1C
};
