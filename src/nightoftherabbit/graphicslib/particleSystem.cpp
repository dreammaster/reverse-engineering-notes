#include "graphicslib/particleSystem.h"

#include <math.h>
#include <string.h>
#include <time.h>

#include "graphicslib/picture.h"

/** The seed of the random numbers of the particles. */
static uint32_t RandomSeed = 12345678;

/** The int that is kept in the place of a float of the particle arrays. */
static int getInt(const float *at) {
	int value;

	memcpy(&value, at, sizeof(value));
	return value;
}

static void setInt(float *at, int value) {
	memcpy(at, &value, sizeof(value));
}

// Confirmed (asm lines 766200-766230)
void Randomize() {
	RandomSeed = static_cast<uint32_t>(time(nullptr));
}

// Confirmed (asm lines 766231-766242)
unsigned Rand0() {
	RandomSeed = RandomSeed * 0x343FD + 0x269EC3;
	return (static_cast<int32_t>(RandomSeed) >> 10) & 0xFFFF;
}

// Confirmed (asm lines 766243-766278)
unsigned Rand(unsigned limit) {
	RandomSeed = RandomSeed * 0x343FD + 0x269EC3;

	uint64_t value = (static_cast<int32_t>(RandomSeed) >> 10) & 0xFFFF;

	if (limit <= 0x7FFF)
		return static_cast<unsigned>((value * limit) >> 16) & 0xFFFFFFFFu;

	return static_cast<unsigned>((value * static_cast<uint64_t>(static_cast<int64_t>(static_cast<int>(limit)))) >> 16);
}

/** A random number from 0 up to 1 (a step of the generator times 2^-16). */
static float nextFraction() {
	return static_cast<float>(Rand0()) * (1.0f / 65536.0f);
}

// Confirmed (asm lines 766279-766330)
int BlendColors(int a, int b, float factor) {
	float rest = 1.0f - factor;
	int red = static_cast<int>(static_cast<float>((a & 0xFF0000) >> 16) * factor + static_cast<float>((b & 0xFF0000) >> 16) * rest);
	int green = static_cast<int>(static_cast<float>((a >> 8) & 0xFF) * factor + static_cast<float>((b >> 8) & 0xFF) * rest);
	int blue = static_cast<int>(static_cast<float>(a & 0xFF) * factor + static_cast<float>(b & 0xFF) * rest);

	return (red << 16) + (green << 8) + blue;
}

// Confirmed (asm lines 769421-769642): everything is set as the defaults of the data are, the curves of the stages are 0,
// the curve of the size 1 and that of the tiles 0.
TParticleEmitter::TParticleEmitter() {
	_position.x = 99999.9f;
	_force = TVector3D(0.0f, -1.0f, 0.0f);
	_rotation.Identity();

	for (int i = 0; i < 4; i++) {
		_stageColors[i] = 0xFFFFFF;

		for (int j = 0; j < 100; j++)
			_stageCurves[i][j] = 0.0f;
	}

	for (int j = 0; j < 100; j++) {
		_sizeCurve[j] = 1.0f;
		_tileCurve[j] = 0.0f;
	}
}

// Confirmed (asm lines 766025-766096)
TParticleEmitter::~TParticleEmitter() {
	delete[] _particles;
	_particles = nullptr;
	delete[] _motions;
	_motions = nullptr;
}

// Confirmed (asm lines 769642-769675)
void TParticleEmitter::Clear() {
	_unknown14B8 = 1.0f;
	delete[] _particles;
	_particles = nullptr;
	delete[] _motions;
	_motions = nullptr;
	_maxParticles = 0;
}

// Confirmed (asm lines 769675-769796)
void TParticleEmitter::InitEmitter(int maxParticles, ParticleType type) {
	_unknown14B8 = 1.0f;
	delete[] _particles;
	_particles = nullptr;
	delete[] _motions;
	_motions = nullptr;
	_maxParticles = 0;
	_particleType = type;

	switch (type) {
	case kBillboard:
		_particleStride = 5;
		break;
	case kRandomTile:
		_particleStride = 7;
		break;
	case kStreak:
		_particleStride = 8;
		break;
	case kAnimatedTile:
		_particleStride = 10;
		break;
	}

	_emitRemainder = 0.0f;
	_motionStride = 7;
	_particles = new float[static_cast<size_t>(_particleStride) * maxParticles];
	_motions = new float[static_cast<size_t>(_motionStride) * maxParticles];
	_maxParticles = maxParticles;
	_count = 0;
	RandomSeed = static_cast<uint32_t>(time(nullptr));
}

