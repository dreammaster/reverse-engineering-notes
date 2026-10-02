// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed in full except ExecuteMatchingAction()/HandlePostExecution()/
// GetActionsToTest()/ExecuteEvent() (Deponia_Linux.asm lines 114890-194554,
// reaching all 50 manifest-listed methods, including GetDirection() - found
// late, while cross-checking this class's real vtable layout during the
// TGObjectManager pass, and added here): the shared base for every
// interactive scene object (TGCharacter already derives from it; TGObject/
// TGItem and others are referenced as doing so too, from other classes'
// call sites, but aren't modeled in this codebase yet). Owns a game-data
// reference, a position/bounding-rect/polygon hit-test area, a primary
// animation plus a list of secondary overlay animations, an optional
// picture and text, a fade-alpha system driven by a TTimer, and an
// active/lifetime pair.
//
// The real class multiply-inherits two small pure-abstract mixins, both
// confirmed by name from their own recovered vtables/RTTI sitting right
// after this class's own in the binary: TAnimationOwner (3 pure virtuals -
// AnimationStopped()/GetOwnerName()/GetOwnerId(), the first 3 slots of this
// class's own primary vtable; TCursorControl reaches them through a
// `_ZThn72_`-style this-adjusting thunk, consistent with holding a
// TAnimationOwner* interface pointer into a larger composite object) and
// TTextOwner (1 pure virtual - TextFinished(), reached from a secondary
// vtable fragment at this object's +0x10 via a constant `_ZThn16_`
// adjustment). Modeled here as a single ordinary class with virtual
// methods instead of replicating the two-vtable ABI trick - nothing in
// this codebase casts a TManagedObject* through a narrower
// TAnimationOwner*/TTextOwner* pointer obtained some other way, so there's
// no behavioral difference.
//
// The vtable cross-check also caught two errors from this class's original
// pass: an invented "OnAlphaChanged()" hook that doesn't exist - the real
// vtable slot SetDestAlpha()/UpdateAlpha() call through (+0xE0) is just
// SetAlpha() itself, already a real, public, confirmed-no-op virtual here;
// and an invented "GetAnimationFrameOverride()" name for what is actually
// the recovered GetDirection() (+0xE8) - same confirmed -1 default, just
// under its real name. Both are fixed in this version.
//
// ExecuteMatchingAction()/HandlePostExecution()/GetActionsToTest()/
// ExecuteEvent() form the actual condition/action-matching engine behind
// Visionaire's scripted events. Implemented in full (Deponia_Linux.asm
// lines 190613-194544) after a dedicated follow-up pass named
// TGEventInfo/TGActionInfo's own fields and TypeActionExecution's 29
// confirmed values (see TGEventInfo.h/TGActionInfo.h) - most fields are
// named by position rather than guessed meaning, the same convention this
// project already used for TGDetectInfo/TypeOrder, since their underlying
// semantics are still opaque even though their structural role in the
// control flow is now fully confirmed. (Established call sites confirm
// TGItem/TGObject override at least HandlePostExecution()/
// AnimationStopped(), so other subclasses are already known to
// participate in this engine too.)
#pragma once

#include <algorithm>
#include <vector>

#include "TGAnimation.h"
#include "TGActionInfo.h"
#include "TGEventInfo.h"
#include "TTimer.h"
#include "WxStub.h"
#include "datastruct/visionaireobject.h"
#include "datastruct/visobjref.h"
#include "vscommon/scripting/id.h"

class TGCharacter;
class TGText;
class TPictureIO;
class TVList;
class TGDetectInfo;
enum class TMouseEventEnum;

// Confirmed call shape only (TManagedObject::ExecuteMatchingAction,
// Deponia_Linux.asm line 55E1E8) - computes some kind of facing angle from
// two screen-space deltas; not reversed beyond that call shape. Returns
// int (the real callee's result lands in a plain integer register, not
// xmm0, despite taking float arguments).
int GetAngle(float dx, float dy);

// Confirmed present (TManagedObject::SetPolygon/IsInside, Deponia_Linux.asm
// lines 191660-191767, 191159-191170) as a 3-qword (begin/end/capacity)
// member - modeled as a plain point list rather than reversing whatever
// internal multi-polygon structure the original used, since nothing here
// depends on more than "the set of points IsPointInsidePolygon() tests
// against".
using TPolygonList = std::vector<wxPoint>;

