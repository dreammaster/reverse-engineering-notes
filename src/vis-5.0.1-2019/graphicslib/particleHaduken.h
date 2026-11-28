// Reconstructed from Deponia_Linux.asm, particleHaduken.cpp (the particle emitters of the player): RandomGenerator
// 689978, the curve evaluators 690199-691242, Emitter 689094-695259, ParticleContainer 695485-698932.
//
// A ParticleContainer is a pool of Particles that one Emitter makes (and, with `_children`, containers that follow the
// particles of this one). The emitter has a lot of curves: a value of a particle at its birth is a MinMaxFloatCurve (the
// curve of its lower and of its upper bound, mixed by a random number, both read at the phase of the emitter), and the
// *OverLife curves are read at the age of the particle (age / life) while it lives. The names of the curves are the names
// of the keys that the scripts give to `particleSystem:new{...}` (and what Emitter::SerializeEmmiter prints).
//
// Not reconstructed: the Serialize functions (the editor writes a particle system with them; the player does not), and
// ParticleContainer::Draw (vertex buffers of the GL backend through `particlePipeline`).
#pragma once

#include <stdint.h>

#include <string>
#include <vector>

class TPictureIO;
class TSubSysTexture;

struct vec2 {
	float x = 0.0f;
	float y = 0.0f;
};

/** Confirmed: xorshift64* on `_state` (rand()), `_state` made from a seed with the 64-bit integer hash of Thomas Wang.
 *  The one that all of the emitters use is the global `generator`. */
class RandomGenerator {
public:
	typedef uint64_t Seed;

	RandomGenerator();

	void setSeed(Seed seed);
	/** The next 64 random bits. */
	uint64_t rand();

	Seed _seed;     // +0x00
	uint64_t _state;  // +0x08
	double _spare;  // +0x10, infinity: nothing in the player reads it
};

extern RandomGenerator generator;

/** A random number from 0 up to (not including) `limit`. */
float RandF(float limit);

/** A line of (time, value) pairs; the value for a time is the linear mix of the two values around it. */
class FloatCurveEvaluater {
public:
	/** The value at `time`: no point 0, one point that point, else before the first point (and in the first segment) the
	 *  line of the first two points is followed backwards. After the last point it is the last value. */
	float eval(float time) const;
	/** `values` is time, value, time, value ... */
	void ParseFloatArray(const std::vector<float> &values);

	std::vector<float> _values;  // +0x00 (the odd places of the array)
	std::vector<float> _times;   // +0x18 (the even places)
};

/** The same with a colour (0xBBGGRR) at the times; the channels are mixed on their own. */
class ColorCurveEvaluater {
public:
	uint32_t eval(float time) const;
	/** `values` is time, colour, time, colour ... (colours as numbers) */
	void ParseFloatArray(const std::vector<float> &values);

	std::vector<float> _times;       // +0x00
	std::vector<uint32_t> _colors;   // +0x18
};

/** Points in a plane. */
class PointCurveEvaluater {
public:
	vec2 eval(float time) const;

	std::vector<float> _times;    // +0x00
	std::vector<vec2> _points;    // +0x18
};

/** A pair of curves: `eval(time, mix)` is the lower one at `time` times (1 - mix) plus the upper one times `mix`. */
class MinMaxFloatCurveEvaluater {
public:
	float eval(float time, float mix) const;
	/** An odd number of values has -10000 between the array of the lower curve and the array of the upper one, an even
	 *  number is cut in two. Each of them is time, value, time, value ... */
	void ParseFloatArray(const std::vector<float> &values);

	FloatCurveEvaluater _min;  // +0x00
	FloatCurveEvaluater _max;  // +0x30
};

