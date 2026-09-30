// Not yet assert-confirmed to a specific file; stays at the top level.
// Embedded by value in TGameControl (confirmed: TGameControl's ctor calls
// TGDialog::TGDialog() directly on an interior pointer, and TGameControl
// exposes it via GetDialog()/StartDialog()/EndDialog()).
//
// Confirmed from TGameControl::DisplayDialog() (Deponia_Linux.asm lines
// 455780-455803): TGDialog's *first member* is a TVisObjRef (the currently
// active dialog target) - DisplayDialog() calls TVisObjRef::IsEmpty()
// directly on TGDialog's own address, and later TGDialog::Draw() on that
// same address, so a dialog is "active" exactly when this field is
// non-empty. SetDialog()/Clear() (TGameControl::StartDialog/EndDialog, asm
// lines 460983-461192) set/clear it, as their names suggest.
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