// Confirmed call shape only (TManagedObject::SetPolygon) - real behavior
// (validating/splitting a raw point list into the original's own polygon
// representation) not reversed; this always accepts the input as-is.
bool CreatePolygonsFromPointList(const std::vector<wxPoint> &points, TPolygonList &outPolygons);
// Confirmed call shape only (TManagedObject::SetPolygon) - the axis-aligned
// bounding box of every point in `polygons`; empty if there are none.
wxRect GetBoundingBox(const TPolygonList &polygons);
// Confirmed call shape only (TManagedObject::IsInside) - a standard
// even-odd ray-casting point-in-polygon test over `polygon` as a single
// point ring.
bool IsPointInsidePolygon(const wxPoint &pt, const TPolygonList &polygon);

class TManagedObject {
public:
	TManagedObject() = default;
	explicit TManagedObject(const TVisObjRef &objRef) : _objRef(objRef) {
	}
	virtual ~TManagedObject() = default;

	// Confirmed (asm lines 190278-190309): clears _text if it's the one
	// finishing.
	void TextFinished(TGText *text) {
		if (_text == text)
			_text = nullptr;
	}

	wxPoint GetPosition() const {
		return _position;
	}
	// Confirmed accessed directly as a private field from TGObjectManager
	// (e.g. IsCurrentObjectEmpty/IsCurrentObjectDetectable/GetCurrentObject,
	// Deponia_Linux.asm lines 189114-189460) - modeled as a public accessor
	// instead of a cross-class friendship, matching TGCharacter::GetRef()'s
	// own already-established pattern for the same field.
	const TVisObjRef &GetRef() const {
		return _objRef;
	}
	// Confirmed virtual (called polymorphically by CompObjectCenter below;
	// this class's own body is a plain field read either way).
	virtual int GetCenter() const {
		return _center;
	}
	// Confirmed no-ops in the base (asm lines 190372-190412) - meant to be
	// overridden by subclasses that care.
	virtual void SetActiveSprite(bool /*active*/) {
	}
	virtual void SetAlpha() {
	}
	virtual void ShowSnoopAnimation(bool /*show*/) {
	}
	// Confirmed (asm line 190356-190364): the base always returns -1; Draw()
	// below passes this through as the primary animation's per-frame
	// override.
	virtual int GetDirection() const {
		return -1;
	}

	// Confirmed (asm lines 191134-191171): active, inside the bounding
	// rect, and inside the hit-test polygon.
	virtual bool IsInside(const wxPoint &pt) const {
		if (!_active)
			return false;
		if (!_boundingRect.Contains(pt))
			return false;
		return IsPointInsidePolygon(pt, _polygons);
	}
	// Confirmed (asm lines 190420-190430): the base ignores `info` entirely,
	// degenerating to the one-argument overload above (a subclass override
	// of either overload still takes effect normally through virtual
	// dispatch).
	virtual bool IsInside(const wxPoint &pt, const TGDetectInfo &/*info*/) const {
		return IsInside(pt);
	}
	virtual bool IsWalkable() const {
		return false;
	}
	// Confirmed no-op in the base (asm lines 190455-190463).
	virtual void AlignCharacter(TVisObjRef &/*character*/) const {
	}

	// Confirmed no-ops/trivial defaults in the base (asm lines 190471-190526,
	// 194554-194560) - meant to be overridden by subclasses with real
	// actions/items/text to offer.
	virtual void HandlePreExecution(TGEventInfo &/*info*/) {
	}
	virtual bool ReceiveItem(TGEventInfo &/*info*/) {
		return false;
	}
	virtual void Load() {
	}
	virtual void Save() {
	}
	virtual void GetActionList(TVList &/*outActions*/) const {
	}
	virtual void DrawSnoopAnimation() {
	}

	// Confirmed (asm lines 190536-190602): clears the primary animation
	// slot if it matches, and removes `animation` from the secondary list
	// either way.
	virtual void AnimationStopped(TGAnimation *animation) {
		if (_currentAnimation == animation)
			_currentAnimation = nullptr;
		auto it = std::find(_animations.begin(), _animations.end(), animation);
		if (it != _animations.end())
			_animations.erase(it);
	}

