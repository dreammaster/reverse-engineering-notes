#include "particleHaduken.h"

#include <math.h>

#include "TPictureMEM.h"
#include "graphicslib/picture.h"
#include "graphicslib/subsys.h"

RandomGenerator generator;

/** The part of the time that the generator and the emitters compute with: what is not 0 before a division. */
static const float kOne = 1.0f;

/** A step of an emitter (ParticleContainer::Update), and of a particle (Emitter::Update), in seconds. */
static const float kEmitterStep = 0.0166f;  // 0x3C87FCB9
static const float kParticleStep = 0.016f;  // 0x3C83126F

// Confirmed (asm lines 689978-690012, 690026-690059): the seed is mixed with Thomas Wang's 64-bit integer hash, and again
// (with the result as the new key) until it is not 0.
static uint64_t mixSeed(uint64_t key) {
	uint64_t result;

	do {
		key = ~key + (key << 21);
		key = key ^ (key >> 24);
		key = key * 265;
		key = key ^ (key >> 14);
		key = key * 21;
		key = key ^ (key >> 28);
		key = key + (key << 31);
		result = key;
	} while (result == 0);

	return result;
}

RandomGenerator::RandomGenerator() {
	_seed = 0xCBBF7A440139408DULL;
	_spare = 1.0 / 0.0;
	_state = mixSeed(_seed);
}

void RandomGenerator::setSeed(Seed seed) {
	_seed = seed;
	_state = mixSeed(seed);
}

// Confirmed (asm lines 690073-690095): xorshift64*.
uint64_t RandomGenerator::rand() {
	uint64_t x = _state;

	x ^= x >> 12;
	x ^= x << 25;
	x ^= x >> 27;
	_state = x;
	return x * 0x2545F4914F6CDD1DULL;
}

/** The next random number of the global generator, from 0 up to 1 (as the emitters have it inline). */
static double nextDouble() {
	return static_cast<double>(generator.rand()) * 5.421010862427522e-20;  // 2^-64
}

/** The same as a float. */
static float nextFloat() {
	return static_cast<float>(nextDouble());
}

/** The same from -1 up to 1 (the double is doubled before it is made a float). */
static float nextSignedFloat() {
	return static_cast<float>(nextDouble() * 2.0) - kOne;
}

// Confirmed (asm lines 690100-690138)
float RandF(float limit) {
	return static_cast<float>(nextDouble() * static_cast<double>(limit));
}

// Confirmed (asm lines 690150-690195, 690312-690360, 1390520-1390524). The float one is in another source file of the
// original.
float EaseLinear(float f, float a, float b) {
	return (b - a) * f + a;
}

vec2 EaseLinear(float f, vec2 a, vec2 b) {
	vec2 result;

	result.x = (b.x - a.x) * f + a.x;
	result.y = (b.y - a.y) * f + a.y;
	return result;
}

/** A colour (0xBBGGRR) on the way from `a` to `b`; each channel is cut to a whole number. */
uint32_t EaseLinearColor(float f, uint32_t a, uint32_t b) {
	uint32_t result = 0;

	for (int shift = 0; shift < 24; shift += 8) {
		int from = (a >> shift) & 0xFF;
		int to = (b >> shift) & 0xFF;
		int value = static_cast<int>((to - from) * f + from);

		result |= static_cast<uint32_t>(value & 0xFF) << shift;
	}

	return result;
}

/** Where `time` is between the point `index - 1` and `index`, as a part of 0 to 1; a segment of no length is 1 long. */
static float segmentPart(float time, float start, float end) {
	float length = end - start;

	if (length == 0.0f)
		length = kOne;

	return (time - start) / length;
}

// Confirmed (asm lines 690199-690300)
float FloatCurveEvaluater::eval(float time) const {
	size_t count = _values.size();

	if (count == 1)
		return _values[0];

	if (count == 0)
		return 0.0f;

	// before the first time: the first segment, followed backwards
	if (_times[0] >= time)
		return EaseLinear(segmentPart(time, _times[0], _times[1]), _values[0], _values[1]);

	size_t index = 0;

	for (;;) {
		size_t next = index + 1;

		if (next == count)
			return 0.0f;  // not reachable with two values or more

		if (_times[next] >= time) {
			return EaseLinear(segmentPart(time, _times[index], _times[next]), _values[index], _values[next]);
		}

		index = next;

		if (index == count - 1)
			return _values[index];  // after the last time
	}
}

