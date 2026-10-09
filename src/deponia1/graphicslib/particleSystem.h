// Reconstructed from Deponia_Linux.asm, particleSystem.cpp (the particle systems that the editor makes - a scene or an
// object links to one): TParticleEmitter (asm 769405-771112), TParticleSystem (asm 771127-771351 and the destructors at
// 766025-766193), Rand0, Rand, Randomize and BlendColors (asm 766200-766330). The game's part (TGParticleEmitter and
// TGParticleSystem, which read an emitter from the data) is in vsplayer/particlesGame.h; the drawing (TGraphicsOGL::Draw
// of a TParticleSystem, asm 708150) is not reconstructed.
//
// An emitter keeps its particles in two arrays, `_particles` for what is drawn and `_motions` for what moves them:
//   _particles[i * _particleStride]: x, y, z, size (floats), colour (an int, 0xRRGGBB) - and with the particle type
//     kRandomTile the column and row of the tile it is (floats); kStreak the x, y, z of where it was; kAnimatedTile
//     the blend (an int, all four bytes alike) and the column and row of two tiles (floats) - 5, 7, 8 or 10 numbers;
//   _motions[i * 7]: vx, vy, vz (floats), the steps that are left of its life, its life (ints), its colour (an int) and
//     its size (a float).
// A particle is let go when its life is over: the last one takes its place. The ages 0 to 98 of the curves are got from
// the life: (life - left) * 99 / life.
#pragma once

#include <vector>

#include "baselib/sort.h"
#include "graphicslib/vector3d.h"

class TPictureIO;

/** A number from 0 to 65535 from the generator of the particles (an LCG, seeded with the time when an emitter is
 *  made: RandomSeed = RandomSeed * 0x343FD + 0x269EC3, and the bits 10 to 25). */
unsigned Rand0();
/** A number from 0 to `limit` - 1 (the product of Rand0() and `limit`, less the 16 low bits). */
unsigned Rand(unsigned limit);
void Randomize();
/** The colour (0xRRGGBB) that is `factor` of `a` and the rest of `b`, each channel cut to a whole number. */
int BlendColors(int a, int b, float factor);

class TParticleEmitter {
public:
	/** What a particle has (see above). */
	enum ParticleType {
		kBillboard = 0,     ///< 5 numbers
		kRandomTile = 1,    ///< 7: a tile of the picture, by chance
		kStreak = 2,        ///< 8: the place it was
		kAnimatedTile = 3   ///< 10: two tiles that it goes through
	};

	TParticleEmitter();
	virtual ~TParticleEmitter();

	/** The emitter has room for `maxParticles` of this type (the particles it had are let go). */
	void InitEmitter(int maxParticles, ParticleType type);
	/** Lets all of the particles go. */
	void Clear();
	/** The emitter makes particles at a point, going `direction` (the matrix of the emitter is made from it) in a cone
	 *  of two angles. */
	void SetEmitter(const TVector3D &position, const TVector3D &direction, float spreadX, float spreadY);
	/** The emitter makes particles in the box between two corners. */
	void SetEmitter(const TVector3D &corner1, const TVector3D &corner2, const TVector3D &direction, float spreadX,
	                float spreadY);
	/** The matrix that turns a direction made for the cone round z into the direction (a yaw, then a pitch, transposed). */
	void SetDirection(const TVector3D &direction);
	/** One step: the particles move, change and die; new ones are made; the keys for the sorting by depth are made. */
	bool IncTime();
	/** The ranks of the particles, the farthest from the viewer first (z, smallest first); a rank is the index times the
	 *  stride of `_particles`. */
	const uint32_t *GetSorterIndices();
	int Update() {
		return 0;
	}
	int Paint() {
		return 0;
	}

