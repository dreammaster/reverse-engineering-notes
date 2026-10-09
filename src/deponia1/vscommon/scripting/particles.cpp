// Confirmed (Deponia_Linux.asm lines 1442308-1445308): how the scripts make particle systems. `particleSystem:new{...}`
// makes a ParticleContainer (graphicslib/particleHaduken.h) from a table of settings and returns a userdata (a pointer to
// the container) with the metatable "Visionaire.TParticles"; its functions are update(), draw(), updateDraw(), and
// assigning to a field of it (`system.warmup = 5`) changes a setting. The keys of the table are the ones that
// Emitter::SerializeEmmiter() writes.
//
// Not reconstructed: more than one picture in `images` (the original packs them into one texture with MaxRectsBinPack
// and the backend, asm 1443770-1444288), so such a container gets no sprites.
#include <string.h>

#include "vscommon/scripting/luaConversion.h"
#include "vscommon/scripting/particles.h"
#include "vscommon/scripting/visLua.h"
#include "vscommon/scripting/visLuaObjects.h"

#include "TCharHolder.h"
#include "WxStub.h"
#include "common/lua/lauxlib.h"
#include "graphicslib/particleHaduken.h"
#include "graphicslib/picture.h"

static const char *const kParticlesMetatable = "Visionaire.TParticles";

/** The userdata that holds the container (an error for scripts when it is something else). */
static ParticleContainer **checkParticles(lua_State *state, int index) {
	return static_cast<ParticleContainer **>(luaL_checkudata(state, index, kParticlesMetatable));
}

static void logNoField(const char *key) {
	if (wxLog::loglevel >= 0)
		wxLog::logexpanded(L"no field %s in particle system", wxString(key).wc_str());
}