/** 0x50 bytes. A pool slot is free when its `_age` is -1. */
struct Particle {
	float _x = 0.0f;               // +0x00
	float _y = 0.0f;               // +0x04
	float _size = 0.0f;            // +0x08, sizeOverLife at this age
	float _rotation = 0.0f;        // +0x0C
	float _alpha = 0.0f;           // +0x10, visibilityOverLife at this age times `_visibility`
	uint8_t _r = 0;                // +0x14
	uint8_t _g = 0;                // +0x15
	uint8_t _b = 0;                // +0x16
	uint8_t _pad17 = 0;            // +0x17
	float _age = 0.0f;             // +0x18, seconds (a step is 0.016); -1: the slot is free
	float _life = 0.0f;            // +0x1C
	float _frameTime = 0.0f;       // +0x20, the age when the picture was changed last
	float _spin = 0.0f;            // +0x24
	float _weight = 0.0f;          // +0x28
	float _velocity = 0.0f;        // +0x2C
	float _direction = 0.0f;       // +0x30, radians
	float _angularVelocity = 0.0f; // +0x34
	float _motionRandomness = 0.0f;// +0x38
	float _baseSize = 0.0f;        // +0x3C, the "size" it was born with
	float _visibility = 0.0f;      // +0x40
	float _pad44 = 0.0f;           // +0x44
	int64_t _frame = 0;            // +0x48, the picture (ParticleContainer::_sprites)
};

/** The picture of a particle in the texture of its container (0x24 bytes): where it is, in parts of the texture. */
struct ParticleSprite {
	float _u = 0.0f;       // +0x00
	float _v = 0.0f;       // +0x04
	float _w = 0.0f;       // +0x08
	float _h = 0.0f;       // +0x0C
	float _half1 = 0.5f;   // +0x10
	float _half2 = 0.5f;   // +0x14
	float _centerX = 0.5f; // +0x18, the key imageCenter
	float _centerY = 0.5f; // +0x1C
	float _aspect = 1.0f;  // +0x20, height / width of the picture
};

/** The basic emitter: it makes particles from nothing (the position of the particle stays what it was). 0x5B8 bytes. */
class Emitter {
public:
	Emitter();
	virtual ~Emitter();

	/** Slot 0x00: the particle lives one step more (moves, turns, grows ...). */
	virtual void Update(Particle &particle);
	/** Slot 0x08: a new particle (the slot is used again). */
	virtual void Emit(Particle &particle);
	/** Slot 0x10: the emitter's time goes on by `step` seconds (it loops `_loops` times, 0: for ever). */
	virtual void UpdateEmitter(float step);

	/** The things that every kind of emitter does for a new particle (everything but its place, direction and speed). */
	void EmitBasic(Particle &particle);
	/** The direction and the speed of a new particle (the last of what each Emit() does). */
	void emitDirection(Particle &particle);

	vec2 _center;                       // +0x08, the key center
	int _currentLoop = 0;               // +0x10
	int _loops = 0;                     // +0x14
	float _length = 0.0f;               // +0x18 (line)
	float _angle = 0.0f;                // +0x1C (line)
	float _radius = 0.0f;               // +0x20 (circle, square)
	float _innerRadius = 0.0f;          // +0x24
	float _sizeX = 0.0f;                // +0x28 (box)
	float _sizeY = 0.0f;                // +0x2C
	bool _directionToRotation = false;  // +0x30
	float _directionToRotationOffset = 0.0f;  // +0x34
	float _time = 0.0f;                 // +0x38
	float _duration = 1.0f;             // +0x3C
	float _phase = 0.0f;                // +0x40, _time / _duration
	float _emitRemainder = 0.0f;        // +0x44, the part of a particle that is not emitted yet

	MinMaxFloatCurveEvaluater _emissionDirection;   // +0x048
	MinMaxFloatCurveEvaluater _life;                // +0x0A8
	MinMaxFloatCurveEvaluater _velocity;            // +0x108
	MinMaxFloatCurveEvaluater _size;                // +0x168
	MinMaxFloatCurveEvaluater _weight;              // +0x1C8
	MinMaxFloatCurveEvaluater _spin;                // +0x228
	MinMaxFloatCurveEvaluater _angularVelocity;     // +0x288
	MinMaxFloatCurveEvaluater _motionRandomness;    // +0x2E8
	MinMaxFloatCurveEvaluater _visibility;          // +0x348
	MinMaxFloatCurveEvaluater _rotation;            // +0x3A8
	FloatCurveEvaluater _velocityOverLife;          // +0x408
	FloatCurveEvaluater _sizeOverLife;              // +0x438
	FloatCurveEvaluater _weightOverLife;            // +0x468
	FloatCurveEvaluater _spinOverLife;              // +0x498
	FloatCurveEvaluater _angularVelocityOverLife;   // +0x4C8
	FloatCurveEvaluater _motionRandomnessOverLife;  // +0x4F8
	FloatCurveEvaluater _visibilityOverLife;        // +0x528
	FloatCurveEvaluater _numberOfEmitted;           // +0x558, particles per step
	ColorCurveEvaluater _colorOverLife;             // +0x588
};

