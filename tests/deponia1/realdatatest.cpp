#include <cstdio>
#include <cstdlib>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "graphicslib/graphics.h"
#include "vscommon/scripting/visLua.h"
#include "vscommon/scripting/command.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"
#include "datastruct/table.h"
#include "datastruct/visionaireobject.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s" "\n", __LINE__, #c); failures++; } } while (0)

class NullGraphics : public TGraphicsInterface {
};

// The real game data (Deponia 1): the path of the game directory (with data.vis) is the argument or REALDATA_DIR; the
// program is run from there.
int main(int argc, char **argv) {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 3;

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"data.vis");
	wxString warning;

	bool pre = control->PreLoad(file, warning, true);
	printf("PreLoad %d\n", (int)pre);
	CHECK(pre);
	InitPlayerCommands(control->GetVisionaire(), wxString(L"."), wxString(L"."), wxString(L"."));
	wxString language;
	bool loaded = control->LoadAndInitGame(file, warning, language, true);
	printf("LoadAndInitGame %d, language '%ls'\n", (int)loaded, language.c_str());
	CHECK(loaded);

	if (loaded) {
		TVisionaire *v = control->GetVisionaire();
		TVisObjRef game = v->GetGame();

		printf("game name '%ls', window %dx%d\n", game.GetStr(kGameGameName).c_str(), game.GetPoint(kGameWindowResolution)->x, game.GetPoint(kGameWindowResolution)->y);
	}
	if (loaded) {
		struct Probe {
			int id;
			int line;
		} probes[] = {{134, 535}, {201, 49}};

		for (const Probe &probe : probes) {
			TVisionaireObject *script = control->GetVisionaire()->GetObjectById(TId(probe.id, 30));

			if (!script)
				continue;

			std::wstring text = script->GetStr(kScriptScript).ToStdWstring();
			size_t pos = 0;

			for (int n = 1; n < probe.line; n++) {
				size_t next = text.find(L"<br/>", pos);

				if (next == std::wstring::npos)
					break;
				pos = next + 5;
			}
			printf("script %d line %d: %ls", probe.id, probe.line, text.substr(pos, 120).c_str());
			putchar(10);
		}
	}
	(void)argc;
	(void)argv;
	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
