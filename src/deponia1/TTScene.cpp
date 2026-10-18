#include "TTScene.h"

#include "TTText.h"
#include "datastruct/visionaireobject.h"
#include "datastruct/vlist.h"
#include "vstables/eCommand.h"
#include "vstables/fieldIds.h"

// The command 11: not in eCommand (TGAction does nothing for it), but a sound of the scene as well.
static const int kCommandSoundOld = 11;

// Confirmed (asm lines 1447687-1447784)
wxRect TTScene::GetLoadingRect() {
	TVList points;
	wxPoint first{-1, -1};
	wxPoint second{-1, -1};

	GetLinks(kScenePoints, TypeOrder::kValue3, points);
	if (points.size() > 0)
		first = *points.at(0)->GetPoint(kPointPosition);
	if (points.size() > 1)
		second = *points.at(1)->GetPoint(kPointPosition);

	// (the second point goes into the width and the height: the original does not make a rectangle of the two)
	wxRect rect;

	rect.x = first.x;
	rect.y = first.y;
	rect.width = second.x;
	rect.height = second.y;
	return rect;
}

// Confirmed (asm lines 1447784-1447916)
wxString TTScene::GetNameLanguage() const {
	if (IsEmpty())
		return wxString();

	TTText name(GetLink(kSceneName));

	return name.GetTextString();
}

// Confirmed (asm lines 1447916-1448630)
void TTScene::GetSoundsOnScene(std::vector<TCharHolder> &sounds) const {
	TVList actions;
	TVList parts;

	sounds.clear();
	GetLinks(kSceneActions, TypeOrder::kValue0, actions);

	for (TVisionaireObject *action : actions) {
		action->GetLinks(kActionActionParts, TypeOrder::kValue0, parts);

		for (TVisionaireObject *part : parts) {
			int command = part->GetInt(kActionPartCommand);

			if (command != kCommandSceneMusic && command != kCommandPlaySound && command != kCommandLoopSound &&
			    command != kCommandSetWalkingSound && command != kCommandSoundOld)
				continue;

			wxFileName path = part->GetPath(kActionPartPath);

			if (path.IsOk())
				sounds.push_back(TCharHolder(path.GetFullPath()));
		}
	}
}