// Confirmed (asm lines 1442567-1443408). The setting `key` is changed to the value at `index`: a number or a string for the
// container (or for the emitter), a boolean for the emitter, and a table (a curve) for the emitter. Oddities kept:
// `warmup` always reads the stack at 4 (where particles_new() has the value, but particles_newindex() has the 4th
// argument), and a table is read before the key is looked at.
static void updateValue(lua_State *state, ParticleContainer &container, const char *key, int index) {
	static std::vector<float> floats;

	Emitter *emitter = container._emitter;

	switch (lua_type(state, index)) {
	case LUA_TNUMBER:
		if (!strcmp(key, "imageChoice")) {
			container._imageRandom = false;
			container._imageInterval = lua_tonumber(state, index);
		} else if (!strcmp(key, "creationRate")) {
			container._creationRate = static_cast<int>(lua_tointeger(state, index));
		} else if (!strcmp(key, "maximum")) {
			container._maximum = static_cast<int>(lua_tointeger(state, index));
		} else if (!strcmp(key, "warmup")) {
			container._warmup = static_cast<int>(lua_tointeger(state, 4));
		} else if (!emitter) {
			logNoField(key);
		} else if (!strcmp(key, "length")) {
			emitter->_length = static_cast<float>(lua_tonumber(state, index));
		} else if (!strcmp(key, "angle")) {
			emitter->_angle = static_cast<float>(lua_tonumber(state, index));
		} else if (!strcmp(key, "radius")) {
			emitter->_radius = static_cast<float>(lua_tonumber(state, index));
		} else if (!strcmp(key, "innerRadius")) {
			emitter->_innerRadius = static_cast<float>(lua_tonumber(state, index));
		} else if (!strcmp(key, "sizeX")) {
			emitter->_sizeX = static_cast<float>(lua_tonumber(state, index));
		} else if (!strcmp(key, "sizeY")) {
			emitter->_sizeY = static_cast<float>(lua_tonumber(state, index));
		} else if (!strcmp(key, "loops")) {
			emitter->_loops = static_cast<int>(lua_tonumber(state, index));
		} else if (!strcmp(key, "duration")) {
			emitter->_duration = static_cast<float>(lua_tonumber(state, index));
		} else if (!strcmp(key, "directionToRotationOffset")) {
			emitter->_directionToRotationOffset = static_cast<float>(lua_tonumber(state, index));
		} else {
			logNoField(key);
		}

		break;

	case LUA_TSTRING:
		if (!strcmp(key, "imageChoice"))
			container._imageRandom = true;
		else if (!strcmp(key, "transferMode"))
			container._additive = !strcmp(lua_tolstring(state, index, nullptr), "add");
		else
			logNoField(key);

		break;

	case LUA_TBOOLEAN:
		if (!emitter)
			logNoField(key);
		else if (!strcmp(key, "directionToRotation"))
			emitter->_directionToRotation = (lua_toboolean(state, index) != 0);
		else
			logNoField(key);

		break;

	case LUA_TTABLE:
		if (!emitter)
			break;

		floats.clear();
		ConvertFromLua(floats, index);

		if (!strcmp(key, "emissionDirection"))
			emitter->_emissionDirection.ParseFloatArray(floats);
		else if (!strcmp(key, "visibility"))
			emitter->_visibility.ParseFloatArray(floats);
		else if (!strcmp(key, "velocityOverLife"))
			emitter->_velocityOverLife.ParseFloatArray(floats);
		else if (!strcmp(key, "angularVelocityOverLife"))
			emitter->_angularVelocityOverLife.ParseFloatArray(floats);
		else if (!strcmp(key, "motionRandomnessOverLife"))
			emitter->_motionRandomnessOverLife.ParseFloatArray(floats);
		else if (!strcmp(key, "sizeOverLife"))
			emitter->_sizeOverLife.ParseFloatArray(floats);
		else if (!strcmp(key, "spinOverLife"))
			emitter->_spinOverLife.ParseFloatArray(floats);
		else if (!strcmp(key, "weightOverLife"))
			emitter->_weightOverLife.ParseFloatArray(floats);
		else if (!strcmp(key, "velocity"))
			emitter->_velocity.ParseFloatArray(floats);
		else if (!strcmp(key, "weight"))
			emitter->_weight.ParseFloatArray(floats);
		else if (!strcmp(key, "life"))
			emitter->_life.ParseFloatArray(floats);
		else if (!strcmp(key, "size"))
			emitter->_size.ParseFloatArray(floats);
		else if (!strcmp(key, "rotation"))
			emitter->_rotation.ParseFloatArray(floats);
		else if (!strcmp(key, "angularVelocity"))
			emitter->_angularVelocity.ParseFloatArray(floats);
		else if (!strcmp(key, "motionRandomness"))
			emitter->_motionRandomness.ParseFloatArray(floats);
		else if (!strcmp(key, "spin"))
			emitter->_spin.ParseFloatArray(floats);
		else if (!strcmp(key, "numberOfEmitted"))
			emitter->_numberOfEmitted.ParseFloatArray(floats);
		else if (!strcmp(key, "visibilityOverLife"))
			emitter->_visibilityOverLife.ParseFloatArray(floats);
		else if (!strcmp(key, "colorOverLife"))
			emitter->_colorOverLife.ParseFloatArray(floats);
		else if (!strcmp(key, "center") && floats.size() == 2) {
			emitter->_center.x = floats[0];
			emitter->_center.y = floats[1];
		} else {
			logNoField(key);
		}

		break;

	default:
		break;
	}
}

// Confirmed (asm lines 1443409-1443449): `system.key = value`
static int particles_newindex(lua_State *state) {
	ParticleContainer **container = checkParticles(state, 1);
	const char *key = luaL_checklstring(state, 2, nullptr);

	updateValue(state, **container, key, 3);
	return 0;
}

/** The name of an image of the settings without the "vispath:" the editor puts in front. */
static wxString imagePath(const TCharHolder &image) {
	wxString path = image;

	if (path.StartsWith(wxString(L"vispath:")))
		path = path.Mid(8, -1);

	return path;
}

