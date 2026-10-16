#include <cstdio>

#include "AppGlobals.h"
#include "datastruct/table.h"
#include "datastruct/visionaireobject.h"
#include "vstables/fieldIds.h"
#include "vstables/records.h"
#include "vstables/visionaireGame.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

typedef TSendEventEnum Event;

// The upgrade of a file of version 0x71: the interfaces of the characters become the game's, each with a class, and
// what pointed at the old interfaces (and their buttons) points at the copies.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	// the record types of the old files (the types are shared: games made after this one have them)
	TVisionaireGame first_;

	first_.InitWithVersion(0x70);

	TVisionaireGame game;
	TVisObjRef gameRef = game.GetGame();
	gameRef.SetValue(kGameWindowResolution, wxPoint{640, 480}, Event::kNoEvent);

	TVisObjRef hero = game.CreateObject(0, gameRef, kGameCharacterLinks);
	hero.SetName(TCharHolder("Hero"));
	gameRef.SetLink(kGameFirstCharacter, hero, true);

	// three interfaces of the hero; the first has two buttons
	TVisObjRef first = game.CreateObject(1, hero, kCharacterInterfaces);
	TVisObjRef second = game.CreateObject(1, hero, kCharacterInterfaces);
	TVisObjRef third = game.CreateObject(1, hero, kCharacterInterfaces);
	TVisObjRef buttonA = game.CreateObject(2, first, kInterfaceButtons);
	TVisObjRef buttonB = game.CreateObject(2, first, kInterfaceButtons);

	// a condition and a value of the first interface (the second condition is there to make the place count)
	TVisObjRef conditionA = game.CreateObject(10, first, kInterfaceConditions);
	TVisObjRef conditionB = game.CreateObject(10, first, kInterfaceConditions);
	TVisObjRef value = game.CreateObject(20, first, kInterfaceValues);

	// the buttons are in a group
	TVList group;
	group.push_back(buttonA);
	group.push_back(buttonB);
	buttonA.SetValue(kButtonGroup, group, false);

	auto addPart = [&](TVisObjRef action, int command, int value, int altValue, int altValue2) {
		TVisObjRef part = game.CreateObject(8, action, kActionActionParts);
		part.SetValue(kActionPartCommand, command, Event::kNoEvent);
		part.SetValue(kActionPartInt, value, Event::kNoEvent);
		part.SetValue(kActionPartAltInt, altValue, Event::kNoEvent);
		part.SetValue(kActionPartAltInt2, altValue2, Event::kNoEvent);
		return part;
	};
	TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);

	// a part that shows the first interface, one that shows the place 2 (the inventory), one with the place 7
	TVisObjRef showFirst = addPart(action, 0x70, 0, 5, 6);
	showFirst.SetLink(kActionPartLink, first, false);
	TVisObjRef showSecondPlace = addPart(action, 0x70, 2, 8, 9);
	TVisObjRef showMisc = addPart(action, 0x38, 7, 1, 2);
	TVisObjRef showThird = addPart(action, 0x38, 0, 3, 4);
	showThird.SetLink(kActionPartLink, third, false);
	// one that hides (0x5A)
	TVisObjRef hide = addPart(action, 0x5A, 4, 0, 0);
	hide.SetLink(kActionPartAltLink, first, false);
	// a part that points at the second button of the first interface
	TVisObjRef pointAtButton = addPart(action, 0x2D, 0, 0, 0);
	pointAtButton.SetLink(kActionPartLink, buttonB, false);
	// an action that is the command of a button
	TVisObjRef buttonAction = game.CreateObject(7, gameRef, kGameActions);
	buttonAction.SetLink(kActionCommand, buttonA, false);

	// an object, a button and an action part that depend on the conditions and the value
	TVisObjRef scene = game.CreateObject(4, gameRef, kGameSceneLinks);
	TVisObjRef gameObject = game.CreateObject(6, scene, kSceneObjects);
	gameObject.SetLink(kObjectCondition, conditionB, false);
	buttonB.SetLink(kButtonCondition, conditionA, false);
	TVisObjRef ifPart = addPart(action, 0x11, 0, 0, 0);
	ifPart.SetLink(kActionPartLink, conditionA, false);
	ifPart.SetLink(kActionPartAltLink, value, false);

	CHECK(game.UpdateVersion(0x71, false));

	// the hero has three interfaces, none of the old ones
	TVList interfaces;

	hero.GetLinks(kCharacterInterfaces, TypeOrder::kValue1, interfaces);
	CHECK(interfaces.size() == 3);
	CHECK(interfaces.at(0)->GetId()[3] == 1);

	TVisObjRef copyFirst(interfaces.at(0));
	TVisObjRef copySecond(interfaces.at(1));
	TVisObjRef copyThird(interfaces.at(2));

	CHECK(!(copyFirst == first));
	CHECK(copyFirst.GetParent().GetId()[3] != 0);

	// the classes: main, inventory and miscellaneous (a game made in memory has its own five already; one loaded from an
	// old file has none)
	TVList classes;

	game.GetList(31, classes, false);
	CHECK(classes.size() == 10);

	TVisObjRef mainClass = copyFirst.GetLink(kInterfaceInterfaceClass);
	TVisObjRef inventoryClass = copySecond.GetLink(kInterfaceInterfaceClass);
	TVisObjRef miscClass = copyThird.GetLink(kInterfaceInterfaceClass);

	CHECK(mainClass.GetStr(kInterfaceClassName).ToStdWstring() == L"MainInterface");
	CHECK(inventoryClass.GetStr(kInterfaceClassName).ToStdWstring() == L"Inventory");
	CHECK(miscClass.GetStr(kInterfaceClassName).ToStdWstring() == L"Miscellaneous");

	// what pointed at the buttons points at the copies of them
	TVList copyButtons;

	copyFirst.GetLinks(kInterfaceButtons, TypeOrder::kValue1, copyButtons);
	CHECK(copyButtons.size() == 2);

	TVisObjRef copyButtonA(copyButtons.at(0));
	TVisObjRef copyButtonB(copyButtons.at(1));

	CHECK(pointAtButton.GetLink(kActionPartLink) == copyButtonB);
	CHECK(buttonAction.GetLink(kActionCommand) == copyButtonA);

	// the conditions and the value
	TVList copyConditions;
	TVList copyValues;

	copyFirst.GetLinks(kInterfaceConditions, TypeOrder::kValue1, copyConditions);
	copyFirst.GetLinks(kInterfaceValues, TypeOrder::kValue1, copyValues);
	CHECK(copyConditions.size() == 2);
	CHECK(copyValues.size() == 1);
	CHECK(copyConditions.size() == 2 && gameObject.GetLink(kObjectCondition) == *copyConditions.at(1));
	CHECK(copyConditions.size() == 2 && copyButtonB.GetLink(kButtonCondition) == *copyConditions.at(0));
	CHECK(copyConditions.size() == 2 && ifPart.GetLink(kActionPartLink) == *copyConditions.at(0));
	CHECK(copyValues.size() == 1 && ifPart.GetLink(kActionPartAltLink) == *copyValues.at(0));

	// the group
	TVList copyGroup;

	copyButtonA.GetLinks(kButtonGroup, TypeOrder::kValue1, copyGroup);
	CHECK(copyGroup.size() == 2);
	CHECK(copyGroup.size() == 2 && copyButtonA == *copyGroup.at(0));
	CHECK(copyGroup.size() == 2 && copyButtonB == *copyGroup.at(1));

	// the parts that show an interface name a class, and the ints have moved
	CHECK(showFirst.GetLink(kActionPartLink) == mainClass);
	CHECK(showFirst.GetInt(kActionPartInt) == 5);
	CHECK(showFirst.GetInt(kActionPartAltInt) == 6);
	CHECK(showFirst.GetInt(kActionPartAltInt2) == 0);
	CHECK(showSecondPlace.GetLink(kActionPartLink) == inventoryClass);
	CHECK(showSecondPlace.GetInt(kActionPartInt) == 8);
	CHECK(showMisc.GetLink(kActionPartLink) == miscClass);
	CHECK(showMisc.GetInt(kActionPartInt) == 1);
	CHECK(showMisc.GetInt(kActionPartAltInt) == 0);
	CHECK(showThird.GetLink(kActionPartLink) == miscClass);
	CHECK(hide.GetLink(kActionPartAltLink).IsEmpty());
	CHECK(hide.GetInt(kActionPartInt) == 0);

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
