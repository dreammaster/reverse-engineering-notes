// Original path confirmed via x_assert() calls:
// src/vsplayer/animationGame.cpp - see manifest/source_layout.tsv.
//
// Confirmed (Deponia_Linux.asm lines 148778-158833): the game's animation
// player, layered on top of TCAnimation (which plays the sprites of one
// animation). A TGAnimation adds what the running game needs:
//  - the animation's frame list (the TTAnimationFrame objects that carry a sound
//    and an action for a sprite index), played in NextSpriteSelected();
//  - the object the animation belongs to (the data's parent, or the character
//    for an outfit's animation), which supplies the rotation, scale, shader and
//    offset the sprites are drawn with;
//  - the owners (TAnimationOwner) that have to be told when it stops;
//  - and, as statics, the list of all running animations (RunningAnimations)
//    with everything to start, hide, preload, find, save and load them.
//
// Original layout: TCAnimation's fields up to +0x78, then +0x78 the owner
// vector, +0x90 the frame list (TVList), +0xA8 the owning object (TVisObjRef);
// 0xB0 bytes in all (THAnimation adds a TEventHandlerInterface at +0xB0).
//
// Animations are all heap allocated (`new THAnimation(state, data)`) and owned by
// RunningAnimations. An animation that has ended (its "active" field is false) is
// deleted by ContinueAnimations() on the next frame unless it was preloaded
// (kAnimationPreloaded), in which case it stays in the list to be shown again.
//
// TODO (low priority, see /TODO.md): the diagnostic overlay of the debugger
// (GetAnimationDetails(), PrintRunningAnimations(), DrawAnimation()) and the 3D
// model and Spine skeleton branches are not reconstructed.
#pragma once

#include <list>
#include <vector>

#include "TAnimationOwner.h"
#include "WxStub.h"
#include "datastruct/vlist.h"
#include "datastruct/visobjref.h"
#include "vscommon/canimation.h"

class TManagedObject;

class TGAnimation : public TCAnimation {
public:
	TGAnimation(const TVisObjRef &active, const TVisObjRef &animation);
	~TGAnimation() override;

	/** Starts the animation (see TCAnimation) and calls the "animation started"
	 *  hook. */
	void Start(bool reverse, float scale) override;
	/** The shown sprite is not needed again: a last loop of an animation that
	 *  is not preloaded, not random and has started. */
	bool CanRemoveCurrentSprite() const override;
	/** Plays what the animation's frames hold for the sprite that was just
	 *  selected: its sound (when the object is on the shown scene) and its
	 *  action. */
	void NextSpriteSelected() const override;

	/** Tells all owners that the animation has stopped, and forgets them. */
	void NotifyOwnersAnimationFinished();
	/** Removes an owner (it must be one). */
	void DetachOwner(TAnimationOwner *owner);
	void SetParallax(int x, int y);

	/** Called before drawing: makes sure the current sprite's picture is loaded. */
	void Prepare();
	/** Draws the current sprite with the properties of the owning object. */
	void Draw(float alpha, unsigned int color, int frame);
	/** Draws the skeleton of a Spine animation mixed with those of `overlays`;
	 *  does nothing without one (see TODO.md). */
	void DrawMixed(float alpha, unsigned int color, const std::vector<TGAnimation *> &overlays);
	void DrawWithLightMap(float alpha, void *lightMap, int frame);

	/** After the game was paused (a menu) the timer is moved on by the time it was
	 *  stopped. */
	void ContinueStoppedAnimation();
	bool IsPreloaded() const;
	void SetStartedByUser(bool startedByUser);

	/** Restores the running state from its saved TSAnimation record (the
	 *  timer, the current sprite). */
	void Load();
	/** Prepares the record for saving: keeps it (with the time it was called) when
	 *  the animation is preloaded or was started by the player and still runs,
	 *  else marks it temporary. */
	void Save();

	// Recovered statics (the original names of the registered hooks and of the
	// list of all animations).
	static void ClearAnimations();
	static void SaveAnimations();
	/** Rebuilds the running animations from the saved TSAnimation records. */
	static void LoadAnimations();
	/** The per-frame update of all running animations; ends and deletes them. */
	static void ContinueAnimations();
	/** Suspend/resume every running animation on entering/leaving a menu scene
	 *  (animations of the scenes that are menus keep running). */
	static void StopRunningAnimations();
	static void ContinueStoppedAnimations();
	/** Hands an animation that was running on `object` before the object was
	 *  rebuilt to the new object. */
	static void ReattachAnimations(TManagedObject &object);

