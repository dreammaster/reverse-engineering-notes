// Confirmed (Deponia_Linux.asm lines 438459-448827, 3144349-3144560): the global `graphics` of the scripts, and the sprites,
// framebuffers, buffers and movies that it makes (the metatables "Visionaire.TGraphics", "Visionaire.Sprite",
// "Visionaire.TFramebuffer", "Visionaire.TBuffer" and "Visionaire.TMovie"). With it the scripts set the matrices that the
// drawing goes through (matrix1, matrix2, textMatrix, invMatrix1), add functions that are called when the scene is drawn
// (addDrawFunc), ask about fonts, animations and characters, and draw by themselves.
//
// Done here are the functions that are about the engine (the fonts, the animations, the scroll position, the shaders'
// uniforms, the sprite objects, drawing sprites, boxes, lines and animations, loading pictures). The ones that need the video
// card itself - the buffers and framebuffers, movies, `clear`, the Box2D debug drawing - are in the tables with the name they
// have, and log that they are not reconstructed when a script calls them.
#include <string.h>

#include <algorithm>
#include <functional>

#include "AppGlobals.h"
#include "Diagnostics.h"
#include "SdlStub.h"
#include "TFramebuffer.h"
#include "TGCharacter.h"
#include "TGScene.h"
#include "TMovie.h"
#include "TPaintControl.h"
#include "TPictureFormat.h"
#include "Easing.h"
#include "Tween.h"
#include "common/lua/lauxlib.h"
#include "graphicslib/graphics.h"
#include "graphicslib/noise1234.h"
#include "graphicslib/picture.h"
#include "graphicslib/shader.h"
#include "graphicslib/subsys.h"
#include "vscommon/cfont.h"
#include "vscommon/objAccess.h"
#include "vscommon/scripting/luaConversion.h"
#include "vscommon/scripting/visLua.h"
#include "vscommon/scripting/visLuaObjects.h"
#include "vsplayer/animationGame.h"
#include "vsplayer/control/gameControl.h"
#include "vsplayer/scripting/luaSystem.h"
#include "vsplayer/scripting/playerCommands.h"

// TGameControl implements every accessor used below, but g_pGameControl is only declared as TMasterControl*.
static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

static const char *const kGraphicsMetatable = "Visionaire.TGraphics";
static const char *const kFramebufferMetatable = "Visionaire.TFramebuffer";

/** The function that is not reconstructed is called by a script. */
static void notReconstructed(const char *name) {
	if (wxLog::loglevel > 0)
		wxLog::logexpanded(L"graphics.%s needs the video card backend, which is not reconstructed", wxString(name).wc_str());
}

#define BACKEND_FUNCTION(name) \
	static int graphics_##name(lua_State *) { \
		notReconstructed(#name); \
		return 0; \
	}

BACKEND_FUNCTION(drawIndexed)
BACKEND_FUNCTION(setupOffsets)
BACKEND_FUNCTION(createIndexBuffer)
BACKEND_FUNCTION(createBuffer)
BACKEND_FUNCTION(createFramebuffer)
BACKEND_FUNCTION(bindFramebuffer)
BACKEND_FUNCTION(clear)
BACKEND_FUNCTION(createBox2DDebugRender)

// The methods of the sprite, framebuffer, buffer and movie objects, which are the backend's.
BACKEND_FUNCTION(framebuffer_bind)
BACKEND_FUNCTION(buffer_bind)
BACKEND_FUNCTION(buffer_update)

static const char *const kSpriteMetatable = "Visionaire.Sprite";

/** The functions of the class Sprite (the table is below, the functions look themselves up in it). */
static const luaL_Reg *spriteFunctions();

/** The picture of the Sprite userdata at `index` (an error for anything else). */
static TPictureIO *checkSprite(lua_State *state, int index) {
	return *static_cast<TPictureIO **>(luaL_checkudata(state, index, kSpriteMetatable));
}

/** Pushes a new Sprite userdata that holds `picture`. */
static void pushSprite(lua_State *state, TPictureIO *picture) {
	TPictureIO **userdata = static_cast<TPictureIO **>(lua_newuserdata(state, sizeof(TPictureIO *)));

	*userdata = picture;
	lua_getfield(state, LUA_REGISTRYINDEX, kSpriteMetatable);
	lua_setmetatable(state, -2);
}

/** Draws with a paint control of its own (the position of the scene does not count), as every script draw function does. */
class ScriptPaintControl {
public:
	ScriptPaintControl(TPaintControl &control) : _previous(TPaintControl::GetCurrent()) {
		control.SetCurrent();
	}
	~ScriptPaintControl() {
		if (_previous)
			_previous->SetCurrent();
	}

private:
	TPaintControl *_previous;
};

// Confirmed (asm lines 440172-440228): Sprite.new() makes an empty picture (the table the function was called on, the first
// argument, is taken off).
static int graphics_sprite_new(lua_State *state) {
	lua_remove(state, 1);
	pushSprite(state, new TPictureIO());
	return 1;
}

// Confirmed (asm lines 442482-442527)
static int graphics_sprite_clear(lua_State *state) {
	checkSprite(state, 1)->Clear();
	return 0;
}

// Confirmed (asm lines 440499-440734): the functions of the class by their name, else the properties of the picture.
static int graphics_sprite_index(lua_State *state) {
	const char *name = (lua_type(state, 2) == LUA_TSTRING) ? lua_tolstring(state, 2, nullptr) : nullptr;

	if (name) {
		for (const luaL_Reg *entry = spriteFunctions(); entry->name; entry++) {
			if (strcmp(entry->name, name) == 0) {
				lua_pushcclosure(state, entry->func, 0);
				return 1;
			}
		}
	}

	TPictureIO *sprite = checkSprite(state, 1);

	if (!name) {
		lua_pushnil(state);
		return 1;
	}

	if (strcmp(name, "position") == 0) {
		ConvertToLua(sprite->GetPosition());
	} else if (strcmp(name, "rotation") == 0) {
		lua_pushnumber(state, sprite->GetRotation());
	} else if (strcmp(name, "scale") == 0 || strcmp(name, "scaleX") == 0) {
		lua_pushnumber(state, sprite->GetScaleX());
	} else if (strcmp(name, "scaleY") == 0) {
		lua_pushnumber(state, sprite->GetScaleY());
	} else if (strcmp(name, "shaderSet") == 0) {
		lua_pushinteger(state, sprite->GetShader());
	} else if (strcmp(name, "rotationCenter") == 0) {
		ConvertToLua(sprite->GetRotationCenter());
	} else if (strcmp(name, "size") == 0) {
		lua_pushnumber(state, sprite->GetSize());
	} else if (strcmp(name, "width") == 0) {
		lua_pushnumber(state, sprite->GetWidth());
	} else if (strcmp(name, "height") == 0) {
		lua_pushnumber(state, sprite->GetHeight());
	} else {
		lua_pushnil(state);
	}

	return 1;
}