	// Confirmed in full (Deponia_Linux.asm lines 190613-190825) - see this
	// class's own header comment and TGActionInfo.h for the flag names
	// used below. (Needs TGCharacter's full definition, so implemented in
	// the .cpp.)
	virtual void HandlePostExecution(TGEventInfo &info, const TGActionInfo &actionInfo);
	// Confirmed in full (Deponia_Linux.asm lines 191775-192289). (Needs
	// TGCharacter's/TTAction's full definitions, so implemented in the
	// .cpp.)
	virtual void ExecuteMatchingAction(TVList &candidates, std::vector<TypeActionExecution> &types,
	                                   const TGEventInfo &info, TGActionInfo &outAction);
	// Confirmed in full (Deponia_Linux.asm lines 193086-193740) - a switch
	// on `info.mouseEvent` that fills `outTypes` with the
	// TypeActionExecution candidates worth testing for this event. (Needs
	// TGCharacter's full definition, so implemented in the .cpp.)
	virtual void GetActionsToTest(TGEventInfo &info, std::vector<TypeActionExecution> &outTypes,
	                              TGActionInfo &outAction);
	// Confirmed in full (Deponia_Linux.asm lines 193750-194544): calls
	// HandlePreExecution(), gets the candidates to test, optionally aligns
	// the event's own character towards this object, then tries
	// ExecuteMatchingAction() against this object's own action list - with
	// a mouse-event-3/4 retry (forcing info.flag8 true) and, if
	// `_hasActionTypeFallback` is set, a second retry against a hardcoded
	// expansion of the original candidate types and a different action
	// list (field 0xAC) when the first two attempts found nothing. (Needs
	// TGCharacter's full definition, so implemented in the .cpp.)
	virtual void ExecuteEvent(TGEventInfo &info);

	// Confirmed (asm lines 190835-190852): records this object as the
	// "clicked without being in reach" target and forwards the mouse event
	// to the global object manager.
	void ClickedWithoutReach(TGCharacter *character, TMouseEventEnum event);

	// Confirmed (asm lines 190861-190890): the given target's own stored
	// screen position matches this object's GetScreenPosition().
	bool IsReached(const TVisObjRef &target) const {
		const wxPoint *pt = target.GetPoint(0x200);
		return pt && *pt == GetScreenPosition();
	}

	// Confirmed (asm lines 190899-190938): deactivating hides (and clears)
	// the primary animation; activating just flips the flag.
	virtual void SetActive(bool active) {
		if (_currentAnimation && _active && !active) {
			TGAnimation::HideAnimation(_currentAnimation, this);
			_currentAnimation = nullptr;
		}
		_active = active;
	}
	bool IsActive() const {
		return _active;
	}

	// Confirmed (asm lines 190946-190972): releases the picture's sprite
	// and the primary animation's sprites, if present. (Needs TPictureIO's
	// full definition, so implemented in the .cpp.)
	void RemoveSprites();

	// Confirmed (asm lines 190980-191074): prepares the primary animation
	// (unless it's a bones animation, which is prepared via the secondary-
	// animation loop below instead) or refreshes the plain picture if
	// there's no primary animation; always prepares every sprite-valid
	// secondary animation. The original re-derives this same "is it a
	// bones animation" outcome through 3 different paths (checking
	// IsSpriteIndexValid()/IsModelAnimation() first) that all funnel into
	// one IsBonesAnimation()-based decision - simplified here to that one
	// check, since IsSpriteIndexValid()/IsModelAnimation() are pure queries
	// with no effect on the actual outcome. (Needs TPictureIO's full
	// definition, so implemented in the .cpp.)
	void Prepare();

	// Confirmed (asm lines 191082-191099): TId from the game-data
	// reference's packed id - approximated via the project's existing
	// PackVisId() helper (the original instead reinterprets TVisObjRef::
	// GetId()'s return as an existing TId object outright, implying TId
	// and the packed id are somehow layout-compatible in the original;
	// neither TId's nor TVisObjRef's real fields are known, so that aliasing
	// trick isn't reproduced).
	virtual TId GetOwnerId() const {
		return TId(PackVisId(_objRef.GetId()), 0);
	}
	// Confirmed (asm lines 191107-191124): the game-data reference's own
	// name.
	virtual wxString GetOwnerName() const {
		return _objRef.GetName();
	}

