// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 175318-186239, 76 methods): an in-game
// character - what walks around a scene. TGCharacter derives from TMCharacter
// (see there), which is a TManagedObject; the reference to its game-data object
// is the inherited _objRef (TGameControl::IsTalking/SetCharacterActiveCommand
// reach it as a field at a known offset).
//
// What it does:
//  - It stands, talks, walks, turns and plays "random" animations; the animation
//    that is shown is the base class's _currentAnimation (kCharacterAnimState,
//    field 0x1FC, tells which kind: TCharacterAnimEnum). Which animation of the
//    current outfit is used for a direction is found by GetDirectionIndex().
//  - It walks along the way a TGWaySystem finds (SetFreeDestination() asks for it,
//    WalkWay() takes the steps every frame, StopWalking() ends it).
//  - It talks with the comments of its comment sets (ShowComment()), carries items
//    (GiveItemTo(), ReceiveItem()), and tells the interfaces about them.
//  - It plays the walking sound while it walks, and starts its random animation
//    when its random timer runs out.
//
// Not reconstructed (see TODO.md): the branches for characters shown as Spine
// skeletons (IsBonesAnimation()), where several animations run at once (the
// animation kind list below holds only the one kind of the one animation here), and
// the scene being drawn through a matrix. Only the sprite animations of the
// original run through the rest.
#pragma once

#include <list>
#include <vector>

#include "TGWaySystem.h"
#include "TMCharacter.h"
#include "TTimer.h"
#include "WxStub.h"
#include "datastruct/visobjref.h"
#include "datastruct/vlist.h"
#include "vstables/records.h"

class TGInterface;
class TGItem;
class TSceneActionArea;

/** The kinds of animation a character plays (the values of kCharacterAnimState and
 *  the kind the Start/StopCharacterAnim functions take; 0 is none). */
enum class TCharacterAnimEnum {
	kNone = 0,
	kWalk = 1,
	kTalk = 2,
	kStand = 3,
	kCharacterAnim = 4,  // an animation started by a script (IsCharacterAnimRunning())
	kRandom = 5,
	kTurn = 6
};

/** The comments of a comment set that are for one command (an empty one: for any). */
struct SCommentSetEntries {
	TVisObjRef set;
	TVList entries;
};

/** The position (a debugging aid) of the last place SetFreeDestination() could not
 *  find a way to. */
extern wxPoint failedPoint;

/** The difference of two directions in degrees (0 to 180). */
int diff(int first, int second);
/** The direction mirrored at the vertical axis. */
int mirror(int direction);

class TGCharacter : public TMCharacter {
public:
	TGCharacter(const TVisObjRef &ref, const TVisObjRef &scene);
	~TGCharacter() override;

	// Virtuals (see TManagedObject for the slots).
	void AnimationStopped(TGAnimation *animation) override;
	void ExecuteEvent(TGEventInfo &info) override;
	bool IsReached(const TVisObjRef &target) const override;
	void AlignCharacter(TVisObjRef &character) const override;
	void ClickedWithoutReach(TGCharacter *character, TMouseEventEnum event) override;
	void Draw() override;
	void Load() override;
	void Save() override;
	void SetAlpha() override;
	bool ReceiveItem(TGEventInfo &info) override;
	wxRect GetCurrentSpriteRect() const override;

	// Confirmed present at a fixed offset (TGameControl::IsTalking compares
	// a TGText's speaker against a TVisObjRef via this field directly,
	// Deponia_Linux.asm lines 461785-461846) - same "TVisObjRef at a known
	// offset, no accessor in the original" pattern as TGDialog/TSText/TGText.
	const TVisObjRef &GetRef() const {
		return _objRef;
	}
	// Confirmed mutated directly (TGameControl::SetCharacterActiveCommand
	// calls TVisObjRef::SetLink() on this field in place, asm lines
	// 465679-465841).
	TVisObjRef &GetRef() {
		return _objRef;
	}

