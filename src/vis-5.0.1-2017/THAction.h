// Confirmed (Deponia_Linux.asm lines 187010-187130, all 5 manifest-listed methods): the running
// action the game creates (see TGAction): it also listens to the changes of its record (the
// handler does nothing).
#pragma once

#include "TGAction.h"
#include "datastruct/eventhandler.h"

class THAction : public TGAction, public TEventHandlerInterface {
public:
	THAction(const TVisObjRef &active, const TVisObjRef &data);
	~THAction() override;

	void OnEvent(TEventEnum event, int field, TVisionaireObject *object) override;
};