// Confirmed (asm lines 769796-770062). The direction is turned to the z axis in two turns, a yaw round y (by the angle
// from z to the direction in the xz plane, with the sign of x) and then a pitch round x (by the angle of what is left of
// the vector to z, with the sign of y); the matrix is the transpose of pitch times yaw.
void TParticleEmitter::SetDirection(const TVector3D &direction) {
	float x = direction.x;
	float y = direction.y;
	float z = direction.z;
	float lengthXZ = sqrtf(x * x + z * z);
	TMatrix3 yaw;

	yaw.Identity();

	float yPrime = y;
	float zPrime = z;

	if (lengthXZ != 0.0f) {
		float angle = acosf(z / lengthXZ);

		if (!(x > 0.0f))
			angle = -angle;

		yaw.RotationY(angle);

		float s = sinf(angle);
		float c = cosf(angle);

		yPrime = (x * 0.0f + y) + z * 0.0f;
		zPrime = (s * x + y * 0.0f) + c * z;
	}

	float lengthYZ = sqrtf(yPrime * yPrime + zPrime * zPrime);
	float pitchAngle = acosf(zPrime / lengthYZ);

	if (yPrime > 0.0f)
		pitchAngle = -pitchAngle;

	TMatrix3 pitch;

	pitch.RotationX(pitchAngle);

	TMatrix3 product = pitch * yaw;

	product.Transpose();
	_rotation = product;
}

// Confirmed (asm lines 770062-770132)
void TParticleEmitter::SetEmitter(const TVector3D &corner1, const TVector3D &corner2, const TVector3D &direction,
                                  float spreadX, float spreadY) {
	_boxMin = corner1;
	_boxMax = corner2;

	if (_boxMin.x > _boxMax.x) {
		float t = _boxMin.x;

		_boxMin.x = _boxMax.x;
		_boxMax.x = t;
	}

	if (_boxMin.y > _boxMax.y) {
		float t = _boxMin.y;

		_boxMin.y = _boxMax.y;
		_boxMax.y = t;
	}

	if (_boxMin.z > _boxMax.z) {
		float t = _boxMin.z;

		_boxMin.z = _boxMax.z;
		_boxMax.z = t;
	}

	_boxSize.z = corner2.z - corner1.z;
	_boxSize.y = corner2.y - corner1.y;
	_boxSize.x = corner2.x - corner1.x;
	_direction = direction;
	_spreadX = spreadX;
	_spreadY = spreadY;
	SetDirection(direction);
	_geometry = 2;
}

// Confirmed (asm lines 770132-770164)
void TParticleEmitter::SetEmitter(const TVector3D &position, const TVector3D &direction, float spreadX, float spreadY) {
	_position = position;
	_direction = direction;
	_spreadX = spreadX;
	_spreadY = spreadY;
	SetDirection(direction);
	_geometry = 1;
}

// Confirmed (asm lines 770164-770208)
const uint32_t *TParticleEmitter::GetSorterIndices() {
	if (_sorter.Grow(_maxParticles, _count, _particleStride))
		_sorter.Sort(_particles + 2, _count);

	return _sorter.GetRanks();
}

