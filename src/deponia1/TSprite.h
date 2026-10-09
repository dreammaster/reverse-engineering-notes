// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed in full (Deponia_Linux.
// asm lines 582998-584513, all 35 manifest-listed methods): describes a
// displayable image reference - a path plus a separate display name, image
// dimensions, a display position, a percentage scale, transparency
// settings, a mirror flag and a pause value. TSprite is genuinely
// polymorphic (its ctor writes a vtable pointer, and it has the classic
// deleting/non-deleting virtual-destructor pair) - modeled here with just a
// virtual destructor, matching TPictureMEM (TPictureIO's base, itself
// derived from TSprite - confirmed by TPictureIO's ctor/dtor and its calls
// to TSprite::Set()/operator==()/SetImageSize() with no `this` offset
// adjustment) already declaring its own destructor virtual.
//
// This corrects a considerably-simplified earlier model: real member
// offsets (confirmed directly from each accessor's own disassembly, not
// inferred): vtable ptr @0x00, _name @0x08 (TCharHolder), _imageWidth
// @0x18, _imageHeight @0x1C, _position @0x20/0x24, _transparentColor @0x28,
// _path @0x30 (TCharHolder), _flags @0x40 (bit 1 = mirrored; other bits
// exist - SetMirrored() only ever touches bit 1, preserving the rest - but
// aren't referenced by any of TSprite's own confirmed methods),
// _transparencyMode @0x44, _scale @0x48 (a float PERCENTAGE, default
// 100.0 - not a 0-1 fraction as an earlier pass had guessed), _pause @0x4C
// (a short, sign-extended back to int by GetPause()).
//
// A genuinely surprising, twice-independently-confirmed find: Set() and
// the copy constructor both deliberately do NOT copy _imageWidth/
// _imageHeight - a copy always starts with width/height at 0, regardless
// of the source's values. Presumably these are meant to be recomputed once
// a copy's image is (re)loaded rather than treated as part of a sprite's
// copyable "identity" - reproduced faithfully here rather than "fixed",
// since two independent methods agree on it.
#pragma once

#include <cstdint>

#include "TCharHolder.h"
#include "WxStub.h"

// Confirmed 3 distinct values (0, 1, 2) read off ToLuaString()'s mode
// dispatch and SetTransparency()/SetTransparentColor()'s own logic; real
// enumerator names aren't recoverable (no debug info), so these are a
// reasonable guess from behavior, not recovered identifiers. Only kColorKey
// (1) is confirmed to have a specific meaning (SetTransparentColor() always
// sets this mode); 0 and 2 (the default) are both used as ToLuaString()
// dispatch targets but nothing in TSprite's own methods distinguishes their
// meaning further.
enum class eTransparencyMode {
	kNone = 0,
	kColorKey = 1,
	kAlpha = 2,
	// Confirmed (TSprite::IsTransparencyEqual, Deponia_Linux.asm line
	// 584266): used as an "any mode matches" wildcard by that one method.
	kAny = -1,
};

class TSprite {
public:
	TSprite() = default;
	TSprite(const TSprite &other);
	explicit TSprite(const wxFileName &path);
	virtual ~TSprite() = default;

	TSprite &operator=(const TSprite &other) {
		Set(other);
		return *this;
	}

	// Confirmed (asm lines 582998-583029): copies everything except
	// _imageWidth/_imageHeight - see the class comment above.
	void Set(const TSprite &other);

	bool operator==(const TSprite &other) const;
	// Confirmed (asm lines 583444-583625): true if the path differs, or the
	// transparency mode differs, or (kColorKey mode only) the transparent
	// color differs - position/mirrored/pause/name never trigger a reload.
	bool CmpReloadNeeded(const TSprite &other) const;
	// Confirmed (asm lines 583633-583654): resets everything Clear()
	// conceptually "owns" back to default - notably NOT _imageWidth/
	// _imageHeight or _name, matching Set()'s/the copy ctor's own omission
	// of the former and this method's own omission of the latter (_name is
	// reassigned "", but TCharHolder's own empty-string-is-a-no-op rule
	// means that's observably unchanged either way).
	void Clear();

	// Confirmed (asm lines 583664-584050): the sprite as the text of a Lua table constructor
	// (`{path='...',position={x=,y=},transparency=eTransparency...,transpcolor=,pause=}`), and the
	// reading of such a text (see TSprite.cpp for its quirk).
	wxString ToLuaString() const;
	bool SetFromLuaString(const wxString &value);

