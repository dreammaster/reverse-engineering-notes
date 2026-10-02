// Not yet assert-confirmed to a specific file; stays at the top level.
// A genuinely distinct, recovered class name (demangled byte-for-byte from
// `TCAnimation::...` symbols) - TGAnimation.h's own class is layered on top
// of this one (TGAnimation : public TCAnimation, confirmed by call sites
// that invoke TCAnimation's own methods directly on a TGAnimation*, e.g.
// TManagedObject::Prepare()/Draw()/RemoveSprites()/IsMovingObject()).
// Only the query-method call shapes TManagedObject depends on are declared
// here - none of their bodies, nor this class's own fields, are reversed;
// this is purely the leaf interface the TManagedObject pass needed, left
// for the animation subsystem's own dedicated future pass.
#pragma once

#include "WxStub.h"
#include "datastruct/visobjref.h"

class TSprite;

class TCAnimation {
public:
	virtual ~TCAnimation() = default;

	// Confirmed call shape only (TManagedObject::Prepare/Draw, Deponia_Linux.
	// asm lines 190994, 191001, 191050, 192775, 192782) - not reversed.
	bool IsSpriteIndexValid() const {
		return false;
	}
	bool IsBonesAnimation() const {
		return false;
	}
	bool IsModelAnimation() const {
		return false;
	}
	// Confirmed call shape only (TManagedObject::IsMovingObject, asm line
	// 191278) - not reversed.
	bool IsMoveAnimation() const {
		return false;
	}
	// Confirmed call shape only (TManagedObject::SetAnimation, asm line
	// 192591) - not reversed.
	TVisObjRef GetDataObject() const {
		return TVisObjRef();
	}
	// Confirmed call shape only (TManagedObject::RemoveSprites, asm line
	// 190964) - not reversed.
	void RemoveSprites() {
	}
	// Confirmed call shape only (TCursorControl::Draw, Deponia_Linux.asm
	// line 622FB0) - not reversed.
	void SetPosition(const wxPoint &/*pos*/, float /*scale*/) {
	}
	// Confirmed call shape only (TCursorControl::GetPositionNextToCursor,
	// Deponia_Linux.asm line 6236DC) - not reversed.
	TSprite *GetCurrentSprite() const {
		return nullptr;
	}
};
