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

// The outfits of the game (table 17) with the speed, whether they slide, and the walk animations with the steps; run from the game directory (GAME_PASSW).
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

	TVList outfits;

	control->GetVisionaire()->GetList(17, outfits, false);

	for (TVisionaireObject *outfit : outfits) {
		TVisObjRef ref(outfit);
		TVisObjRef owner = ref.GetParent();
		TVList walks;

		ref.GetLinks(kOutfitWalkAnimations, TypeOrder::kValue0, walks);
		std::printf("outfit '%ls' of '%ls': speed %d slide %d walks %d\n", outfit->GetName().c_str().c_str(),
		            owner.IsEmpty() ? L"-" : owner.GetName().c_str().c_str(), outfit->GetInt(kOutfitCharacterSpeed),
		            (int)outfit->GetBool(kOutfitSlideWalkAnimation), (int)walks.size());

		for (TVisionaireObject *walk : walks) {
			std::vector<float> steps;
			std::vector<TSprite> sprites;

			TVisObjRef(walk).GetFloats(kAnimationWalkSteps, steps);
			TVisObjRef(walk).GetSprites(kAnimationSprites, sprites);
			std::printf("   walk '%ls': steps %d sprites %d first %f\n", walk->GetName().c_str().c_str(), (int)steps.size(), (int)sprites.size(),
			            steps.empty() ? 0.0f : steps[0]);
		}
	}
	return 0;
}