	/** Starts (or shows again) the animation `dataObject` on behalf of `owner` and
	 *  returns it (null when it has no frames, or is a stale one). `frame` (1 based;
	 *  negative: from the start) only applies to a preloaded one that is shown. */
	static TGAnimation *StartAnimation(const TVisObjRef &dataObject, TAnimationOwner *owner, bool reverse,
	                                   float scale, int frame);
	/** Starts an animation for the script interface; always a new one, owned by
	 *  nobody. */
	static TGAnimation *StartLuaAnimation(const TVisObjRef &dataObject, bool reverse, float scale);

	/** Stops `animation` being displayed on behalf of `owner` (a null owner is
	 *  confirmed distinct from a real one at one call site): when it has other
	 *  owners it keeps running. Takes a TManagedObject* directly rather than the
	 *  real TAnimationOwner interface TManagedObject implements it through (see
	 *  TManagedObject.h's own header comment) - TCursorControl needed that
	 *  narrower interface (see the overload below), but nothing here casts a
	 *  TManagedObject* through it, so this overload is unaffected. */
	static void HideAnimation(TGAnimation *animation, TManagedObject *owner);
	/** The same for an owner that only implements the narrower TAnimationOwner
	 *  interface (TCursorControl isn't a TManagedObject). */
	static void HideAnimation(TGAnimation *animation, TAnimationOwner *owner);
	/** Hides whatever animation is currently tied to `dataObject`, wherever it is
	 *  being shown. */
	static void HideAnimation(const TVisObjRef &dataObject);
	/** Frees what was preloaded for `dataObject`'s animation (a running one that
	 *  is preloaded is stopped and deleted). */
	static void UnloadAnimation(const TVisObjRef &dataObject);
	/** Keeps the pictures of `dataObject`'s animation loaded: marks the running
	 *  animation as preloaded, or creates an idle preloaded one. */
	static void PreloadAnimation(const TVisObjRef &dataObject);

	/** Whether the animation of `dataObject` is running. */
	static bool AnimationIsRunning(const TVisObjRef &dataObject);
	/** The animation whose running state (TSAnimation record) is `state`, or the
	 *  one that plays the data object; null when there is none. */
	static TGAnimation *GetAnimationByObject(const TVisObjRef &state);
	static TGAnimation *GetAnimation(const TVisObjRef &dataObject);
	/** The animation `name` of a scene object; with a second name the one of
	 *  that name below the first. An empty reference when there is none. */
	static TVisObjRef GetObjectAnimationByName(const TVisObjRef &object, const wxString &name);
	static TVisObjRef GetObjectAnimationByName(const TVisObjRef &object, const wxString &name,
	                                           const wxString &subName);
	/** The animation `name` of a character, looked up in the animations of its
	 *  outfits. */
	static TVisObjRef GetCharacterAnimationByName(TVisObjRef &character, wxString &name);
	/** The running state of the running animation of that name / data id. */
	static TVisObjRef GetAnimationByName(const wxString &name);
	static TVisObjRef GetAnimationById(int id);
	/** The running state of every running animation. */
	static void GetAnimations(TVList &list);

	static void RegisterEventHandlerAnimStarted(const wxString &handler);
	static void RegisterEventHandlerAnimStopped(const wxString &handler);
	/** The registered names of the Lua event handlers called when an animation
	 *  starts / stops. */
	static wxString GetEventHandlerAnimStarted();
	static wxString GetEventHandlerAnimStopped();

	static unsigned long GetAnimationCount();
	/** The memory the pictures of all running animations take. */
	static long GetSizeOfAllAnimations();
	static std::list<TGAnimation *> &GetRunningAnimations();

private:
	/** The shared tail of the hide functions: ends the animation at `position` of
	 *  the list (and removes it unless it is preloaded). */
	static void hideAnimation(std::list<TGAnimation *>::iterator position, const TVisObjRef &dataObject);

	std::vector<TAnimationOwner *> _owners;  // +0x78
	TVList _frames;                          // +0x90, the TTAnimationFrame objects
	TVisObjRef _ownerObject;                 // +0xA8, the object (or character) it belongs to

	static std::list<TGAnimation *> RunningAnimations;
	static wxString EventHandlerAnimStarted;
	static wxString EventHandlerAnimStopped;
};