// Confirmed (asm lines 696150-696230): the places of the array that are even are times, the odd ones values.
void FloatCurveEvaluater::ParseFloatArray(const std::vector<float> &values) {
	for (size_t i = 0; i < values.size(); i++) {
		if (i & 1)
			_values.push_back(values[i]);
		else
			_times.push_back(values[i]);
	}
}

// Confirmed (asm lines 690363-690523)
uint32_t ColorCurveEvaluater::eval(float time) const {
	size_t count = _colors.size();

	if (count == 1)
		return _colors[0];

	if (count == 0)
		return 0;

	if (_times[0] >= time)
		return EaseLinearColor(segmentPart(time, _times[0], _times[1]), _colors[0], _colors[1]);

	size_t index = 0;

	for (;;) {
		size_t next = index + 1;

		if (next == count)
			return 0;

		if (_times[next] >= time)
			return EaseLinearColor(segmentPart(time, _times[index], _times[next]), _colors[index], _colors[next]);

		index = next;

		if (index == count - 1)
			return _colors[index];
	}
}

// Confirmed (asm lines 697440-697540): a colour is the number (cut to a whole one).
void ColorCurveEvaluater::ParseFloatArray(const std::vector<float> &values) {
	for (size_t i = 0; i < values.size(); i++) {
		if (i & 1)
			_colors.push_back(static_cast<uint32_t>(static_cast<int64_t>(values[i])));
		else
			_times.push_back(values[i]);
	}
}

// Confirmed (asm lines 691054-691190)
vec2 PointCurveEvaluater::eval(float time) const {
	size_t count = _points.size();

	if (count == 1)
		return _points[0];

	size_t timeCount = _times.size();

	for (size_t i = 0; i < timeCount; i++) {
		if (_times[i] < time) {
			if (i == timeCount - 1)
				return _points[i];

			continue;
		}

		if (i == 0)
			return EaseLinear(segmentPart(time, _times[0], _times[1]), _points[0], _points[1]);

		return EaseLinear(segmentPart(time, _times[i - 1], _times[i]), _points[i - 1], _points[i]);
	}

	return vec2();
}

// Confirmed (asm lines 691201-691240)
float MinMaxFloatCurveEvaluater::eval(float time, float mix) const {
	float lower = _min.eval(time);
	float upper = _max.eval(time);

	return (kOne - mix) * lower + upper * mix;
}

// Confirmed (asm lines 696248-696870). With an odd number of values the first -10000 is the divider; with no divider
// the original goes on with a size of -1 (a std::length_error), here the upper curve is then empty.
void MinMaxFloatCurveEvaluater::ParseFloatArray(const std::vector<float> &values) {
	std::vector<float> lower;
	std::vector<float> upper;

	if (values.size() & 1) {
		size_t divider = 0;

		while (divider < values.size() && values[divider] != -10000.0f)
			divider++;

		lower.assign(values.begin(), values.begin() + divider);

		if (divider + 1 < values.size())
			upper.assign(values.begin() + divider + 1, values.end());
	} else {
		size_t half = values.size() / 2;

		lower.assign(values.begin(), values.begin() + half);
		upper.assign(values.begin() + half, values.end());
	}

	_min.ParseFloatArray(lower);
	_max.ParseFloatArray(upper);
}

/** Confirmed (asm lines 689141-689255), with the saturation and the value 1: the hue (0 to 255, a circle) as a colour
 *  with the channels 0 to 255. */
static void HSVToRGB(float *r, float *g, float *b, float hue) {
	float degrees = hue * 1.4117647f;  // 360 / 255

	if (degrees == 360.0f) {
		*r = 1.0f;
		*g = 0.0f;
		*b = 0.0f;
	} else {
		float sector = degrees / 60.0f;
		int index = static_cast<int>(floorf(sector));
		float part = sector - static_cast<float>(index);
		float down = kOne - part;

		part = kOne - down;

		switch (index) {
		case 0:
			*r = 1.0f;
			*g = part;
			*b = 0.0f;
			break;
		case 1:
			*r = down;
			*g = 1.0f;
			*b = 0.0f;
			break;
		case 2:
			*r = 0.0f;
			*g = 1.0f;
			*b = part;
			break;
		case 3:
			*r = 0.0f;
			*g = down;
			*b = 1.0f;
			break;
		case 4:
			*r = part;
			*g = 0.0f;
			*b = 1.0f;
			break;
		case 5:
			*r = 1.0f;
			*g = 0.0f;
			*b = down;
			break;
		default:
			break;
		}
	}

	*r *= 255.0f;
	*g *= 255.0f;
	*b *= 255.0f;
}

