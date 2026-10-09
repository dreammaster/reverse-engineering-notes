// Reconstructed from Deponia_Linux.asm, particlesGame.cpp (the game's side of the particle systems of graphicslib/
// particleSystem.h): TGParticleEmitter reads an emitter from the data (a "particle" object of a particle container),
// TGParticleSystem makes the emitters and the textures of a container. Not reconstructed: TGParticleEmitter::Save() and
// Load() (asm 1378389-1378934, 1379800-1380801: the editor writes an emitter to a file and reads it).
#pragma once

#include "WxStub.h"
#include "datastruct/visobjref.h"
#include "graphicslib/particleSystem.h"

class TGParticleEmitter : public TParticleEmitter {
public:
	/** Sets the emitter from the particle object `particle` (the fields kParticle...), then lets it run for the lead time of
	 *  its container. False for an empty reference. */
	bool Init(const TVisObjRef &particle);
};

class TGParticleSystem : public TParticleSystem {
public:
	/** Makes the emitters of the particles of the container `container` (an empty reference makes none: false), each with
	 *  the texture of its particle. The name is not used. */
	bool Init(const TVisObjRef &container, const wxString &name);
};
