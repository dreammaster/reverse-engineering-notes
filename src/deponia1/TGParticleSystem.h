// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed call shapes only (TGScene::EndScene()/BeginScene()/Draw()/
// ~TGScene(), Deponia_Linux.asm lines 166435-172880): a scene embeds one
// TGParticleSystem (+0x238) and may also own a heap ParticleContainer
// (+0x290) built from a Lua "particleSystem:new(...)" expression. None of
// the three classes' own methods are reversed beyond the call shapes below.
#pragma once

#include "WxStub.h"
#include "datastruct/visobjref.h"

struct vec2 {
	float x = 0.0f;
	float y = 0.0f;
};

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

class ParticleContainer {
public:
	~ParticleContainer();
	void Update(bool flag1, const vec2 &offset, float deltaTime, bool flag2);
	void Draw();
};
