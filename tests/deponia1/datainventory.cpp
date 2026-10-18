#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "baselib/composedfile.h"
#include "TComposedFileManager.h"
#include "graphicslib/graphics.h"
#include "vscommon/scripting/visLua.h"
#include "vscommon/scripting/command.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"
#include "datastruct/table.h"
#include "datastruct/visionaireobject.h"
#include "datastruct/vlist.h"
#include "TSprite.h"
#include "vstables/records.h"

class NullGraphics : public TGraphicsInterface {
};

// What the real game data holds (run from the game directory; argument: the directory for the dumped scripts).
int main(int argc, char **argv) {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 0;

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"data.vis");
	wxString warning;

	if (!control->PreLoad(file, warning, true))
		return 1;
	InitPlayerCommands(control->GetVisionaire(), wxString(L"."), wxString(L"."), wxString(L"."));
	wxString language;
	if (!control->LoadAndInitGame(file, warning, language, true))
		return 2;

	TComposedFile *container = TComposedFileManager::GetMainContainer();
	std::map<std::string, int> extensions;
	std::map<std::string, long long> sizes;

	for (int i = 0; i < container->GetNumberOfEntries(); i++) {
		SEntryInfo *entry = container->GetEntry(i);
		std::string name(entry->filename.begin(), entry->filename.end());
		size_t dot = name.rfind('.');
		std::string ext = dot == std::string::npos ? "(none)" : name.substr(dot);

		extensions[ext]++;
		sizes[ext] += entry->byteSize;
	}
	for (auto &e : extensions)
		printf("ext %s: %d files, %lld bytes\n", e.first.c_str(), e.second, sizes[e.first]);

	{
		TVList animations;
		int model = 0;
		int bones = 0;
		int total = 0;
		std::map<std::string, int> spriteExt;

		control->GetVisionaire()->GetList(9, animations, false);
		for (TVisionaireObject *object : animations) {
			TVisObjRef ref(object);
			TTAnimation animation(ref);

			total++;
			if (TTAnimation::IsModelAnimation(ref))
				model++;
			if (TTAnimation::IsBonesAnimation(ref))
				bones++;

			std::vector<TSprite> sprites;

			ref.GetSprites(kAnimationSprites, sprites);
			for (const TSprite &sprite : sprites) {
				std::string path(static_cast<const char *>(const_cast<TSprite &>(sprite).GetPathNonConst().mb_str()));
				size_t dot = path.find('#');
				std::string clean = path.substr(0, dot);
				size_t d2 = clean.rfind('.');

				spriteExt[d2 == std::string::npos ? "(none)" : clean.substr(d2)]++;
			}
		}
		printf("animations: %d, model %d, bones %d\n", total, model, bones);
		for (auto &e : spriteExt)
			printf("animation sprite ext %s: %d\n", e.first.c_str(), e.second);
	}

	{
		TVList parts;
		std::map<std::string, int> pathExt;

		control->GetVisionaire()->GetList(8, parts, false);
		for (TVisionaireObject *object : parts) {
			wxFileName path = object->GetPath(kActionPartPath);
			std::string full(static_cast<const char *>(path.GetFullPath().mb_str()));
			size_t hash = full.find('#');
			std::string clean = full.substr(0, hash);
			size_t dot = clean.rfind('.');

			if (clean.empty())
				continue;
			pathExt[dot == std::string::npos ? "(none)" : clean.substr(dot)]++;
		}
		for (auto &e : pathExt)
			printf("actionpart path ext %s: %d\n", e.first.c_str(), e.second);

		TVList fonts;

		control->GetVisionaire()->GetList(3, fonts, false);
		std::map<std::string, int> fontExt;
		for (TVisionaireObject *object : fonts) {
			wxFileName path = object->GetPath(kFontTrueTypeFontPath);
			std::string full(static_cast<const char *>(path.GetFullPath().mb_str()));
			size_t hash = full.find('#');
			std::string clean = full.substr(0, hash);
			size_t dot = clean.rfind('.');

			fontExt[dot == std::string::npos ? "(none)" : clean.substr(dot)]++;
		}
		for (TVisionaireObject *object : fonts) {
			std::wstring kerning = object->GetStr(kFontKerning).ToStdWstring();

			if (object->GetId24() == 57 || object->GetId24() == 58) {
				printf("font %d kerning (%d chars):", object->GetId24(), (int)kerning.size());
				for (size_t i = 0; i < kerning.size() && i < 120; i++) {
					if (kerning[i] < 32 || kerning[i] > 126)
						printf("[%d]", (int)kerning[i]);
					else
						putchar((int)kerning[i]);
				}
				putchar(10);
			}
		}
		for (auto &e : fontExt)
			printf("font file ext %s: %d\n", e.first.c_str(), e.second);
	}

	if (argc > 1) {
		TVList scripts;

		control->GetVisionaire()->GetList(30, scripts, false);
		for (TVisionaireObject *script : scripts) {
			std::string text(static_cast<const char *>(script->GetStr(kScriptScript).mb_str()));
			size_t pos = 0;

			while ((pos = text.find("<br/>", pos)) != std::string::npos) {
				text.replace(pos, 5, "\n");
				pos++;
			}
			std::string name = std::string(argv[1]) + "/script_" + std::to_string(script->GetId24()) + "_" + std::string(script->GetName().mb_str()) + ".lua";
			FILE *f = fopen(name.c_str(), "wb");

			if (f) {
				fwrite(text.data(), 1, text.size(), f);
				fclose(f);
			}
		}
		printf("scripts: %d\n", (int)scripts.size());
	}
	return 0;
}
