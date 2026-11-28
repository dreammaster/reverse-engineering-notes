#include <cstdio>
#include <cstdlib>
#include <list>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TTScene.h"
#include "graphicslib/graphics.h"
#include "vstables/records.h"
#include "vstables/visionaireGame.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/visLua.h"

class NullGraphics : public TGraphicsInterface {
};

// The lists of the console (scenes, characters, conditions, values) of the real data: the first lines of each (run from the game directory; GAME_PASSW).
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 0;

	graphics = new NullGraphics();
	if (getenv("GAME_PASSW"))
		passw = wxString(getenv("GAME_PASSW"));
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"data.vis");
	wxString warning;

	if (!control->PreLoad(file, warning, true)) {
		std::printf("not preloaded\n");
		return 1;
	}
	InitPlayerCommands(control->GetVisionaire(), wxString(L"."), wxString(L"."), wxString(L"."));
	wxString language;
	if (!control->LoadAndInitGame(file, warning, language, true)) {
		std::printf("not loaded\n");
		return 1;
	}

	TVisionaireGame &game = *control->GetVisionaire();
	std::list<wxString> lines;

	TTScene::PrintScenes(lines, game);
	TTCharacter::PrintCharacters(lines, game);
	TTCondition::PrintConditions(lines, game);
	TTValue::PrintValues(lines, game);

	int count = 0;

	std::printf("%d lines\n", static_cast<int>(lines.size()));

	for (const wxString &line : lines) {
		if (count++ < 14 || line.ToStdWstring().find(L"All ") == 0)
			std::printf("%ls\n", line.wc_str());
	}
	return 0;
}
