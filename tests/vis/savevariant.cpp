#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#include "AppGlobals.h"
#include "TComposedFileManager.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "baselib/xmlWriter.h"
#include "datastruct/visionaireobject.h"
#include "datastruct/vlist.h"
#include "graphicslib/graphics.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/visLua.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"

class NullGraphics : public TGraphicsInterface {
};

// A savegame loaded over the game data, changed as a list of edits says, and saved again (the data of the savegame, VBIN, go to the
// output file; savevariant.py puts them into a container again): savevariant <savegame> <edits file> <output> (run from the game
// directory; GAME_PASSW). The lines of the edits file:
//     cond <name> <0|1>              a condition (table 10)
//     value <name> <int> [<string>]  a value (table 20): the number, and the string if there is one
//     var <name> <text>              the text of a global script variable (table 34)
//     fixscales                      the scales of all objects (table 6) to 1 and the size of the running animations (table 26)
//                                    that is 0 to -1 (what a savegame of before the fix of the defaults of TTObject::OnInit has wrong)
int main(int argc, char **argv) {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 0;

	if (argc < 4) {
		std::printf("usage: savevariant <savegame> <edits file> <output>\n");
		return 1;
	}

	graphics = new NullGraphics();
	if (getenv("GAME_PASSW"))
		passw = wxString(getenv("GAME_PASSW"));
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"data.vis");
	wxString warning;

	if (!control->PreLoad(file, warning, true)) {
		std::printf("the game was not preloaded\n");
		return 1;
	}

	InitPlayerCommands(control->GetVisionaire(), wxString(L"."), wxString(L"."), wxString(L"."));

	wxString language;

	if (!control->LoadAndInitGame(file, warning, language, true)) {
		std::printf("the game did not load\n");
		return 1;
	}

	TVisionaireGame *game = control->GetVisionaire();
	wxString containerId(L"SAVEGAMEPWD30");
	wxFileName saveFile(wxString(argv[1]).ToStdWstring());

	saveFile.NormalizePath();

	if (!TComposedFileManager::SetSavegameFile(saveFile, containerId)) {
		std::printf("the savegame container did not open\n");
		return 1;
	}

	wxFileName saveDataFile;

	saveDataFile.SetFullName(L"vtp_savedata.xml#g#-01#00000#");

	if (!game->LoadSaveGame(saveDataFile, containerId)) {
		std::printf("the savegame did not load\n");
		return 1;
	}

	std::ifstream edits(argv[2]);
	std::string line;
	int done = 0;

	while (std::getline(edits, line)) {
		std::istringstream words(line);
		std::string kind, name;

		words >> kind >> name;

		if (kind == "cond") {
			int value = 0;
			TVisObjRef condition;

			words >> value;

			if (!game->GetObjectByName(wxString(name.c_str()), 10, condition)) {
				std::printf("no condition '%s'\n", name.c_str());
				continue;
			}

			condition.SetValue(kConditionValue, value != 0, TSendEventEnum::kNoEvent);
			done++;
		} else if (kind == "value") {
			int number = 0;
			std::string text;
			TVisObjRef value;

			words >> number;
			std::getline(words >> std::ws, text);

			if (!game->GetObjectByName(wxString(name.c_str()), 20, value)) {
				std::printf("no value '%s'\n", name.c_str());
				continue;
			}

			value.SetValue(kValueInt, number, TSendEventEnum::kNoEvent);

			if (!text.empty())
				value.SetValue(kValueString, wxString(text.c_str()), TSendEventEnum::kNoEvent);
			done++;
		} else if (kind == "fixscales") {
			TVList objects;
			TVList animations;

			game->GetList(6, objects, false);

			for (TVisionaireObject *record : objects) {
				TVisObjRef object(record);

				object.SetValue(kObjectScale, 1.0f, TSendEventEnum::kNoEvent);
				object.SetValue(kObjectScaleX, 1.0f, TSendEventEnum::kNoEvent);
				object.SetValue(kObjectScaleY, 1.0f, TSendEventEnum::kNoEvent);
				done++;
			}

			game->GetList(26, animations, false);

			for (TVisionaireObject *record : animations) {
				TVisObjRef animation(record);

				if (animation.GetFloat(kAnimationSize) == 0.0f) {
					animation.SetValue(kAnimationSize, -1.0f, TSendEventEnum::kNoEvent);
					done++;
				}
			}
		} else if (kind == "var") {
			std::string text;
			bool found = false;
			TVList variables;

			std::getline(words >> std::ws, text);
			game->GetList(34, variables, false);

			for (TVisionaireObject *record : variables) {
				TVisObjRef variable(record);

				if (variable.GetBool(kScriptVariableIsGlobalVar) && record->GetName() == TCharHolder(wxString(name.c_str()))) {
					variable.SetValue(kScriptVariableValue, wxString(text.c_str()), TSendEventEnum::kNoEvent);
					found = true;
					done++;
					break;
				}
			}

			if (!found)
				std::printf("no global variable '%s'\n", name.c_str());
		}
	}

	TXMLStringWriter resaved;

	if (!game->SaveSaveGame(resaved)) {
		std::printf("the savegame could not be saved again\n");
		return 1;
	}

	std::FILE *out = std::fopen(argv[3], "wb");

	if (!out) {
		std::printf("cannot write %s\n", argv[3]);
		return 1;
	}

	std::fwrite(resaved.GetBuffer().GetData(), 1, resaved.GetBuffer().GetLen(), out);
	std::fclose(out);
	std::printf("%d edits, wrote %d bytes\n", done, (int)resaved.GetBuffer().GetLen());
	return 0;
}