// Confirmed (asm lines 1443770-1444288 for the single picture, 1444288-1444426 for the centers): the pictures of
// `images` (the table at stack index 4) and `imageCenter` (of the settings, at index 2).
static void loadImages(lua_State *state, ParticleContainer &container) {
	std::vector<TCharHolder> images;

	ConvertFromLua(images, 4);

	for (const TCharHolder &image : images)
		container._images.push_back(std::string(imagePath(image).mb_str()));

	if (images.size() == 1) {
		wxFileName file(imagePath(images[0]).ToStdWstring());

		file.NormalizePath();
		// (the original first sets a byte at +0xC0 of the picture, which picture.h has no name for)
		container._picture->LoadPicture(file, TPictureIO::eLoadSetting::Normal);

		ParticleSprite sprite;

		sprite._u = 0.0f;
		sprite._w = 1.0f;
		sprite._h = 1.0f;
		sprite._aspect = static_cast<float>(container._picture->GetHeight()) /
		                 static_cast<float>(container._picture->GetWidth());
		container._sprites.push_back(sprite);
	}

	// TODO: with 0 or 2 or more pictures the original makes a texture of them all (MaxRectsBinPack, the backend) and a
	// sprite for each; not reconstructed.

	lua_pushstring(state, "imageCenter");
	lua_gettable(state, 2);

	if (lua_type(state, -1) == LUA_TTABLE) {
		lua_pushnil(state);

		size_t number = 0;

		while (lua_next(state, -2)) {
			if (lua_type(state, -1) == LUA_TTABLE) {
				std::vector<float> center;

				ConvertFromLua(center, 7);

				if (number < container._sprites.size() && center.size() == 2) {
					container._sprites[number]._centerX = center[0];
					container._sprites[number]._centerY = center[1];
				}

				number++;
			}

			lua_settop(state, -2);
		}
	}

	lua_settop(state, -2);
}

/** The emitter of the kind `type` (null: there is no such kind). */
static Emitter *makeEmitter(lua_State *state, const char *type) {
	if (!strcmp(type, "line"))
		return new LineEmitter();

	if (!strcmp(type, "basic"))
		return new Emitter();

	if (!strcmp(type, "circle"))
		return new CircleEmitter();

	if (!strcmp(type, "square"))
		return new SquareEmitter();

	if (!strcmp(type, "box"))
		return new BoxEmitter();

	if (!strcmp(type, "point"))
		return new PointEmitter();

	if (!strcmp(type, "image")) {
		ImageEmitter *emitter = new ImageEmitter();

		lua_pushstring(state, "imageType");
		lua_gettable(state, 4);

		const char *imageType = lua_tolstring(state, 5, nullptr);
		int mode = (imageType && !strcmp(imageType, "alpha")) ? 0 : 1;

		lua_settop(state, -2);
		lua_pushstring(state, "image");
		lua_gettable(state, 4);

		const char *image = lua_tolstring(state, 5, nullptr);

		lua_settop(state, -2);

		if (image) {
			wxString path(image);

			if (path.StartsWith(wxString(L"vispath:")))
				path = path.Mid(8, -1);

			emitter->CreateWithImage(path.mb_str(), mode);
		}

		return emitter;
	}

	return nullptr;
}

// Confirmed (asm lines 1443643-1443770 and 1444429-1445260): the table `emitter` (at stack index 4) is an emitter of the
// kind `emitterType` and the settings in it.
static void makeEmitterFromTable(lua_State *state, ParticleContainer &container, const char *key) {
	lua_pushstring(state, "emitterType");
	lua_gettable(state, 4);

	const char *type = lua_tolstring(state, 5, nullptr);

	lua_settop(state, -2);

	Emitter *emitter = makeEmitter(state, type ? type : "");

	if (emitter) {
		container._emitter = emitter;
		emitter->_phase = 0.0f;
		emitter->_emitRemainder = 0.0f;
	} else if (wxLog::loglevel >= 0) {
		// the original names the key of the table here ("emitter"), not the kind that it did not know
		wxLog::logexpanded(L"no emittertype %s", wxString(key).wc_str());
	}

	lua_pushnil(state);

	while (lua_next(state, 4)) {
		if (lua_isstring(state, 5)) {
			const char *name = lua_tolstring(state, 5, nullptr);

			if (strcmp(name, "emitterType") && strcmp(name, "image") && strcmp(name, "imageType"))
				updateValue(state, container, name, 6);
		}

		lua_settop(state, -2);
	}
}

