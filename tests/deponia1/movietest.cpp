#include <cstdio>
#include <vector>

#include "AppGlobals.h"
#include "TMovie.h"
#include "THGameControl.h"
#include "graphicslib/graphics.h"
#include "vsplayer/scripting/luaSystem.h"
#include "vscommon/scripting/visLua.h"
#include "vstables/visionaireGame.h"
#include "vscommon/objAccess.h"
#include "vstables/fieldIds.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

struct MockPlayer : TMoviePlayer {
	static int created;
	static MockPlayer *last;
	int framesLeft = 3;
	wxString openedPath;
	TMovieSettings settings;
	float lastSeek = -1;
	int blend = -1;
	wxColour colour;
	bool loop = false;
	bool closed = false;
	bool paused = false;

	MockPlayer() { created++; last = this; }
	bool Open(const wxString &path, const TMovieSettings &s) override { openedPath = path; settings = s; return true; }
	bool Frame(const std::function<void(void *)> &eventFunction) override {
		if (eventFunction) {
			SDL_Event e;
			e.type = SDL_MOUSEBUTTONDOWN;
			e.button.x = 11;
			e.button.y = 22;
			eventFunction(&e);
		}
		return --framesLeft > 0;
	}
	void Close() override { closed = true; }
	void Pause() override { paused = true; }
	void Resume() override { paused = false; }
	void Seek(float s) override { lastSeek = s; }
	float GetTime() override { return 1.5f; }
	float GetDuration() override { return 10.0f; }
	float GetCompletition() override { return 150.0f; }
	int GetWidth() override { return 640; }
	int GetHeight() override { return 480; }
	void SetBlend(int b) override { blend = b; }
	void SetColour(const wxColour &c) override { colour = c; }
	void SetLoop(bool l) override { loop = l; }
	bool Draw(float, float, float, float) override { return true; }
};
int MockPlayer::created = 0;
MockPlayer *MockPlayer::last = nullptr;

static TMoviePlayer *makeMock() {
	return new MockPlayer();
}

class RecordingGraphics : public TGraphicsInterface {
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
	graphics = new RecordingGraphics();

	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	control->GetVisionaire()->NewGame();
	g_createMoviePlayer = makeMock;

	// a movie of a script
	LuaDoString("m = graphics.movieOpen('vispath:video/intro.avi'); m.blend = 3; m.color = {10, 20, 30, 40}; m.loop = true;"
	            "w = m.width; h = m.height; d = m:getDuration(); t = m:getTime(); ok = m:draw(1, 2); m:seek(4.5); m:pause();"
	            "v = graphics.openedVideos(); n = #v; c = v[1] and v[1].file or 'none'");
	CHECK(MockPlayer::created == 1);
	CHECK(MockPlayer::last && MockPlayer::last->blend == 3);
	CHECK(MockPlayer::last && MockPlayer::last->colour.red == 10 && MockPlayer::last->colour.alpha == 40);
	CHECK(MockPlayer::last && MockPlayer::last->loop && MockPlayer::last->lastSeek == 4.5f && MockPlayer::last->paused);
	CHECK(MockPlayer::last && MockPlayer::last->openedPath.ToStdWstring().find(L"intro.avi") != std::wstring::npos);

	lua_getfield(L, LUA_GLOBALSINDEX, "w"); CHECK(lua_tonumber(L, -1) == 640); lua_settop(L, 0);
	lua_getfield(L, LUA_GLOBALSINDEX, "d"); CHECK(lua_tonumber(L, -1) == 10); lua_settop(L, 0);
	lua_getfield(L, LUA_GLOBALSINDEX, "ok"); CHECK(lua_toboolean(L, -1) == 1); lua_settop(L, 0);
	lua_getfield(L, LUA_GLOBALSINDEX, "n"); printf("open videos: %g\n", lua_tonumber(L, -1)); lua_settop(L, 0);

	LuaDoString("m:finish(); w2 = m.width");
	CHECK(MockPlayer::last->closed);
	lua_getfield(L, LUA_GLOBALSINDEX, "w2"); CHECK(lua_isnil(L, -1)); lua_settop(L, 0);

	// the movie of the master control
	CHECK(!control->IsVideoPlaying());
	wxFileName file(L"video/cutscene.avi");
	int result = control->PlayAVI(file, true, HandleSoundsEnum::kPause);
	CHECK(result == 0 || result == 1);
	printf("PlayAVI -> %d, playing %d\n", result, control->IsVideoPlaying());
	CHECK(control->IsVideoPlaying());
	int frames = 0;
	while (control->VideoFrame())
		frames++;
	printf("frames %d\n", frames);
	CHECK(frames == 2);
	CHECK(!control->IsVideoPlaying());

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