	// Confirmed (asm lines 191206-191233): compares GetCenter() - called
	// polymorphically on both sides, matching GetCenter()'s own virtual-ness
	// above.
	static bool CompObjectCenter(const TManagedObject *a, const TManagedObject *b) {
		return a->GetCenter() < b->GetCenter();
	}

	// Confirmed (asm lines 191241-191261): hides the primary animation (if
	// any) and empties the secondary list, keeping its capacity.
	void ClearAnimations() {
		if (_currentAnimation)
			TGAnimation::HideAnimation(_currentAnimation, this);
		_currentAnimation = nullptr;
		_animations.clear();
	}
	// Confirmed (asm lines 191269-191286).
	virtual bool IsMovingObject() const {
		return _currentAnimation && _currentAnimation->IsMoveAnimation();
	}
	// Confirmed (asm lines 191294-191304).
	wxRect GetBoundingRect() const {
		return _boundingRect;
	}

	// Confirmed in full (asm lines 191329-191403): clamps target to
	// [0,100], snapshots the current alpha as the fade's "from" value, and
	// either arms a gradual fade (duration != 0, applied frame-by-frame via
	// UpdateAlpha()) or - when old and new differ - applies it immediately
	// (duration == 0; the original's own "duration == 0" branch computes an
	// elapsed-vs-duration interpolation too, but elapsed is never negative
	// so it always resolves to "snap to target" in practice).
	void SetDestAlpha(int targetPercent, int durationMs) {
		int clamped = std::max(std::min(targetPercent, 100), 0);
		_alphaDurationMs = durationMs;
		_alphaFrom = _alpha;
		_alphaTarget = static_cast<float>(clamped) / 100.0f;
		if (durationMs == 0 && _alphaFrom != _alphaTarget) {
			_alpha = _alphaTarget;
			SetAlpha();
		}
		_timer.SetTime();
	}
	// Confirmed in full (asm lines 191411-191458): the per-frame fade step -
	// lerps from _alphaFrom to _alphaTarget over _alphaDurationMs, clamping
	// to the target once elapsed time catches up.
	void UpdateAlpha() {
		if (_alphaTarget == _alpha)
			return;
		long elapsed = _timer.GetTime();
		_alpha = (elapsed >= _alphaDurationMs || _alphaDurationMs == 0)
		         ? _alphaTarget
		         : _alphaFrom + (_alphaTarget - _alphaFrom) * (static_cast<float>(elapsed) /
		             static_cast<float>(_alphaDurationMs));
		SetAlpha();
	}
	float GetAlpha() const {
		return _alpha;
	}

	// Confirmed (asm lines 191484-191528): x_assert(text != nullptr) skipped
	// per this project's usual treatment; hands this object to the text as
	// its owner.
	virtual void SetText(TGText *text);

	// Confirmed (asm lines 191536-191575): like ClearAnimations(), but hides
	// every secondary animation too (with a null owner, unlike the primary
	// slot's HideAnimation(..., this) - a confirmed asymmetry, not an
	// oversight) rather than just discarding them.
	void RemoveAnimations() {
		if (_currentAnimation)
			TGAnimation::HideAnimation(_currentAnimation, this);
		_currentAnimation = nullptr;
		for (TGAnimation *anim : _animations)
			TGAnimation::HideAnimation(anim, static_cast<TManagedObject *>(nullptr));
		_animations.clear();
	}

	void SetLifetime(int lifetime) {
		_lifetime = lifetime;
	}
	int GetLifetime() const {
		return _lifetime;
	}
	// Confirmed (asm lines 191617-191631): floored at 0.
	void DecreaseLifetime() {
		_lifetime = std::max(_lifetime - 1, 0);
	}

