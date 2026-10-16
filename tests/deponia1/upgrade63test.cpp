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

// The first upgrades (version 0x63 and the ones after it): the fields that old files do not have get their values, the
// commands that were renumbered get their new numbers, and the links that changed their order are swapped.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	// the record types of the old files
	TVisionaireGame first;

	first.InitWithVersion(0x63);

	TVisionaireGame game;
	TVisObjRef gameRef = game.GetGame();

	gameRef.SetValue(kGameWindowResolution, wxPoint{640, 480}, Event::kNoEvent);
	gameRef.SetValue(kGameAbout, wxString(L"one<br>two<br>"), Event::kNoEvent);

	TVisObjRef scene = game.CreateObject(4, gameRef, kGameSceneLinks);
	TVisObjRef hero = game.CreateObject(0, gameRef, kGameCharacterLinks);

	gameRef.SetLink(kGameFirstCharacter, hero, true);
	TVisObjRef object = game.CreateObject(6, scene, kSceneObjects);

	// a button of the old kind: type, action and the "own character"
	TVisObjRef interface = game.CreateObject(1, hero, kCharacterInterfaces);
	TVisObjRef button = game.CreateObject(2, interface, kInterfaceButtons);

	button.SetValue(kButtonType, 3, Event::kNoEvent);
	button.SetValue(kButtonAction, 5, Event::kNoEvent);
	button.SetValue(kButtonUseOnOwnCharacter, true, Event::kNoEvent);

	// a dialog part
	TVisObjRef dialog = game.CreateObject(11, hero, kCharacterDialogs);
	TVisObjRef dialogPart = game.CreateObject(12, dialog, kDialogDialogParts);

	dialogPart.SetValue(kDialogPartConditionValue, true, Event::kNoEvent);

	// the action parts: a command that is renumbered, ones that swap their links, one that moves a link
	TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
	TVisObjRef target = game.CreateObject(6, scene, kSceneObjects);
	TVisObjRef other = game.CreateObject(6, scene, kSceneObjects);
	auto addPart = [&](int command) {
		TVisObjRef part = game.CreateObject(8, action, kActionActionParts);

		part.SetValue(kActionPartCommand, command, Event::kNoEvent);
		return part;
	};
	TVisObjRef renumbered = addPart(0x0B);
	TVisObjRef swapped = addPart(0x32);

	swapped.SetLink(kActionPartLink, target, false);
	swapped.SetLink(kActionPartAltLink, other, false);
	TVisObjRef moved = addPart(0x50);

	moved.SetLink(kActionPartAltLink, target, false);
	TVisObjRef interfacePart = addPart(1);
	TVisObjRef buttonsPart = addPart(4);
	TVisObjRef gone = addPart(0x61);
	TVisObjRef gone2 = addPart(0x60);
	TVisObjRef doubled = addPart(0x77);
	TVisObjRef bird = addPart(0x29);

	// an item with two actions, and a dialog part with an action of its own
	object.SetValue(kObjectIsItem, true, Event::kNoEvent);

	TVisObjRef itemAction6 = game.CreateObject(7, object, kObjectActions);
	TVisObjRef itemAction3 = game.CreateObject(7, object, kObjectActions);

	itemAction6.SetValue(kActionExecutionType, 6, Event::kNoEvent);
	itemAction3.SetValue(kActionExecutionType, 3, Event::kNoEvent);

	TVisObjRef partAction = game.CreateObject(7, gameRef, kGameActions);

	dialogPart.SetName(TCharHolder("Part"));
	dialogPart.SetLink(kDialogPartAction, partAction, false);
	TVisObjRef setInterface = addPart(0x2D);

	setInterface.SetValue(kActionPartInt, 7, Event::kNoEvent);

	// a font: the space is as wide as the "a"
	TVisObjRef font = game.CreateObject(3, gameRef, kGameFontLinks);
	std::vector<wxRect> letters;

	for (int i = 0; i < 3; i++) {
		wxRect r;

		r.x = i * 10;
		r.y = 0;
		r.width = 7 + i;
		r.height = 12;
		letters.push_back(r);
	}
	font.SetValue(kFontLetters, letters, Event::kNoEvent);
	font.SetValue(kFontAlphabet, wxString(L"bca"), Event::kNoEvent);

	// a text with line breaks and spaces at its end
	TVisObjRef text = game.CreateObject(14, scene, kSceneTexts);
	TVisObjRef language = game.CreateObject(19, text, kTextAll);

	language.SetValue(kTextLanguageText, wxString(L"first<br>second  \n "), Event::kNoEvent);

	// a cursor with an animation
	TVisObjRef cursor = game.CreateObject(15, gameRef, kGameCursors);

	cursor.SetValue(kCursorHotspot, wxPoint{4, 5}, Event::kNoEvent);

	CHECK(game.UpdateVersion(0x63, false));

	CHECK(hero.GetInt(kCharacterVisibility) == 100);
	CHECK(hero.GetInt(kCharacterDestVisibility) == 100);
	CHECK(object.GetInt(kObjectVisibility) == 100);
	CHECK(scene.GetInt(kSceneBrightness) == 100);

	CHECK(button.GetInt(kButtonCommandType) == 3);
	CHECK(button.GetInt(kButtonType) == 5);
	CHECK(button.GetBool(kButtonUseOnCurrentCharacter));

	CHECK(dialogPart.GetBool(kDialogPartConditionNegate) == false);

	CHECK(renumbered.GetInt(kActionPartCommand) == 0x3B);
	CHECK(swapped.GetLink(kActionPartLink) == other);
	CHECK(swapped.GetLink(kActionPartAltLink) == target);
	CHECK(moved.GetLink(kActionPartLink) == target);
	CHECK(moved.GetLink(kActionPartAltLink).IsEmpty());

	// (the fixes of the later versions go on with the commands: 1 and 4 become 0x8B and 0x8C; 0x61 becomes 0x1F with the
	// int cleared; 0x60 becomes 0x2F; 0x77 becomes 0x1F with int 1 but 0x1F was swapped before; 0x29 becomes 0x8D)
	CHECK(interfacePart.GetInt(kActionPartCommand) == 0x8B);
	CHECK(interfacePart.GetInt(kActionPartInt) == 0);  // (turned round again by the fix of 0x6C)
	CHECK(buttonsPart.GetInt(kActionPartCommand) == 0x8C);
	CHECK(buttonsPart.GetInt(kActionPartInt) == 1);
	CHECK(gone.GetInt(kActionPartCommand) == 0x1F);
	CHECK(gone2.GetInt(kActionPartCommand) == 0x2F);
	CHECK(doubled.GetInt(kActionPartCommand) == 0x1F);
	CHECK(doubled.GetInt(kActionPartInt) == 1);
	CHECK(bird.GetInt(kActionPartCommand) == 0x8D);
	CHECK(bird.GetInt(kActionPartInt) == 0);  // (1 from 0x68, turned round by 0x69)

	// the actions of items have the new execution types (0x11 and 0x14; later fixes may go on with them)
	CHECK(itemAction6.GetInt(kActionExecutionType) != 6);
	CHECK(itemAction3.GetInt(kActionExecutionType) != 3);

	// the action of the dialog part is a linked action of its own, named like the part
	CHECK(dialogPart.GetLink(kDialogPartAction).IsEmpty());
	CHECK(dialogPart.GetLink(kDialogPartLinkedAction) == partAction);
	CHECK(partAction.GetName().GetFullPath().ToStdWstring() == L"Part");  // (the part itself is named by where it is, by 0x6B)

	// the fonts are no TrueType, and the 0x2D commands lose their int
	CHECK(!font.GetBool(kFontTrueTypeFont));
	CHECK(font.GetFloat(kFontSize) == 12.0f);
	CHECK(setInterface.GetInt(kActionPartInt) == 0);

	// the space is as wide as the "a" (the third letter, 9 wide)
	CHECK(font.GetInt(kFontSpaceWidth) == 9);

	CHECK(gameRef.GetStr(kGameAbout).ToStdWstring() == L"one<br/>two<br/>");
	// (the languages of the text are made a list by a later fix, and the objects go)
	std::vector<TTextLanguage> texts;

	text.GetTextsCopy(kTextTextLanguages, texts);
	CHECK(texts.size() == 1);
	CHECK(texts.size() == 1 && wxString(texts[0].text.GetFullPath()).ToStdWstring() == L"first<br/>second");

	CHECK(gameRef.GetBool(kGameKeepCharacterSpritesDuringMenus));
	CHECK(gameRef.GetBool(kGameAutoHideInterfacesInMenu));
	// (the interface of the character is the game's now, a copy of the one made)
	TVList interfaces;

	hero.GetLinks(kCharacterInterfaces, TypeOrder::kValue1, interfaces);
	CHECK(interfaces.size() == 1);
	CHECK(interfaces.size() == 1 && TVisObjRef(interfaces.at(0)).GetBool(kInterfaceVisible));

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