	float _emitRemainder = 0.0f;          // +0x08, the part of a particle that is not made yet
	TVector3D _position;                  // +0x0C (the point of kind 1)
	TVector3D _boxMin;                    // +0x18
	TVector3D _boxMax;                    // +0x24
	TVector3D _boxSize;                   // +0x30
	TVector3D _direction;                 // +0x3C
	float _spreadX = 0.0f;                // +0x48, radians (kParticlePhiX)
	float _spreadY = 0.0f;                // +0x4C (kParticlePhiY)
	TMatrix3 _rotation;                   // +0x50, turns a direction round z into _direction
	RadixSort _sorter;                    // +0x78
	int _tilesX = 1;                      // +0x1498
	int _tilesY = 1;                      // +0x149C
	int _maxTile = 0;                     // +0x14A0, -1: all of the tiles
	int _geometry = 0;                    // +0x14A4, where the particles are made: 0 nowhere, 1 at a point, 2 in a box
	int _particleType = 0;                // +0x14A8
	int _forceType = 0;                   // +0x14AC, 0 a constant force, 1 a force to a point, 2 a force that falls with the square of the distance
	int _materialMode = 0;                // +0x14B0, 0: the particles are sorted by depth (kParticleMaterialMode)
	int _unknown14B4 = -1;                // +0x14B4
	float _unknown14B8 = 1.0f;            // +0x14B8
	float *_particles = nullptr;          // +0x14C0
	int _particleStride = 0;              // +0x14C8
	float *_motions = nullptr;            // +0x14D0
	int _motionStride = 7;                // +0x14D8
	int _maxParticles = 0;                // +0x14DC
	int _count = 0;                       // +0x14E0
	float _perTime = 10.0f;               // +0x14E4, particles per step
	TVector3D _forcePosition;             // +0x14E8
	TVector3D _force;                     // +0x14F4
	float _forceRotationX = 0.0f;         // +0x1500
	float _forceRotationZ = 0.0f;         // +0x1504
	float _forceStrength = 0.0f;          // +0x1508
	float _forceBlend = 0.99899f;         // +0x150C, how much of its speed a particle keeps in a step
	float _minSize = 0.03f;               // +0x1510
	float _maxSize = 0.3f;                // +0x1514
	float _unknown1518 = 1.0f;            // +0x1518
	float _minVelocity = 1.0f;            // +0x151C
	float _maxVelocity = 1.0f;            // +0x1520
	int _minLife = 10000;                 // +0x1524, steps
	int _maxLife = 10000;                 // +0x1528
	int _minColor = 0xFFFFFF;             // +0x152C
	int _maxColor = 0xFFFFFF;             // +0x1530
	int _activeStages = 1;                // +0x1534
	int _stageColors[4];                  // +0x1538, the colours that the particle goes through
	float _stageCurves[4][100];           // +0x1548, how much of each over the life
	float _sizeCurve[100];                // +0x1B88
	float _tileCurve[100];                // +0x1D18
	float _pixelScale = 0.0f;             // +0x1EA8, set by TParticleSystem::SetWindowSize()
	float _scaleFactor = 1.0f;            // +0x1EAC
};

class TParticleSystem {
public:
	TParticleSystem();
	virtual ~TParticleSystem();

	/** Lets the emitters and their textures go. */
	void Clear();
	/** One step of each emitter. */
	bool IncTime();
	void SetScaleFactor(float factor);
	/** The game is `width` x `height`: the emitters get the scale that the camera has for the height. */
	void SetWindowSize(int width, int height);
	/** The width and the height of the window, and the number at +0x50. */
	TVector3D GetWindowSize() const;

	std::vector<TParticleEmitter *> _emitters;   // +0x08
	std::vector<TPictureIO *> _textures;          // +0x20
	float _unknown38 = 0.0f;                      // +0x38
	float _unknown3C = 0.0f;                      // +0x3C
	float _cameraZ = -3.0f;                       // +0x40
	float _cameraZoom = 0.66666669f;              // +0x44
	float _windowWidth = 0.0f;                    // +0x48
	float _windowHeight = 0.0f;                   // +0x4C
	float _unknown50 = 0.0f;                      // +0x50
	float _scaleFactor = 1.0f;                    // +0x54
};