	// Confirmed (asm lines 584058-584086): _name's own wxString conversion/
	// assignment - a separate display name from _path.
	wxString GetName() const {
		return _name;
	}
	void SetName(const TCharHolder &value) {
		_name = value;
	}
	TCharHolder &GetNameNonConst() {
		return _name;
	}

	void SetImageSize(int width, int height) {
		_imageWidth = width;
		_imageHeight = height;
	}
	int GetWidth() const {
		return _imageWidth;
	}
	int GetHeight() const {
		return _imageHeight;
	}
	// Confirmed (asm lines 584413-584442): the actual displayed size, i.e.
	// the raw image dimension scaled by GetSize()'s percentage.
	float GetSizedWidth() const {
		return static_cast<float>(_imageWidth) * _scale / 100.0f;
	}
	float GetSizedHeight() const {
		return static_cast<float>(_imageHeight) * _scale / 100.0f;
	}

	bool IsEmpty() const {
		return !_path.IsOk();
	}
	wxPoint GetPosition() const {
		return _position;
	}
	// Confirmed (asm lines 584450-584465): always updates the position;
	// only updates the scale when it's above exactly 0.0 (a literal float
	// constant in the disassembly, not a guessed threshold).
	void SetPosition(const wxPoint &pos, float scale) {
		_position = pos;
		if (scale > 0.0f)
			_scale = scale;
	}
	// Confirmed (asm lines 584473-584481): despite the name, this returns
	// the scale PERCENTAGE (a float), not a wxSize.
	float GetSize() const {
		return _scale;
	}

	void SetTransparency(eTransparencyMode mode, unsigned int color) {
		_transparencyMode = mode;
		if (mode == eTransparencyMode::kColorKey)
			_transparentColor = color;
	}
	eTransparencyMode GetTransparency() const {
		return _transparencyMode;
	}
	// Confirmed (asm lines 584225-584235): always switches to kColorKey
	// mode as a side effect.
	void SetTransparentColor(const unsigned int &color) {
		_transparencyMode = eTransparencyMode::kColorKey;
		_transparentColor = color;
	}
	unsigned int GetTransparentColor() const {
		return _transparentColor;
	}
	// Confirmed (asm lines 584260-584288): asymmetric - only `other`'s mode
	// is inspected. other.GetTransparency()==kAny always matches;
	// other.GetTransparency()==kColorKey compares only the transparent
	// colors (regardless of this sprite's own mode); any other mode compares
	// this->GetTransparency() == other.GetTransparency() directly.
	bool IsTransparencyEqual(const TSprite &other) const;

	wxFileName GetPath() const {
		return _path;
	}
	TCharHolder &GetPathNonConst() {
		return _path;
	}
	void SetPath(const TCharHolder &value) {
		_path = value;
	}

	// Confirmed (asm lines 584368-584404): bit 1 (0x02) of an otherwise-
	// unconfirmed flags byte - SetMirrored() preserves the other bits.
	bool IsMirrored() const {
		return (_flags & kMirroredFlag) != 0;
	}
	void SetMirrored(bool mirrored) {
		if (mirrored)
			_flags |= kMirroredFlag;
		else
			_flags &= static_cast<std::uint8_t>(~kMirroredFlag);
	}

	// Bit 0 of the flags byte: TPictureMEM sets it when it has made its pixels itself (CopyFrom(), ResizeImage()) and
	// CopyFrom() asks it of its source (asm lines 758785-758947).
	bool IsMemoryImage() const {
		return (_flags & kMemoryImageFlag) != 0;
	}
	void SetMemoryImage() {
		_flags |= kMemoryImageFlag;
	}

	// Confirmed (asm lines 584489-584513): stored as a 16-bit value,
	// sign-extended back to int by GetPause().
	void SetPause(int value) {
		_pause = static_cast<short>(value);
	}
	int GetPause() const {
		return _pause;
	}

private:
	static constexpr std::uint8_t kMemoryImageFlag = 0x01;
	static constexpr std::uint8_t kMirroredFlag = 0x02;

	TCharHolder _name;                   // +0x08
	int _imageWidth = 0;                 // +0x18
	int _imageHeight = 0;                // +0x1C
	wxPoint _position;                   // +0x20/+0x24
	unsigned int _transparentColor = 0;  // +0x28
	TCharHolder _path;                   // +0x30
	std::uint8_t _flags = 0;             // +0x40
	eTransparencyMode _transparencyMode = eTransparencyMode::kAlpha;  // +0x44
	float _scale = 100.0f;               // +0x48
	short _pause = -1;                   // +0x4C
};