// Confirmed (asm lines 442254-442482): sprite.path = "..." (a string), sprite.position / rotationCenter = {x, y} (a table),
// sprite.shaderSet / rotation / scale / scaleX / scaleY / matrixId = n (a number). `scale` sets the scale of both axes.
// Anything else is let go without a word.
static int graphics_sprite_newindex(lua_State *state) {
	TPictureIO *sprite = checkSprite(state, 1);
	const char *name = lua_tolstring(state, 2, nullptr);
	int valueType = lua_type(state, 3);

	if (!name)
		return 0;

	if (valueType == LUA_TSTRING) {
		if (strcmp(name, "path") == 0)
			sprite->SetPath(TCharHolder(lua_tolstring(state, 3, nullptr)));
	} else if (valueType == LUA_TTABLE) {
		wxPoint point;

		if (strcmp(name, "position") == 0) {
			ConvertFromLua(point, 3);
			sprite->SetPosition(point, -1.0f);
		} else if (strcmp(name, "rotationCenter") == 0) {
			ConvertFromLua(point, 3);
			sprite->SetRotationCenter(point);
		}
	} else if (valueType == LUA_TNUMBER) {
		if (strcmp(name, "shaderSet") == 0) {
			sprite->SetShader(static_cast<int>(lua_tointeger(state, 3)));
		} else if (strcmp(name, "rotation") == 0) {
			sprite->SetRotation(static_cast<float>(lua_tonumber(state, 3)));
		} else if (strcmp(name, "scale") == 0) {
			sprite->SetScale(static_cast<float>(lua_tonumber(state, 3)), static_cast<float>(lua_tonumber(state, 3)));
		} else if (strcmp(name, "scaleX") == 0) {
			sprite->SetScaleX(static_cast<float>(lua_tonumber(state, 3)));
		} else if (strcmp(name, "scaleY") == 0) {
			sprite->SetScaleY(static_cast<float>(lua_tonumber(state, 3)));
		} else if (strcmp(name, "matrixId") == 0) {
			sprite->SetMatrixId(static_cast<int>(lua_tonumber(state, 3)));
		}
	}

	return 0;
}

// Confirmed (asm lines 438474-438488): a Sprite called like a function does nothing.
static int graphics_sprite_call(lua_State *) {
	return 0;
}

// Confirmed (asm lines 442740-442790)
static int graphics_sprite_gc(lua_State *state) {
	TPictureIO **sprite = static_cast<TPictureIO **>(luaL_checkudata(state, 1, kSpriteMetatable));

	delete *sprite;
	*sprite = nullptr;
	return 0;
}

// Confirmed (asm lines 442527-442644): graphics.drawSprite(sprite [, alpha = 1 [, colour = 0xFFFFFF]]) draws the picture
// where it is (its position, rotation, scale ...) with the paint control of its own.
static int graphics_drawSprite(lua_State *state) {
	static TPaintControl control;
	ScriptPaintControl paint(control);
	TPictureIO *sprite = checkSprite(state, 1);
	double alpha = luaL_optnumber(state, 2, 1.0);
	double color = luaL_optnumber(state, 3, 16777215.0);

	if (sprite->RefreshSprite(false))
		sprite->Draw(static_cast<float>(alpha), static_cast<unsigned int>(static_cast<int>(color)));

	return 0;
}

// Confirmed (asm lines 441595-442254): graphics.drawSpriteWithNineRect(sprite, destRect, nineRect [, colour = 0xFFFFFF
// [, alpha = 1]]): the picture is stretched over `destRect` in nine parts, the four corners keep their size. `nineRect`
// holds the widths of the borders of the picture: x the left, y the top, width the right, height the bottom one. The
// parts are drawn in the order top left, top, top right, left, middle, right, bottom left, bottom, bottom right.
static int graphics_drawSpriteWithNineRect(lua_State *state) {
	static TPaintControl control;
	ScriptPaintControl paint(control);
	TPictureIO *sprite = checkSprite(state, 1);
	wxRect dest;
	wxRect nine;

	if (!ConvertFromLua(dest, 2))
		{
		lua_pushstring(state, "destRect invalid");
		return lua_error(state);
	}

	if (!ConvertFromLua(nine, 3))
		{
		lua_pushstring(state, "ninerect invalid");
		return lua_error(state);
	}

	unsigned int color = static_cast<unsigned int>(luaL_optinteger(state, 4, 0xFFFFFF));
	float alpha = static_cast<float>(luaL_optnumber(state, 5, 1.0));

	if (!sprite->RefreshSprite(false))
		return 0;

	const int width = sprite->GetWidth();
	const int height = sprite->GetHeight();
	const int left = nine.x;
	const int top = nine.y;
	const int right = nine.width;
	const int bottom = nine.height;

	// the middle part of the picture, and of the destination (never negative)
	const int middleWidth = width - right - left;
	const int middleHeight = height - bottom - top;
	const int destMiddleWidth = std::max(0, dest.width - (left + right));
	const int destMiddleHeight = std::max(0, dest.height - (top + bottom));

	// the edges of the destination: after the left border, after the middle, after the top border, after the middle
	const int destX1 = dest.x + left;
	const int destX2 = destX1 + destMiddleWidth;
	const int destX3 = dest.x + dest.width;
	const int destY1 = dest.y + top;
	const int destY2 = destY1 + destMiddleHeight;
	const int destY3 = dest.y + dest.height;

	const int sourceX[3] = {0, left, width - right};
	const int sourceW[3] = {left, middleWidth, right};
	const int sourceY[3] = {0, top, height - bottom};
	const int sourceH[3] = {top, middleHeight, bottom};
	const int destX[3] = {dest.x, destX1, destX2};
	const int destW[3] = {left, destMiddleWidth, destX3 - destX2};
	const int destY[3] = {dest.y, destY1, destY2};
	const int destH[3] = {top, destMiddleHeight, destY3 - destY2};

	for (int row = 0; row < 3; row++) {
		for (int column = 0; column < 3; column++) {
			wxRect source;
			FloatRect target;
			wxPoint center;

			center.x = -1;
			center.y = -1;
			source.x = sourceX[column];
			source.y = sourceY[row];
			source.width = sourceW[column];
			source.height = sourceH[row];
			target.x = static_cast<float>(destX[column]);
			target.y = static_cast<float>(destY[row]);
			target.width = static_cast<float>(destW[column]);
			target.height = static_cast<float>(destH[row]);
			graphics->Draw(sprite->GetSpriteHandle(), source, target, alpha, false, color, -1, 0.0f, center, 1.0f, 1.0f, 1);
		}
	}

	return 0;
}

