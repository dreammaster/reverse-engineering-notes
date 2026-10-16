#include <cmath>
#include <cstdio>
#include <string>

#include "AppGlobals.h"
#include "TCharHolder.h"
#include "graphicslib/graphics.h"
#include "graphicslib/particleHaduken.h"
#include "vscommon/scripting/visLua.h"
#include "vstables/visionaireGame.h"
#include "fakepictures.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

class NullGraphics : public TGraphicsInterface {
};

static ParticleContainer *containerOf(const char *name) {
	lua_getfield(L, LUA_GLOBALSINDEX, name);

	ParticleContainer **userdata = static_cast<ParticleContainer **>(lua_touserdata(L, -1));
	ParticleContainer *result = userdata ? *userdata : nullptr;

	lua_settop(L, -2);
	return result;
}

static void run(const char *code) {
	LuaDoString(code, "test");
}

// The particle systems that the scripts make with `particleSystem:new{...}`: the settings of the container and of the emitter,
// the kinds of emitter and where they put the particles, and the changes of the fields afterwards.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	useFakePictures();
	writeFakePicture("pics/dot.png", 4, 3);

	graphics = new NullGraphics();

	TVisionaireGame game;

	InitLua(&game, wxString(L"."), wxString(L"."), wxString());

	// a box emitter with the curves
	run("ps = particleSystem:new{maximum = 50, creationRate = 5, transferMode = 'add', imageChoice = 0.25, "
	    "emitter = {emitterType = 'box', center = {100, 200}, sizeX = 40, sizeY = 20, duration = 2, loops = 0, "
	    "life = {0, 60, 0, 60}, velocity = {0, 0, 0, 0}, emissionDirection = {0, 0, 0, 0}, numberOfEmitted = {0, 10}, size = {0, 1, 0, 1}, "
	    "visibility = {0, 1, 0, 1}, sizeOverLife = {0, 1, 1, 2}, visibilityOverLife = {0, 1}, velocityOverLife = {0, 1}, "
	    "colorOverLife = {0, 255, 1, 65280}}}");

	ParticleContainer *box = containerOf("ps");

	CHECK(box != nullptr);
	CHECK(box && box->_maximum == 50 && box->_creationRate == 5);
	CHECK(box && box->_additive);
	CHECK(box && !box->_imageRandom && box->_imageInterval == 0.25);
	CHECK(box && box->_emitter != nullptr);
	CHECK(box && dynamic_cast<BoxEmitter *>(box->_emitter) != nullptr);

	if (box && box->_emitter) {
		Emitter *emitter = box->_emitter;

		CHECK(emitter->_center.x == 100.0f && emitter->_center.y == 200.0f);
		CHECK(emitter->_sizeX == 40.0f && emitter->_sizeY == 20.0f);
		CHECK(emitter->_duration == 2.0f);
		CHECK(emitter->_numberOfEmitted.eval(0.5f) == 10.0f);
		CHECK(emitter->_sizeOverLife.eval(0.5f) == 1.5f);
		CHECK(emitter->_colorOverLife._colors.size() == 2);

		run("ps:update()");
		printf("after one step: %d particles\n", box->_count);
		CHECK(box->_count > 0 && box->_count <= 5);

		bool inside = true;

		for (const Particle &particle : box->_particles) {
			if (particle._age < 0.0f)
				continue;
			if (std::fabs(particle._x - 100.0f) > 20.5f || std::fabs(particle._y - 200.0f) > 10.5f)
				inside = false;
		}
		CHECK(inside);

		for (int i = 0; i < 20; i++)
			run("ps:update()");
		printf("after twenty-one: %d particles\n", box->_count);
		CHECK(box->_count > 5 && box->_count <= 50);

		// the kept settings change by assignment: numbers, strings, flags and curves
		run("ps.maximum = 30; ps.creationRate = 2; ps.transferMode = 'blend'; ps.imageChoice = 'random'; ps.sizeX = 60; "
		    "ps.angle = 1.5; ps.length = 10; ps.radius = 7; ps.innerRadius = 2; ps.sizeY = 8; ps.loops = 3; ps.duration = 4; "
		    "ps.directionToRotation = true; ps.directionToRotationOffset = 0.5; ps.center = {1, 2}; "
		    "ps.visibility = {0, 0.5, 0, 0.5}; ps.velocityOverLife = {0, 2}; ps.angularVelocityOverLife = {0, 1}; "
		    "ps.motionRandomnessOverLife = {0, 1}; ps.spinOverLife = {0, 1}; ps.weightOverLife = {0, 1}; "
		    "ps.weight = {0, 1, 0, 1}; ps.spin = {0, 1, 0, 1}; ps.rotation = {0, 1, 0, 1}; ps.angularVelocity = {0, 1, 0, 1}; "
		    "ps.motionRandomness = {0, 1, 0, 1}; ps.size = {0, 2, 0, 2}; ps.life = {0, 30, 0, 30}; ps.velocity = {0, 1, 0, 1}; "
		    "ps.emissionDirection = {0, 1, 0, 1}; ps.numberOfEmitted = {0, 1}");
		CHECK(box->_maximum == 30 && box->_creationRate == 2);
		CHECK(!box->_additive);
		CHECK(box->_imageRandom);
		CHECK(emitter->_sizeX == 60.0f && emitter->_sizeY == 8.0f);
		CHECK(emitter->_angle == 1.5f && emitter->_length == 10.0f);
		CHECK(emitter->_radius == 7.0f && emitter->_innerRadius == 2.0f);
		CHECK(emitter->_loops == 3 && emitter->_duration == 4.0f);
		CHECK(emitter->_directionToRotation && emitter->_directionToRotationOffset == 0.5f);
		CHECK(emitter->_center.x == 1.0f && emitter->_center.y == 2.0f);
		// (the curves that are given again are added to the old ones, not put in place of them)
		CHECK(emitter->_life._min._values.size() == 2);
		CHECK(emitter->_life._min._values[0] == 60.0f && emitter->_life._min._values[1] == 30.0f);
		CHECK(emitter->_visibility._min._values.size() == 2);

		// a field that is not one
		run("ps.nothing = 5; ps.nothing = 'text'; ps.nothing = true; ps.nothing = {1, 2}");

		// drawn
		run("ps:draw(); ps:updateDraw()");
	}

	// the other kinds
	struct Kind {
		const char *type;
		const char *extra;
	};
	const Kind kinds[] = {
		{"circle", "radius = 30, innerRadius = 10"},
		{"square", "radius = 30, innerRadius = 10"},
		{"line", "length = 40, angle = 0"},
		{"point", ""},
		{"basic", ""},
	};

	for (const Kind &kind : kinds) {
		std::string code = std::string("kind = particleSystem:new{maximum = 200, creationRate = 100, emitter = {emitterType = '") +
		                   kind.type + "', center = {50, 60}, life = {0, 60, 0, 60}, velocity = {0, 0, 0, 0}, emissionDirection = {0, 0, 0, 0}, "
		                   "numberOfEmitted = {0, 100}, " + kind.extra + "}}";

		run(code.c_str());

		ParticleContainer *container = containerOf("kind");

		CHECK(container != nullptr && container->_emitter != nullptr);

		if (!container || !container->_emitter)
			continue;

		run("kind:update()");
		CHECK(container->_count > 20);

		float minDistance = 1e9f, maxDistance = 0.0f;
		float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;

		for (const Particle &particle : container->_particles) {
			if (particle._age < 0.0f)
				continue;

			float dx = particle._x - 50.0f, dy = particle._y - 60.0f;
			float distance = std::sqrt(dx * dx + dy * dy);

			minDistance = std::min(minDistance, distance);
			maxDistance = std::max(maxDistance, distance);
			minX = std::min(minX, particle._x);
			maxX = std::max(maxX, particle._x);
			minY = std::min(minY, particle._y);
			maxY = std::max(maxY, particle._y);
		}

		printf("%s: distance %g to %g, x %g to %g, y %g to %g\n", kind.type, minDistance, maxDistance, minX, maxX, minY, maxY);

		if (std::string(kind.type) == "circle") {
			CHECK(minDistance >= 9.9f && maxDistance <= 30.1f);
		} else if (std::string(kind.type) == "square") {
			// (the ring of the square: both coordinates are at least `inner` from the center)
			CHECK(maxDistance <= 30.0f * 1.5f);
			CHECK(maxX - minX > 20.0f && maxY - minY > 20.0f);
		} else if (std::string(kind.type) == "line") {
			CHECK(maxX - minX > 20.0f && maxX <= 90.1f && minX >= 9.9f);
			CHECK(maxY - minY < 0.01f);
		} else if (std::string(kind.type) == "point") {
			CHECK(maxDistance < 0.01f);
		} else {
			// (a basic emitter gives no place: the particles are where the slot was, at the corner)
			CHECK(maxX - minX < 0.01f);
		}
	}

	// the angle of a line
	run("line = particleSystem:new{maximum = 100, creationRate = 50, emitter = {emitterType = 'line', center = {0, 0}, "
	    "length = 40, angle = 1.5707963, life = {0, 60, 0, 60}, velocity = {0, 0, 0, 0}, emissionDirection = {0, 0, 0, 0}, "
	    "numberOfEmitted = {0, 50}}}");
	{
		ParticleContainer *line = containerOf("line");

		CHECK(line != nullptr);
		if (line) {
			run("line:update()");

			float spreadX = 0.0f, spreadY = 0.0f;

			for (const Particle &particle : line->_particles) {
				if (particle._age < 0.0f)
					continue;
				spreadX = std::max(spreadX, std::fabs(particle._x));
				spreadY = std::max(spreadY, std::fabs(particle._y));
			}
			printf("line at 90 degrees: x %g, y %g\n", spreadX, spreadY);
			CHECK(spreadX < 1.0f && spreadY > 20.0f);
		}
	}

	// the motion: velocity along the direction, weight, and the life that ends
	run("moving = particleSystem:new{maximum = 20, creationRate = 1, emitter = {emitterType = 'point', center = {0, 0}, "
	    "life = {0, 0.1, 0, 0.1}, velocity = {0, 10, 0, 10}, emissionDirection = {0, 0, 0, 0}, numberOfEmitted = {0, 1}, "
	    "velocityOverLife = {0, 1}, sizeOverLife = {0, 1}, visibilityOverLife = {0, 1}, visibility = {0, 1, 0, 1}}}");
	{
		ParticleContainer *moving = containerOf("moving");

		CHECK(moving != nullptr);
		if (moving) {
			int most = 0;

			for (int i = 0; i < 30; i++) {
				run("moving:update()");
				most = std::max(most, moving->_count);
			}
			// (the life is a tenth of a second, a step 0.016: a particle lives 7 steps, one is made each step)
			printf("moving: at most %d, now %d\n", most, moving->_count);
			CHECK(most >= 2 && most <= 10);
			CHECK(moving->_count <= most);

			bool moved = false;

			for (const Particle &particle : moving->_particles) {
				if (particle._age > 0.0f && particle._x > 0.05f)
					moved = true;
			}
			CHECK(moved);
		}
	}

	// the warm-up: steps are done when the system is made
	run("warm = particleSystem:new{warmup = 5, maximum = 100, creationRate = 3, emitter = {emitterType = 'point', "
	    "center = {0, 0}, life = {0, 60, 0, 60}, velocity = {0, 0, 0, 0}, emissionDirection = {0, 0, 0, 0}, numberOfEmitted = {0, 3}}}");
	{
		ParticleContainer *warm = containerOf("warm");

		CHECK(warm != nullptr);
		// (the original reads the warm-up from the wrong place of the stack, where it finds the value of the key being read)
		printf("warm: %d steps, %d particles\n", warm ? warm->_warmup : -1, warm ? warm->_count : -1);
		CHECK(warm && warm->_warmup == 5);
		CHECK(warm && warm->_count > 0);
	}

	// the pictures of the system: one picture is the sprite of it, and the center of it is kept
	run("pic = particleSystem:new{maximum = 10, images = {'vispath:pics/dot.png'}, imageCenter = {{1, 2}}, "
	    "emitter = {emitterType = 'point', center = {0, 0}, life = {0, 60, 0, 60}, velocity = {0, 0, 0, 0}, emissionDirection = {0, 0, 0, 0}, "
	    "numberOfEmitted = {0, 1}}}");
	{
		ParticleContainer *pic = containerOf("pic");

		CHECK(pic != nullptr);
		if (pic) {
			CHECK(pic->_images.size() == 1);
			CHECK(pic->_images.size() == 1 && pic->_images[0] == "pics/dot.png");
			printf("sprites: %d\n", (int)pic->_sprites.size());
			CHECK(pic->_sprites.size() == 1);
			CHECK(pic->_sprites.size() == 1 && pic->_sprites[0]._centerX == 1.0f && pic->_sprites[0]._centerY == 2.0f);
			CHECK(pic->_sprites.size() == 1 && std::fabs(pic->_sprites[0]._aspect - 0.75f) < 0.001f);
			run("pic:update(); pic:draw()");
		}
	}

	// an emitter made from a picture: a particle for each pixel with an alpha, with the colour of it in the mode that is not alpha
	run("img = particleSystem:new{maximum = 100, creationRate = 50, emitter = {emitterType = 'image', image = 'pics/dot.png', "
	    "imageType = 'color', center = {0, 0}, life = {0, 60, 0, 60}, velocity = {0, 0, 0, 0}, emissionDirection = {0, 0, 0, 0}, "
	    "numberOfEmitted = {0, 50}}}");
	{
		ParticleContainer *img = containerOf("img");
		ImageEmitter *emitter = img ? dynamic_cast<ImageEmitter *>(img->_emitter) : nullptr;

		CHECK(emitter != nullptr);
		if (emitter) {
			printf("image emitter: %d points of %dx%d\n", (int)emitter->_points.size(), emitter->_width, emitter->_height);
			CHECK(emitter->_points.size() == 12);
			CHECK(emitter->_colors.size() == 12);
			CHECK(emitter->_imageType == 1);
			run("img:update()");
			CHECK(img->_count > 0);
		}
	}
	run("imga = particleSystem:new{emitter = {emitterType = 'image', image = 'vispath:pics/dot.png', imageType = 'alpha'}}");
	{
		ParticleContainer *imga = containerOf("imga");
		ImageEmitter *emitter = imga ? dynamic_cast<ImageEmitter *>(imga->_emitter) : nullptr;

		CHECK(emitter != nullptr);
		CHECK(emitter && emitter->_imageType == 0 && emitter->_colors.empty() && emitter->_points.size() == 12);
	}

	// an emitter that is not one, and a container made with nothing
	run("none = particleSystem:new{emitter = {emitterType = 'weird'}}");
	{
		ParticleContainer *none = containerOf("none");

		CHECK(none != nullptr && none->_emitter == nullptr);
		run("none.sizeX = 5; none.angle = 1; none.directionToRotation = true; none.visibility = {0, 1}");
	}
	// (a call without the table of settings is no use: the original looks at the first argument, which is the system itself)
	run("empty = particleSystem:new({})");
	CHECK(containerOf("empty") != nullptr);

	// the functions of the metatable (`particleSystem.update` is the function of the same name)
	run("fn = particleSystem.update; has_new = particleSystem.new ~= nil; nothing = particleSystem.nothing; "
	    "numeric = particleSystem[1]");
	lua_getfield(L, LUA_GLOBALSINDEX, "has_new");
	CHECK(lua_toboolean(L, -1));
	lua_settop(L, 0);

	// all of them let go
	run("ps = nil; kind = nil; line = nil; moving = nil; warm = nil; pic = nil; img = nil; imga = nil; none = nil; empty = nil; "
	    "collectgarbage()");

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
