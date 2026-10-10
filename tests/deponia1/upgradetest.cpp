#include <cstdio>

#include "AppGlobals.h"
#include "TTText.h"
#include "datastruct/table.h"
#include "vstables/eCommand.h"
#include "vstables/fieldIds.h"
#include "vstables/records.h"
#include "vstables/visionaireGame.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

typedef TSendEventEnum Event;

int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	// the record types of the old files (the types are shared: games made after this one have them)
	TVisionaireGame first;

	first.InitWithVersion(0x64);

	TVisionaireGame game;
	TVisObjRef gameRef = game.GetGame();
	printf("probe: %d\n", gameRef.GetInt(kGameLeftMouseClick));
	gameRef.SetValue(kGameWindowResolution, wxPoint{640, 480}, Event::kNoEvent);

	TVisObjRef scene = game.CreateObject(4, gameRef, kGameSceneLinks);
	scene.SetName(TCharHolder("Hall"));
	TVisObjRef hero = game.CreateObject(0, gameRef, kGameCharacterLinks);
	hero.SetName(TCharHolder("Hero"));
	gameRef.SetLink(kGameFirstCharacter, hero, true);
	TVisObjRef outfit = game.CreateObject(17, hero, kCharacterOutfits);
	TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
	action.SetName(TCharHolder("Act"));
	action.SetValue(kActionExecutionType, 0x112, Event::kNoEvent);
	TVisObjRef other = game.CreateObject(7, gameRef, kGameActions);
	other.SetValue(kActionExecutionType, 0x11A, Event::kNoEvent);
	TVisObjRef idle = game.CreateObject(7, gameRef, kGameActions);
	idle.SetValue(kActionExecutionType, 5, Event::kNoEvent);

	// IF_CURRENT_OBJECT parts
	auto addPart = [&](int command, int value, int alt) {
		TVisObjRef part = game.CreateObject(8, action, kActionActionParts);
		part.SetValue(kActionPartCommand, command, Event::kNoEvent);
		part.SetValue(kActionPartInt, value, Event::kNoEvent);
		part.SetValue(kActionPartAltInt, alt, Event::kNoEvent);
		return part;
	};
	TVisObjRef p0 = addPart(0x67, 0, 2);
	TVisObjRef p1 = addPart(0x67, 1, 9);
	TVisObjRef p2 = addPart(0x67, 2, 2);
	TVisObjRef p3 = addPart(0x67, 2, 7);
	TVisObjRef p4 = addPart(0x42, 0, 2);
	TVisObjRef p5 = addPart(0x7B, 3, 0);

	// a text with its languages as objects (the old way)
	TVisObjRef text = game.CreateObject(14, scene, kSceneTexts);
	(void)text;

	// two points that are related to each other twice
	TVisObjRef pointA = game.CreateObject(5, scene, kScenePoints);
	TVisObjRef pointB = game.CreateObject(5, scene, kScenePoints);
	(void)pointA;
	(void)pointB;

	printf("start\n");
	CHECK(game.UpdateVersion(0x64, false));
	CHECK(p0.GetInt(kActionPartInt) == 3);
	CHECK(p0.GetInt(kActionPartAltInt) == 0);
	CHECK(p1.GetInt(kActionPartInt) == 0);
	CHECK(p2.GetInt(kActionPartInt) == -1);
	CHECK(p3.GetInt(kActionPartInt) == 1);
	CHECK(p4.GetInt(kActionPartAltInt) == 3);
	CHECK(p5.GetInt(kActionPartInt) >= 3);
	CHECK(action.GetInt(kActionExecutionType) == 0x40000051);
	CHECK(other.GetInt(kActionExecutionType) == 0x11A + 0x3FFFFF20);
	CHECK(idle.GetInt(kActionExecutionType) == 5);
	CHECK(gameRef.GetInt(kGamePreloadPicThreads) == 3);
	CHECK(gameRef.GetInt(kGamePictureCacheSize) == 40);
	CHECK(gameRef.GetBool(kGameSmoothScrolling));
	CHECK(outfit.GetInt(kOutfitRandomMinTime) == 10000);
	CHECK(outfit.GetInt(kOutfitModelShadowVisibility) == 100 || outfit.GetInt(kOutfitModelShadowVisibility) == -1);
	CHECK(outfit.GetInt(kOutfitModelShadow) == 2);
	CHECK(outfit.GetBool(kOutfitSlideWalkAnimation));

	// the current version changes nothing
	const int before = gameRef.GetInt(kGamePreloadPicThreads);
	gameRef.SetValue(kGamePreloadPicThreads, 9, Event::kNoEvent);
	CHECK(game.UpdateVersion(0xBA, false));
	CHECK(gameRef.GetInt(kGamePreloadPicThreads) == 9);
	(void)before;

	// a version too old is refused
	CHECK(!game.UpdateVersion(0x60, false));

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
