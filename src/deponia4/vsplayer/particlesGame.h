// Reconstructed from Deponia_Linux.asm, particlesGame.cpp (the game's side of the particle systems of graphicslib/
// particleSystem.h): TGParticleEmitter reads an emitter from the data (a "particle" object of a particle container),
// TGParticleSystem makes the emitters and the textures of a container. Save() and Load() (the editor writes an
// emitter to a file and reads it) are in particlesFile.cpp.
#pragma once

#include "WxStub.h"
#include "datastruct/visobjref.h"
#include "graphicslib/particleSystem.h"

class TGParticleEmitter : public TParticleEmitter {
public:
	/** Sets the emitter from the particle object `particle` (the fields kParticle...), then lets it run for the lead time of
	 *  its container. False for an empty reference. */
	bool Init(const TVisObjRef &particle);

	/** Writes the settings of the particle object to a file (version 6: ints, floats, the four colour curves, the force,
	 *  the size curve, the texture path in 32-bit characters, the active stages, the tiles curve ...). False if the file
	 *  can not be made. */
	static bool Save(const char *filename, const TVisObjRef &particle);
	/** Reads such a file into the particle object (what a file of an older version does not have is set to a default).
	 *  False if the file can not be opened. */
	static bool Load(const char *filename, TVisObjRef &particle);
};

class TGParticleSystem : public TParticleSystem {
public:
	/** Makes the emitters of the particles of the container `container` (an empty reference makes none: false), each with
	 *  the texture of its particle. The name is not used. */
	bool Init(const TVisObjRef &container, const wxString &name);
};
