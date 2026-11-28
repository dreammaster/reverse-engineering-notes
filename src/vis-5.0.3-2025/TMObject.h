// Not yet assert-confirmed to a specific file; stays at the top level (the assert in
// SetActive() names src/vsplayer/objectManaged.cpp - see manifest/source_layout.tsv).
//
// Confirmed in full (Deponia_Linux.asm lines 116072-116550, all 7 manifest-listed
// methods): the managed object between TManagedObject and TGObject - what is specific
// to an object of a scene (not a character or an item): it owns the picture of the
// object's sprite (an embedded TPictureIO at +0xB8, which the base's picture pointer
// points to), its actions are the object's own list, its name is a language text, and
// it shows the object's animation while it is active.
//
// Original layout: +0xB8 the TPictureIO (0xE8 bytes: it runs to +0x1A0), +0x1A0 whether
// the object is shown by its animation, +0x1A4/+0x1A8 the sprite's own position (before
// the object's offset was added to it).
#pragma once

#include "TManagedObject.h"
#include "graphicslib/picture.h"

class TMObject : public TManagedObject {
public:
	/** `animated`: the object is shown by its animation also when it has a sprite (an object
	 *  without a sprite is always). */
	TMObject(const TVisObjRef &ref, bool animated);
	~TMObject() override = default;

	/** The object's visibility, in percent, is saved to its data. */
	void SetAlpha() override;
	/** The list of the object's actions (kObjectActions). */
	void GetActionList(TVList &actions) const override;
	/** The object's name in the current language. */
	wxString GetLanguageName() const override;
	/** Activating starts the object's animation (when it is shown by one); deactivating hides
	 *  the animation and the sprite. */
	void SetActive(bool active) override;

protected:
	TPictureIO _sprite;     // +0xB8
	bool _animated;         // +0x1A0
	wxPoint _spritePosition;  // +0x1A4
};