// Confirmed (asm lines 693497-693700): everything is 0, the duration is 1.
Emitter::Emitter() {
}

Emitter::~Emitter() {
}

// Confirmed (asm lines 689094-689130, the same in each kind of emitter): the time goes on; at the end it begins again
// (always with 0 loops, else until it has done them) or, after the last loop, stays at the end.
void Emitter::UpdateEmitter(float step) {
	_time += step;

	if (_time > _duration) {
		bool again = false;

		if (_loops == 0) {
			again = true;
		} else if (_loops > _currentLoop) {
			_currentLoop++;
			again = (_loops > _currentLoop);
		}

		if (again) {
			_time = 0.0f;
			_phase = 0.0f / _duration;
			return;
		}

		_time = _duration;
	}

	_phase = _time / _duration;
}

// Confirmed (asm lines 689414-689490): as the others, and the direction of the line.
void LineEmitter::UpdateEmitter(float step) {
	_cos = static_cast<float>(cos(static_cast<double>(_angle)));
	_sin = static_cast<float>(sin(static_cast<double>(_angle)));

	Emitter::UpdateEmitter(step);
}

// Confirmed (asm lines 690535-690770). The position of the particle before it moved is kept for the next one in a
// static variable of the function (the same for all of the emitters).
void Emitter::Update(Particle &particle) {
	static vec2 oldPosition;

	if (_directionToRotation) {
		oldPosition.x = particle._x;
		oldPosition.y = particle._y;
	}

	particle._age += kParticleStep;

	float age = particle._age / particle._life;

	if (particle._angularVelocity != 0.0f)
		particle._direction += _angularVelocityOverLife.eval(age) * particle._angularVelocity;

	if (particle._motionRandomness > 0.0f) {
		float shake = nextSignedFloat() * particle._motionRandomness;

		particle._direction += _motionRandomnessOverLife.eval(age) * shake;
	}

	float speed = particle._velocity * _velocityOverLife.eval(age);
	double direction = static_cast<double>(particle._direction);

	particle._x = static_cast<float>(cos(direction)) * speed + particle._x;
	particle._y = static_cast<float>(sin(direction)) * speed + particle._y;

	if (particle._weight != 0.0f)
		particle._y = _weightOverLife.eval(age) * particle._weight + particle._y;

	if (particle._spin != 0.0f)
		particle._rotation += _spinOverLife.eval(age) * particle._spin;

	if (_directionToRotation) {
		particle._rotation = _directionToRotationOffset
		                     - atan2f(particle._y - oldPosition.y, particle._x - oldPosition.x);
	}

	particle._size = _sizeOverLife.eval(age);
	particle._alpha = _visibilityOverLife.eval(age) * particle._visibility;

	if (!_colorOverLife._colors.empty()) {
		uint32_t color = _colorOverLife.eval(age);

		particle._r = static_cast<uint8_t>(color);
		particle._g = static_cast<uint8_t>(color >> 8);
		particle._b = static_cast<uint8_t>(color >> 16);
	}
}

// Confirmed (asm lines 693701-694135): the values that a particle is born with. The random numbers are taken one for each
// of the curves, in this order.
void Emitter::EmitBasic(Particle &particle) {
	particle._frameTime = 0.0f;
	particle._age = 0.0f;

	particle._life = _life.eval(_phase, nextFloat());
	particle._visibility = _visibility.eval(_phase, nextFloat());
	particle._baseSize = _size.eval(_phase, nextFloat());
	particle._angularVelocity = _angularVelocity.eval(_phase, nextFloat());
	particle._weight = _weight.eval(_phase, nextFloat());
	particle._rotation = _rotation.eval(_phase, nextFloat());
	particle._size = _sizeOverLife.eval(0.0f);
	particle._spin = _spin.eval(_phase, nextFloat());
	particle._motionRandomness = _motionRandomness.eval(_phase, nextFloat());
}

/** The last of what each Emit() does: the direction and the speed (two more random numbers). */
void Emitter::emitDirection(Particle &particle) {
	particle._direction = _emissionDirection.eval(_phase, nextFloat());
	particle._velocity = _velocity.eval(_phase, nextFloat());
}

