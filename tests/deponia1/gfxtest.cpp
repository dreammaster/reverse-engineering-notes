#include <cstdio>
#include <vector>

#include "AppGlobals.h"
#include "TPaintControl.h"
#include "graphicslib/graphics.h"
#include "graphicslib/picture.h"
#include "vsplayer/scripting/luaSystem.h"
#include "vscommon/scripting/visLua.h"
#include "vscommon/scripting/luaConversion.h"
#include "vstables/visionaireGame.h"
#include "vscommon/objAccess.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

struct DrawCall {
	wxRect src;
	FloatRect dst;
	float alpha;
	unsigned color;
};

class RecordingGraphics : public TGraphicsInterface {
public:
	std::vector<DrawCall> draws;
	std::vector<wxRect> boxes;
	std::vector<std::pair<wxPoint, wxPoint>> lines;

	void Draw(TSpriteHandle *, const wxRect &src, const FloatRect &dst, float alpha, bool, const unsigned int &color, int, float,
	          const wxPoint &, float, float, int) override {
		draws.push_back({src, dst, alpha, color});
	}
	void DrawBox(const wxRect &rect, unsigned int, float) override {
		boxes.push_back(rect);
	}
	void DrawLine(const wxPoint &a, const wxPoint &b, unsigned int, float) override {
		lines.push_back({a, b});
	}
};

int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	TVisionaireGame game;
	game.NewGame();
	InitObjectAccess(&game);
	InitLua(&game, wxString(L"C:/app"), wxString(), wxString());

	luaopen_Graphics(L);
	luaopen_GraphicsObject(L);

	RecordingGraphics *recorder = new RecordingGraphics();
	graphics = recorder;

	// a picture of 30 x 20 made in memory, as a Sprite of the script
	TPictureIO *picture = new TPictureIO();
	CHECK(picture->CreateEmptySprite(30, 20, 4, false));
	TPictureIO **userdata = static_cast<TPictureIO **>(lua_newuserdata(L, sizeof(TPictureIO *)));
	*userdata = picture;
	lua_getfield(L, LUA_REGISTRYINDEX, "Visionaire.Sprite");
	lua_setmetatable(L, -2);
	lua_setfield(L, LUA_GLOBALSINDEX, "sp");

	LuaDoString("r = {sp.width, sp.height, sp.scale, sp.rotation, sp.matrixId, tostring(sp.position.x)}");
	LuaDoString("sp.position = {x = 5, y = 6}; sp.rotation = 0.5; sp.scale = 2; sp.shaderSet = 3;"
	            "q = {sp.position.x, sp.position.y, sp.rotation, sp.scaleX, sp.scaleY, sp.shaderSet}");

	lua_getfield(L, LUA_GLOBALSINDEX, "q");
	lua_rawgeti(L, -1, 1); CHECK(lua_tonumber(L, -1) == 5); lua_settop(L, -2);
	lua_rawgeti(L, -1, 2); CHECK(lua_tonumber(L, -1) == 6); lua_settop(L, -2);
	lua_rawgeti(L, -1, 3); CHECK(lua_tonumber(L, -1) == 0.5); lua_settop(L, -2);
	lua_rawgeti(L, -1, 4); CHECK(lua_tonumber(L, -1) == 2); lua_settop(L, -2);
	lua_rawgeti(L, -1, 5); CHECK(lua_tonumber(L, -1) == 2); lua_settop(L, -2);
	lua_rawgeti(L, -1, 6); CHECK(lua_tonumber(L, -1) == 3); lua_settop(L, -2);
	lua_settop(L, 0);

	LuaDoString("graphics.drawSpriteWithNineRect(sp, {x = 10, y = 20, width = 100, height = 50}, {x = 4, y = 5, width = 6, height = 7}, 255, 0.5)");
	CHECK(recorder->draws.size() == 9);

	if (recorder->draws.size() == 9) {
		// top left: source 0,0,4,5 -> 10,20,4,5; the middle: source 4,5,20,8 -> 14,25,80,38; bottom right: 24,13,6,7 -> 104,63,6,7
		const DrawCall &tl = recorder->draws[0];
		CHECK(tl.src.x == 0 && tl.src.y == 0 && tl.src.width == 4 && tl.src.height == 5);
		CHECK(tl.dst.x == 10 && tl.dst.y == 20 && tl.dst.width == 4 && tl.dst.height == 5);
		const DrawCall &c = recorder->draws[4];
		CHECK(c.src.x == 4 && c.src.y == 5 && c.src.width == 20 && c.src.height == 8);
		CHECK(c.dst.x == 14 && c.dst.y == 25 && c.dst.width == 90 && c.dst.height == 38);
		const DrawCall &br = recorder->draws[8];
		CHECK(br.src.x == 24 && br.src.y == 13 && br.src.width == 6 && br.src.height == 7);
		CHECK(br.dst.x == 104 && br.dst.y == 63 && br.dst.width == 6 && br.dst.height == 7);
		CHECK(recorder->draws[0].alpha == 0.5f && recorder->draws[0].color == 255);
		printf("center dst %g,%g %gx%g\n", c.dst.x, c.dst.y, c.dst.width, c.dst.height);
	}

	LuaDoString("graphics.drawBox(1, 2, 3, 4, 255); graphics.drawLine(1, 2, 3, 4, 255)");
	CHECK(recorder->boxes.size() == 1 && recorder->boxes[0].width == 3);
	CHECK(recorder->lines.size() == 1 && recorder->lines[0].second.y == 4);

	recorder->draws.clear();
	LuaDoString("graphics.drawSprite(sp, 0.25, 255)");
	CHECK(recorder->draws.size() == 1);

	LuaDoString("m = graphics.loadFromFile('vispath:nothing/here.png')");
	lua_getfield(L, LUA_GLOBALSINDEX, "m");
	CHECK(lua_isnil(L, -1));
	lua_settop(L, 0);
	LuaDoString("collectgarbage()");

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
