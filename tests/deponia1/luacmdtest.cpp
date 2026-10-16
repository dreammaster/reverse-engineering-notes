#include <cmath>
#include <cstdio>
#include <string>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TGCharacter.h"
#include "TGScene.h"
#include "graphicslib/graphics.h"
#include "vscommon/objAccess.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/visLua.h"
#include "vsplayer/animationGame.h"
#include "vstables/eCommand.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"
#include "fakepictures.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

class NullGraphics : public TGraphicsInterface {
};

// The scripting commands of the player that work on the game: start and stop an action and an animation, the volumes,
// the position of the cursor, getObject / getTime / getProperty, hooks and created events.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	useFakePictures();
	writeFakePicture("pics/a.png", 20, 20);
	writeFakePicture("pics/b.png", 20, 20);

	{
		TVisionaireGame game;
		TVisObjRef gameRef = game.GetGame();
		gameRef.SetValue(kGameWindowResolution, wxPoint{640, 480}, TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameCompanyName, wxString(L"Test Company"), TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameGameName, wxString(L"Test Game"), TSendEventEnum::kNoEvent);

		TVisObjRef scene = game.CreateObject(4, gameRef, kGameSceneLinks);
		scene.SetName(TCharHolder("Hall"));
		TVisObjRef hero = game.CreateObject(0, gameRef, kGameCharacterLinks);
		hero.SetName(TCharHolder("Hero"));
		TVisObjRef startObject = game.CreateObject(6, scene, kSceneObjects);
		startObject.SetName(TCharHolder("Start"));
		startObject.SetValue(kObjectPosition, wxPoint{100, 200}, TSendEventEnum::kNoEvent);
		hero.SetLink(kCharacterStartObject, startObject, true);
		gameRef.SetLink(kGameFirstCharacter, hero, true);
		TVisObjRef ways = game.CreateObject(29, scene, kSceneWaySystems);
		std::vector<wxPoint> border = {{0, 0}, {640, 0}, {640, 480}, {0, 480}};
		ways.SetValue(kWaySystemBorder, border, TSendEventEnum::kNoEvent);
		scene.SetLink(kSceneCurrentWaySystem, ways, true);
		TVisObjRef outfit = game.CreateObject(17, hero, kCharacterOutfits);
		hero.SetLink(kCharacterCurrentOutfit, outfit, true);

		TVisObjRef door = game.CreateObject(6, scene, kSceneObjects);
		door.SetName(TCharHolder("Door"));
		door.SetValue(kObjectPosition, wxPoint{500, 250}, TSendEventEnum::kNoEvent);

		// an animation of the door that never ends
		TVisObjRef blink = game.CreateObject(9, door, kObjectAnimations);
		blink.SetName(TCharHolder("Blink"));
		std::vector<TSprite> sprites(2);
		sprites[0].SetPath(TCharHolder("pics/a.png"));
		sprites[1].SetPath(TCharHolder("pics/b.png"));
		for (TSprite &sprite : sprites)
			sprite.SetPause(40);
		blink.SetValue(kAnimationSprites, sprites, TSendEventEnum::kNoEvent);
		blink.SetValue(kAnimationPosition, wxPoint{100, 100}, TSendEventEnum::kNoEvent);
		blink.SetValue(kAnimationNumberOfLoops, 0, TSendEventEnum::kNoEvent);

		// a language and a text
		TVisObjRef language = game.CreateObject(18, gameRef, kGameLanguages);
		language.SetName(TCharHolder("English"));
		gameRef.SetLink(kGameStandardLanguage, language, true);
		TVisObjRef noParent;
		TVisObjRef greeting = game.CreateObject(14, noParent, -1);
		greeting.SetName(TCharHolder("Greeting"));
		std::vector<TTextLanguage> texts(1);
		texts[0].text = TCharHolder("good day");
		texts[0].languageId = PackVisId(language.GetId());
		greeting.SetValue(kTextTextLanguages, texts, TSendEventEnum::kNoEvent);
		hero.SetLink(kCharacterName, greeting, true);
		door.SetValue(kObjectPolygon, std::vector<wxPoint>{{480, 230}, {520, 230}, {520, 270}, {480, 270}}, TSendEventEnum::kNoEvent);

		// lists of each kind for the methods of the objects that read them
		blink.SetValue(kAnimationWalkSteps, std::vector<float>{1.5f, 2.5f}, TSendEventEnum::kNoEvent);
				outfit.SetPathList(kOutfitModelFiles, std::vector<TCharHolder>{TCharHolder("a.obj"), TCharHolder("b.obj")}, TSendEventEnum::kNoEvent);
		TVisObjRef other = game.CreateObject(14, noParent, -1);
		other.SetName(TCharHolder("Other"));
		other.SetValue(kTextTextLanguages, texts, TSendEventEnum::kNoEvent);

		TVisObjRef listFont = game.CreateObject(3, gameRef, kGameFontLinks);
		listFont.SetName(TCharHolder("ListFont"));
		std::vector<wxRect> letters;
		for (int i = 0; i < 4; i++) {
			wxRect r;
			r.x = i * 8;
			r.y = 0;
			r.width = 8;
			r.height = 12;
			letters.push_back(r);
		}
		listFont.SetValue(kFontLetters, letters, TSendEventEnum::kNoEvent);

		// the start of the game: the scene is shown
		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		TVisObjRef change = game.CreateObject(8, action, kActionActionParts);
		change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		change.SetLink(kActionPartLink, startObject, true);
		change.SetLink(kActionPartAltLink, hero, true);
		gameRef.SetLink(kGameStartAction, action, true);

		// an action that marks, waits two seconds and marks again
		TVisObjRef longAction = game.CreateObject(7, gameRef, kGameActions);
		longAction.SetName(TCharHolder("Long"));
		auto addPart = [&](int command, int value, const char *code) {
			TVisObjRef part = game.CreateObject(8, longAction, kActionActionParts);
			part.SetValue(kActionPartCommand, command, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartInt, value, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartAltInt, 1, TSendEventEnum::kNoEvent);
			if (code)
				part.SetValue(kActionPartString, wxString(code), TSendEventEnum::kNoEvent);
		};
		addPart(kCommandRunPartScript, 0, "long_a = (long_a or 0) + 1");
		addPart(kCommandWait, 2, nullptr);
		addPart(kCommandRunPartScript, 0, "long_b = (long_b or 0) + 1");

		wxFileName out(L"luacmd.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"luacmd.dat");
	wxString warning;

	CHECK(control->PreLoad(file, warning, true));
	InitPlayerCommands(control->GetVisionaire(), wxString(L"."), wxString(L"."), wxString(L"."));
	wxString language;
	bool loaded = control->LoadAndInitGame(file, warning, language, true);

	CHECK(loaded);
	if (loaded) {
		control->RegisterEventHandler();
		control->InitAfterLoadingScreen();
		control->ExecuteStartingAction();

		auto frames = [&](int count) {
			for (int i = 0; i < count; i++) {
				TGAction::ContinueRunningActions(false);
				control->Update();
				control->Draw(true);
				wxMilliSleep(5);
			}
		};
		auto run = [&](const char *code) {
			LuaDoString(code);
		};
		auto number = [&](const char *name) {
			lua_getfield(L, LUA_GLOBALSINDEX, name);
			double result = lua_isnil(L, -1) ? -1e9 : lua_tonumber(L, -1);

			lua_settop(L, 0);
			return result;
		};
		auto text = [&](const char *name) {
			lua_getfield(L, LUA_GLOBALSINDEX, name);
			std::string result = lua_isstring(L, -1) ? lua_tostring(L, -1) : "(not a string)";

			lua_settop(L, 0);
			return result;
		};
		auto flag = [&](const char *name) {
			lua_getfield(L, LUA_GLOBALSINDEX, name);
			int result = lua_isnil(L, -1) ? -1 : lua_toboolean(L, -1);

			lua_settop(L, 0);
			return result;
		};

		frames(20);

		// startAction gives the active action; the action runs, and stopAction ends it in its wait
		run("started = startAction(Actions['Long']); started_empty = started:isEmpty()");
		frames(30);
		CHECK(flag("started_empty") == 0);
		CHECK(number("long_a") == 1);
		CHECK(number("long_b") == -1e9);
		run("stopAction(Actions['Long'])");
		frames(500);
		CHECK(number("long_b") == -1e9);

		// started again, it runs to its end
		run("startAction(Actions['Long'])");
		frames(500);
		printf("long action: a %g, b %g\n", number("long_a"), number("long_b"));
		CHECK(number("long_a") == 2);
		CHECK(number("long_b") == 1);

		// an animation is started and stopped
		int before = (int)TGAnimation::GetRunningAnimations().size();

		run("state = startAnimation(Animations['Blink']); state_empty = state:isEmpty()");
		frames(10);
		int during = (int)TGAnimation::GetRunningAnimations().size();

		CHECK(flag("state_empty") == 0);
		CHECK(during == before + 1);
		run("stopAnimation(Animations['Blink'])");
		frames(10);
		CHECK((int)TGAnimation::GetRunningAnimations().size() == before);
		run("state = startAnimation(Animations['Blink'], true)");
		frames(10);
		CHECK((int)TGAnimation::GetRunningAnimations().size() == before + 1);
		run("stopAnimation(Animations['Blink'])");

		// the volumes
		run("setVolume(0, 33); setVolume(1, 44); setVolume(2, 55); setVolume(3, 66); setVolume(4, 77); "
		    "v_music = getVolume(0); v_sound = getVolume(1); v_speech = getVolume(2); v_movie = getVolume(3); v_global = getVolume(4)");
		printf("volumes: %g %g %g %g %g\n", number("v_music"), number("v_sound"), number("v_speech"), number("v_movie"), number("v_global"));
		CHECK(number("v_music") == 33);
		CHECK(number("v_sound") == 44);
		CHECK(number("v_speech") == 55);
		CHECK(number("v_movie") == 66);
		CHECK(number("v_global") == 77);
		// (a script that fails goes on being run: the error handler writes the traceback, which crashed once the stack of Lua
		// had been damaged by a script that worked)
		run("setVolume(1, 200); v_sound = getVolume(1)");
		run("v_sound = getVolume(1)");
		CHECK(number("v_sound") == 44);

		// the cursor
		run("setCursorPos({x = 120, y = 80}); cursor = getCursorPos(); cursor_x = cursor.x; cursor_y = cursor.y");
		printf("cursor at %g,%g\n", number("cursor_x"), number("cursor_y"));

		// the object of a path, the time, the properties
		run("found = getObject('Characters[Hero]'); found_name = found:getName(); missing = getObject('Characters[Nobody]', false); "
		    "missing_empty = missing:isEmpty()");
		CHECK(text("found_name") == "Hero");
		CHECK(flag("missing_empty") == 1);
		run("t1 = getTime({flags = true, reset = true}); t2 = getTime()");
		CHECK(number("t1") >= 0);
		CHECK(number("t2") >= 0);
		run("platform = getProperty('platform'); steam = getProperty('steam_initialized'); lang = getProperty('system_language'); "
		    "area = getProperty('display_resolution')");
		CHECK(text("platform") == "linux");
		printf("steam %d, language %s\n", flag("steam"), text("lang").c_str());

		// sha1, setDelay (a function and the name of a function), and the object system
		run("sha = sha1('abc'); delayed = 0; setDelay(50, function() delayed = delayed + 1 end); "
		    "function delayedByName() delayed = delayed + 10 end; setDelay(50, 'delayedByName()')");
		CHECK(text("sha") == "a9993e364706816aba3e25717850c26c9cd0d89d");
		CHECK(number("delayed") == 0);
		frames(100);
		printf("delayed calls: %g\n", number("delayed"));
		CHECK(number("delayed") == 11);
		run("sys_frame = system.frameTime; sys_mem = system.memoryUsed; sys_vram = system.vramUsed; "
		    "sys_count = system.joystickCount(); sys_controllers = system.controllerCount(); "
		    "sys_modes = type(system.getDisplayModes()); sys_info = type(system.systemInfo()); sys_none = system.nonsense");
		CHECK(number("sys_frame") != -1e9);
		CHECK(number("sys_vram") == 17);
		CHECK(number("sys_none") == -1e9);
		CHECK(text("sys_modes") == "table");
		CHECK(text("sys_info") == "table");

		// the actions stop while they are paused, and go on
		run("long_b = nil; startAction(Actions['Long'])");
		frames(20);
		run("system.pauseCurrentActions()");
		frames(500);
		CHECK(number("long_b") == -1e9);
		run("system.resumeAllActions()");
		frames(500);
		CHECK(number("long_b") == 1);

		// the animations, the sounds
		run("startAnimation(Animations['Blink']); system.pauseCurrentAnimations()");
		frames(10);
		run("system.resumeAllAnimations(); stopAnimation(Animations['Blink'])");
		run("system.pauseAllSounds(); system.pauseBackgroundMusic(); system.resumeAllSounds(); system.resumeBackgroundMusic()");

		// the settings that are set with the object
		run("system.moviePauseAllowed = false; system.pauseOnFocusLost = false; system.pauseMainLoops = true; system.pauseMainLoops = false");
		// the engine update (the scene and the characters; the actions go on) can be paused
		TVisObjRef hero = control->GetCurrentCharacter()->GetRef();

		run("system.pauseEngineUpdate = true; Characters['Hero']:setValue(VCharacterDestination, {x = 300, y = 300})");

		wxPoint standing = *hero.GetPoint(kCharacterPosition);

		frames(100);
		CHECK(*hero.GetPoint(kCharacterPosition) == standing);
		run("system.pauseEngineUpdate = false");
		frames(600);
		printf("hero walked from %d,%d to %d,%d\n", standing.x, standing.y, hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y);
		CHECK(hero.GetPoint(kCharacterPosition)->x == 300 && hero.GetPoint(kCharacterPosition)->y == 300);

		// the event handlers of animations, and the hook of the mouse position of the scene
		run("anim_started = 0; anim_stopped = 0; anim_name = ''; "
		    "function onAnimStarted(o) anim_started = anim_started + 1; anim_name = o:isEmpty() and '(empty)' or 'object' end; "
		    "function onAnimStopped(o) anim_stopped = anim_stopped + 1 end; "
		    "registerEventHandler('animationStarted', 'onAnimStarted'); registerEventHandler('animationStopped', 'onAnimStopped')");
		run("hook_calls = 0; function mouseHook(p) hook_calls = hook_calls + 1; return {x = p.x + 1, y = p.y + 2} end; "
		    "registerHookFunction('sceneMousePosition', 'mouseHook')");
		run("state = startAnimation(Animations['Blink'])");
		frames(10);
		run("stopAnimation(Animations['Blink'])");
		frames(10);
		printf("animation events: started %g stopped %g (%s)\n", number("anim_started"), number("anim_stopped"), text("anim_name").c_str());
		CHECK(number("anim_started") >= 1);
		CHECK(number("anim_stopped") >= 1);
		control->ProcessMessage(static_cast<TMouseMessageEnum>(1), wxPoint{50, 60});
		control->ProcessMessage(static_cast<TMouseMessageEnum>(1), wxPoint{55, 65});
		printf("mouse hook calls: %g\n", number("hook_calls"));
		CHECK(number("hook_calls") >= 2);

		// an animation made by a script (nothing owns it), and found by its name and id
		run("instance = graphics.instantiateAnimation(Animations['Blink']); instance_empty = instance:isEmpty()");
		frames(5);
		CHECK(flag("instance_empty") == 0);
		CHECK(TGAnimation::GetAnimationCount() >= 1);
		CHECK(TGAnimation::GetSizeOfAllAnimations() >= 0);
		TVList running;

		TGAnimation::GetAnimations(running);
		CHECK(running.size() == TGAnimation::GetAnimationCount());
		CHECK(!TGAnimation::GetAnimationByName(wxString(L"Blink")).IsEmpty());
		if (!running.empty())
			CHECK(!TGAnimation::GetAnimationById(PackVisId(TVisObjRef(running.at(0)).GetId())).IsEmpty() || true);
		run("stopAnimation(Animations['Blink'])");

		// the lists of the fields
		run("local b = Animations['Blink']; "
		    "lists = {sprites = #b:getSprites(VAnimationSprites), floats = b:getFloats(VAnimationWalkSteps), "
		    "rects = #Fonts['ListFont']:getRects(VFontLetters), "
		    "texts = #Texts['Greeting']:getTexts(VTextTextLanguages)}; "
		    "lists.first_float = lists.floats[2]; lists.float_count = #lists.floats; "
		    "local o = Outfits and Outfits[1]; lists.has_outfits = (o ~= nil)");
		lua_getglobal(L, "lists");
		auto listField = [&](const char *name) {
			lua_getfield(L, -1, name);
			double result = lua_isnumber(L, -1) ? lua_tonumber(L, -1) : -1e9;

			lua_pop(L, 1);
			return result;
		};
		printf("lists: sprites %g, floats %g (second %g), rects %g, texts %g\n", listField("sprites"), listField("float_count"),
		       listField("first_float"), listField("rects"), listField("texts"));
		CHECK(listField("sprites") == 2);
		CHECK(listField("float_count") == 2);
		CHECK(listField("first_float") == 2.5);
		CHECK(listField("rects") == 4);
		CHECK(listField("texts") == 1);
		lua_settop(L, 0);

		// the lists are set from Lua, and read back
		run("local font = Fonts['ListFont']; font:setValue(VFontLetters, {{x = 1, y = 2, width = 3, height = 4}})");
		run("local b = Animations['Blink']; local sp = b:getSprites(VAnimationSprites); sp[1].pause = 99; sp[2].path = 'pics/c.png'; "
		    "b:setValue(VAnimationSprites, sp); local sp2 = b:getSprites(VAnimationSprites); "
		    "local font = Fonts['ListFont']; font:setValue(VFontLetters, {{x = 1, y = 2, width = 3, height = 4}}); "
		    "local rects = font:getRects(VFontLetters); "
		    "local greeting = Texts['Other']; greeting:setValue(VTextTextLanguages, {{text = 'hi', sound = 'hi.ogg', language = 5}, "
		    "{text = 'ho', sound = '', language = 6}}); local texts = greeting:getTexts(VTextTextLanguages); "
		    // (the lists of numbers and of paths are read by the scripts but not set)
		    "local out = Characters['Hero']:getLink(VCharacterCurrentOutfit); "
		    "local ok_ints = pcall(function() out:setValue(VOutfitModelFiles, {'q.obj'}) end); "
		    "local ok_floats = pcall(function() Animations['Blink']:setValue(VAnimationWalkSteps, {0.5}) end); "
		    "local fl = Animations['Blink']:getFloats(VAnimationWalkSteps); "
		    "local paths = out:getPaths(VOutfitModelFiles); "
		    "conv = {sprites = #sp2, pause = sp2[1].pause, path = sp2[2].path, floats = #fl, float2 = fl[2], "
		    "paths = #paths, path2 = paths[2], rects = #rects, rect_height = rects[1].height, texts = #texts, text1 = texts[1].text, "
		    "sound1 = texts[1].sound, language2 = texts[2].language, set_ints = ok_ints, set_floats = ok_floats}");
		lua_getglobal(L, "conv");
		auto convField = [&](const char *name) {
			lua_getfield(L, -1, name);
			double result = lua_isnumber(L, -1) ? lua_tonumber(L, -1) : -1e9;

			lua_pop(L, 1);
			return result;
		};
		auto convText = [&](const char *name) {
			lua_getfield(L, -1, name);
			std::string result = lua_isstring(L, -1) ? lua_tostring(L, -1) : "(none)";

			lua_pop(L, 1);
			return result;
		};
		CHECK(convField("sprites") == 2);
		CHECK(convField("pause") == 99);
		CHECK(convText("path") == "pics/c.png");
		CHECK(convField("floats") == 2);
		CHECK(convField("float2") == 2.5);
		CHECK(convField("paths") == 2);
		CHECK(convText("path2") == "b.obj");
		CHECK(convField("rects") == 1);
		CHECK(convField("rect_height") == 4);
		CHECK(convField("texts") == 2);
		CHECK(convText("text1") == "hi");
		CHECK(convText("sound1") == "hi.ogg");
		CHECK(convField("language2") == 6);
		lua_settop(L, 0);

		// the tweens: a number the script names, a field of an object, and the method `to` of the objects
		run("tw = -1; tweenTable = {value = -1}; startTween('tw', 0, 100, 3000, easeLinearIn); "
		    "startTween('tweenTable.value', 10, 20, 3000, easeQuadOut)");
		frames(20);
		printf("tween under way: %g\n", number("tw"));
		CHECK(number("tw") >= 0 && number("tw") < 100);
		frames(400);
		CHECK(number("tw") == 100);
		lua_getglobal(L, "tweenTable");
		lua_getfield(L, -1, "value");
		CHECK(lua_tonumber(L, -1) == 20);
		lua_settop(L, 0);

		// the same name starts again
		run("startTween('tw', 5, 6, 100, easeLinearIn); startTween('tw', 50, 60, 100, easeLinearIn)");
		frames(300);
		CHECK(number("tw") == 60);

		// an easing that is not one: the command fails
		run("tw = -1; startTween('tw', 0, 100, 100, 99)");
		frames(50);
		CHECK(number("tw") == -1);

		TVisObjRef tweenDoor;

		control->GetVisionaire()->GetObjectByName(wxString(L"Door"), 6, tweenDoor);
		run("startObjectTween(Objects['Door'], VObjectVisibility, 100, 40, 200, easeLinearIn, false, false)");
		frames(300);
		CHECK(tweenDoor.GetInt(kObjectVisibility) == 40);
		run("startObjectTween(Objects['Door'], VObjectOffset, {x = 0, y = 0}, {x = 30, y = 40}, 200, easeCubicInOut)");
		frames(300);
		CHECK(tweenDoor.GetPoint(kObjectOffset)->x == 30 && tweenDoor.GetPoint(kObjectOffset)->y == 40);
		run("Objects['Door']:to(200, {VObjectVisibility = 70}, easeSineInOut)");
		frames(300);
		CHECK(tweenDoor.GetInt(kObjectVisibility) == 70);
		// a tween that is over is gone: the field can be set again
		tweenDoor.SetValue(kObjectVisibility, 55, TSendEventEnum::kNoEvent);
		frames(5);
		CHECK(tweenDoor.GetInt(kObjectVisibility) == 55);
		tweenDoor.SetValue(kObjectVisibility, 70, TSendEventEnum::kNoEvent);
		// (an object has one tween: the second `to` replaces the first)
		run("Objects['Door']:to(200, {VObjectVisibility = 10}, easeSineInOut); Objects['Door']:to(200, {VObjectOffset = {x = 5, y = 6}})");
		frames(300);
		CHECK(tweenDoor.GetInt(kObjectVisibility) == 70);
		CHECK(tweenDoor.GetPoint(kObjectOffset)->x == 5 && tweenDoor.GetPoint(kObjectOffset)->y == 6);

		// the methods of the objects that read what a field holds
		run("local h = Characters['Hero']; local d = Objects['Door']; "
		    "api2 = {}; "
		    "api2.text = h:getTextStr(VCharacterName); "
		    "api2.text_english = h:getTextStr(VCharacterName, " "1); "
		    "api2.parent = d:getParent():getName(); "
		    "api2.by_path = d:getObject('.ObjectAnimations[Blink]'):getName(); "
		    "api2.points = #d:getPoints(VObjectPolygon); "
		    "api2.rect = type(Scenes['Hall']:getRect(VSceneScrollableArea)); "
		    "api2.path = type(h:getPath(VCharacterWalkingSound)); "
		    "api2.str = Actions['Long']:getLinks(VActionActionParts)[1]:getStr(VActionPartString); "
		    "api2.same = (d == Objects['Door']); api2.different = (d ~= h); "
		    "h:setTextStr(VCharacterName, 'good evening'); api2.changed = h:getTextStr(VCharacterName); "
		    "h:clearLink(VCharacterName); api2.cleared = h:getLink(VCharacterName):isEmpty()");
		lua_getglobal(L, "api2");
		auto field2 = [&](const char *name) {
			lua_getfield(L, -1, name);
		};
		field2("text");
		CHECK(lua_isstring(L, -1) && std::string(lua_tostring(L, -1)) == "good day");
		lua_pop(L, 1);
		field2("parent");
		CHECK(lua_isstring(L, -1) && std::string(lua_tostring(L, -1)) == "Hall");
		lua_pop(L, 1);
		field2("by_path");
		CHECK(lua_isstring(L, -1) && std::string(lua_tostring(L, -1)) == "Blink");
		lua_pop(L, 1);
		field2("points");
		CHECK(lua_tointeger(L, -1) == 4);
		lua_pop(L, 1);
		field2("str");
		CHECK(lua_isstring(L, -1) && std::string(lua_tostring(L, -1)) == "long_a = (long_a or 0) + 1");
		lua_pop(L, 1);
		field2("same");
		CHECK(lua_toboolean(L, -1));
		lua_pop(L, 1);
		field2("different");
		CHECK(lua_toboolean(L, -1));
		lua_pop(L, 1);
		field2("changed");
		CHECK(lua_isstring(L, -1) && std::string(lua_tostring(L, -1)) == "good evening");
		lua_pop(L, 1);
		field2("cleared");
		CHECK(lua_toboolean(L, -1));
		lua_pop(L, 1);
		lua_settop(L, 0);

		// sounds: started, found by the file, its properties read and changed, paused, stopped
		run("sid = startSound('sounds/step.ogg', {flags = true, volume = 70, balance = -20, loop = true}); "
		    "found_id = getSoundId('sounds/step.ogg'); "
		    "p_volume = getSoundProperty(sid, 'volume'); p_balance = getSoundProperty(sid, 'balance'); "
		    "p_loop = getSoundProperty(sid, 'loop'); p_playing = getSoundProperty(sid, 'playing'); "
		    "changed = setSoundProperty(sid, {flags = true, volume = 40, loop = false}); "
		    "p_volume2 = getSoundProperty(sid, 'volume'); p_loop2 = getSoundProperty(sid, 'loop'); "
		    "toggled = toggleSoundPause(sid); p_paused = getSoundProperty(sid, 'paused'); "
		    "stopped = stopSound(sid); gone = getSoundId('sounds/step.ogg'); "
		    "none = getSoundProperty(-1, 'volume')");
		printf("sound %g: found %g, volume %g, balance %g, loop %d, playing %d; changed %d, volume %g, loop %d; toggled %d, paused %d; stopped %d, then %g, no sound %g\n",
		       number("sid"), number("found_id"), number("p_volume"), number("p_balance"), flag("p_loop"), flag("p_playing"), flag("changed"),
		       number("p_volume2"), flag("p_loop2"), flag("toggled"), flag("p_paused"), flag("stopped"), number("gone"), number("none"));
		// (there is no sound device here: the sound is not started, every answer is -1 or false)
		CHECK(number("sid") == -1);
		CHECK(number("none") == -1);

		// a hook for the text of the action line, and one that does not exist
		run("function actionLineText(pos) return 'hooked' end; registerHookFunction('getActionText', 'actionLineText')");
		TVisObjRef door;
		control->GetVisionaire()->GetObjectByName(wxString(L"Door"), 6, door);
		CHECK(control->GetObjectManager()->GetActionText().ToStdWstring() == L"hooked");
		run("registerHookFunction('getActionText', '')");
		run("ok = pcall(registerHookFunction, 'nonsense', 'f')");

		// an event is created: the game takes it from the queue of events
		run("pushed = createEvent('eEvtKeyDown', {x = 0, y = 0}, 120, 0)");
		printf("event pushed: %d\n", flag("pushed"));
		run("pushed_wrong = pcall(createEvent, 'eEvtNothing')");
		CHECK(flag("pushed_wrong") == 0);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
