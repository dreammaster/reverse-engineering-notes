#include <cstdio>

#include "AppGlobals.h"
#include "TCharHolder.h"
#include "datastruct/table.h"
#include "vstables/fieldIds.h"
#include "vstables/records.h"
#include "vstables/visionaireGame.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

typedef TSendEventEnum Event;

// The upgrade of a file of version 0xB3: the rules of the editor's build for the containers of the game.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	TVisionaireGame game;
	TVisObjRef gameRef = game.GetGame();

	gameRef.SetValue(kGameWindowResolution, wxPoint{640, 480}, Event::kNoEvent);
	gameRef.SetValue(kGameSceneComposedFiles, 3, Event::kNoEvent);
	gameRef.SetValue(kGameSceneComposedFile, wxFileName(L"scenes"), Event::kNoEvent);
	gameRef.SetValue(kGameCharacterComposedFiles, 1, Event::kNoEvent);
	gameRef.SetValue(kGameCharacterComposedFile, wxFileName(L"characters"), Event::kNoEvent);
	gameRef.SetValue(kGameMovieComposedFiles, 2, Event::kNoEvent);
	gameRef.SetValue(kGameMovieComposedFile, wxFileName(L"movies"), Event::kNoEvent);
	gameRef.SetValue(kGameInterfaceComposedFiles, 0, Event::kNoEvent);
	gameRef.SetValue(kGameGameComposedFiles, 5, Event::kNoEvent);
	gameRef.SetValue(kGameGameComposedFile, wxFileName(L"game"), Event::kNoEvent);

	CHECK(game.UpdateVersion(0xB3, false));

	std::vector<TCharHolder> rules;

	gameRef.GetStrings(kGameBuildRules, rules);

	// the movies, the scenes and the game: more than one container; the characters and the interfaces: one or none
	CHECK(rules.size() == 4);

	if (rules.size() == 4) {
		CHECK(rules[0].GetFullPath().ToStdWstring() == L"\\eActionParts(VActionPartCommand=66);movies.vv<3c>;copy");
		CHECK(rules[1].GetFullPath().ToStdWstring() == L"\\eScenes;scenes.vs<3i>");
		CHECK(rules[2].GetFullPath().ToStdWstring() == L"\\eObjects;game.vi<3i>");
		CHECK(rules[3].GetFullPath().ToStdWstring() == L"\\eGame;<Main.vis>");
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
