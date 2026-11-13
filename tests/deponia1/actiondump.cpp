#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "graphicslib/graphics.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"
#include "datastruct/visionaireobject.h"
#include "datastruct/vlist.h"
#include "vstables/records.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/visLua.h"

class NullGraphics : public TGraphicsInterface {
};

// The parts of the actions given by their ids (table 7), as `actiondump <id> ...`; run from the game directory (GAME_PASSW).
// Given a name instead of a number it lists the actions of that name (table 7).
static void dumpAction(TVisionaireGame *game, TVisionaireObject *action, int depth);

static void dumpPart(TVisionaireGame *game, TVisionaireObject *part, int depth) {
	TVisObjRef partRef(part);
	TVisObjRef link = partRef.GetLink(kActionPartLink);
	TVisObjRef altLink = partRef.GetLink(kActionPartAltLink);
	std::vector<TCharHolder> strings;

	partRef.GetStrings(kActionPartString, strings);

	std::printf("%*s part %d: command %d int %d alt %d alt2 %d str '%ls' link %ls(%d) alt-link %ls(%d)\n", depth * 2, "", part->GetId24(),
	            part->GetInt(kActionPartCommand), part->GetInt(kActionPartInt), part->GetInt(kActionPartAltInt),
	            part->GetInt(kActionPartAltInt2), strings.empty() ? L"" : strings[0].c_str().c_str(),
	            link.IsEmpty() ? L"-" : link.GetName().c_str().c_str(), link.IsEmpty() ? 0 : link.GetObjectPointer()->GetId24(),
	            altLink.IsEmpty() ? L"-" : altLink.GetName().c_str().c_str(), altLink.IsEmpty() ? 0 : altLink.GetObjectPointer()->GetId24());
	(void)game;
}

static void dumpAction(TVisionaireGame *game, TVisionaireObject *action, int depth) {
	TVisObjRef owner = TVisObjRef(action).GetParent();

	std::printf("%*saction '%ls' id %d (of %ls)\n", depth * 2, "", action->GetName().c_str().c_str(), action->GetId24(),
	            owner.IsEmpty() ? L"-" : owner.GetName().c_str().c_str());

	TVList parts;

	TVisObjRef(action).GetList(kActionActionParts, parts);

	for (TVisionaireObject *part : parts)
		dumpPart(game, part, depth + 1);
}

int main(int argc, char **argv) {
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

	TVisionaireGame *game = control->GetVisionaire();
	TVList actions;

	game->GetList(7, actions, false);

	for (int i = 1; i < argc; i++) {
		int id = std::atoi(argv[i]);

		// `cmd:<n>` lists the parts that have the command n, with their action and the owner of it
		if (std::strncmp(argv[i], "cmd:", 4) == 0) {
			int command = std::atoi(argv[i] + 4);

			for (TVisionaireObject *action : actions) {
				TVList parts;

				TVisObjRef(action).GetList(kActionActionParts, parts);

				for (TVisionaireObject *part : parts) {
					if (part->GetInt(kActionPartCommand) != command)
						continue;

					TVisObjRef owner = TVisObjRef(action).GetParent();

					std::printf("action '%ls' id %d (of %ls): part %d int %d alt %d\n", action->GetName().c_str().c_str(), action->GetId24(),
					            owner.IsEmpty() ? L"-" : owner.GetName().c_str().c_str(), part->GetId24(), part->GetInt(kActionPartInt),
					            part->GetInt(kActionPartAltInt));
				}
			}
			continue;
		}

		for (TVisionaireObject *action : actions) {
			bool match = id > 0 ? action->GetId24() == id : action->GetName() == TCharHolder(wxString(argv[i]));

			// `of:<name>` takes the actions of the objects that have that name (a scene, an object...)
			if (!match && std::strncmp(argv[i], "of:", 3) == 0) {
				TVisObjRef owner = TVisObjRef(action).GetParent();

				match = !owner.IsEmpty() && owner.GetName() == TCharHolder(wxString(argv[i] + 3));
			}

			if (match)
				dumpAction(game, action, 0);
		}
	}
	return 0;
}
