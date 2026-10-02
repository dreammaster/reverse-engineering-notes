#include "vsplayer/control/cursorControl.h"

#include "AppGlobals.h"
#include "TGAnimation.h"
#include "TGItem.h"
#include "datastruct/visionaireobject.h"
#include "vsplayer/control/gameControl.h"

// TGameControl implements GetObjectManager(), but g_pGameControl is only
// declared as TMasterControl* (AppGlobals.h) - same cast already
// established at TManagedObject::ClickedWithoutReach's own call site.
static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

TCursorControl::~TCursorControl() {
	delete _heldItem;
	for (SCursor *cursor : _cursors) {
		if (!cursor)
			continue;
		TGAnimation::UnloadAnimation(cursor->downImage);
		TGAnimation::UnloadAnimation(cursor->upImage);
		delete cursor;
	}
}

void TCursorControl::GetSCursor(const TVisObjRef &/*source*/, SCursor &/*out*/) {
}

void TCursorControl::AnimationStopped(TGAnimation *animation) {
	if (_currentAnimation == animation)
		_currentAnimation = nullptr;
}

wxString TCursorControl::GetOwnerName() const {
	return wxString();
}

TId TCursorControl::GetOwnerId() const {
	return TId(-1, -1);
}

void TCursorControl::ClearCurrentAnimation() {
	_currentAnimation = nullptr;
}

void TCursorControl::Draw() {
	if (_heldItem) {
		_heldItem->SetCenteredPosition(_position);
		_heldItem->Draw();
		return;
	}
	if (_currentAnimation) {
		_currentAnimation->SetPosition(_position, -1.0f);
		_currentAnimation->Draw(1.0f, 0xFFFFFFFFu, -1);
	}
}

void TCursorControl::setActiveCursorImage(bool useActiveImage) {
	if (_activeCursor == _cursors.end())
		return;
	SCursor *cursor = *_activeCursor;
	if (cursor->active == useActiveImage)
		return;
	cursor->active = useActiveImage;
	if (_currentAnimation) {
		TGAnimation::HideAnimation(_currentAnimation, this);
		_currentAnimation = nullptr;
	}
	TVisObjRef &image = useActiveImage ? cursor->downImage : cursor->upImage;
	_currentAnimation = TGAnimation::StartAnimation(image, this, false, 1.0f, -1);
}

void TCursorControl::SetActiveCursor() {
	setActiveCursorImage(true);
}

void TCursorControl::SetInactiveCursor() {
	setActiveCursorImage(false);
}

void TCursorControl::SetCursorPosition(int x, int y) {
	_position.x = x;
	_position.y = y;
}

wxPoint TCursorControl::GetPositionNextToCursor() const {
	wxPoint pos = _position;
	if (_heldItem)
		return _heldItem->GetPositionNextToItem();
	if (_currentAnimation) {
		if (TSprite *sprite = _currentAnimation->GetCurrentSprite()) {
			float width = sprite->GetSizedWidth();
			TVisObjRef dataObject = _currentAnimation->GetDataObject();
			if (const wxPoint *pt = dataObject.GetPoint(0x16F))
				pos.x = static_cast<int>(static_cast<float>(pos.x) + (width - static_cast<float>(pt->x)));
		}
	}
	return pos;
}

void TCursorControl::SetMoveObject(const TVisObjRef &item) {
	delete _heldItem;
	_heldItem = nullptr;
	if (!item.IsEmpty())
		_heldItem = new TGItem(item, false);
}

void TCursorControl::ReleaseMoveObject() {
	delete _heldItem;
	_heldItem = nullptr;
	bool detectable = gameControl()->GetObjectManager()->IsCurrentObjectDetectable();
	setActiveCursorImage(detectable);
}

bool TCursorControl::IsActiveMoveObject() const {
	return _heldItem != nullptr;
}

bool TCursorControl::IsCursorActive() {
	if (_activeCursor == _cursors.end())
		return false;
	return (*_activeCursor)->active;
}

static bool matchesCursorId(SCursor *cursor, int cursorId, bool byId) {
	if (byId)
		return cursor->id == cursorId;
	for (int linkedId : cursor->linkedIds)
		if (linkedId == cursorId)
			return true;
	return false;
}

void TCursorControl::SetCursor(bool useActiveImage, int cursorId, bool byId) {
	_activeCursor = _cursors.end();
	for (std::vector<SCursor *>::iterator it = _cursors.begin(); it != _cursors.end(); ++it) {
		if (matchesCursorId(*it, cursorId, byId)) {
			_activeCursor = it;
			break;
		}
	}

	if (_currentAnimation) {
		TGAnimation::HideAnimation(_currentAnimation->GetDataObject());
		_currentAnimation = nullptr;
	}

	if (_activeCursor == _cursors.end())
		return;
	SCursor *cursor = *_activeCursor;
	cursor->active = useActiveImage;
	TVisObjRef &image = useActiveImage ? cursor->downImage : cursor->upImage;
	_currentAnimation = TGAnimation::StartAnimation(image, this, false, 1.0f, -1);
}

void TCursorControl::SetCursor(int cursorId, bool byId) {
	bool useActiveImage = _activeCursor != _cursors.end() && (*_activeCursor)->active;
	SetCursor(useActiveImage, cursorId, byId);
}

void TCursorControl::Clear() {
	delete _heldItem;
	_heldItem = nullptr;
	for (SCursor *cursor : _cursors) {
		if (!cursor)
			continue;
		TGAnimation::UnloadAnimation(cursor->downImage);
		TGAnimation::UnloadAnimation(cursor->upImage);
		delete cursor;
	}
	_cursors.clear();
	_activeCursor = _cursors.end();
	_position = wxPoint();
	_currentAnimation = nullptr;
}

void TCursorControl::LoadCursor(const TVisObjRef &cursor) {
	int id = PackVisId(cursor.GetId());
	for (SCursor *existing : _cursors)
		if (existing->id == id)
			return;

	SCursor *newCursor = new SCursor();
	newCursor->id = id;
	GetSCursor(cursor, *newCursor);
	TGAnimation::PreloadAnimation(newCursor->downImage);
	TGAnimation::PreloadAnimation(newCursor->upImage);
	_cursors.push_back(newCursor);
	_activeCursor = _cursors.end();
}

void TCursorControl::LinkButtonCursor(int cursorId, int linkedId) {
	for (SCursor *cursor : _cursors) {
		if (cursor->id == cursorId) {
			cursor->linkedIds.push_back(linkedId);
			return;
		}
	}
}
