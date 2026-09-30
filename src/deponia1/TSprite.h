// Not yet assert-confirmed to a specific file; stays at the top level.
//
// TPictureMEM's base class (inferred: TPictureIO's ctor/dtor call
// TPictureMEM::TPictureMEM()/~TPictureMEM() on `this` with no offset
// adjustment, and TPictureIO also calls TSprite::Set/operator==/
// SetImageSize on `this` with no offset adjustment - only explained by
// TPictureMEM : public TSprite, chaining through to TPictureIO). Fields
// below are inferred from TPictureIO::GetSpriteName's usage (a path, an id,
// and a "type" checked against 1) - real names/full field set unconfirmed.
#pragma once

#include "TCharHolder.h"
#include "WxStub.h"

// This class's real field layout is now known to be considerably larger and
// differently-ordered than what's modeled below: TSprite::GetPath()'s own
// disassembly reads its path field at offset +0x30 (not +0), GetWidth()/
// GetHeight() at +0x18/+0x1C, and SetPosition()/GetSize() touch a position
// at +0x20 and a scale float at +0x48 - none of which line up with the
// simple 5-field struct here. Untangling the real layout is its own
// dedicated pass (TSprite is otherwise a stub, ~35 methods); only
// SetPosition() below is added against confirmed behavior (used by
// TLoadingControl::UpdateStatus()) rather than the real offset, using this
// project's own invented _position/_scale fields instead.
class TSprite {
public:
	TSprite() = default;

	bool operator==(const TSprite &other) const;
	void Set(const TSprite &other);
	void SetImageSize(int width, int height);
	// Confirmed call shape only (TGameControl::Update, Deponia_Linux.asm
	// line 469789) - a non-const accessor for _path, distinct from any
	// (unconfirmed) const counterpart; not reversed beyond that call shape.
	TCharHolder &GetPathNonConst() {
		return _path;
	}
	// Confirmed (TSprite::SetPosition's own disassembly, Deponia_Linux.asm
	// lines 584452-584465): always sets the position; only updates the
	// scale when it's above some unresolved threshold constant - simplified
	// here to "any positive scale," since the real threshold isn't known.
	void SetPosition(const wxPoint &pos, float scale) {
		_position = pos;
		if (scale > 0.0f)
			_scale = scale;
	}

	TCharHolder _path;
	int _id = 0;
	int _type = 0;
	int _imageWidth = 0;
	int _imageHeight = 0;
	wxPoint _position;
	float _scale = 1.0f;
};
