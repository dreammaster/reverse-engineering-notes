#include "TSceneActionArea.h"

#include "AppGlobals.h"
#include "TGScene.h"
#include "datastruct/vlist.h"
#include "datastruct/visionaireobject.h"
#include "vsplayer/control/gameControl.h"
#include "vstables/fieldIds.h"

// Confirmed (asm lines 260143-260376)
TSceneActionArea::TSceneActionArea(const TVisObjRef &ref) : TVisObjRef(ref), _executeAlways(false) {
	std::vector<wxPoint> points;

	GetPoints(kActionAreaPolygon, points);
	if (!CreatePolygonsFromPointList(points, _polygons) && wxLog::loglevel > 0) {
		wxLog::logexpanded(L"Illegal polygon border for action area '%ls' (id: %d)", GetNameWithParents(3).c_str(),
		                   GetObjectPointer()->GetId24());
	}

	_bounds = GetBoundingBox(_polygons);

	TVList actions;
	GetLinks(kActionAreaActions, TypeOrder::kValue0, actions);
	for (TVisionaireObject *action : actions) {
		if (action->GetBool(kAreaActionExecuteAlways)) {
			_executeAlways = true;
			break;
		}
	}
}

// Confirmed (asm lines 260415-260600)
bool TSceneActionArea::CanTrigger(const TVisObjRef &character) const {
	if (!_executeAlways) {
		TGScene *scene = static_cast<TGameControl *>(g_pGameControl)->GetScene();

		if (!(character.GetLink(kCharacterScene) == scene->GetRef()))
			return false;
	}

	TVList actions;
	GetLinks(kActionAreaActions, TypeOrder::kValue0, actions);
	for (TVisionaireObject *action : actions) {
		TVisObjRef actionRef(action);
		TVisObjRef actionCharacter = actionRef.GetLink(kAreaActionCharacter);

		if (actionCharacter.IsAnyObject())
			return true;
		if (!actionCharacter.IsEmpty() && actionCharacter == character)
			return true;
	}
	return false;
}

// Confirmed (asm lines 260601-260644)
bool TSceneActionArea::IsInside(const wxPoint &point) const {
	if (!_bounds.Contains(point))
		return false;

	return IsPointInsidePolygon(point, _polygons);
}