// Confirmed (asm lines 770208-771112). The first loop moves the particles that are there, the second makes new ones;
// the random numbers are taken in the order of the original (two angles, the colour, the speed, the place of a box, the
// size, the tile and the life).
bool TParticleEmitter::IncTime() {
	int tiles = _tilesX * _tilesY;
	float maxTile;

	if (_maxTile == -1 || _maxTile >= tiles)
		maxTile = static_cast<float>(tiles - 1);
	else
		maxTile = static_cast<float>(_maxTile);

	float blend = _forceBlend;
	int index = 0;

	while (index < _count) {
		float *particle = _particles + static_cast<size_t>(index) * _particleStride;
		float *motion = _motions + static_cast<size_t>(index) * _motionStride;

		if (_particleType == kStreak) {
			particle[5] = particle[0];
			particle[6] = particle[1];
			particle[7] = particle[2];
		}

		float x = particle[0] + motion[0];
		float y = particle[1] + motion[1];
		float z = particle[2] + motion[2];

		particle[0] = x;
		particle[1] = y;
		particle[2] = z;

		if (_forceType == 1) {
			float dx = x - _forcePosition.x;
			float dy = y - _forcePosition.y;
			float dz = z - _forcePosition.z;
			float length = sqrtf(dx * dx + dy * dy + dz * dz);
			float rest = 1.0f - blend;

			motion[2] = (_force.z / length) * rest + motion[2] * blend;
			motion[1] = (_force.y / length) * rest + motion[1] * blend;
			motion[0] = (_force.x / length) * rest + motion[0] * blend;
		} else if (_forceType == 2) {
			float dx = x - _forcePosition.x;
			float dy = y - _forcePosition.y;
			float dz = z - _forcePosition.z;
			float length = sqrtf(dx * dx + dy * dy + dz * dz);
			float distance = (length > 0.1f) ? length : 0.1f;
			float rest = 1.0f - blend;

			motion[2] = (((dz / distance) * _forceStrength) / distance) * rest + motion[2] * blend;
			motion[1] = (((dy / distance) * _forceStrength) / distance) * rest + motion[1] * blend;
			motion[0] = (((dx / distance) * _forceStrength) / distance) * rest + motion[0] * blend;
		} else if (_forceType == 0) {
			float rest = 1.0f - blend;

			motion[2] = _force.z * rest + motion[2] * blend;
			motion[1] = _force.y * rest + motion[1] * blend;
			motion[0] = rest * _force.x + motion[0] * blend;
		}

		int left = getInt(motion + 3) - 1;
		int total = getInt(motion + 4);

		setInt(motion + 3, left);

		int age = (total != 0) ? ((total - left) * 99) / total : 0;
		int color = getInt(motion + 5);

		setInt(particle + 4, color);

		for (int stage = 0; stage < _activeStages; stage++) {
			color = BlendColors(_stageColors[stage], color, _stageCurves[stage][age]);
			setInt(particle + 4, color);
		}

		particle[3] = _sizeCurve[age] * motion[6];

		if (_particleType == kAnimatedTile) {
			float position = maxTile * _tileCurve[age];
			int tile = static_cast<int>(position);
			float tileFloat = static_cast<float>(tile);
			int level = static_cast<int>((position - tileFloat) * 255.0f);

			setInt(particle + 5, level * 0x01010101);

			int next = tile;
			float nextFloat = tileFloat;

			if (maxTile > tileFloat) {
				next = tile + 1;
				nextFloat = static_cast<float>(next);
			}

			int row = tile / _tilesX;
			int nextRow = next / _tilesX;

			particle[6] = tileFloat - static_cast<float>(_tilesX * row);
			particle[7] = static_cast<float>(row);
			particle[8] = nextFloat - static_cast<float>(_tilesX * nextRow);
			particle[9] = static_cast<float>(nextRow);
		}

		if (left != 0) {
			index++;
			continue;
		}

		// the life is over: the last particle takes its place
		_count--;

		if (index >= _count)
			break;

		memcpy(particle, _particles + static_cast<size_t>(_count) * _particleStride, sizeof(float) * _particleStride);
		memcpy(motion, _motions + static_cast<size_t>(_count) * _motionStride, sizeof(float) * _motionStride);
	}

	float wanted = _emitRemainder + _perTime;
	int make = static_cast<int>(wanted);

	_emitRemainder = wanted - static_cast<float>(make);

	if (make > 0 && _maxParticles > _count) {
		int last = _count + make - 1;

		for (int next = _count; ;) {
			float *particle = _particles + static_cast<size_t>(next) * _particleStride;
			float *motion = _motions + static_cast<size_t>(next) * _motionStride;

			// the direction: a cone round z, given by two angles
			float angleA = nextFraction() * _spreadX - _spreadX * 0.5f;
			float sinA = sinf(angleA);
			float cosA = cosf(angleA);
			float angleB = nextFraction() * _spreadY - _spreadY * 0.5f;
			float sinB = sinf(angleB);
			float cosB = cosf(angleB);
			float colorFraction = nextFraction();
			float speed = nextFraction() * (_maxVelocity - _minVelocity) + _minVelocity;
			float zero = 0.0f * speed;
			float aY = speed * sinA;
			float aX = speed * cosA;
			float vy = zero * cosA + aY;
			float xTurned = aX - sinA * zero;
			float vz = zero * sinB + xTurned * cosB;
			float vx = zero * cosB - xTurned * sinB;
			float vxOut = _rotation._m[0] * vx + _rotation._m[1] * vy + _rotation._m[2] * vz;
			float vyOut = _rotation._m[3] * vx + _rotation._m[4] * vy + _rotation._m[5] * vz;
			float vzOut = vx * _rotation._m[6] + vy * _rotation._m[7] + vz * _rotation._m[8];

			// the place
			if (_geometry == 1) {
				particle[0] = _position.x * _pixelScale;
				particle[1] = -_pixelScale * _position.y;
				particle[2] = _pixelScale * _position.z;
			} else if (_geometry == 2) {
				float negative = -_pixelScale;
				float zStart = _boxMin.z * _pixelScale;
				float yStart = _boxMin.y * negative;
				float xStart = _boxMin.x * _pixelScale;

				particle[2] = static_cast<float>(Rand0()) * _boxSize.z * (1.0f / 65536.0f) * _pixelScale + zStart;
				particle[1] = static_cast<float>(Rand0()) * _boxSize.y * (1.0f / 65536.0f) * negative + yStart;
				particle[0] = static_cast<float>(Rand0()) * _boxSize.x * (1.0f / 65536.0f) * _pixelScale + xStart;
			}

			float size = (_maxSize - _minSize) * static_cast<float>(Rand0()) * (1.0f / 65536.0f) + _minSize;

			motion[6] = size;
			particle[3] = size * _sizeCurve[0];

			int color = BlendColors(_minColor, _maxColor, colorFraction);

			setInt(motion + 5, color);
			setInt(particle + 4, color);

			for (int stage = 0; stage < _activeStages; stage++) {
				color = BlendColors(_stageColors[stage], color, _stageCurves[stage][0]);
				setInt(particle + 4, color);
			}

			if (_particleType == kStreak) {
				particle[5] = particle[0];
				particle[6] = particle[1];
				particle[7] = particle[2];
			} else if (_particleType == kAnimatedTile) {
				float position = maxTile * _tileCurve[0];
				int tile = static_cast<int>(position);
				float tileFloat = static_cast<float>(tile);
				int level = static_cast<int>((position - tileFloat) * 255.0f);

				setInt(particle + 5, level * 0x01010101);

				int nextTile = tile;
				float nextFloat = tileFloat;

				if (maxTile > tileFloat) {
					nextTile = tile + 1;
					nextFloat = static_cast<float>(nextTile);
				}

				int row = tile / _tilesX;
				int nextRow = nextTile / _tilesX;

				particle[6] = tileFloat - static_cast<float>(_tilesX * row);
				particle[7] = static_cast<float>(row);
				particle[8] = nextFloat - static_cast<float>(_tilesX * nextRow);
				particle[9] = static_cast<float>(nextRow);
			} else if (_particleType == kRandomTile) {
				int tile = static_cast<int>(Rand(static_cast<unsigned>(static_cast<int>(maxTile + 1.0f))));

				particle[6] = static_cast<float>(tile / _tilesX);
				particle[5] = static_cast<float>(tile % _tilesX);
			}

			motion[0] = vxOut;
			motion[1] = vyOut;
			motion[2] = vzOut;

			int range = _maxLife - _minLife;
			int life = _minLife + static_cast<int>(Rand(static_cast<unsigned>(range)));

			setInt(motion + 4, life);
			setInt(motion + 3, life);
			_count++;

			if (next == last)
				break;

			next++;

			if (_maxParticles <= next)
				break;
		}
	}

	if (_materialMode == 0) {
		if (_sorter.Grow(_maxParticles, _count, _particleStride))
			_sorter.Sort(_particles + 2, _count);
	}

	return true;
}