	// Confirmed static (no implicit `this` - asm lines 178297-178307): `direction` is
	// a 0-359 compass value, and the result wraps the same way.
	static int GetOppositeDirection(int direction) {
		int sum = direction + 180;
		return sum >= 360 ? direction - 180 : sum;
	}
	/** The registered name of the Lua function that chooses the animation index for a
	 *  direction ("CharacterDirectionHook"). */
	static void RegisterHookFunctionGetCharacterAnimationIndex(const wxString &name);

	// Setting a character up.
	/** Takes the outfit, comment set, walking sound and random time from the data. */
	void Init();
	/** Puts the character in a scene at a position (and turns it to `direction` unless
	 *  that is -1); runs the actions of the action areas it is in. */
	void AssignToScene(const TVisObjRef &scene, const wxPoint &position, int direction);
	/** Makes the character wear an outfit: its animations of each kind (and the
	 *  directions they are for) are collected. */
	void SetCurrentOutfit(const TVisObjRef &outfit);
	void SetCurrentCommentSet(const TVisObjRef &commentSet);
	/** Sets the way system the character walks on (from a scene's way system object). */
	void SetWaySystem(const TVisObjRef &waySystem, bool init);
	/** Sets the way system's size scaling up and works out the character's size
	 *  at its position (confirmed in full, asm lines 175894-175958). */
	void InitWaySystem(int spriteHeight);
	/** Moves the character into the walkable area if it is not in it (confirmed in
	 *  full, asm lines 175992-176011). */
	void CheckCharacterPosition();
	TGWaySystem &GetWaySystem() {
		return _waySystem;
	}
	void SetInterfaces();
	std::list<TGInterface *> GetInterfaces() const;

	// Walking.
	/** Walks to `destination` (a way is searched; none found: stays where it is). */
	/** Whether the walk animation waits for a steady direction (+0xB8; ShowFrame sets it while a controller walks the character). */
	void SetHarmonizeWalk(bool harmonize) {
		_harmonizeWalk = harmonize;
	}
	void SetFreeDestination(wxPoint destination, bool keepDestinationObject, bool noWayPoints,
	                        bool useTriangles);
	/** The per-frame step along the way. */
	void WalkWay();
	/** The per-frame look at who the character follows / who it is to act on. */
	void UpdateCharacter();
	/** Ends the walk where the character is; `restartAnim`: starts the standing
	 *  (or talking) animation. */
	void StopWalking(bool restartAnim);
	/** Puts the character on the end of its way at once. */
	void SetOnDestination();
	/** Whether it stands (or is on its way to) the place of a character; `strict`
	 *  tests for the exact place of a destination. */
	bool IsReached(const TVisObjRef &target, bool strict) const;
	bool IsWalking() const;
	bool IsWalkingToWayPoint() const;
	bool StandingAt(const wxPoint &position) const;
	/** Turns the character to a direction (starting the turn animation if it has
	 *  one for it). */
	void CheckTurning(int direction);
	void SetCurrentDirection(int direction);
	/** Which animation (index into the list of the kind) a direction is for. */
	int GetDirectionIndex(int direction, TCharacterAnimEnum kind);
	int GetStandingDirection() const;
	/** The way point nearest to the position. */
	TTPoint GetPointNextTo(wxPoint &position);
	/** After a click on `other` this character walks to it. */
	void SetActionCharacter(TGCharacter &other, TMouseEventEnum event);
	bool TurnCharacter();