// Confirmed (asm lines 1443450-1443595): particleSystem:new{settings}
static int particles_new(lua_State *state) {
	ParticleContainer *container = new ParticleContainer();

	container->_count = 0;

	if (lua_type(state, 1) == LUA_TTABLE) {
		lua_pushnil(state);

		while (lua_next(state, 2)) {
			if (lua_isstring(state, 3)) {
				const char *key = lua_tolstring(state, 3, nullptr);
				int type = lua_type(state, 4);

				if (type == LUA_TNUMBER || type == LUA_TSTRING) {
					updateValue(state, *container, key, 4);
				} else if (type == LUA_TTABLE) {
					if (!strcmp(key, "images"))
						loadImages(state, *container);
					else if (!strcmp(key, "emitter"))
						makeEmitterFromTable(state, *container, key);
				}
			}

			lua_settop(state, -2);
		}
	}

	for (int i = 0; i < container->_warmup; i++)
		container->Update(false, vec2(), 0.0f, true);

	ParticleContainer **userdata = static_cast<ParticleContainer **>(lua_newuserdata(state, sizeof(ParticleContainer *)));

	*userdata = container;
	lua_getfield(state, LUA_REGISTRYINDEX, kParticlesMetatable);
	lua_setmetatable(state, -2);
	return 1;
}

// Confirmed (asm lines 1442381-1442414): a step with the container as the script's own (no phase, children moved).
// The original does not look whether the container is still there; here a container that __gc has let go is not touched.
static int particles_update(lua_State *state) {
	ParticleContainer **container = checkParticles(state, 1);

	if (*container)
		(*container)->Update(false, vec2(), 0.0f, true);

	return 0;
}

// Confirmed (asm lines 1442415-1442454)
static int particles_gc(lua_State *state) {
	ParticleContainer **container = checkParticles(state, 1);

	delete *container;
	*container = nullptr;
	return 1;
}

// Confirmed (asm lines 1442455-1442477)
static int particles_draw(lua_State *state) {
	ParticleContainer **container = checkParticles(state, 1);

	if (*container)
		(*container)->Draw();

	return 0;
}

// Confirmed (asm lines 1442478-1442516)
static int particles_updatedraw(lua_State *state) {
	ParticleContainer **container = checkParticles(state, 1);

	if (*container) {
		(*container)->Update(false, vec2(), 0.0f, true);
		(*container)->Draw();
	}

	return 0;
}

static int particles_index(lua_State *state);

// Confirmed (the table particles_meths in .data)
static const luaL_Reg particles_meths[] = {
	{"new", particles_new},
	{"draw", particles_draw},
	{"update", particles_update},
	{"updateDraw", particles_updatedraw},
	{"__index", particles_index},
	{"__newindex", particles_newindex},
	{"__gc", particles_gc},
	{nullptr, nullptr}
};

// Confirmed (asm lines 1442308-1442365): the field `name` is the function of that name from particles_meths, nothing
// when there is none (so `particleSystem.new` and `container.update` come from here, not from a table).
static int particles_index(lua_State *state) {
	int type = lua_type(state, 2);
	const char *name = lua_tolstring(state, 2, nullptr);

	if (type != LUA_TSTRING)
		return 0;

	for (const luaL_Reg *entry = particles_meths; entry->name; entry++) {
		if (!strcmp(name, entry->name)) {
			lua_pushcclosure(state, entry->func, 0);
			return 1;
		}
	}

	return 0;
}

// Confirmed (asm lines 1442517-1442566): the metatable has the functions (its `__index` is particles_index()), and the
// global `particleSystem` is an empty table with that metatable. The metatable stays on the stack, as in the original.
int luaopen_Particles(lua_State *state) {
	luaL_newmetatable(state, kParticlesMetatable);
	lua_pushlstring(state, "__index", 7);
	lua_pushvalue(state, -2);
	lua_rawset(state, -3);

	luaL_openlib(state, nullptr, particles_meths, 0);

	lua_createtable(state, 0, 0);
	lua_getfield(state, LUA_REGISTRYINDEX, kParticlesMetatable);
	lua_setmetatable(state, -2);
	lua_setfield(state, LUA_GLOBALSINDEX, "particleSystem");
	return 0;
}

// Confirmed (asm lines 172999-173008, 259264-259275, in TGScene::BeginScene() and TGObject::SetActive()): the callers take
// the container out of the userdata (`lua_touserdata(L, -1)`, the pointer then 0) and leave the userdata on the stack; here
// it is taken off.
ParticleContainer *takeParticleContainer() {
	ParticleContainer **userdata = static_cast<ParticleContainer **>(lua_touserdata(L, -1));

	if (!userdata)
		return nullptr;

	ParticleContainer *container = *userdata;

	*userdata = nullptr;
	lua_settop(L, -2);
	return container;
}
