#include <cstdio>
#include <vector>

#include "AppGlobals.h"
#include "TPaintControl.h"
#include "MD5.h"
#include "TPolygonList.h"
#include "TMemoryBuffer.h"
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
	{
		// MD5 as text, and the cipher that repeats the first 16 characters of it
		char *empty = MD5String("", 0);
		char *abc = MD5String("abc", 3);
		char *longer = MD5String("The quick brown fox jumps over the lazy dog", 43);

		CHECK(std::string(empty) == "d41d8cd98f00b204e9800998ecf8427e");
		CHECK(std::string(abc) == "900150983cd24fb0d6963f7d28e17f72");
		CHECK(std::string(longer) == "9e107d9d372bb6826bd81d3542a419d6");
		std::free(empty);
		std::free(abc);
		std::free(longer);

		TMemoryBuffer buffer;
		const char plain[] = "0123456789abcdef0123456789abcdef-tail";

		buffer.AppendData(plain, sizeof(plain));
		CHECK(buffer.Encrypt(wxString(L"abc"), nullptr, 0));
		// the key is "900150983cd24fb0", the first 16 characters of the text of the digest of "abc"
		CHECK(buffer.GetData()[0] == ('0' ^ '9'));
		CHECK(buffer.GetData()[1] == ('1' ^ '0'));
		CHECK(buffer.GetData()[15] == ('f' ^ '0'));
		CHECK(buffer.GetData()[16] == ('0' ^ '9'));
		CHECK(buffer.Decrypt(wxString(L"abc"), nullptr, 0));
		CHECK(std::string((const char *)buffer.GetData()) == plain);
	}

	{
		// the scan-line test of a point in a polygon: a square and a "U" with the notch on top
		std::vector<wxPoint> square = {{10, 10}, {50, 10}, {50, 50}, {10, 50}};
		CHECK(IsPointInsidePolygon(wxPoint{30, 30}, square));
		CHECK(!IsPointInsidePolygon(wxPoint{5, 30}, square));
		CHECK(!IsPointInsidePolygon(wxPoint{55, 30}, square));
		CHECK(!IsPointInsidePolygon(wxPoint{30, 5}, square));
		CHECK(!IsPointInsidePolygon(wxPoint{30, 55}, square));
		CHECK(!IsPointInsidePolygon(wxPoint{10, 30}, square));
		CHECK(!IsPointInsidePolygon(wxPoint{50, 30}, square));

		std::vector<wxPoint> notch = {{0, 0}, {30, 0}, {30, 20}, {20, 20}, {20, 10}, {10, 10}, {10, 20}, {0, 20}};
		CHECK(IsPointInsidePolygon(wxPoint{5, 5}, notch));
		CHECK(IsPointInsidePolygon(wxPoint{25, 15}, notch));
		CHECK(!IsPointInsidePolygon(wxPoint{15, 15}, notch));
		CHECK(IsPointInsidePolygon(wxPoint{15, 5}, notch));

		std::vector<wxPoint> twoPoints = {{0, 0}, {10, 10}};
		CHECK(!IsPointInsidePolygon(wxPoint{5, 5}, twoPoints));

		// a diamond: the row of the left and right vertices touches them
		std::vector<wxPoint> diamond = {{20, 0}, {40, 20}, {20, 40}, {0, 20}};
		CHECK(IsPointInsidePolygon(wxPoint{20, 20}, diamond));
		CHECK(IsPointInsidePolygon(wxPoint{5, 20}, diamond));
		CHECK(!IsPointInsidePolygon(wxPoint{2, 5}, diamond));
	}

	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	TVisionaireGame game;
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