// Confirmed (asm lines 439476-439554): graphics.drawBox(x, y, width, height, colour [, alpha = 1]) fills a rectangle.
static int graphics_drawBox(lua_State *state) {
	wxRect rect;

	rect.x = static_cast<int>(luaL_checkinteger(state, 1));
	rect.y = static_cast<int>(luaL_checkinteger(state, 2));
	rect.width = static_cast<int>(luaL_checkinteger(state, 3));
	rect.height = static_cast<int>(luaL_checkinteger(state, 4));

	unsigned int color = static_cast<unsigned int>(luaL_checkinteger(state, 5));

	graphics->DrawBox(rect, color, static_cast<float>(luaL_optnumber(state, 6, 1.0)));
	return 0;
}

// Confirmed (asm lines 439398-439476): graphics.drawLine(x1, y1, x2, y2, colour [, alpha = 1])
static int graphics_drawLine(lua_State *state) {
	wxPoint from;
	wxPoint to;

	from.x = static_cast<int>(luaL_checkinteger(state, 1));
	from.y = static_cast<int>(luaL_checkinteger(state, 2));
	to.x = static_cast<int>(luaL_checkinteger(state, 3));
	to.y = static_cast<int>(luaL_checkinteger(state, 4));

	unsigned int color = static_cast<unsigned int>(luaL_checkinteger(state, 5));

	graphics->DrawLine(from, to, color, static_cast<float>(luaL_optnumber(state, 6, 1.0)));
	return 0;
}

// Confirmed (asm lines 447048-447358): graphics.loadFromFile("path") loads a picture; "vispath:" in front of the path is
// left off. The answer is the Sprite, nil when the picture cannot be loaded.
static int graphics_loadFromFile(lua_State *state) {
	TPictureIO *picture = new TPictureIO();

	pushSprite(state, picture);

	wxString path;

	toUTF(&path, luaL_checklstring(state, 1, nullptr));

	if (path.StartsWith(wxString(L"vispath:")))
		path = path.Mid(8, -1);

	wxFileName file(path.ToStdWstring());

	file.NormalizePath();

	if (!picture->LoadPicture(file, TPictureIO::eLoadSetting::Normal)) {
		lua_settop(state, -2);
		lua_pushnil(state);
	}

	return 1;
}

/** A Sprite made of the picture in the string at the first place, which `format` reads (the Sprite is the answer even when
 *  the picture could not be read). */
static int loadMemoryPicture(lua_State *state, TPictureFormat &format) {
	TPictureIO *picture = new TPictureIO();

	pushSprite(state, picture);

	size_t length = 0;
	const char *data = luaL_checklstring(state, 1, &length);
	int width = 0;
	int height = 0;

	if (format.ReadHeader(const_cast<char *>(data), length, &width, &height) && format.ReadData(*picture))
		picture->CreateSprite(false);

	return 1;
}

// Confirmed (asm lines 443741-443885, 443560-443641, 442984-443087): graphics.loadMemoryPNG / loadMemoryJPG / loadMemoryWEBP
// ("file contents") make a Sprite from the picture in the string. (The original decodes a JPG straight with jpgd and hands
// the pixels to CreateSprite(pixels, width, height, 4); the result is the same.)
static int graphics_loadMemoryPNG(lua_State *state) {
	TPicturePNG format;

	return loadMemoryPicture(state, format);
}

static int graphics_loadMemoryJPG(lua_State *state) {
	TPictureJPG format;

	return loadMemoryPicture(state, format);
}

static int graphics_loadMemoryWEBP(lua_State *state) {
	TPictureWebP format;

	return loadMemoryPicture(state, format);
}

// Confirmed (asm lines 445380-445502): graphics.drawAnimation(animation [, alpha = 1 [, colour = 0xFFFFFF]]) draws the
// frame an animation is on.
static int graphics_drawAnimation(lua_State *state) {
	LuaVisionaireObject *object = CheckVisionaireObject(state, 1, true);
	float alpha = static_cast<float>(luaL_optnumber(state, 1, 1.0));
	unsigned int color = static_cast<unsigned int>(luaL_optinteger(state, 2, 0xFFFFFF));
	TVisObjRef ref(object->object);
	TGAnimation *animation = TGAnimation::GetAnimationByObject(ref);

	if (animation) {
		animation->Draw(alpha, color, -1);
	} else if (wxLog::loglevel >= 0) {
		wxString id = IdStr(*object->object);

		wxLog::logexpanded(L"graphics_drawAnimation: Animation %s not found", id.wc_str());
	}

	return 0;
}

// Confirmed (asm lines 445502-445659): graphics.instantiateAnimation(animation [, reverse = 0 [, scale = 100]]) starts a new
// animation of the data object (that nothing owns), and answers the object that holds its state; nothing when it cannot be
// started.
static int graphics_instantiateAnimation(lua_State *state) {
	LuaVisionaireObject *object = CheckVisionaireObject(state, 1, true);
	lua_Integer reverse = luaL_optinteger(state, 1, 0);
	lua_Integer scale = luaL_optinteger(state, 2, 100);
	TVisObjRef ref(object->object);
	TGAnimation *animation = TGAnimation::StartLuaAnimation(ref, reverse == 1, static_cast<float>(scale));

	if (animation) {
		CreateVisionaireObject(state, animation->GetState());
		return 1;
	}

	if (wxLog::loglevel >= 0) {
		wxString id = IdStr(ref);

		wxLog::logexpanded(L"Can't start animation %s", id.wc_str());
	}

	return 0;
}

static const char *const kMovieMetatable = "Visionaire.TMovie";

/** The functions of the class TMovie (the table is below, movie_index() looks itself up in it). */
static const luaL_Reg *movieFunctions();

/** The pointer to the movie of the TMovie userdata at `index` (an error for anything else); the movie is null once it is let go. */
static TMovie **checkMovie(lua_State *state, int index) {
	return static_cast<TMovie **>(luaL_checkudata(state, index, kMovieMetatable));
}

