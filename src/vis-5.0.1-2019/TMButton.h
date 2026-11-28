// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed in full (Deponia_Linux.asm lines 165577-166300, all 9 manifest-listed
// methods): the managed object for a button of an interface (an inventory slot, a scroll
// arrow, a command...): it holds two sprite pictures - the one shown when the button is
// active (hovered; field kButtonActiveSprite) and the one when it is not
// (kButtonInactiveSprite) - and the base's picture pointer is set to one of them by
// SetActiveSprite(). Its hit area is the button's own polygon, its actions the button's
// own list, and it shows the button's animation while it is active.
//
// Original layout: +0xB8 the active picture, +0x1A0 the inactive one (0xE8 bytes each);
// THButton continues at +0x288.
#pragma once

#include "TManagedObject.h"
#include "graphicslib/picture.h"

class TMButton : public TManagedObject {
public:
	explicit TMButton(const TVisObjRef &ref);
	~TMButton() override = default;

	/** Chooses which of the two pictures is the one shown. */
	void SetActiveSprite(bool active) override;
	/** Lets go of both pictures and of the animation. */
	void RemoveSprites() override;
	/** Activating starts the button's animation; deactivating hides it and the pictures. */
	void SetActive(bool active) override;
	/** The list of the button's actions (kButtonActions). */
	void GetActionList(TVList &actions) const override;
	/** The animation shown is the one the button's data names; other animations go to the
	 *  list of the ones shown besides it. */
	void SetAnimation(TGAnimation *animation) override;

	/** Shows the button's animation again (when the button is active and has one that is
	 *  not shown yet). */
	void StartAnimation();

protected:
	TPictureIO _activePicture;    // +0xB8
	TPictureIO _inactivePicture;  // +0x1A0
};
