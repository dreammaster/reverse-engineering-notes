// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed call shapes only (TGScene::EndScene()/BeginScene()/Draw()/
// ~TGScene(), Deponia_Linux.asm lines 166435-172880): a scene embeds one
// TGParticleSystem (+0x238) and may also own a heap ParticleContainer
// (+0x290) built from a Lua "particleSystem:new(...)" expression (see
// graphicslib/particleHaduken.h for the container and
// vscommon/scripting/particles.h for the script side). The other two classes'
// own methods are not reversed beyond the call shapes below.
#pragma once

#include "WxStub.h"
#include "datastruct/visobjref.h"
#include "graphicslib/particleHaduken.h"

class TParticleSystem {
public:
	void Clear();
	void IncTime();
	void SetWindowSize(int width, int height);
};

class TGParticleSystem : public TParticleSystem {
public:
	void Init(const TVisObjRef &scene, const wxString &name);
};
