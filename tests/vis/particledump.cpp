#include <cstdio>
#include <cstdlib>
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

static std::string listInts(TVisionaireObject *o, int field) {
	std::vector<int> v;
	TVisObjRef(o).GetInts(field, v);
	std::string s = "[" + std::to_string(v.size()) + ":";
	for (size_t i = 0; i < v.size() && i < 12; i++)
		s += " " + std::to_string(v[i]);
	return s + (v.size() > 12 ? " ...]" : "]");
}

static std::string listFloats(TVisionaireObject *o, int field) {
	std::vector<float> v;
	TVisObjRef(o).GetFloats(field, v);
	std::string s = "[" + std::to_string(v.size()) + ":";
	for (size_t i = 0; i < v.size() && i < 12; i++)
		s += " " + std::to_string(v[i]);
	return s + (v.size() > 12 ? " ...]" : "]");
}

// Prints the particle systems of the real data (run from the game directory; GAME_PASSW).
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 3;

	graphics = new NullGraphics();
	if (getenv("GAME_PASSW"))
		passw = wxString(getenv("GAME_PASSW"));
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"data.vis");
	wxString warning;

	if (!control->PreLoad(file, warning, true)) {
		printf("not preloaded\n");
		return 1;
	}
	InitPlayerCommands(control->GetVisionaire(), wxString(L"."), wxString(L"."), wxString(L"."));
	wxString language;
	if (!control->LoadAndInitGame(file, warning, language, true)) {
		printf("not loaded\n");
		return 1;
	}

	TVList containers;
	control->GetVisionaire()->GetList(23, containers, false);
	printf("containers (table 23): %d\n", (int)containers.size());
	for (TVisionaireObject *c : containers) {
		TVisionaireObject *o = c;
		printf("container '%ls' id %d parent '%ls'\n", c->GetName().c_str().c_str(), c->GetId24(), TVisObjRef(c).GetParent().GetName().c_str().c_str());
		printf("  LeadTime = %d\n", o->GetInt(kParticleContainerLeadTime));
	}

	{
		TVList scenes;

		control->GetVisionaire()->GetList(4, scenes, false);
		for (TVisionaireObject *sc : scenes) {
			TVisObjRef link = TVisObjRef(sc).GetLink(kSceneParticleSystem);

			if (sc->GetName() == TCharHolder("1002") || sc->GetName() == TCharHolder("1003") || sc->GetName() == TCharHolder("2018"))
				printf("SCENE %ls id %d link empty %d\n", sc->GetName().c_str().c_str(), sc->GetId24(), (int)link.IsEmpty());

			if (!link.IsEmpty())
				printf("scene '%ls' (id %d) has container id %d\n", sc->GetName().c_str().c_str(), sc->GetId24(), link.GetObjectPointer()->GetId24());
		}

		TVList objects;

		control->GetVisionaire()->GetList(6, objects, false);
		for (TVisionaireObject *ob : objects) {
			TVisObjRef link = TVisObjRef(ob).GetLink(kObjectParticleSystem);

			if (!link.IsEmpty())
				printf("object '%ls' (id %d, scene '%ls') has container id %d\n", ob->GetName().c_str().c_str(), ob->GetId24(),
				       TVisObjRef(ob).GetParent().GetName().c_str().c_str(), link.GetObjectPointer()->GetId24());
		}
	}

	TVList list;
	control->GetVisionaire()->GetList(22, list, false);
	printf("particles (table 22): %d\n", (int)list.size());
	for (TVisionaireObject *o : list) {
		printf("particle '%ls' id %d\n", o->GetName().c_str().c_str(), o->GetId24());
		printf("  kParticleContainerParticles = %s\n", std::string("links:") + std::to_string(TVisObjRef(o).GetLinksListSize(kParticleContainerParticles)).c_str());
		printf("  kParticleContainerCameraZPosition = %s\n", std::to_string(o->GetFloat(kParticleContainerCameraZPosition)).c_str());
		printf("  kParticleContainerCameraZoom = %s\n", std::to_string(o->GetFloat(kParticleContainerCameraZoom)).c_str());
		printf("  kParticleContainerLeadTime = %s\n", std::to_string(o->GetInt(kParticleContainerLeadTime)).c_str());
		printf("  kParticleContainerSettings = %s\n", std::string(o->GetStr(kParticleContainerSettings).mb_str()).c_str());
		printf("  kParticleContainerPreviewScene = %s\n", std::string("(kLink)").c_str());
		printf("  kParticleTexture = %s\n", std::string(o->GetPath(kParticleTexture).GetFullPath().mb_str()).c_str());
		printf("  kParticleMaterialMode = %s\n", std::to_string(o->GetInt(kParticleMaterialMode)).c_str());
		printf("  kParticleTilesX = %s\n", std::to_string(o->GetInt(kParticleTilesX)).c_str());
		printf("  kParticleTilesY = %s\n", std::to_string(o->GetInt(kParticleTilesY)).c_str());
		printf("  kParticleType = %s\n", std::to_string(o->GetInt(kParticleType)).c_str());
		printf("  kParticleXPos = %s\n", std::to_string(o->GetFloat(kParticleXPos)).c_str());
		printf("  kParticleYPos = %s\n", std::to_string(o->GetFloat(kParticleYPos)).c_str());
		printf("  kParticleEmitterType = %s\n", std::to_string(o->GetInt(kParticleEmitterType)).c_str());
		printf("  kParticleEmitterWidth = %s\n", std::to_string(o->GetFloat(kParticleEmitterWidth)).c_str());
		printf("  kParticleEmitterHeight = %s\n", std::to_string(o->GetFloat(kParticleEmitterHeight)).c_str());
		printf("  kParticleEmitterDepth = %s\n", std::to_string(o->GetFloat(kParticleEmitterDepth)).c_str());
		printf("  kParticleMaxParticles = %s\n", std::to_string(o->GetInt(kParticleMaxParticles)).c_str());
		printf("  kParticlePerTime = %s\n", std::to_string(o->GetFloat(kParticlePerTime)).c_str());
		printf("  kParticlePhiX = %s\n", std::to_string(o->GetInt(kParticlePhiX)).c_str());
		printf("  kParticlePhiY = %s\n", std::to_string(o->GetInt(kParticlePhiY)).c_str());
		printf("  kParticleMinSize = %s\n", std::to_string(o->GetFloat(kParticleMinSize)).c_str());
		printf("  kParticleMaxSize = %s\n", std::to_string(o->GetFloat(kParticleMaxSize)).c_str());
		printf("  kParticleMinVelocity = %s\n", std::to_string(o->GetFloat(kParticleMinVelocity)).c_str());
		printf("  kParticleMaxVelocity = %s\n", std::to_string(o->GetFloat(kParticleMaxVelocity)).c_str());
		printf("  kParticleMinLife = %s\n", std::to_string(o->GetInt(kParticleMinLife)).c_str());
		printf("  kParticleMaxLife = %s\n", std::to_string(o->GetInt(kParticleMaxLife)).c_str());
		printf("  kParticleMinColor = %s\n", std::to_string(o->GetInt(kParticleMinColor)).c_str());
		printf("  kParticleMaxColor = %s\n", std::to_string(o->GetInt(kParticleMaxColor)).c_str());
		printf("  kParticleRotationX = %s\n", std::to_string(o->GetInt(kParticleRotationX)).c_str());
		printf("  kParticleRotationZ = %s\n", std::to_string(o->GetInt(kParticleRotationZ)).c_str());
		printf("  kParticleActiveStages = %s\n", std::to_string(o->GetInt(kParticleActiveStages)).c_str());
		printf("  kParticleBlendColor0 = %s\n", std::to_string(o->GetInt(kParticleBlendColor0)).c_str());
		printf("  kParticleBlendColor1 = %s\n", std::to_string(o->GetInt(kParticleBlendColor1)).c_str());
		printf("  kParticleBlendColor2 = %s\n", std::to_string(o->GetInt(kParticleBlendColor2)).c_str());
		printf("  kParticleBlendColor3 = %s\n", std::to_string(o->GetInt(kParticleBlendColor3)).c_str());
		printf("  kParticleBlendColorCurve0 = %s\n", listFloats(o, kParticleBlendColorCurve0).c_str());
		printf("  kParticleBlendColorCurve1 = %s\n", listFloats(o, kParticleBlendColorCurve1).c_str());
		printf("  kParticleBlendColorCurve2 = %s\n", listFloats(o, kParticleBlendColorCurve2).c_str());
		printf("  kParticleBlendColorCurve3 = %s\n", listFloats(o, kParticleBlendColorCurve3).c_str());
		printf("  kParticleColorControlPoints0 = %s\n", listInts(o, kParticleColorControlPoints0).c_str());
		printf("  kParticleColorControlPoints1 = %s\n", listInts(o, kParticleColorControlPoints1).c_str());
		printf("  kParticleColorControlPoints2 = %s\n", listInts(o, kParticleColorControlPoints2).c_str());
		printf("  kParticleColorControlPoints3 = %s\n", listInts(o, kParticleColorControlPoints3).c_str());
		printf("  kParticleForceType = %s\n", std::to_string(o->GetInt(kParticleForceType)).c_str());
		printf("  kParticleForceStrength = %s\n", std::to_string(o->GetFloat(kParticleForceStrength)).c_str());
		printf("  kParticleForceRotationX = %s\n", std::to_string(o->GetInt(kParticleForceRotationX)).c_str());
		printf("  kParticleForceRotationZ = %s\n", std::to_string(o->GetInt(kParticleForceRotationZ)).c_str());
		printf("  kParticleForceXPos = %s\n", std::to_string(o->GetFloat(kParticleForceXPos)).c_str());
		printf("  kParticleForceYPos = %s\n", std::to_string(o->GetFloat(kParticleForceYPos)).c_str());
		printf("  kParticleForceZPos = %s\n", std::to_string(o->GetFloat(kParticleForceZPos)).c_str());
		printf("  kParticleForceBlend = %s\n", std::to_string(o->GetFloat(kParticleForceBlend)).c_str());
		printf("  kParticleBlendSizeCurve = %s\n", listFloats(o, kParticleBlendSizeCurve).c_str());
		printf("  kParticleSizeControlPoints = %s\n", listInts(o, kParticleSizeControlPoints).c_str());
		printf("  kParticleBlendMaxTile = %s\n", std::to_string(o->GetInt(kParticleBlendMaxTile)).c_str());
		printf("  kParticleBlendTilesCurve = %s\n", listFloats(o, kParticleBlendTilesCurve).c_str());
		printf("  kParticleBlendTilesControlPoints = %s\n", listInts(o, kParticleBlendTilesControlPoints).c_str());
		printf("  kParticlePosX = %s\n", std::to_string(o->GetInt(kParticlePosX)).c_str());
		printf("  kParticlePosY = %s\n", std::to_string(o->GetInt(kParticlePosY)).c_str());
		printf("  kParticleEmitterSizeX = %s\n", std::to_string(o->GetInt(kParticleEmitterSizeX)).c_str());
		printf("  kParticleEmitterSizeY = %s\n", std::to_string(o->GetInt(kParticleEmitterSizeY)).c_str());
		printf("  kParticleEmitterSizeZ = %s\n", std::to_string(o->GetInt(kParticleEmitterSizeZ)).c_str());
	}
	return 0;
}
