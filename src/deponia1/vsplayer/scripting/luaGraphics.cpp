// Confirmed (Deponia_Linux.asm lines 438459-448827, 3144349-3144560): the global `graphics` of the scripts, and the sprites,
// framebuffers, buffers and movies that it makes (the metatables "Visionaire.TGraphics", "Visionaire.Sprite",
// "Visionaire.TFramebuffer", "Visionaire.TBuffer" and "Visionaire.TMovie"). With it the scripts set the matrices that the
// drawing goes through (matrix1, matrix2, textMatrix, invMatrix1), add functions that are called when the scene is drawn
// (addDrawFunc), ask about fonts, animations and characters, and draw by themselves.
//
// Done here are the functions that are about the engine (the fonts, the animations, the scroll position, the shaders'
// uniforms ...). The ones that need the video card - drawing sprites, boxes and lines, the buffers and framebuffers, the
// sprite objects, movies, loading pictures from memory, the Box2D debug drawing - are in the tables with the name they have, and log that they are not reconstructed when a script calls them.
#include <string.h>

#include <algorithm>
#include <functional>

#include "AppGlobals.h"
#include "Diagnostics.h"
#include "SdlStub.h"
#include "TFramebuffer.h"
#include "TGCharacter.h"
#include "TGScene.h"
#include "Easing.h"
#include "Tween.h"
#include "common/lua/lauxlib.h"
#include "graphicslib/graphics.h"
#include "graphicslib/noise1234.h"
#include "graphicslib/shader.h"
#include "graphicslib/subsys.h"
#include "vscommon/cfont.h"
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

BACKEND_FUNCTION(drawSprite)
BACKEND_FUNCTION(drawSpriteWithNineRect)
BACKEND_FUNCTION(drawBox)
BACKEND_FUNCTION(drawLine)
BACKEND_FUNCTION(loadFromFile)
BACKEND_FUNCTION(loadMemoryJPG)
BACKEND_FUNCTION(loadMemoryPNG)
BACKEND_FUNCTION(loadMemoryWEBP)
BACKEND_FUNCTION(movieOpen)
BACKEND_FUNCTION(drawIndexed)
BACKEND_FUNCTION(setupOffsets)
BACKEND_FUNCTION(createIndexBuffer)
BACKEND_FUNCTION(createBuffer)
BACKEND_FUNCTION(createFramebuffer)
BACKEND_FUNCTION(bindFramebuffer)
BACKEND_FUNCTION(instantiateAnimation)
BACKEND_FUNCTION(drawAnimation)
BACKEND_FUNCTION(clear)
BACKEND_FUNCTION(createBox2DDebugRender)

// The methods of the sprite, framebuffer, buffer and movie objects, which are the backend's.
BACKEND_FUNCTION(sprite_new)
BACKEND_FUNCTION(sprite_clear)
BACKEND_FUNCTION(sprite_index)
BACKEND_FUNCTION(sprite_newindex)
BACKEND_FUNCTION(sprite_call)
BACKEND_FUNCTION(framebuffer_bind)
BACKEND_FUNCTION(buffer_bind)
BACKEND_FUNCTION(buffer_update)
BACKEND_FUNCTION(movie_draw)
BACKEND_FUNCTION(movie_finish)
BACKEND_FUNCTION(movie_seek)
BACKEND_FUNCTION(movie_getDuration)
BACKEND_FUNCTION(movie_getTime)
BACKEND_FUNCTION(movie_pause)
BACKEND_FUNCTION(movie_resume)
BACKEND_FUNCTION(movie_index)
BACKEND_FUNCTION(movie_newindex)

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

// Confirmed (asm lines 440847-441099): a table with an entry for each movie that is open ({video = path, ...}); no movie is open
// here (the movies are not reconstructed).
static int graphics_openedVideos(lua_State *state) {
	lua_createtable(state, 0, 0);
	return 1;
}

// Confirmed (asm lines 3144349-3144560, the tables of functions)
static const luaL_Reg graphics_sprite[] = {
	{"new", graphics_sprite_new},
	{"clear", graphics_sprite_clear},
	{"__index", graphics_sprite_index},
	{"__newindex", graphics_sprite_newindex},
	{"__call", graphics_sprite_call},
	{"__gc", graphics_gc},
	{nullptr, nullptr}
};

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
	{"__gc", graphics_gc},
	{nullptr, nullptr}
};

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