	// Confirmed (asm lines 191639-191767): rebuilds the hit-test polygon
	// from a raw point list and refreshes the cached bounding rect from it;
	// logs (not byte-exact, see CreatePolygonsFromPointList's own comment)
	// when the point list couldn't be turned into a polygon.
	void SetPolygon(std::vector<wxPoint> &points) {
		bool ok = CreatePolygonsFromPointList(points, _polygons);
		if (!ok && wxLog::loglevel > 0) {
			wxString fmt;
			toUTF(&fmt, "Invalid polygon for object '%s' (id %d)");
			wxLog::logexpanded(fmt.wc_str(), _objRef.GetNameWithParents(3).wc_str(), PackVisId(_objRef.GetId()));
		}
		_boundingRect = GetBoundingBox(_polygons);
	}

	// Confirmed (asm lines 192564-192696): an animation whose own data
	// object matches this object's "0xAB" link becomes the primary
	// animation (hiding whatever was primary before, if different);
	// anything else is added to the secondary list instead (without
	// duplicating an entry already present).
	void SetAnimation(TGAnimation *animation) {
		if (!animation)
			return;
		if (_objRef.GetLink(0xAB) == animation->GetDataObject()) {
			if (_currentAnimation && _currentAnimation != animation)
				TGAnimation::HideAnimation(_currentAnimation, this);
			_currentAnimation = animation;
			return;
		}
		if (std::find(_animations.begin(), _animations.end(), animation) == _animations.end())
			_animations.push_back(animation);
	}

	// Confirmed (asm lines 192704-192913) except the Lua "ObjectRenderHook"
	// support at the top (skipped - the same standing Lua-bridge-contract
	// gap used throughout this project; it would only ever trigger for a
	// game script that registers TManagedObject::HookFunctionRender, which
	// nothing in this reimplementation can do anyway). Updates the fade,
	// then draws the primary animation (or DrawMixed() with the secondary
	// list, for a bones animation) or the plain picture if there's no
	// primary animation; then the text, if any; then every sprite-valid
	// secondary animation on top.
	// (Needs TPictureIO's and TGText's full definitions, so implemented in
	// the .cpp.)
	virtual void Draw();

protected:
	TVisObjRef _objRef;

	// Confirmed virtual (IsReached() above calls it polymorphically; no
	// TManagedObject-level body is confirmed, so this default - falling
	// back to the plain stored position - is a reasonable guess, not
	// recovered evidence).
	virtual wxPoint GetScreenPosition() const {
		return _position;
	}

	// Confirmed a bool field at a fixed offset distinct from every field
	// below (GetActionsToTest()/ExecuteEvent()/ExecuteMatchingAction(),
	// Deponia_Linux.asm lines 55EC79, 55F3FA, 55E027) - when set,
	// GetActionsToTest() treats this object as reached without actually
	// calling IsReached(), and ExecuteEvent()/ExecuteMatchingAction() skip
	// the auto-facing-angle update they'd otherwise apply to the event's
	// own character; real meaning/intent not resolved (plausibly "this
	// kind of object doesn't participate in reach-distance mechanics at
	// all," e.g. a UI button rather than a scene object), named for its
	// observed effect rather than recovered. Confirmed set true by
	// TMSavegameArea's own constructor, independently of this class.
	bool _bypassReachCheck = false;
	// Confirmed a second, separate bool field (ExecuteEvent(),
	// Deponia_Linux.asm line 55F640) - when set, skips ExecuteEvent()'s own
	// trailing HandlePostExecution() call entirely; named for its observed
	// effect, not recovered. Also confirmed set true by TMSavegameArea's
	// own constructor.
	bool _skipFinalPostExecution = false;
	// Confirmed a third, separate bool field (ExecuteEvent(),
	// Deponia_Linux.asm line 55F7E3) - gates a hardcoded fallback retry
	// (expanding the original candidate types and trying a different
	// action list, field 0xAC) when nothing else matched; named for its
	// observed effect, not recovered.
	bool _hasActionTypeFallback = false;

private:
	wxPoint _position;
	int _center = -1;
	wxRect _boundingRect;
	bool _active = true;
	TPictureIO *_picture = nullptr;
	unsigned int _color = 0xFFFFFFFF;
	int _lifetime = 0;
	TGAnimation *_currentAnimation = nullptr;
	std::vector<TGAnimation *> _animations;
	TGText *_text = nullptr;
	float _alpha = 1.0f;
	float _alphaFrom = 0.0f;
	float _alphaTarget = 1.0f;
	int _alphaDurationMs = 0;
	TTimer _timer;
	TPolygonList _polygons;
};
