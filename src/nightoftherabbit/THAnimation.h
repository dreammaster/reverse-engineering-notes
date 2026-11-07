// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 259729-260106): the TGAnimation the game
// actually creates (every `new` in TGAnimation's start/load functions makes a
// THAnimation, 0xB8 bytes): a TGAnimation that is also a TEventHandlerInterface
// (a second vtable at +0xB0) and registers itself with its running state
// (TSAnimation) record to hear when one of the record's fields changes. Like the
// other TH* classes it is the thin layer that connects the data object to the
// running game.
#pragma once

#include "datastruct/eventhandler.h"
#include "vsplayer/animationGame.h"

class THAnimation : public TGAnimation, public TEventHandlerInterface {
public:
	THAnimation(const TVisObjRef &active, const TVisObjRef &animation);
	~THAnimation() override;

	/** A change of one of the state record's fields: setting the animation active
	 *  (which is how a preloaded animation is shown) starts it, and the first/last
	 *  frame are kept within the sprites. */
	void OnEvent(TEventEnum event, int field, TVisionaireObject *object) override;
};
