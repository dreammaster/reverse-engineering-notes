// The lists that the console prints (Deponia 1's `TTScene::PrintScenes`, `TTCharacter::PrintCharacters`, `TTCondition::PrintConditions`,
// `TTValue::PrintValues`, asm 1448630, 1475646, 1450703 and 1476802): each makes the lines of one table, by name.
#include <algorithm>
#include <cstdarg>
#include <cwchar>
#include <list>

#include "TTScene.h"
#include "datastruct/visionaireobject.h"
#include "vstables/fieldIds.h"
#include "vstables/records.h"
#include "vstables/visionaireGame.h"

// Confirmed (asm 667c80, `cmpName`): the order of the lists, by the names of the objects.
static bool cmpName(const TVisionaireObject *left, const TVisionaireObject *right) {
	return left->GetName().Cmp(right->GetName()) < 0;
}

static const wchar_t *trueOrFalse(bool value) {
	return value ? L"T" : L"F";
}

static wxString format(const wchar_t *text, ...);

static wxString formatV(const wchar_t *text, va_list arguments) {
	wchar_t buffer[1024];

	std::vswprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), text, arguments);
	return wxString(buffer);
}

static wxString format(const wchar_t *text, ...) {
	va_list arguments;

	va_start(arguments, text);

	wxString result = formatV(text, arguments);

	va_end(arguments);
	return result;
}

// Confirmed (asm 1448630-1449430): table 4. A line of the scene (name, id, menu or playable scene, the brightness in percent), then one of its
// music (the file, the volume, the balance), then an empty one.
void TTScene::PrintScenes(std::list<wxString> &lines, TVisionaireGame &game) {
	TVList scenes;

	game.GetList(4, scenes, false);

	if (scenes.size() == 0) {
		lines.push_back(wxString(L"This Game Has No Scenes"));
		return;
	}

	lines.push_back(wxString(L"All Scenes:"));
	std::sort(scenes.begin(), scenes.end(), cmpName);

	for (TVisionaireObject *object : scenes) {
		TVisObjRef scene(object);
		const wchar_t *kind = scene.GetBool(kSceneIsMenu) ? L"Menu" : L"Playable Scene";

		lines.push_back(format(L"Scene: \"%ls\" (Id: %d), %ls, Brightness: %d%%", scene.GetName().c_str().c_str(), PackVisId(scene.GetId()), kind,
		                       scene.GetInt(kSceneBrightness)));
		lines.push_back(format(L"Background Music: %ls, Volume: %d, Balance: %d", scene.GetPath(kSceneBackgroundMusic).GetFullPath().wc_str(),
		                       scene.GetInt(kSceneMusicVolume), scene.GetInt(kSceneMusicBalance)));
		lines.push_back(wxString());
	}
}

// Confirmed (asm 1475646-1476457): table 0. The line of the character (name, id, the scene and its id), the line of whether it is active (T or F)
// and its visibility, an empty one. (An empty table says "This Game Has No Scenes": the message of the scenes is used.)
void TTCharacter::PrintCharacters(std::list<wxString> &lines, TVisionaireGame &game) {
	TVList characters;

	game.GetList(0, characters, false);

	if (characters.size() == 0) {
		lines.push_back(wxString(L"This Game Has No Scenes"));
		return;
	}

	lines.push_back(wxString(L"All Characters:"));
	std::sort(characters.begin(), characters.end(), cmpName);

	for (TVisionaireObject *object : characters) {
		TVisObjRef character(*object);
		TVisObjRef scene = character.GetLink(kCharacterScene);

		lines.push_back(format(L"Character: \"%ls\" (Id: %d), Current Scene: \"%ls\" (Id: %d)", character.GetName().c_str().c_str(),
		                       PackVisId(character.GetId()), scene.GetName().c_str().c_str(), PackVisId(scene.GetId())));
		lines.push_back(format(L"Active: %ls, Visibility: %d%%", trueOrFalse(character.GetBool(kCharacterActive)),
		                       character.GetInt(kCharacterVisibility)));
		lines.push_back(wxString());
	}
}

// Confirmed (asm 1450703-1451235): table 10, a line for each condition with its value (T or F).
void TTCondition::PrintConditions(std::list<wxString> &lines, TVisionaireGame &game) {
	TVList conditions;

	game.GetList(10, conditions, false);

	if (conditions.size() == 0) {
		lines.push_back(wxString(L"This Game Has No Conditions"));
		return;
	}

	lines.push_back(wxString(L"All Conditions:"));
	std::sort(conditions.begin(), conditions.end(), cmpName);

	for (TVisionaireObject *object : conditions) {
		TVisObjRef condition(object);

		lines.push_back(format(L"Condition: \"%ls\" (Id: %d), Value: %ls", condition.GetName().c_str().c_str(), PackVisId(condition.GetId()),
		                       trueOrFalse(condition.GetBool(kConditionValue))));
	}
}

// Confirmed (asm 1476802-1477353): table 20, a line for each value with its number.
void TTValue::PrintValues(std::list<wxString> &lines, TVisionaireGame &game) {
	TVList values;

	game.GetList(20, values, false);

	if (values.size() == 0) {
		lines.push_back(wxString(L"This Game Has No Values"));
		return;
	}

	lines.push_back(wxString(L"All Values:"));
	std::sort(values.begin(), values.end(), cmpName);

	for (TVisionaireObject *object : values) {
		TVisObjRef value(object);

		lines.push_back(format(L"Value: \"%ls\" (Id: %d), Value: %d", value.GetName().c_str().c_str(), PackVisId(value.GetId()),
		                       value.GetInt(kValueInt)));
	}
}