// Confirmed (asm lines 446320-447048): graphics.movieOpen("path") starts a movie that the script draws itself (movie:draw()),
// with the settings of the game for the subtitles. The answer is the movie, also when it cannot be played.
static int graphics_movieOpen(lua_State *state) {
	TMovie **userdata = static_cast<TMovie **>(lua_newuserdata(state, sizeof(TMovie *)));
	TMovie *movie = new TMovie();

	*userdata = movie;
	lua_getfield(state, LUA_REGISTRYINDEX, kMovieMetatable);
	lua_setmetatable(state, -2);

	const char *name = luaL_checklstring(state, 1, nullptr);
	TVisObjRef game = gameControl()->GetVisionaire()->GetGame();
	TMovieSettings settings;

	settings.subtitlePosition = *game.GetPoint(kGameVideoSubtitlePosition);
	settings.subtitleLanguage = game.GetStr(kGameVideoSubtitleLanguage);
	settings.audioLanguage = game.GetStr(kGameVideoAudioLanguage);

	wxString path;

	toUTF(&path, name);
	movie->Initialize(false);

	if (path.StartsWith(wxString(L"vispath:")))
		path = path.Mid(8, -1);

	TVisObjRef subtitleFont = game.GetLink(kGameVideoSubtitleFont);

	if (subtitleFont.IsEmpty())
		subtitleFont = game.GetLink(kGameActionTextFont);

	movie->SetInScene();
	settings.fontId = PackVisId(subtitleFont.GetId());
	settings.fontManager = gameControl()->GetFontManager();

	wxFileName file(path.ToStdWstring());

	file.NormalizePath();
	movie->PlayCutScene(file, settings, false, false);

	// where the script opened it (for openedVideos)
	lua_getfield(state, LUA_GLOBALSINDEX, "debug");
	lua_pushstring(state, "traceback");
	lua_gettable(state, -2);
	lua_remove(state, -2);
	lua_pcall(state, 0, 1, 0);

	wxString callstack;

	toUTF(&callstack, lua_tolstring(state, -1, nullptr));
	movie->_callstack = callstack;
	lua_settop(state, -2);
	return 1;
}

// Confirmed (asm lines 440847-441099): a table of the movies that are open ({video = name in the game, file = what is played,
// callstack = where the script opened it}).
static int graphics_openedVideos(lua_State *state) {
	lua_createtable(state, 0, 0);

	lua_Integer number = 1;

	for (TMovie *movie : TMovie::s_openMovies) {
		lua_pushinteger(state, number++);
		lua_createtable(state, 0, 0);
		lua_pushstring(state, "video");
		lua_pushstring(state, movie->_file.GetFullPath().mb_str());
		lua_settable(state, -3);
		lua_pushstring(state, "file");
		lua_pushstring(state, movie->_path.mb_str());
		lua_settable(state, -3);
		lua_pushstring(state, "callstack");
		lua_pushstring(state, movie->_callstack.mb_str());
		lua_settable(state, -3);
		lua_settable(state, -3);
	}

	return 1;
}

// Confirmed (asm lines 441099-441176): movie:draw([x = 0 [, y = 0 [, width = -1 [, height = -1]]]]) shows the picture of the
// movie at once; true when there was one.
static int graphics_movie_draw(lua_State *state) {
	TMovie **movie = checkMovie(state, 1);
	double x = luaL_optnumber(state, 2, 0.0);
	double y = luaL_optnumber(state, 3, 0.0);
	double width = luaL_optnumber(state, 4, -1.0);
	double height = luaL_optnumber(state, 5, -1.0);

	lua_pushboolean(state, *movie && (*movie)->NowaitDraw(static_cast<float>(x), static_cast<float>(y),
	                                                      static_cast<float>(width), static_cast<float>(height)));
	return 1;
}

// Confirmed (asm lines 445082-445231 and 445231-445380): the movie ends and goes; the same when the script lets go of it.
static int graphics_movie_finish(lua_State *state) {
	TMovie **movie = checkMovie(state, 1);

	if (*movie) {
		(*movie)->Finish();
		delete *movie;
		*movie = nullptr;
	}

	return 0;
}

// Confirmed (asm lines 441176-441226)
static int graphics_movie_seek(lua_State *state) {
	TMovie **movie = checkMovie(state, 1);
	double seconds = luaL_checknumber(state, 2);

	if (*movie)
		(*movie)->Seek(static_cast<float>(seconds));
	return 0;
}

// Confirmed (asm lines 441406-441456 and 441456-441505)
static int graphics_movie_getDuration(lua_State *state) {
	TMovie **movie = checkMovie(state, 1);

	lua_pushnumber(state, *movie ? (*movie)->GetDuration() : 0.0);
	return 1;
}

static int graphics_movie_getTime(lua_State *state) {
	TMovie **movie = checkMovie(state, 1);

	lua_pushnumber(state, *movie ? (*movie)->GetTime() : 0.0);
	return 1;
}

// Confirmed (asm lines 441505-441595): the original answers 1 value, whatever is on top of the stack (the movie itself).
static int graphics_movie_pause(lua_State *state) {
	TMovie **movie = checkMovie(state, 1);

	if (*movie)
		(*movie)->Pause();

	return 1;
}

static int graphics_movie_resume(lua_State *state) {
	TMovie **movie = checkMovie(state, 1);

	if (*movie)
		(*movie)->Resume();

	return 1;
}

// Confirmed (asm lines 440360-440499): the functions of the class by their name, and the size of the movie.
static int graphics_movie_index(lua_State *state) {
	int keyType = lua_type(state, 2);
	const char *name = lua_tolstring(state, 2, nullptr);

	if (keyType == LUA_TSTRING) {
		for (const luaL_Reg *entry = movieFunctions(); entry->name; entry++) {
			if (strcmp(entry->name, name) == 0) {
				lua_pushcclosure(state, entry->func, 0);
				return 1;
			}
		}
	}

	TMovie **movie = checkMovie(state, 1);

	if (!*movie || keyType != LUA_TSTRING)
		return 0;

	if (strcmp(name, "width") == 0) {
		lua_pushnumber(state, (*movie)->GetWidth());
		return 1;
	}

	if (strcmp(name, "height") == 0) {
		lua_pushnumber(state, (*movie)->GetHeight());
		return 1;
	}

	return 0;
}

