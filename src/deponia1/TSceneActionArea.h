// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 260143-260645, 169215-169506): a 0x38-byte
// object built from a TVisObjRef of one of a scene's "action area" links (field
// 0x2A9): the handle itself (it derives from TVisObjRef), the area's polygons,
// their bounding box and whether any of its actions executes always (even when
// the character is not in the scene).
#pragma once

#include "TPolygonList.h"
#include "datastruct/visobjref.h"

class TSceneActionArea : public TVisObjRef {
public:
	explicit TSceneActionArea(const TVisObjRef &ref);
	~TSceneActionArea() = default;

	/** Whether one of the area's actions can be triggered by this character:
	 *  one that executes always needs nothing, the others need the character
	 *  to be in the current scene; then the action's character must be any
	 *  character or this one. */
	bool CanTrigger(const TVisObjRef &character) const;
	/** Whether the point is within the area. */
	bool IsInside(const wxPoint &point) const;

private:
	TPolygonList _polygons;
	wxRect _bounds;
	bool _executeAlways;
};