/** Particles on a square ring (`_radius` the outside, `_innerRadius` the inside half-size). */
class SquareEmitter : public Emitter {
public:
	void Emit(Particle &particle) override;
};

/** Particles on a disk (a ring when `_innerRadius` is more than 0). */
class CircleEmitter : public Emitter {
public:
	void Emit(Particle &particle) override;
};

/** Particles in a box of `_sizeX` x `_sizeY` around the center. */
class BoxEmitter : public Emitter {
public:
	void Emit(Particle &particle) override;
};

/** Particles on a line of `_length` through the center at `_angle`. 0x5C0 bytes. */
class LineEmitter : public Emitter {
public:
	void Emit(Particle &particle) override;
	void UpdateEmitter(float step) override;

	float _cos = 0.0f;  // +0x5B8
	float _sin = 0.0f;  // +0x5BC
};

/** Particles at the center. */
class PointEmitter : public Emitter {
public:
	void Emit(Particle &particle) override;
};

/** Particles at the places of a picture whose alpha is 128 or more. 0x600 bytes. */
class ImageEmitter : public Emitter {
public:
	void Emit(Particle &particle) override;

	/** Reads the picture `path` and keeps the places of its pixels: `mode` 1 also keeps their colours (they become the
	 *  colours of the particles). */
	void CreateWithImage(const char *path, int mode);

	struct Point {
		float _x;
		float _y;
	};

	std::vector<Point> _points;       // +0x5B8
	std::vector<uint32_t> _colors;    // +0x5D0
	int _width = 0;                   // +0x5E8
	int _height = 0;                  // +0x5EC
	int _imageType = 0;               // +0x5F0, the argument `mode` of CreateWithImage()
	std::string _path;                // +0x5F8
};

/** Confirmed (asm lines 690150, 690312, 1390520): the value on the way from `a` to `b` when `f` (0 to 1) of it is gone. */
float EaseLinear(float f, float a, float b);
vec2 EaseLinear(float f, vec2 a, vec2 b);
uint32_t EaseLinearColor(float f, uint32_t a, uint32_t b);

class ParticleContainer {
public:
	ParticleContainer();
	~ParticleContainer();

	ParticleContainer(const ParticleContainer &) = delete;
	ParticleContainer &operator=(const ParticleContainer &) = delete;

	/** One step. `usePhase`: the phase of the emitter is `phase`; `pos` is where new particles begin (only with
	 *  `usePhase`); `updateChildren` is true for the container that the script made (it updates its own children, and the
	 *  existing particles with `usePhase`). The children of the particles are updated with the age of the particle as
	 *  their phase. */
	void Update(bool usePhase, const vec2 &pos, float phase, bool updateChildren);
	void Draw();

	int _creationRate = 0;      // +0x00, the most particles that one step makes
	int _maximum = 0;           // +0x04, the most particles alive
	int _count = 0;             // +0x08, the particles alive
	int _warmup = 0;            // +0x0C, steps run when the container is made
	float _unknown10 = 1.0f;    // +0x10
	Emitter *_emitter = nullptr;                       // +0x18
	std::vector<Particle> _particles;                  // +0x20
	std::vector<ParticleSprite> _sprites;              // +0x38
	std::vector<ParticleContainer> _children;          // +0x50
	std::vector<float> _vertices;                      // +0x68, what Draw() fills for the video card
	std::vector<std::string> _images;                  // +0x80
	TPictureIO *_picture = nullptr;                    // +0x98
	TSubSysTexture *_texture = nullptr;                // +0xA0, the texture with all of the pictures
	bool _imageRandom = false;                         // +0xA8, a new particle gets a picture by chance, else they go round
	bool _additive = true;                             // +0xA9, transferMode "add" (else "blend")
	double _imageInterval = 0.0;                       // +0xB0, seconds between the pictures of a particle that goes round
};