// Confirmed (asm lines 441226-441406): movie.color = {red, green, blue, alpha}, movie.loop = true, movie.blend = n.
static int graphics_movie_newindex(lua_State *state) {
	TMovie **movie = checkMovie(state, 1);
	const char *name = lua_tolstring(state, 2, nullptr);
	int valueType = lua_type(state, 3);

	if (!*movie || !name)
		return 0;

	if (valueType == LUA_TNUMBER) {
		if (strcmp(name, "blend") == 0)
			(*movie)->SetBlend(static_cast<int>(lua_tonumber(state, 3)));
	} else if (valueType == LUA_TTABLE) {
		if (strcmp(name, "color") == 0) {
			long channel[4];

			for (int i = 0; i < 4; i++) {
				lua_pushinteger(state, i + 1);
				lua_gettable(state, -2);
				channel[i] = static_cast<long>(lua_tointeger(state, -1));
				lua_settop(state, -2);
			}

			(*movie)->SetColour(wxColour(static_cast<unsigned char>(channel[0]), static_cast<unsigned char>(channel[1]),
			                             static_cast<unsigned char>(channel[2]), static_cast<unsigned char>(channel[3])));
		}
	} else if (valueType == LUA_TBOOLEAN) {
		if (strcmp(name, "loop") == 0)
			(*movie)->SetLoop(lua_toboolean(state, 3) != 0);
	}

	return 0;
}

/** The shader of the script's number (from 1), null when there is none. */
static TShader *shaderByNumber(lua_Integer number) {
	if (number < 1 || static_cast<size_t>(number) > shader_list.size())
		return nullptr;

	auto it = shader_list.begin();

	std::advance(it, number - 1);
	return *it;
}

// Confirmed (asm lines 438488-438684): graphics.font = <font object>, matrix1 / matrix2 / textMatrix / invMatrix1 = {9 numbers},
// fontShaderIndizes = {...}, clipboard = "text", fontShader = n, defaultShader = n.
static int graphics_newindex(lua_State *state) {
	const char *name = lua_tolstring(state, 2, nullptr);

	switch (lua_type(state, 3)) {
	case LUA_TUSERDATA:
		if (!strcmp(name, "font")) {
			LuaVisionaireObject *object = CheckVisionaireObject(state, 3, true);

			g_pGameControl->GetFontManager()->SetCurrentFont(TVisObjRef(object->object));
		}

		break;

	case LUA_TTABLE:
		if (!strcmp(name, "matrix1")) {
			matrix1.clear();
			ConvertFromLua(matrix1, 3);
		} else if (!strcmp(name, "matrix2")) {
			matrix2.clear();
			ConvertFromLua(matrix2, 3);
		} else if (!strcmp(name, "textMatrix")) {
			textMatrix.clear();
			ConvertFromLua(textMatrix, 3);
		} else if (!strcmp(name, "invMatrix1")) {
			invMatrix1.clear();
			ConvertFromLua(invMatrix1, 3);
		} else if (!strcmp(name, "fontShaderIndizes")) {
			fontShaderIndizes.clear();
			ConvertFromLua(fontShaderIndizes, 3);
		}

		break;

	case LUA_TSTRING:
		if (!strcmp(name, "clipboard"))
			SDL_SetClipboardText(lua_tolstring(state, 3, nullptr));

		break;

	case LUA_TNUMBER:
		if (!strcmp(name, "fontShader"))
			fontShader = static_cast<int>(lua_tointeger(state, 3));

		if (!strcmp(name, "defaultShader"))
			defaultShader = static_cast<int>(lua_tointeger(state, 3));

		break;

	default:
		break;
	}

	return 0;
}

// Confirmed (asm lines 438739-438799): the scale and the position of matrix1, as x scale, y scale, x, y (without a matrix:
// 1, 1, 0, 0).
static int graphics_getMatrix1Properties(lua_State *state) {
	if (matrix1.size() != 9) {
		lua_pushnumber(state, 1.0);
		lua_pushnumber(state, 1.0);
		lua_pushnumber(state, 0.0);
		lua_pushnumber(state, 0.0);
		return 4;
	}

	lua_pushnumber(state, matrix1[0]);
	lua_pushnumber(state, matrix1[4]);
	lua_pushnumber(state, matrix1[6]);
	lua_pushnumber(state, matrix1[7]);
	return 4;
}

/** The object of the script at the first place as a reference, a character turned into its animation. */
static TVisObjRef objectOrCharacterAnimation(lua_State *state) {
	LuaVisionaireObject *object = CheckVisionaireObject(state, 1, true);
	TVisObjRef ref(object->object);

	if (ref.GetId()[3] == 0)
		ref = gameControl()->GetCharacter(ref)->GetCharacterAnim();

	return ref;
}

/** The animation of an animation object or of an object of the table 9 (the animation of a character: both are asked for
 *  the animations that run); null when the object is neither. */
static TGAnimation *animationOfObject(const TVisObjRef &ref) {
	unsigned char table = ref.GetId()[3];

	if (table != 0x1A && table != 9)
		return nullptr;

	return (table == 0x1A) ? TGAnimation::GetAnimationByObject(ref) : TGAnimation::GetAnimation(ref);
}

// Confirmed (asm lines 438799-438884): the place of the text of a character: the middle of the top of its picture.
static int graphics_getCharacterTextPosition(lua_State *state) {
	LuaVisionaireObject *object = CheckVisionaireObject(state, 1, true);
	TVisObjRef ref(object->object);

	if (ref.GetId()[3] != 0)
		return 0;

	wxRect rect = gameControl()->GetCharacter(ref)->GetCurrentSpriteRect();
	wxPoint point;

	point.x = rect.GetLeft() + rect.GetWidth() / 2;
	point.y = rect.GetTop();

	ConvertToLua(point);
	return 1;
}

// Confirmed (asm lines 438884-439030): the rectangle of what is drawn of the picture of the animation, in the picture.
static int graphics_getAnimationInnerSize(lua_State *state) {
	TVisObjRef ref = objectOrCharacterAnimation(state);
	TGAnimation *animation = animationOfObject(ref);

	if (!animation)
		return 0;

	TPictureIO *sprite = animation->GetCurrentSprite();
	TSpriteHandle *handle = sprite->GetSpriteHandle();

	if (!handle || handle->parts.empty())
		return 0;

	const TSpritePartHandle *part = handle->parts[0];
	wxRect rect;

	rect.x = part->left;
	rect.y = part->top;
	rect.width = part->right - part->left;
	rect.height = part->bottom - part->top;
	ConvertToLua(rect);
	return 1;
}

// Confirmed (asm lines 439030-439155): the size of the picture of the animation.
static int graphics_getAnimationSize(lua_State *state) {
	TVisObjRef ref = objectOrCharacterAnimation(state);
	TGAnimation *animation = animationOfObject(ref);

	if (!animation)
		return 0;

	TPictureIO *sprite = animation->GetCurrentSprite();

	wxPoint size;

	size.x = sprite->GetWidth();
	size.y = sprite->GetHeight();
	ConvertToLua(size);
	return 1;
}

