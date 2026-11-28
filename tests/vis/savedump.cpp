#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "AppGlobals.h"
#include "TComposedFileManager.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "graphicslib/graphics.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/visLua.h"
#include "datastruct/visionaireobject.h"
#include "datastruct/vlist.h"
#include "vstables/fieldIds.h"
#include "baselib/xmlWriter.h"
#include "vstables/visionaireGame.h"

class NullGraphics : public TGraphicsInterface {
};

// All the game data as XML text after a savegame was loaded over it: savedump <savegame file> <output file> (run from the game
// directory; GAME_PASSW). Two savegames can then be compared with a text diff.
int main(int argc, char **argv) {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 0;

	if (argc < 3) {
		std::printf("usage: savedump <savegame file> <output file>\n");
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
	wxString containerId(L"SAVEGAMEPWD30");
	wxFileName saveFile(wxString(argv[1]).ToStdWstring());

	saveFile.NormalizePath();

	// `none` as the savegame: the game data as they are after loading (what a new game would save)
	if (std::string(argv[1]) != "none") {
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

	}

	// the number of records of each table, and the records of the tables that only a savegame has (the running animations,
	// actions and texts...) by name and id
	for (int table = 0; table < 40; table++) {
		TVList records;

		game->GetList(table, records, false);
		std::printf("table %d: %d records\n", table, (int)records.size());

		if (table == 34) {
			for (TVisionaireObject *record : records) {
				TVisObjRef variable(record);

				if (!variable.GetBool(kScriptVariableIsGlobalVar))
					continue;

				std::printf("GLOBAL '%ls' keyType %d valueType %d value '%ls' items %d\n", record->GetName().c_str().c_str(),
				            variable.GetInt(kScriptVariableKeyType), variable.GetInt(kScriptVariableValueType),
				            variable.GetStr(kScriptVariableValue).wc_str(), (int)variable.GetLinksListSize(kScriptVariableItems));
			}
		} else if (table >= 22) {
			for (TVisionaireObject *record : records)
				std::printf("   table %d record '%ls' id %d\n", table, record->GetName().c_str().c_str(), record->GetId24());
		}
	}

	// savedump <savegame> <output> <resaved>: the loaded game is saved again as a savegame (what TGameControl::SaveGame does after
	// Save()) and the data go to the third file, to be compared with the data of the savegame that was read
	if (argc > 3) {
		TXMLStringWriter resaved;

		if (!game->SaveSaveGame(resaved)) {
			std::printf("the savegame could not be saved again\n");
		} else {
			std::FILE *again = std::fopen(argv[3], "wb");

			if (again) {
				std::fwrite(resaved.GetBuffer().GetData(), 1, resaved.GetBuffer().GetLen(), again);
				std::fclose(again);
				std::printf("resaved %d bytes\n", (int)resaved.GetBuffer().GetLen());
			}
		}
	}

	// savedump <savegame> <output> <resaved> <savegame as xml>: the content of a savegame (what the savegame has, as text)
	if (argc > 4) {
		TXMLStringWriter xml;

		xml.SetSaveGame(true);

		if (game->SaveData(xml, nullptr, true)) {
			xml.FinishWrite();
			xml.GetBufferNonConst().AppendByte(0);

			std::FILE *xmlOut = std::fopen(argv[4], "wb");

			if (xmlOut) {
				std::fwrite(xml.GetBuffer().GetData(), 1, xml.GetBuffer().GetLen() - 1, xmlOut);
				std::fclose(xmlOut);
				std::printf("savegame xml %d bytes\n", (int)xml.GetBuffer().GetLen());
			}
		}
	}

	wxString text = game->SaveDataGameToString();
	std::FILE *out = std::fopen(argv[2], "wb");

	if (!out) {
		std::printf("cannot write %s\n", argv[2]);
		return 1;
	}

	std::string encoded(static_cast<const char *>(text.mb_str()));
	const char *bytes = encoded.c_str();

	std::fwrite(bytes, 1, std::strlen(bytes), out);
	std::fclose(out);
	std::printf("wrote %s (%d characters)\n", argv[2], (int)std::strlen(bytes));
	return 0;
}