// Confirmed (asm lines 695129-695255): the basic emitter does not give the particle a place.
void Emitter::Emit(Particle &particle) {
	EmitBasic(particle);
	emitDirection(particle);
}

// Confirmed (asm lines 694137-694365): the particle is in a square ring: both coordinates are by chance between
// -(radius - inner) and (radius - inner), and moved away from the center by `inner`.
void SquareEmitter::Emit(Particle &particle) {
	EmitBasic(particle);

	float width = _radius - _innerRadius;
	float offsetX = nextSignedFloat() * width;
	float offsetY = nextSignedFloat() * width;

	offsetY = (offsetY > 0.0f) ? offsetY + _innerRadius : offsetY - _innerRadius;
	offsetX = (offsetX > 0.0f) ? offsetX + _innerRadius : offsetX - _innerRadius;

	particle._y = offsetY + _center.y;
	particle._x = offsetX + _center.x;

	emitDirection(particle);
}

// Confirmed (asm lines 694369-694612): the distance from the center is the square root of a random number (for an even
// spread), the angle a random number times 6.28.
void CircleEmitter::Emit(Particle &particle) {
	EmitBasic(particle);

	float root = sqrtf(nextFloat());
	float angle = static_cast<float>(nextDouble() * 6.28000020980835);
	float distance = (_radius - _innerRadius) * root + _innerRadius;

	particle._y = -sinf(angle) * distance + _center.y;
	particle._x = distance * cosf(angle) + _center.x;

	emitDirection(particle);
}

// Confirmed (asm lines 694617-694815)
void BoxEmitter::Emit(Particle &particle) {
	EmitBasic(particle);

	particle._y = (nextFloat() - 0.5f) * _sizeY + _center.y;
	particle._x = (nextFloat() - 0.5f) * _sizeX + _center.x;

	emitDirection(particle);
}

// Confirmed (asm lines 694820-694990)
void LineEmitter::Emit(Particle &particle) {
	EmitBasic(particle);

	float along = nextSignedFloat();

	particle._y = _sin * along * _length + _center.y;
	particle._x = along * _cos * _length + _center.x;

	emitDirection(particle);
}

// Confirmed (asm lines 694996-695125)
void PointEmitter::Emit(Particle &particle) {
	EmitBasic(particle);

	particle._x = _center.x;
	particle._y = _center.y;

	emitDirection(particle);
}

// Confirmed (asm lines 695259-695480): a place of the picture by chance (the index is cut from the float product), moved
// by half the size of the picture, and its colour with the image type 1.
void ImageEmitter::Emit(Particle &particle) {
	EmitBasic(particle);

	if (!_points.empty()) {
		float last = static_cast<float>(_points.size() - 1);
		int index = static_cast<int>(nextFloat() * last);
		const Point &point = _points[index];

		particle._y = point._y - static_cast<float>(_height / 2) + _center.y;
		particle._x = point._x - static_cast<float>(_width / 2) + _center.x;

		if (_imageType == 1) {
			uint32_t color = _colors[index];

			particle._r = static_cast<uint8_t>(color);
			particle._g = static_cast<uint8_t>(color >> 8);
			particle._b = static_cast<uint8_t>(color >> 16);
		}
	}

	emitDirection(particle);
}

// Confirmed (asm lines 697548-697780). The pixels with an alpha of 128 or more are the places; TPictureIO has no
// decoder yet (see picture.h), so with it no place is found.
void ImageEmitter::CreateWithImage(const char *path, int mode) {
	_path = path;
	_imageType = mode;

	TPictureIO picture;
	wxString name;

	toUTF(&name, path);

	wxFileName file(name.ToStdWstring());

	file.NormalizePath();

	if (!file.IsOk())
		return;

	// the original passes 2 (a load setting that picture.h does not name)
	if (!picture.LoadPicture(file, static_cast<TPictureIO::eLoadSetting>(2)))
		return;

	_width = picture.GetWidth();
	_height = picture.GetHeight();

	const uint32_t *pixels = picture.GetMemoryData();

	if (pixels) {
		for (int y = 0; y < _height; y++) {
			for (int x = 0; x < _width; x++, pixels++) {
				if ((*pixels >> 24) < 128)
					continue;

				Point point = { static_cast<float>(x), static_cast<float>(y) };

				_points.push_back(point);

				if (mode == 1)
					_colors.push_back(*pixels);
			}
		}
	}

	picture.Clear();
}