// Confirmed (asm lines 771127-771155)
TParticleSystem::TParticleSystem() {
}

// Confirmed (asm lines 766096-766193)
TParticleSystem::~TParticleSystem() {
	Clear();
}

// Confirmed (asm lines 771155-771216)
void TParticleSystem::Clear() {
	for (size_t i = 0; i < _emitters.size(); i++) {
		delete _emitters[i];
		delete _textures[i];
	}

	_emitters.clear();
	_textures.clear();
}

// Confirmed (asm lines 771216-771276)
bool TParticleSystem::IncTime() {
	for (size_t i = 0; i < _emitters.size(); i++) {
		if (!_emitters[i]->IncTime())
			return false;
	}

	return true;
}

// Confirmed (asm lines 771276-771309)
void TParticleSystem::SetScaleFactor(float factor) {
	_scaleFactor = factor;

	for (TParticleEmitter *emitter : _emitters)
		emitter->_scaleFactor = factor;
}

// Confirmed (asm lines 771309-771351)
void TParticleSystem::SetWindowSize(int width, int height) {
	_windowWidth = static_cast<float>(width);
	_windowHeight = static_cast<float>(height);

	float scale = -2.0f * _cameraZ * _cameraZoom / _windowHeight;

	for (TParticleEmitter *emitter : _emitters)
		emitter->_pixelScale = scale;
}

// Confirmed (asm lines 771351-771370)
TVector3D TParticleSystem::GetWindowSize() const {
	return TVector3D(_windowWidth, _windowHeight, _unknown50);
}