// Confirmed (asm lines 439155-439281)
static int graphics_getCurrentSpritePath(lua_State *state) {
	TVisObjRef ref = objectOrCharacterAnimation(state);
	TGAnimation *animation = animationOfObject(ref);

	if (!animation)
		return 0;

	lua_pushstring(state, animation->GetCurrentSprite()->GetPathNonConst().mb_str());
	return 1;
}

// Confirmed (asm lines 439727-439764)
static int graphics_getScrollPosition(lua_State *state) {
	const FloatPoint &scroll = gameControl()->GetScene()->GetFloatScrollPos();
	float y = scroll.y;

	lua_pushnumber(state, scroll.x);
	lua_pushnumber(state, y);
	return 2;
}

// Confirmed (asm lines 439764-439807)
static int graphics_setScrollPosition(lua_State *state) {
	double x = luaL_checknumber(state, 1);
	double y = luaL_checknumber(state, 2);
	wxRealPoint position;

	position.x = static_cast<float>(x);
	position.y = static_cast<float>(y);
	gameControl()->GetScene()->SetScrollPos(position);
	return 0;
}

// Confirmed (asm lines 439807-439878): the number of the attribute `name` of shader n; nothing for a shader that does not exist.
static int graphics_getAttrib(lua_State *state) {
	TShader *shader = shaderByNumber(luaL_checkinteger(state, 1));

	if (!shader)
		return 0;

	lua_pushinteger(state, shader->GetAttrib(luaL_checklstring(state, 2, nullptr)));
	return 1;
}

// Confirmed (asm lines 439878-439950): the colour of the current font as three numbers from 0 to 1 (red, green, blue); without a
// font 1, 1, 1.
static int graphics_fontColor(lua_State *state) {
	TCFont *font = g_pGameControl->GetFontManager()->GetCurrentFont();
	int red = 0xFF;
	int green = 0xFF;
	int blue = 0xFF;

	if (font) {
		int color = font->GetFontObject().GetInt(kFontColor);

		red = color & 0xFF;
		green = (color >> 8) & 0xFF;
		blue = (color >> 16) & 0xFF;
	}

	lua_pushnumber(state, static_cast<float>(red) / 255.0f);
	lua_pushnumber(state, static_cast<float>(green) / 255.0f);
	lua_pushnumber(state, static_cast<float>(blue) / 255.0f);
	return 3;
}

// Confirmed (asm lines 439950-439975)
static int graphics_fontLineHeight(lua_State *state) {
	lua_pushinteger(state, g_pGameControl->GetFontManager()->GetLineHeight());
	return 1;
}

// Confirmed (asm lines 440087-440152): graphics.noise(x [, y]) is the simplex noise of one or two numbers; with more arguments
// it gives nothing.
static int graphics_noise(lua_State *state) {
	double x = luaL_checknumber(state, 1);
	double y = luaL_optnumber(state, 2, 0.0);

	switch (lua_gettop(state)) {
	case 1:
		lua_pushnumber(state, SimplexNoise1234::noise(static_cast<float>(x)));
		return 1;
	case 2:
		lua_pushnumber(state, SimplexNoise1234::noise(static_cast<float>(x), static_cast<float>(y)));
		return 1;
	default:
		return 0;
	}
}

// Confirmed (asm lines 439975-440087): graphics.noise2(x [, y [, z [, w]]]) is the Perlin noise of one to four numbers.
static int graphics_noise2(lua_State *state) {
	double x = luaL_checknumber(state, 1);
	double y = luaL_optnumber(state, 2, 0.0);
	double z = luaL_optnumber(state, 3, 0.0);
	double w = luaL_optnumber(state, 4, 0.0);
	float fx = static_cast<float>(x);
	float fy = static_cast<float>(y);
	float fz = static_cast<float>(z);
	float fw = static_cast<float>(w);

	switch (lua_gettop(state)) {
	case 1:
		lua_pushnumber(state, Noise1234::noise(fx));
		return 1;
	case 2:
		lua_pushnumber(state, Noise1234::noise(fx, fy));
		return 1;
	case 3:
		lua_pushnumber(state, Noise1234::noise(fx, fy, fz));
		return 1;
	case 4:
		lua_pushnumber(state, Noise1234::noise(fx, fy, fz, fw));
		return 1;
	default:
		return 0;
	}
}

// Confirmed (asm lines 440152-440172)
static int graphics_isUpsideDown(lua_State *state) {
	lua_pushboolean(state, g_subSys && g_subSys->_isUpsideDown);
	return 1;
}

// Confirmed (asm lines 440228-440260)
static int graphics_clipboard(lua_State *state) {
	char *text = SDL_GetClipboardText();

	lua_pushstring(state, text);
	SDL_free(text);
	return 1;
}

// Confirmed (asm lines 440260-440291): graphics.setDebugRenderOffset(x, y), where the Box2D world is.
static int graphics_box2DOffset(lua_State *state) {
	b2xoffset = static_cast<float>(lua_tonumber(state, 1));
	b2yoffset = static_cast<float>(lua_tonumber(state, 2));
	return 0;
}

// The window gets the event that makes it follow the aspect ratio of the game (lock) or not (unlock).
static void pushAspectEvent() {
	SDL_Event event;

	memset(&event, 0, sizeof(event));
	event.type = 0x200;
	event.window.event = 6;

	if (SDL_PushEvent(&event) == -1 && wxLog::loglevel > 0)
		wxLog::logexpanded(L"Event can't be pushed: %s", wxString(SDL_GetError()).wc_str());
}

// Confirmed (asm lines 440333-440360)
static int graphics_lockAspectRatio(lua_State *) {
	g_unlockAspect = false;
	pushAspectEvent();
	return 0;
}

// Confirmed (asm lines 447596-447695)
static int graphics_unlockAspectRatio(lua_State *) {
	g_unlockAspect = true;
	pushAspectEvent();
	return 0;
}

// Confirmed (asm lines 440734-440800): the function the shader calls (as the script command shaderCallback does).
static int graphics_shaderCallback(lua_State *state) {
	TShader *shader = shaderByNumber(luaL_checkinteger(state, 1));

	if (shader)
		shader->_callback = luaL_checklstring(state, 2, nullptr);

	return 0;
}

// Confirmed (asm lines 442889-442956)
static int graphics_shaderDrawCallback(lua_State *state) {
	TShader *shader = shaderByNumber(luaL_checkinteger(state, 1));

	if (shader)
		shader->_drawCallback = luaL_checklstring(state, 2, nullptr);

	return 0;
}

