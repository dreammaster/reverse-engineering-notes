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
#include "TComposedFileManager.h"
#include "baselib/file.h"
#include "graphicslib/picture.h"
#include "TSprite.h"
#include "vstables/records.h"
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
	if (loaded) {
		// every picture of the animations is decoded
		TVList animations;
		int decoded = 0;
		int failed = 0;
		long long alphaSum = 0;
		int shown = 0;
		int limit = getenv("REALDATA_ALL") ? 1000000 : 400;

		control->GetVisionaire()->GetList(9, animations, false);
		for (TVisionaireObject *object : animations) {
			std::vector<TSprite> sprites;

			TVisObjRef(object).GetSprites(kAnimationSprites, sprites);
			for (TSprite &sprite : sprites) {
				wxFileName path(sprite.GetPathNonConst());

				// (a path without the reference into a container is a file of the editor that is not in the game)
				if (!path.IsOk() || path.GetFullPath().IsEmpty() || path.GetFullPath().ToStdWstring().find(L'#') == std::wstring::npos)
					continue;

				TPictureIO picture;

				if (picture.LoadPicture(path, TPictureIO::eLoadSetting::ForceReload) && picture.GetMemoryData()) {
					decoded++;
					for (int y = 0; y < picture.GetMemHeight(); y += 16)
						for (int x = 0; x < picture.GetMemWidth(); x += 16)
							alphaSum += (unsigned char)picture.GetLine(y)[x * 4 + 3];
					if (shown++ < 3)
						printf("picture %ls: %dx%d\n", path.GetFullName().wc_str(), picture.GetMemWidth(), picture.GetMemHeight());
				} else {
					failed++;
					if (failed < 5)
						printf("FAILED %ls\n", path.GetFullPath().wc_str());
				}
				if (decoded + failed >= limit)
					break;
			}
			if (decoded + failed >= limit)
				break;
		}
		printf("decoded %d, failed %d, alpha sum %lld\n", decoded, failed, alphaSum);
		CHECK(failed == 0);
	}
	if (loaded) {
		TVisObjRef g = control->GetVisionaire()->GetGame();

		printf("scene files %d '%ls', character files %d '%ls', interface files %d '%ls', movie files %d '%ls', game files %d '%ls'\n",
		       g.GetInt(kGameSceneComposedFiles), g.GetPath(kGameSceneComposedFile).GetFullPath().wc_str(),
		       g.GetInt(kGameCharacterComposedFiles), g.GetPath(kGameCharacterComposedFile).GetFullPath().wc_str(),
		       g.GetInt(kGameInterfaceComposedFiles), g.GetPath(kGameInterfaceComposedFile).GetFullPath().wc_str(),
		       g.GetInt(kGameMovieComposedFiles), g.GetPath(kGameMovieComposedFile).GetFullPath().wc_str(),
		       g.GetInt(kGameGameComposedFiles), g.GetPath(kGameGameComposedFile).GetFullPath().wc_str());
	}
	if (loaded) {
		const int volumes[] = {0, 1, 24, 25, 26, 35, 36, 37, 38, 73};

		for (int volume : volumes) {
			wchar_t name[128];

			swprintf(name, 128, L"x.webp#s#%03d#00000#", volume);

			TFile probe;
			bool opened = TComposedFileManager::Open(probe, wxFileName(std::wstring(name)));

			printf("scene volume %d: open %d length %ld\n", volume, (int)opened, opened ? (long)probe.GetFileLength() : -1L);
		}
	}
	(void)argc;
	(void)argv;
	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
