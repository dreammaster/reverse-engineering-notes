#include "TManagedObject.h"

#include <algorithm>
#include <limits>

#include "AppGlobals.h"
#include "TGCharacter.h"
#include "TGObjectManager.h"
#include "TGText.h"
#include "graphicslib/picture.h"
#include "vsplayer/control/gameControl.h"

bool CreatePolygonsFromPointList(const std::vector<wxPoint> &points, TPolygonList &outPolygons) {
	outPolygons = points;
	return true;
}

wxRect GetBoundingBox(const TPolygonList &polygons) {
	if (polygons.empty())
		return wxRect();
	int minX = std::numeric_limits<int>::max();
	int minY = std::numeric_limits<int>::max();
	int maxX = std::numeric_limits<int>::min();
	int maxY = std::numeric_limits<int>::min();
	for (const wxPoint &pt : polygons) {
		minX = std::min(minX, pt.x);
		minY = std::min(minY, pt.y);
		maxX = std::max(maxX, pt.x);
		maxY = std::max(maxY, pt.y);
	}
	wxRect result;
	result.x = minX;
	result.y = minY;
	result.width = maxX - minX;
	result.height = maxY - minY;
	return result;
}

bool IsPointInsidePolygon(const wxPoint &pt, const TPolygonList &polygon) {
	if (polygon.size() < 3)
		return false;
	bool inside = false;
	for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
		const wxPoint &a = polygon[i];
		const wxPoint &b = polygon[j];
		if ((a.y > pt.y) != (b.y > pt.y)) {
			double intersectX = a.x + static_cast<double>(pt.y - a.y) / (b.y - a.y) * (b.x - a.x);
			if (pt.x < intersectX)
				inside = !inside;
		}
	}
	return inside;
}

void TManagedObject::ClickedWithoutReach(TGCharacter *character, TMouseEventEnum event) {
	_objRef.SetLink(0x203, character->GetRef(), true);
	static_cast<TGameControl *>(g_pGameControl)->GetObjectManager()->SaveEventInfo(event);
}

void TManagedObject::SetText(TGText *text) {
	_text = text;
	if (text)
		text->SetOwner(this);
}

void TManagedObject::RemoveSprites() {
	if (_picture)
		_picture->RemoveSprite();
	if (_currentAnimation)
		_currentAnimation->RemoveSprites();
}

void TManagedObject::Prepare() {
	if (_active) {
		if (_currentAnimation) {
			if (!_currentAnimation->IsBonesAnimation())
				_currentAnimation->Prepare();
		} else if (_picture) {
			_picture->RefreshSprite(false);
		}
	}
	for (TGAnimation *anim : _animations)
		if (anim->IsSpriteIndexValid())
			anim->Prepare();
}

void TManagedObject::Draw() {
	if (!_active)
		return;
	UpdateAlpha();
	if (_currentAnimation) {
		if (_currentAnimation->IsSpriteIndexValid()) {
			if (_currentAnimation->IsBonesAnimation())
				_currentAnimation->DrawMixed(_alpha, _color, _animations);
			else
				_currentAnimation->Draw(_alpha, _color, GetAnimationFrameOverride());
		}
	} else if (_picture) {
		_picture->Draw(_alpha, _color);
	}
	if (_text) {
		_text->CalculateCurrentText();
		if (_text)
			_text->Draw(_alpha);
	}
	for (TGAnimation *anim : _animations)
		if (anim->IsSpriteIndexValid())
			anim->Draw(_alpha, _color, -1);
}