// Confirmed (asm lines 442956-442984): graphics.lightmapCallback("name") is the Lua function that may change the colour of a
// lightmap pixel (TGScene::GetTint).
static int graphics_lightmapCallback(lua_State *state) {
	TGScene::LuaLightmapCallback = luaL_checklstring(state, 1, nullptr);
	return 0;
}

// Confirmed (asm lines 439554-439612)
static int graphics_shaderUse(lua_State *state) {
	TShader *shader = shaderByNumber(luaL_checkinteger(state, 1));

	if (shader) {
		shader->Use();
		shader->AfterUse();
	}

	return 0;
}

// Confirmed (asm lines 445852-446320): graphics.shaderUniform(shader, "name", value): the uniform `name` of a shader is a number
// (with the name "i_..." an integer), a table of 2, 3, 4, 9 or 16 numbers (a vector or a matrix), a string with the path of a
// texture ("_t_..."), or the framebuffer object ("_t_..."). The name in the shader is without the first three characters when
// they are "_t_" or "i_".
static int graphics_shaderUniform(lua_State *state) {
	static std::vector<float> values;

	const char *name = luaL_checklstring(state, 2, nullptr);
	lua_Integer number = luaL_checkinteger(state, 1);
	bool integer = false;
	size_t length = strlen(name);

	if (length > 3 && name[0] == '_' && name[1] == 'i' && name[2] == '_') {
		name += 3;
		integer = true;
	}

	TShader *shader = shaderByNumber(number);

	if (!shader)
		return 0;

	switch (lua_type(state, 3)) {
	case LUA_TNUMBER:
		if (integer)
			shader->SetUniform(name, static_cast<int>(luaL_checknumber(state, 3)));
		else
			shader->SetUniform(name, static_cast<float>(luaL_checknumber(state, 3)));

		return 0;

	case LUA_TTABLE:
		values.clear();
		ConvertFromLua(values, 3);

		switch (values.size()) {
		case 2:
			shader->SetUniform(name, values[0], values[1]);
			break;
		case 3:
			shader->SetUniform(name, values[0], values[1], values[2]);
			break;
		case 4:
			shader->SetUniform(name, values[0], values[1], values[2], values[3]);
			break;
		case 9:
			shader->SetUniformMatrix3(name, values.data());
			break;
		case 16:
			shader->SetUniformMatrix4(name, values.data());
			break;
		default:
			break;
		}

	// (a table does not stop at the "_t_" check: the original goes on with it)
	// fall through

	case LUA_TUSERDATA:
		if (length > 3 && name[0] == '_' && name[1] == 't' && name[2] == '_') {
			TFramebuffer **framebuffer = static_cast<TFramebuffer **>(luaL_checkudata(state, 3, kFramebufferMetatable));

			if (!framebuffer)
				luaL_argerror(state, 3, "invalid object");

			shader->SetUniformFramebuffer(name + 3, *framebuffer);
		}

		return 0;

	case LUA_TSTRING:
		if (length > 3 && name[0] == '_' && name[1] == 't' && name[2] == '_') {
			wxString path;

			toUTF(&path, luaL_checklstring(state, 3, nullptr));

			if (path.StartsWith(wxString(L"vispath:")))
				path = path.Mid(8, -1);

			shader->SetUniformTexture(name + 3, path.mb_str());
		}

		return 0;

	default:
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"invalid argument to fastShaderUniform");

		return 0;
	}
}

// Confirmed (asm lines 445659-445742): the size of a text in the current font.
static int graphics_fontDimension(lua_State *state) {
	const char *text = luaL_checklstring(state, 1, nullptr);
	wxString converted;
	wxPoint size;

	toUTF(&converted, text);
	g_pGameControl->GetFontManager()->GetTextDimension(converted, size);
	ConvertToLua(size);
	return 1;
}

// Confirmed (asm lines 445742-445852): graphics.drawFont("text", x, y [, scale]) prints the text with the current font, left aligned.
static int graphics_drawFont(lua_State *state) {
	const char *text = luaL_checklstring(state, 1, nullptr);
	lua_Integer x = luaL_checkinteger(state, 2);
	lua_Integer y = luaL_checkinteger(state, 3);
	float scale = static_cast<float>(luaL_optnumber(state, 4, 1.0));
	wxString converted;

	toUTF(&converted, text);

	wxPoint position;

	position.x = static_cast<int>(x);
	position.y = static_cast<int>(y);

	g_pGameControl->GetFontManager()->PrintText(converted, TextAlignmentEnum::kLeft, position, scale, nullptr);
	return 0;
}

// Confirmed (asm lines 448282-448596): the lines of a text after the automatic line breaks of the current font (a table of strings).
static int graphics_performLinebreaks(lua_State *state) {
	const char *text = luaL_checklstring(state, 1, nullptr);
	wxString converted;
	std::list<wxString> lines;

	toUTF(&converted, text);
	g_pGameControl->GetFontManager()->PerformAutoLineBreak(converted, lines);

	std::vector<TCharHolder> holders;

	for (const wxString &line : lines)
		holders.push_back(TCharHolder(line.mb_str()));

	ConvertToLua(holders);
	return 1;
}

// Confirmed (asm lines 443885-445082): graphics.evalTween(from, to, part, easing): the value of a tween from `from` to `to` when
// `part` (0 to 1, cut) of it is gone, by one of the easings (the numbers of the script commands); an unknown number is linear.
static int graphics_evalTween(lua_State *state) {
	double begin = luaL_checknumber(state, 1);
	double end = luaL_checknumber(state, 2);
	double part = luaL_checknumber(state, 3);
	int easing = static_cast<int>(luaL_checkinteger(state, 4));
	std::function<double(double)> chosen = easingByNumber(easing);

	if (!chosen)
		chosen = Easing::LinearInOut;

	Tween tween(begin, end, 1.0, chosen, false, false);
	float time = 0.0f;

	if (part >= 1.0)
		time = 1.0f;
	else if (part > 0.0)
		time = static_cast<float>(part);

	tween.Update(time);
	lua_pushnumber(state, tween.GetValue());
	return 1;
}

