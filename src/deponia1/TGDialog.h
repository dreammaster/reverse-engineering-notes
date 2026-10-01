// Not yet assert-confirmed to a specific file; stays at the top level.
// Embedded by value in TGameControl (confirmed: TGameControl's ctor calls
// TGDialog::TGDialog() directly on an interior pointer, and TGameControl
// exposes it via GetDialog()/StartDialog()/EndDialog()).
//
// Confirmed from TGameControl::DisplayDialog() (Deponia_Linux.asm lines
// 455780-455803): TVisObjRef::IsEmpty()/TGDialog::Draw() are both called on
// TGDialog's own base address, so a dialog is "active" exactly when
// whatever lives at that address is non-empty. SetDialog()/Clear()
// (TGameControl::StartDialog/EndDialog, asm lines 460983-461192) set/clear
// it, as their names suggest.
//
// IMPORTANT, found while reading TGDialog::TGDialog()/Clear() directly
// (asm lines 220105-220404) for a planned fuller pass: this is a much
// bigger and more complex class than the rest of this header (and the
// current Clear()/SetDialog()/IsEmpty()/GetTarget() implementations below)
// assume. TGDialog actually derives from an entirely unreversed TTDialog
// base (the ctor's very first call) - for the "TVisObjRef at TGDialog's own
// address" claim above to be consistent with the constructor evidence
// below, TTDialog's own first field would need to itself be a TVisObjRef
// (TTDialog has no vtable in this constructor), which hasn't been verified.
// Confirmed TGDialog-own fields after the TTDialog base, in order: a
// std::vector<std::wstring> (+0x08 begin/+0x10 end), two independently
// `operator delete`d raw buffers (+0x20, +0x38) each paired with an
// unexplored qword, a TVList (+0x50, destroyed via TVList::clear()/
// ~TVList() - likely the set of selectable dialog choices), a
// std::vector<wxRect> (+0x20's buffer is actually THIS - SetScrollButtons/
// HandleMouseMove stride it in 16-byte steps and call wxRect methods on
// it) paired with a parallel std::vector<int> (+0x38, each int being a
// "dialog part" index, read by HandleMouseMove via GetCurrentDialogPart()'s
// own +0x68 field), an int "current hovered dialog part" at +0x68
// (default/cleared to -1), a byte at +0x6C ("has more than one page of
// choices, i.e. needs scroll buttons" per SetScrollButtons' own
// GetBottom()-comparison logic) plus 3 more bytes at +0x6D/+0x6E/+0x6F
// (hover state for the up/down scroll buttons and the text area,
// respectively, set by HandleMouseMove), THREE TVisObjRef fields (+0x70,
// +0x78, +0x80 - not just the one this header's existing model assumes),
// a TTimer (+0x88), four ints (+0x98-0xA4), and FIVE embedded TPictureIO
// sprites at +0xA8/+0x190/+0x278/+0x360/+0x448 (each stride 0xE8 bytes,
// matching TPictureIO's own real size) - at least a background, the two
// scroll buttons (+0x98's wxRect and +0x278's sprite are both referenced
// by HandleMouseMove), and likely a character portrait and a text/name
// plate. None of this is modeled below; SetScrollButtons()/
// HandleMouseMove()/HandleMouseClick()/HandleMouseWheel()/Draw()/
// GetDialogCharacter()/Load()/Save() all depend on it and need their own
// dedicated pass (including reversing TTDialog itself) rather than being
// guessed at here - left as an honest, now-documented gap.
#pragma once

#include "WxStub.h"
#include "datastruct/visobjref.h"

enum class TMouseMessageEnum;

class TGDialog {
public:
	TGDialog() = default;

	// Confirmed call shape only (TGameControl::HandleMouseMove,
	// Deponia_Linux.asm line 472265) - not reversed beyond that.
	void HandleMouseMove(const wxPoint &pos);
	// Confirmed call shape only (TGameControl::HandleMouseUp, Deponia_Linux.
	// asm line 472603) - fired for msg values 2/4 (left/right button
	// released) while a dialog is active; not reversed beyond that.
	void HandleMouseClick();
	// Confirmed call shape only (TGameControl::HandleMouseUp, Deponia_Linux.
	// asm line 472595) - fired for msg values 12/13 (the two confirmed
	// wheel-direction messages, see TMouseMessageEnum) while a dialog is
	// active; not reversed beyond that.
	void HandleMouseWheel(TMouseMessageEnum msg);
	// Confirmed call shape only (TGameControl::Update, Deponia_Linux.asm line
	// 469668) - checked (once a dialog is active) to decide whether the
	// cursor should show its active or inactive state; not reversed beyond
	// that call shape.
	bool IsActiveDialogPart() const;

	bool IsEmpty() const {
		return _target.IsEmpty();
	}
	// Confirmed used directly (TGameControl::Save passes &_dialog itself
	// as a TVisObjRef* to SetLink(), asm line 462856 - the same "TVisObjRef
	// at a known offset" pattern as TGCharacter/TGScene/TSText/TGText).
	const TVisObjRef &GetTarget() const {
		return _target;
	}
	void Draw();
	void SetDialog(const TVisObjRef &dialog) {
		_target = dialog;
	}
	void Clear() {
		_target = TVisObjRef();
	}

private:
	TVisObjRef _target;
};
