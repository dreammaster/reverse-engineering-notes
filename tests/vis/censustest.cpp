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
#include "vstables/records.h"

class NullGraphics : public TGraphicsInterface {
};

// Which records and commands the real game data has: what a player has to be able to do for this game (run from the game directory).
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

	if (!control->PreLoad(file, warning, true))
		return 1;
	InitPlayerCommands(control->GetVisionaire(), wxString(L"."), wxString(L"."), wxString(L"."));
	wxString language;
	if (!control->LoadAndInitGame(file, warning, language, true))
		return 2;

	static const struct {
		int table;
		const char *name;
	} tables[] = {
		{0, "Character"}, {1, "Interface"}, {2, "Button"}, {3, "Font"}, {4, "Scene"}, {5, "Point"}, {6, "Object"}, {7, "Action"},
		{8, "ActionPart"}, {9, "Animation"}, {10, "Condition"}, {11, "Dialog"}, {12, "DialogPart"}, {13, "Sprite"}, {14, "Text"},
		{15, "Cursor"}, {16, "CommentSet"}, {17, "Outfit"}, {18, "Language"}, {19, "TextLanguage"}, {20, "Value"}, {21, "Loading"},
		{22, "Particle"}, {23, "ParticleContainer"}, {27, "CommentSetEntry"}, {28, "AnimationFrame"}, {29, "WaySystem"},
		{30, "Script"}, {31, "InterfaceClass"}, {32, "ActionArea"}, {33, "AreaAction"}, {35, "Model"}, {36, "Shader"}, {37, "AudioBus"},
		{38, "Event"}, {39, "Plugin"}, {40, "BlockContainer"}, {41, "Block"}};

	for (auto &t : tables) {
		TVList list;

		control->GetVisionaire()->GetList(t.table, list, false);
		printf("table %2d %-18s %d records\n", t.table, t.name, (int)list.size());
	}

	// the commands of the action parts, by number
	{
		TVList parts;
		std::map<int, int> commands;

		control->GetVisionaire()->GetList(8, parts, false);
		for (TVisionaireObject *object : parts)
			commands[object->GetInt(kActionPartCommand)]++;
		for (auto &c : commands)
			printf("command %3d: %d action parts\n", c.first, c.second);
	}

	// the conditions: how many have the lists of Night of the Rabbit (ConditionConditions and ConditionConditionsNegate), and whether they agree
	{
		TVList list;
		int variables = 0;
		int withList = 0;
		int mismatch = 0;
		int oldLinks = 0;
		std::map<int, int> sizes;

		control->GetVisionaire()->GetList(10, list, false);
		for (TVisionaireObject *object : list) {
			TVisObjRef ref(object);

			if (ref.GetBool(kConditionIsVariable)) {
				variables++;
				continue;
			}

			TVList items;
			std::vector<bool> negate;

			ref.GetLinks(kConditionConditions, TypeOrder::kValue0, items);
			ref.GetBools(kConditionConditionsNegate, negate);
			if (!items.empty())
				withList++;
			sizes[(int)items.size()]++;
			if (items.size() != negate.size())
				mismatch++;
			if (!ref.GetLink(kConditionCondition1).IsEmpty() || !ref.GetLink(kConditionCondition2).IsEmpty())
				oldLinks++;
		}
		printf("conditions: %d, variables %d, with a list %d, list and flags of different size %d, with the old two links %d\n", (int)list.size(), variables, withList, mismatch, oldLinks);
		for (auto &e : sizes)
			printf("conditions with %d sub-conditions: %d\n", e.first, e.second);
	}

	// scenes, objects, buttons and characters: what needs more than pictures and sounds from a renderer
	{
		struct Feature {
			const char *name;
			int table;
			int field;
			bool link;
			int count;
			int of;
		};
		Feature features[] = {
			{"scene with a lightmap", 4, kSceneLightMap, false, 0, 0},
			{"scene with a particle system", 4, kSceneParticleSystem, true, 0, 0},
			{"scene with a shader set", 4, kSceneShaderSet, true, 0, 0},
			{"scene with audio busses", 4, kSceneAudioBusses, true, 0, 0},
			{"object with a particle system", 6, kObjectParticleSystem, true, 0, 0},
			{"object with a shader set", 6, kObjectShaderSet, true, 0, 0},
			{"button with a shader set", 2, kButtonShaderSet, true, 0, 0},
			{"character with a shader set", 0, kCharacterShaderSet, true, 0, 0},
		};

		for (Feature &f : features) {
			TVList list;

			control->GetVisionaire()->GetList(f.table, list, false);
			for (TVisionaireObject *object : list) {
				TVisObjRef ref(object);

				f.of++;
				if (f.link) {
					if (f.field == kSceneAudioBusses ? ref.GetLinksListSize(f.field) > 0 : !ref.GetLink(f.field).IsEmpty())
						f.count++;
				} else if (!object->GetPath(f.field).GetFullPath().IsEmpty()) {
					f.count++;
				}
			}
			printf("%s: %d of %d\n", f.name, f.count, f.of);
		}
	}
	return 0;
}