// Confirmed (asm lines 448596-448827): graphics.addDrawFunc("name" [, where]) lets TMasterControl::Draw() call the global Lua
// function `name` after the scene (where 0, the default), after the interfaces (1) or before the scene (-1); a name is only in
// a list once.
static int graphics_addDrawFunc(lua_State *state) {
	std::string name = luaL_checklstring(state, 1, nullptr);
	lua_Integer where = luaL_optinteger(state, 2, 0);
	std::vector<std::string> *list = nullptr;

	if (where == 0)
		list = &luaDrawAfterScene;
	else if (where == 1)
		list = &luaDrawAfterInterfaces;
	else if (where == -1)
		list = &luaDrawBeforeScene;

	if (list && std::find(list->begin(), list->end(), name) == list->end())
		list->push_back(name);

	return 0;
}

// Confirmed (asm lines 443087-443383): the name leaves all of the three lists.
static int graphics_removeDrawFunc(lua_State *state) {
	std::string name = luaL_checklstring(state, 1, nullptr);

	for (std::vector<std::string> *list : {
	            &luaDrawAfterInterfaces, &luaDrawAfterScene, &luaDrawBeforeScene
	        })
		list->erase(std::remove(list->begin(), list->end(), name), list->end());

	return 0;
}

// Confirmed (asm lines 3144349-3144560, the tables of functions)
static const luaL_Reg graphics_sprite[] = {
	{"new", graphics_sprite_new},
	{"clear", graphics_sprite_clear},
	{"__index", graphics_sprite_index},
	{"__newindex", graphics_sprite_newindex},
	{"__call", graphics_sprite_call},
	{"__gc", graphics_sprite_gc},
	{nullptr, nullptr}
};

static const luaL_Reg *spriteFunctions() {
	return graphics_sprite;
}

static const luaL_Reg graphics_framebuffer[] = {
	{"bind", graphics_framebuffer_bind},
	{"__gc", graphics_gc},
	{nullptr, nullptr}
};

static const luaL_Reg graphics_buffer[] = {
	{"bind", graphics_buffer_bind},
	{"update", graphics_buffer_update},
	{"__gc", graphics_gc},
	{nullptr, nullptr}
};

static const luaL_Reg graphics_movie[] = {
	{"draw", graphics_movie_draw},
	{"finish", graphics_movie_finish},
	{"seek", graphics_movie_seek},
	{"getDuration", graphics_movie_getDuration},
	{"getTime", graphics_movie_getTime},
	{"pause", graphics_movie_pause},
	{"resume", graphics_movie_resume},
	{"__index", graphics_movie_index},
	{"__newindex", graphics_movie_newindex},
	{"__gc", graphics_movie_finish},
	{nullptr, nullptr}
};

static const luaL_Reg *movieFunctions() {
	return graphics_movie;
}

static const luaL_Reg graphics_meths[] = {
	{"drawFont", graphics_drawFont},
	{"drawSprite", graphics_drawSprite},
	{"drawSpriteWithNineRect", graphics_drawSpriteWithNineRect},
	{"drawBox", graphics_drawBox},
	{"drawLine", graphics_drawLine},
	{"addDrawFunc", graphics_addDrawFunc},
	{"removeDrawFunc", graphics_removeDrawFunc},
	{"clipboard", graphics_clipboard},
	{"loadFromFile", graphics_loadFromFile},
	{"loadMemoryJPG", graphics_loadMemoryJPG},
	{"loadMemoryPNG", graphics_loadMemoryPNG},
	{"loadMemoryWEBP", graphics_loadMemoryWEBP},
	{"isUpsideDown", graphics_isUpsideDown},
	{"unlockAspectRatio", graphics_unlockAspectRatio},
	{"lockAspectRatio", graphics_lockAspectRatio},
	{"evalTween", graphics_evalTween},
	{"noise", graphics_noise},
	{"noise2", graphics_noise2},
	{"performLinebreaks", graphics_performLinebreaks},
	{"fontDimension", graphics_fontDimension},
	{"fontLineHeight", graphics_fontLineHeight},
	{"fontColor", graphics_fontColor},
	{"lightmapCallback", graphics_lightmapCallback},
	{"shaderUniform", graphics_shaderUniform},
	{"shaderCallback", graphics_shaderCallback},
	{"shaderDrawCallback", graphics_shaderDrawCallback},
	{"shaderUse", graphics_shaderUse},
	{"shaderAttrib", graphics_getAttrib},
	{"movieOpen", graphics_movieOpen},
	{"setScrollPosition", graphics_setScrollPosition},
	{"getScrollPosition", graphics_getScrollPosition},
	{"drawIndexed", graphics_drawIndexed},
	{"setupOffsets", graphics_setupOffsets},
	{"createIndexBuffer", graphics_createIndexBuffer},
	{"createBuffer", graphics_createBuffer},
	{"createFramebuffer", graphics_createFramebuffer},
	{"bindFramebuffer", graphics_bindFramebuffer},
	{"instantiateAnimation", graphics_instantiateAnimation},
	{"drawAnimation", graphics_drawAnimation},
	{"getCurrentSpritePath", graphics_getCurrentSpritePath},
	{"getAnimationSize", graphics_getAnimationSize},
	{"getAnimationInnerSize", graphics_getAnimationInnerSize},
	{"getCharacterTextPosition", graphics_getCharacterTextPosition},
	{"getMatrix1Properties", graphics_getMatrix1Properties},
	{"clear", graphics_clear},
	{"createBox2DDebugRender", graphics_createBox2DDebugRender},
	{"setDebugRenderOffset", graphics_box2DOffset},
	{"openedVideos", graphics_openedVideos},
	{"__newindex", graphics_newindex},
	{"__gc", graphics_gc},
	{nullptr, nullptr}
};

/** A metatable with `__index` as itself and the functions of `functions`. */
static void makeClass(lua_State *state, const char *metatable, const luaL_Reg *functions) {
	luaL_newmetatable(state, metatable);
	lua_pushlstring(state, "__index", 7);
	lua_pushvalue(state, -2);
	lua_rawset(state, -3);
	luaL_openlib(state, nullptr, functions, 0);
}

// Confirmed (asm lines 447452-447586): the classes of the drawing and the global `graphics`.
void luaopen_Graphics(lua_State *state) {
	makeClass(state, "Visionaire.Sprite", graphics_sprite);
	makeClass(state, kFramebufferMetatable, graphics_framebuffer);
	makeClass(state, "Visionaire.TBuffer", graphics_buffer);
	makeClass(state, "Visionaire.TMovie", graphics_movie);
}

void luaopen_GraphicsObject(lua_State *state) {
	makeClass(state, kGraphicsMetatable, graphics_meths);
	lua_createtable(state, 0, 0);
	lua_getfield(state, LUA_REGISTRYINDEX, kGraphicsMetatable);
	lua_setmetatable(state, -2);
	lua_setfield(state, LUA_GLOBALSINDEX, "graphics");
}
