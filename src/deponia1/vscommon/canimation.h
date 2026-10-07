// Original path confirmed via x_assert() calls (TCAnimation::GetCurrentSpritePosition):
// src/vscommon/canimation.cpp - see manifest/source_layout.tsv.
//
// Confirmed (Deponia_Linux.asm lines 1381174-1384914, 158833): a genuinely
// distinct, recovered class name (demangled byte-for-byte from `TCAnimation::...`
// symbols) - TGAnimation.h's own class is layered on top of this one
// (TGAnimation : public TCAnimation).
//
// A TCAnimation plays one animation: the sprites of a TTAnimation data record,
// stepped through in order (forward, backward or at random), each shown for its
// own pause (or the animation's), for a number of loops. Its running state is
// kept in a TSAnimation record (a savegame object, so the position in the
// animation survives saving): the loops left, the current sprite index, the
// position and scale, whether it is running and so on.
//
// Original layout: +0x08 the TSAnimation state, +0x10 the TTAnimation data
// object, +0x18 the TPictureIO list (one per sprite), +0x30 the current sprite
// (null when none is shown), +0x38 the rendered sprite of a model, +0x40 the
// ModelAnimation, +0x48 the SpineSkeleton, +0x50 its animation, +0x58 a path,
// +0x60 the model's tick, +0x64 a flag set for the animations of outfits (and
// models), +0x65 paused, +0x66 a flag set by Start()/read elsewhere, +0x68 a
// TTimer. The virtuals are Start(), CanRemoveCurrentSprite(),
// NextSpriteSelected(), GetAnimationLoops() and WaitBetweenLoops().
//
// NOT reconstructed: the 3D-model (ModelContainer/ModelAnimation) and Spine
// skeleton (SpineContainer/SpineSkeleton) animation kinds the original also
// plays through this class. They are not used by 2D sprite animations (the whole
// of Deponia 1's); every branch for them is left out, so IsModelAnimation()/
// IsBonesAnimation() are always false and an animation of those kinds plays no
// sprites.
#pragma once

#include <vector>

#include "TTimer.h"
#include "WxStub.h"
#include "datastruct/visobjref.h"

class TPictureIO;
class TSprite;

class TCAnimation {
public:
	TCAnimation(const TVisObjRef &active, const TVisObjRef &animation);
	virtual ~TCAnimation();

	/** Starts the animation: the loop count and direction from the data
	 *  (`reverse` flips it), at the data's position and the given scale. */
	virtual void Start(bool reverse, float scale);
	/** Whether the shown sprite can be released when it is replaced (it is not
	 *  needed again: the last loop of a sequential animation). */
	virtual bool CanRemoveCurrentSprite() const;
	/** Called after the sprite index changed; does nothing here. */
	virtual void NextSpriteSelected() const;
	/** The loops that are left (0 = endless). */
	virtual int GetAnimationLoops() const;
	/** Whether a random-loop animation waits between its loops (always). */
	virtual bool WaitBetweenLoops() const;

	void SetPaused(bool paused) {
		_paused = paused;
	}
	TVisObjRef GetDataObject() const {
		return _data;
	}
	/** The TSAnimation record that holds the running state. */
	const TVisObjRef &GetState() const {
		return _state;
	}

	void RefreshSprites();
	/** Starts loading the pictures; the first one only unless `all`. */
	void PreloadSprites(bool all);
	void RemoveSprites();

	/** Selects the first / next sprite of a pass (by direction and order) and
	 *  calls NextSpriteSelected(). */
	void FirstSprite();
	void NextSprite();
	/** Whether the pass through the sprites is over. */
	bool EofSprite() const;

	/** The pause of the current sprite (its own, if the data object lets
	 *  sprites have their own, else the animation's) in milliseconds, and how
	 *  far into it the animation is (0 to 1). */
	int GetCurrentPause() const;
	float GetPauseCompletion() const;
	/** Steps to the next sprite when the pause has run out (or `force`), handles
	 *  the end of a pass (loops, random waits, finishing) and sets the shown
	 *  sprite. */
	void SetCurrentSprite(bool force);

	TPictureIO *GetCurrentSprite() const {
		return _currentSprite;
	}
	int GetCurrentSpriteIndex() const;
	int GetCurrentSpriteIndexOrTick() const;
	int GetFrameCount() const;
	void SetPosition(const wxPoint &position, float scale);
	/** Where the current sprite is drawn: its position (mirrored sprites by the
	 *  animation's mirror offset) scaled by the scale and offset by the position. */
	wxPoint GetCurrentSpritePosition() const;
	long GetCalledTime() const;

	bool IsSpriteIndexValid() const;
	bool IsRandomOrder() const;
	bool IsLoopRandom() const;
	bool IsFinished() const;
	bool IsWaiting() const;
	bool IsEndless() const;
	bool IsMoveAnimation() const;
	bool IsSpriteAnimation() const {
		return true;
	}
	bool IsModelAnimation() const {
		return false;
	}
	bool IsBonesAnimation() const {
		return false;
	}

protected:
	/** The pause (ms) the current sprite is shown for. */
	int currentPause() const;
	/** Starts the wait between two loops of a random-loop animation. */
	void startRandomWait();

	TVisObjRef _state;   // +0x08, a TSAnimation
	TVisObjRef _data;    // +0x10, a TTAnimation
	std::vector<TPictureIO *> _sprites;  // +0x18
	TPictureIO *_currentSprite;          // +0x30
	bool _isOutfit;      // +0x64 (the animation belongs to an outfit)
	bool _paused;        // +0x65
	bool _flag66;        // +0x66
	TTimer _timer;       // +0x68
};
