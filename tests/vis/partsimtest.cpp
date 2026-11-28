#include <cmath>
#include <cstdio>
#include <cstring>

#include "AppGlobals.h"
#include "graphicslib/particleSystem.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

static bool near(float a, float b, float epsilon = 1e-4f) {
	return std::fabs(a - b) <= epsilon;
}

static int intAt(const float *at) {
	int value;

	std::memcpy(&value, at, sizeof(value));
	return value;
}

// An emitter that makes particles of a fixed life that fly along z, so that the count and the places are known.
static void plain(TParticleEmitter &emitter, int maxParticles, TParticleEmitter::ParticleType type) {
	emitter.InitEmitter(maxParticles, type);
	emitter._pixelScale = 1.0f;
	emitter._perTime = 3.0f;
	emitter._minLife = emitter._maxLife = 5;
	emitter._minVelocity = emitter._maxVelocity = 2.0f;
	emitter._minSize = emitter._maxSize = 0.5f;
	emitter._forceBlend = 1.0f;
	emitter._materialMode = 1;
	emitter.SetEmitter(TVector3D(10.0f, 20.0f, 0.0f), TVector3D(0.0f, 0.0f, 1.0f), 0.0f, 0.0f);
}

// The simulation of the particles of the editor's systems: the numbers of them, where they are made and how they fly, the
// forces, the colours and sizes over the life, the kinds of particle and their sorting by depth.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);

	// the generator and the blending of colours
	Randomize();
	for (int i = 0; i < 100; i++) {
		CHECK(Rand0() < 65536);
		CHECK(Rand(10) < 10);
	}
	CHECK(Rand(1) == 0);
	CHECK(Rand(0) == 0);
	CHECK(BlendColors(0xFF0000, 0x0000FF, 1.0f) == 0xFF0000);
	CHECK(BlendColors(0xFF0000, 0x0000FF, 0.0f) == 0x0000FF);
	CHECK(BlendColors(0xFF0000, 0x0000FF, 0.5f) == 0x7F007F);
	CHECK(BlendColors(0x102030, 0x102030, 0.3f) >= 0x0F1F2F);

	// the count: three a step, each of a life of five steps (the first are let go in the sixth step)
	{
		TParticleEmitter emitter;

		plain(emitter, 100, TParticleEmitter::kBillboard);
		CHECK(emitter._particleStride == 5);
		CHECK(emitter._count == 0);

		int expected[] = {3, 6, 9, 12, 15, 15, 15, 15};

		for (int step = 0; step < 8; step++) {
			CHECK(emitter.IncTime());
			if (emitter._count != expected[step])
				printf("step %d: %d particles (expected %d)\n", step, emitter._count, expected[step]);
			CHECK(emitter._count == expected[step]);
		}

		// the places: made at the position (the y is turned), moving 2 along z each step
		emitter.Clear();
		CHECK(emitter._maxParticles == 0);
		CHECK(emitter._particles == nullptr);

		plain(emitter, 100, TParticleEmitter::kBillboard);
		emitter.IncTime();
		CHECK(near(emitter._particles[0], 10.0f) && near(emitter._particles[1], -20.0f) && near(emitter._particles[2], 0.0f));
		CHECK(near(emitter._particles[3], 0.5f));
		emitter.IncTime();
		CHECK(near(emitter._particles[0], 10.0f) && near(emitter._particles[1], -20.0f) && near(emitter._particles[2], 2.0f));
		CHECK(near(emitter._particles[5 * 3 + 2], 0.0f));
		CHECK(intAt(emitter._particles + 4) == 0xFFFFFF);
	}

	// fractions of a particle are kept, and the room is limited
	{
		TParticleEmitter emitter;

		plain(emitter, 8, TParticleEmitter::kBillboard);
		emitter._perTime = 2.5f;
		emitter.IncTime();
		CHECK(emitter._count == 2);
		emitter.IncTime();
		CHECK(emitter._count == 5);
		emitter.IncTime();
		CHECK(emitter._count == 7);
		emitter.IncTime();
		CHECK(emitter._count == 8);
		emitter.IncTime();
		CHECK(emitter._count == 8);

		// nothing is made when the rate is under one, until it adds up to one
		TParticleEmitter slow;

		plain(slow, 8, TParticleEmitter::kBillboard);
		slow._perTime = 0.4f;
		slow.IncTime();
		slow.IncTime();
		CHECK(slow._count == 0);
		slow.IncTime();
		CHECK(slow._count == 1);
	}

	// made in a box
	{
		TParticleEmitter emitter;

		plain(emitter, 200, TParticleEmitter::kBillboard);
		emitter._perTime = 50.0f;
		emitter.SetEmitter(TVector3D(0.0f, 0.0f, 0.0f), TVector3D(100.0f, 50.0f, 10.0f), TVector3D(0.0f, 0.0f, 1.0f), 0.0f, 0.0f);
		CHECK(emitter._geometry == 2);
		emitter.IncTime();

		bool inside = true;
		float minX = 1000.0f, maxX = -1000.0f;

		for (int i = 0; i < emitter._count; i++) {
			const float *particle = emitter._particles + i * emitter._particleStride;

			if (particle[0] < 0.0f || particle[0] > 100.0f || particle[1] > 0.0f || particle[1] < -50.0f || particle[2] < 0.0f ||
			    particle[2] > 10.0f)
				inside = false;
			minX = std::min(minX, particle[0]);
			maxX = std::max(maxX, particle[0]);
		}
		CHECK(emitter._count == 50);
		CHECK(inside);
		CHECK(maxX - minX > 40.0f);

		// the corners given the wrong way round are put in order for the box, but the size is the one of the corners as they were
		// given (so the particles are made on the other side of the lower corner: the same in the original)
		TParticleEmitter reversed;

		plain(reversed, 200, TParticleEmitter::kBillboard);
		reversed._perTime = 50.0f;
		reversed.SetEmitter(TVector3D(100.0f, 50.0f, 10.0f), TVector3D(0.0f, 0.0f, 0.0f), TVector3D(0.0f, 0.0f, 1.0f), 0.0f, 0.0f);
		CHECK(near(reversed._boxMin.x, 0.0f) && near(reversed._boxMax.x, 100.0f));
		CHECK(near(reversed._boxMin.y, 0.0f) && near(reversed._boxMax.y, 50.0f));
		CHECK(near(reversed._boxSize.x, -100.0f) && near(reversed._boxSize.y, -50.0f));
		reversed.IncTime();

		bool lower = true;

		for (int i = 0; i < reversed._count; i++) {
			const float *particle = reversed._particles + i * reversed._particleStride;

			if (particle[0] > 0.0f || particle[0] < -100.0f)
				lower = false;
		}
		CHECK(lower);
	}

	// the direction: turned to z (a yaw, then a pitch), and a cone
	{
		TParticleEmitter emitter;

		plain(emitter, 100, TParticleEmitter::kBillboard);
		emitter.SetEmitter(TVector3D(0.0f, 0.0f, 0.0f), TVector3D(1.0f, 0.0f, 0.0f), 0.0f, 0.0f);
		emitter.IncTime();
		emitter.IncTime();
		// (flying along x: the speed 2)
		CHECK(near(emitter._particles[0], 2.0f, 1e-3f) && near(emitter._particles[1], 0.0f, 1e-3f) &&
		      near(emitter._particles[2], 0.0f, 1e-3f));

		emitter.Clear();
		plain(emitter, 100, TParticleEmitter::kBillboard);
		emitter.SetEmitter(TVector3D(0.0f, 0.0f, 0.0f), TVector3D(0.0f, 1.0f, 0.0f), 0.0f, 0.0f);
		emitter.IncTime();
		emitter.IncTime();
		CHECK(near(emitter._particles[0], 0.0f, 1e-3f) && near(emitter._particles[1], 2.0f, 1e-3f) &&
		      near(emitter._particles[2], 0.0f, 1e-3f));

		// a cone: the particles spread, but the speed is the same
		emitter.Clear();
		plain(emitter, 100, TParticleEmitter::kBillboard);
		emitter._perTime = 30.0f;
		emitter.SetEmitter(TVector3D(0.0f, 0.0f, 0.0f), TVector3D(0.0f, 0.0f, 1.0f), 1.0f, 1.0f);
		emitter.IncTime();
		emitter.IncTime();

		float spread = 0.0f;

		for (int i = 0; i < 30; i++) {
			const float *particle = emitter._particles + i * emitter._particleStride;

			spread = std::max(spread, std::fabs(particle[0]) + std::fabs(particle[1]));
			CHECK(std::sqrt(particle[0] * particle[0] + particle[1] * particle[1] + particle[2] * particle[2]) < 2.001f);
		}
		CHECK(spread > 0.05f);
	}

	// the forces: constant, to a point and from a point (with how much of its speed a particle keeps)
	{
		TParticleEmitter emitter;

		plain(emitter, 100, TParticleEmitter::kBillboard);
		emitter._forceType = 0;
		emitter._forceBlend = 0.5f;
		emitter._force = TVector3D(0.0f, -4.0f, 0.0f);
		emitter.IncTime();
		emitter.IncTime();
		// the motion of the first one after one step with the force: y -2, z 1
		CHECK(near(emitter._motions[1], -2.0f) && near(emitter._motions[2], 1.0f));

		TParticleEmitter toPoint;

		plain(toPoint, 100, TParticleEmitter::kBillboard);
		toPoint._forceType = 1;
		toPoint._forceBlend = 0.5f;
		toPoint._forcePosition = TVector3D(0.0f, 0.0f, 0.0f);
		toPoint._force = TVector3D(0.0f, 0.0f, 0.0f);
		toPoint.IncTime();
		toPoint.IncTime();
		CHECK(near(toPoint._motions[2], 1.0f));

		TParticleEmitter away;

		plain(away, 100, TParticleEmitter::kBillboard);
		away._forceType = 2;
		away._forceBlend = 0.5f;
		away._forceStrength = 10.0f;
		away._forcePosition = TVector3D(0.0f, 0.0f, -4.0f);
		away.IncTime();
		away.IncTime();

		float after = away._motions[2];

		printf("repelled: z speed %f\n", after);
		CHECK(after != 2.0f);
	}

	// the colours: the stages blend over the colour of the particle, by the curve of the age; the size follows its curve
	{
		TParticleEmitter emitter;

		plain(emitter, 100, TParticleEmitter::kBillboard);
		emitter._minColor = emitter._maxColor = 0xFF0000;
		emitter._activeStages = 1;
		emitter._stageColors[0] = 0x0000FF;

		for (int i = 0; i < 100; i++) {
			emitter._stageCurves[0][i] = (i >= 40) ? 1.0f : 0.0f;
			emitter._sizeCurve[i] = 1.0f - i / 99.0f;
		}

		emitter.IncTime();
		CHECK(intAt(emitter._particles + 4) == 0xFF0000);
		CHECK(near(emitter._particles[3], 0.5f));

		for (int i = 0; i < 4; i++)
			emitter.IncTime();

		// (the first particle is four steps old of five: the age 79)
		CHECK(intAt(emitter._particles + 4) == 0x0000FF);
		CHECK(near(emitter._particles[3], 0.5f * (1.0f - 79 / 99.0f), 1e-3f));
	}

	// the kinds of particle
	{
		TParticleEmitter streak;

		plain(streak, 100, TParticleEmitter::kStreak);
		CHECK(streak._particleStride == 8);
		streak.IncTime();
		streak.IncTime();
		// the place it was
		CHECK(near(streak._particles[7], 0.0f) && near(streak._particles[2], 2.0f));
		streak.IncTime();
		CHECK(near(streak._particles[7], 2.0f) && near(streak._particles[2], 4.0f));

		TParticleEmitter random;

		plain(random, 100, TParticleEmitter::kRandomTile);
		CHECK(random._particleStride == 7);
		random._tilesX = 4;
		random._tilesY = 2;
		random._maxTile = -1;
		random._perTime = 20.0f;
		random.IncTime();

		bool tilesGood = true;

		for (int i = 0; i < random._count; i++) {
			const float *particle = random._particles + i * 7;

			if (particle[5] < 0.0f || particle[5] > 3.0f || particle[6] < 0.0f || particle[6] > 1.0f)
				tilesGood = false;
		}
		CHECK(tilesGood);

		TParticleEmitter animated;

		plain(animated, 100, TParticleEmitter::kAnimatedTile);
		CHECK(animated._particleStride == 10);
		animated._tilesX = 4;
		animated._tilesY = 2;
		animated._maxTile = 5;

		for (int i = 0; i < 100; i++)
			animated._tileCurve[i] = i / 99.0f;

		animated.IncTime();
		animated.IncTime();
		animated.IncTime();

		const float *first = animated._particles;

		// (the third step: the age 39 of 99; tile 5 * 0.39 = 1.96: the tile 1 and the next, the blend 0.96)
		printf("animated: tiles %g,%g and %g,%g, blend %d\n", first[6], first[7], first[8], first[9], intAt(first + 5) & 0xFF);
		CHECK(near(first[6], 1.0f) && near(first[7], 0.0f));
		CHECK(near(first[8], 2.0f) && near(first[9], 0.0f));
		CHECK((intAt(first + 5) & 0xFF) > 200);
	}

	// sorting by depth: the ranks are the particles with the smallest z first
	{
		TParticleEmitter emitter;

		plain(emitter, 100, TParticleEmitter::kBillboard);
		emitter._materialMode = 0;
		emitter._perTime = 4.0f;
		emitter.IncTime();
		emitter.IncTime();
		emitter.IncTime();

		const uint32_t *ranks = emitter.GetSorterIndices();
		bool sorted = true;

		for (int i = 1; i < emitter._count; i++) {
			if (emitter._particles[ranks[i] + 2] < emitter._particles[ranks[i - 1] + 2])
				sorted = false;
		}
		CHECK(emitter._count == 12);
		CHECK(sorted);
	}

	// the systems: all of the emitters take a step; the scale and the size of the window
	{
		TParticleSystem system;
		TParticleEmitter *first = new TParticleEmitter();
		TParticleEmitter *second = new TParticleEmitter();

		plain(*first, 50, TParticleEmitter::kBillboard);
		plain(*second, 50, TParticleEmitter::kBillboard);
		system._emitters.push_back(first);
		system._emitters.push_back(second);
		system._textures.push_back(nullptr);
		system._textures.push_back(nullptr);
		CHECK(system.IncTime());
		CHECK(first->_count == 3 && second->_count == 3);

		system.SetScaleFactor(2.0f);
		CHECK(first->_scaleFactor == 2.0f && second->_scaleFactor == 2.0f);
		system.SetWindowSize(640, 480);

		TVector3D size = system.GetWindowSize();

		CHECK(near(size.x, 640.0f) && near(size.y, 480.0f));
		CHECK(near(first->_pixelScale, -2.0f * system._cameraZ * system._cameraZoom / 480.0f));
		system.Clear();
		CHECK(system._emitters.empty());
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
