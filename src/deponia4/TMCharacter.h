// Not yet assert-confirmed to a specific file; stays at the top level (the assert in
// SetAnimation() names src/vsplayer/characterManaged.cpp - see manifest/
// source_layout.tsv).
//
// Confirmed in full (Deponia_Linux.asm lines 137965-138511, all 10 manifest-listed
// methods): the managed object between TManagedObject and TGCharacter - what is
// specific to a character in the data: the actions are the character's own list,
// the direction is its own field, it is hit by its sprite's opaque pixels (and only
// by the character that is not the one detecting), and its name is a language text.
// It has one pure virtual of its own, GetCurrentSpriteRect() (vtable slot 0x100),
// that TGCharacter implements.
//
// The constructor clears two of TManagedObject's flags (_bypassReachCheck and
// _skipFinalPostExecution, +0x24 and +0x50).
#pragma once

#include "TManagedObject.h"

class TMCharacter : public TManagedObject {
public:
	explicit TMCharacter(const TVisObjRef &ref);

	/** The object's own centre (the base's own field). */
	int GetCenter() const override;
	/** The list of the character's actions (kCharacterActions). */
	void GetActionList(TVList &actions) const override;
	/** The character's direction (kCharacterDirection). */
	int GetDirection() const override;
	/** The character's name in the current language. */
	wxString GetLanguageName() const override;

	/** Takes the animation to show (hiding the one shown before, if another). */
	void SetAnimation(TGAnimation *animation) override;

	/** Whether the point is on an opaque pixel of the character's sprite. */
	bool IsInside(const wxPoint &position) const override;
	/** The same, but only when the object detecting it (the character in `info`) is not this
	 *  character and detects characters at all. */
	bool IsInside(const wxPoint &position, const TGDetectInfo &info) const override;

	/** The rectangle the character's sprite is shown in. */
	virtual wxRect GetCurrentSpriteRect() const = 0;
};