// Confirmed (asm lines 696062-696148). The picture (TPictureIO) is made at once.
ParticleContainer::ParticleContainer() {
	_picture = new TPictureIO();
}

// Confirmed (asm lines 695920-696060)
ParticleContainer::~ParticleContainer() {
	delete _picture;
	delete _emitter;
	delete _texture;
}

// Confirmed (asm lines 698425-698925 and the copy of it for children, 697953-698424). The order of the random numbers
// (the hue of a new particle, then what the emitter takes, then the picture) is the original's.
void ParticleContainer::Update(bool usePhase, const vec2 &pos, float phase, bool updateChildren) {
	Emitter *emitter = _emitter;

	if (usePhase)
		emitter->_phase = phase;

	emitter->UpdateEmitter(kEmitterStep);

	float wanted = emitter->_numberOfEmitted.eval(emitter->_phase) + emitter->_emitRemainder;
	int toEmit = static_cast<int>(wanted);

	emitter->_emitRemainder = wanted - static_cast<float>(toEmit);

	int emitted = 0;
	// With usePhase the container either moves the particles it has (updateChildren) or makes new ones (not).
	bool updateExisting = !usePhase || updateChildren;
	bool canSpawn = !usePhase || !updateChildren;
	size_t particleCount = _particles.size();

	for (size_t i = 0; i < particleCount; i++) {
		// each particle moves the children along
		for (size_t c = 0; c < _children.size(); c++) {
			Particle &moved = _particles[i];
			vec2 where;

			where.x = moved._x;
			where.y = moved._y;
			_children[c].Update(false, where, moved._age / moved._life, false);
		}

		Particle &particle = _particles[i];

		if (particle._age != -1.0f && particle._age < particle._life) {
			if (!updateExisting)
				continue;

			emitter->Update(particle);

			if (!_imageRandom && static_cast<double>(particle._age - particle._frameTime) > _imageInterval) {
				size_t spriteCount = _sprites.size();

				particle._frameTime = particle._age;

				if (spriteCount == 0)
					spriteCount = 1;

				particle._frame = static_cast<int64_t>((static_cast<uint64_t>(particle._frame) + 1) % spriteCount);
			}

			continue;
		}

		// the slot is free (age -1) or its particle is over
		if (canSpawn && toEmit > emitted && _count < _maximum) {
			if (particle._age == -1.0f)
				_count++;

			emitted++;
			emitter->Emit(particle);

			if (usePhase) {
				particle._x = pos.x;
				particle._y = pos.y;
			}

			continue;
		}

		if (particle._age != -1.0f) {
			particle._age = -1.0f;
			_count--;
		}
	}

	if (canSpawn && emitted < _creationRate && toEmit > emitted && _count < _maximum) {
		do {
			Particle fresh;

			_particles.push_back(fresh);

			Particle &particle = _particles.back();

			if (usePhase) {
				particle._x = pos.x;
				particle._y = pos.y;
			}

			float r = 0.0f;
			float g = 0.0f;
			float b = 0.0f;

			HSVToRGB(&r, &g, &b, static_cast<float>(nextDouble() * 255.0));
			particle._r = static_cast<uint8_t>(static_cast<int>(r));
			particle._g = static_cast<uint8_t>(static_cast<int>(g));
			particle._b = static_cast<uint8_t>(static_cast<int>(b));
			particle._alpha = 0.0f;
			emitter->Emit(particle);

			if (_imageRandom) {
				float spriteCount = static_cast<float>(_sprites.size());
				float last = static_cast<float>(static_cast<double>(spriteCount) - 0.001);

				particle._frame = static_cast<int64_t>(static_cast<float>(nextDouble() * static_cast<double>(last)));
			} else {
				particle._frame = 0;
			}

			_count++;
			emitted++;
		} while (emitted < _creationRate && emitted != toEmit && _count < _maximum);
	}

	if (updateChildren) {
		vec2 origin;

		for (size_t c = 0; c < _children.size(); c++)
			_children[c].Update(false, origin, 0.0f, true);
	}
}

// TODO: the drawing needs the vertex buffers of the GL backend (`particlePipeline`, asm 696877-697440).
void ParticleContainer::Draw() {
}
