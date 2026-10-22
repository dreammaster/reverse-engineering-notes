// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 219228-219860, all 7 manifest-listed methods): the
// scene the game actually creates (TSceneControl makes two of them): a TGScene that is also
// a TEventHandlerInterface (a second vtable at +0x378) and listens to the scene's data
// object. Like the other TH* classes it is the thin layer that connects the data object to
// the running game: a change of the scene's brightness, music, scrolling, worktop area,
// lightmap or current way system is taken over when it happens.
#pragma once

#include "TGScene.h"
#include "datastruct/eventhandler.h"

class THScene : public TGScene, public TEventHandlerInterface {
public:
	THScene();
	~THScene() override;

	/** A change of a field of the scene's data object. */
	void OnEvent(TEventEnum event, int field, TVisionaireObject *object) override;

	/** Listens to the data object of the scene `scene` (and no longer to the one the scene has
	 *  now). */
	void RegisterEvents(TVisObjRef &scene);
	/** Stops listening to the scene's data object. */
	void UnregisterEvents();
};