	// Animations.
	void StartWalkAnim();
	void StartStandingAnim();
	void StartTalkAnim();
	void StartRandomAnim();
	void StartFittingAnimation();
	/** Starts the animation `data` as an animation of `kind`; returns it. */
	TGAnimation *StartCharacterAnim(TVisObjRef &data, TCharacterAnimEnum kind, bool flag);
	void StopCharacterAnim(TCharacterAnimEnum kind, bool restart);
	void StopWalkAnim();
	void StopTalkAnim();
	void StopStandingAnim();
	bool IsCharacterAnimRunning() const;
	TVisObjRef GetCharacterAnim();
	bool IsSpineAnimation() const;
	void ClearSprites();
	void PreloadAnimations();
	void UnloadAnimations();
	void AllowUnloadingOutfitAnimations(bool allow);
	/** Works out the character's size (percent) at its position. */
	void SetCurrentSize();
	/** The size of the sprite shown. */
	wxSize GetCurrentDimension() const;
	void UpdateSpriteRect();
	void SetSpritePosition();
	/** The time the random animation starts after the character did nothing. */
	void SetRandomTime();
	void CheckRandomTimer();
	void AdjustTimers();

	// Walking sound.
	bool IsWalkingSoundPlaying() const;
	wxFileName GetWalkingSound() const;
	void SetCurrentWalkingSound(const wxFileName &file);
	void CheckWalkingSound();
	void StopWalkingSound();

	// Items and talking.
	bool GiveItemTo(TVisObjRef &receiver, const TVisObjRef &item);
	void GiveAllItemsTo(TVisObjRef &receiver);
	/** The character says a comment for `command` (one of its comment sets). */
	void ShowComment(const TVisObjRef &command);
	void UpdateActionAreas();
	void AlignToObject(const TVisObjRef &object);

protected:
	std::list<TGInterface *> _interfaces;        // +0xC0
	std::vector<TGItem *> _items;                // +0xD0 (not used so far)
	wxRealPoint _realPosition;                   // +0xE8, the position with its fractions
	int _state;                                  // +0xF0, kCharacterState: 2 standing, 3 walking, 4 anim
	int _previousState;                          // +0xF4, the state to return to after state 4
	int _directionIndex;                         // +0xF8, the standing animation's index
	std::vector<int> _turnFrom;                  // +0x100, the directions a turn animation turns from
	std::vector<int> _turnTo;                    // +0x118, ... and to
	std::vector<int> _walkDirections;            // +0x130, the directions of the walk animations
	TTimer _walkTimer;                           // +0x148, since the last step
	wxFileName _walkingSound;                    // +0x158
	bool _hasWalkingSound;                       // +0x160, the sound file is valid
	bool _walkingSoundPlaying;                   // +0x161
	TGWaySystem _waySystem;                      // +0x168, what the character walks on
	TVisObjRef _outfit;                          // +0x250
	TTimer _idleTimer;                           // +0x270
	int _walkFrame;                              // +0x280, the walk animation frame last stepped on
	bool _allowUnloadingOutfitAnimations;        // +0x284
	TVList _walkAnimations;                      // +0x288
	TVList _talkAnimations;                      // +0x2A0
	std::vector<int> _talkDirections;            // +0x2B8
	TVList _standAnimations;                     // +0x2D0
	std::vector<int> _standDirections;           // +0x2E8
	TVList _turnAnimations;                      // +0x300, each four times (see CheckTurning)
	TTimer _randomTimer;                         // +0x318
	int _randomTime;                             // +0x328, ms until the random animation
	TTimer _followTimer;                         // +0x330
	int _followReach;                            // +0x340
	std::vector<TCharacterAnimEnum> _animKinds;  // +0x348, the kind of each running animation
	std::vector<SCommentSetEntries *> _commentSets;  // +0x360
	bool _hasWaySystem;                          // +0x378
	std::list<TSceneActionArea *> _activeAreas;  // +0x380, the action areas it is in now
	int _lastWalkIndex;                          // +0x390, the last two walk animation indices
	int _lastWalkIndex2;                         // +0x394
	bool _harmonizeWalk;                         // +0xB8, wait for a steady direction to switch walk animations

	/** The registered name of the Lua function that chooses the animation index for a
	 *  direction (the original's recovered static of this name; only stored, see
	 *  GetDirectionIndex()). */
	static wxString HookFunctionGetCharacterAnimationIndex;
};
