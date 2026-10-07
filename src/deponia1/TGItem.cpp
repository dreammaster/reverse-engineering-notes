#include "TGItem.h"

#include "AppGlobals.h"
#include "TGActionInfo.h"
#include "TGInterface.h"
#include "TGPlaceHolder.h"
#include "datastruct/visionaire.h"
#include "vsplayer/animationGame.h"
#include "vsplayer/control/cursorControl.h"
#include "vsplayer/control/gameControl.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"

// TGameControl implements everything used from it here, but g_pGameControl is only
// declared as TMasterControl* (AppGlobals.h) - the same cast the other classes use.
static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

// The top left corner of a picture of the given size that is centred on `center`.
static wxPoint centredCorner(const wxPoint &center, const TSprite &sprite) {
	wxPoint corner;

	corner.x = center.x - (int)sprite.GetSizedWidth() / 2;
	corner.y = center.y - (int)sprite.GetSizedHeight() / 2;
	return corner;
}

// Confirmed (asm lines 261052-261101). The item is active from the start, its animation
// (if any) plays once.
TGItem::TGItem(const TVisObjRef &ref, bool animated) : TMObject(ref, animated) {
	_bypassReachCheck = true;
	_skipFinalPostExecution = false;
	_active = false;

	TVisObjRef animation = _objRef.GetLink(kObjectAnimation);

	if (!animation.IsEmpty())
		animation.SetValue(kAnimationNumberOfLoops, 0, TSendEventEnum::kNoEvent);

	TMObject::SetActive(true);
}

// Confirmed (asm lines 260820-260908): the picture (the animation's current one, or the
// item's own) is moved so that it is centred on the item's position.
void TGItem::Draw() {
	if (_currentAnimation && _currentAnimation->IsSpriteIndexValid()) {
		TPictureIO *sprite = _currentAnimation->GetCurrentSprite();

		if (sprite) {
			sprite->RefreshSprite(false);
			_currentAnimation->SetPosition(centredCorner(_centeredPosition, *sprite), -1.0f);
		}
	} else {
		_sprite.RefreshSprite(false);
		_sprite.SetPosition(centredCorner(_centeredPosition, _sprite), -1.0f);
	}

	TManagedObject::Draw();
}

// Confirmed (asm lines 260959-261023): the first active interface that has a place holder
// for the item gets the event.
void TGItem::HandlePreExecution(TGEventInfo &info) {
	std::list<TGInterface *> interfaces = gameControl()->GetActiveInterfaces();

	for (TGInterface *shown : interfaces) {
		TGPlaceHolder *placeHolder = shown->GetPlaceHolder(this);

		if (placeHolder) {
			placeHolder->ExecuteEvent(info);
			break;
		}
	}
}

// Confirmed (asm lines 260668-260805): a click (mouse event 1, 3 or 4) that no action was
// run for. With the game's draggable items on and a draggable command the item is the one
// that moves with the cursor; the commands 1 and 2 (use) take the item as the used one.
void TGItem::HandlePostExecution(TGEventInfo &info, const TGActionInfo &actionInfo) {
	bool click = (info.mouseEvent == 1 || info.mouseEvent == 3 || info.mouseEvent == 4);

	if (actionInfo.flag4 || !info.action.IsEmpty() || !click) {
		TManagedObject::HandlePostExecution(info, actionInfo);
		return;
	}

	TVisObjRef game = _objRef.GetVisionaire()->GetGame();
	bool dragged = false;

	if (game.GetBool(kGameDraggableItems) && info.command.GetBool(kButtonDraggable)) {
		g_pGameControl->GetCursorControl()->SetMoveObject(_objRef);
		dragged = true;
	}

	int command = info.command.GetInt(kButtonCommandType);

	if (command == 1 || command == 2 || dragged) {
		game.SetLink(kGameUsedItem, _objRef, true);
		game.SetValue(kGameUsedItemPicked, dragged, TSendEventEnum::kSendEvent);
		return;
	}

	TManagedObject::HandlePostExecution(info, actionInfo);
}

// Confirmed (asm lines 261129-261190)
void TGItem::StartAnimation() {
	if (!IsActive())
		return;

	TVisObjRef animation = _objRef.GetLink(kObjectAnimation);

	if (animation.IsEmpty() || _currentAnimation)
		return;

	SetActive(false);
	SetActive(true);
}

// Confirmed (asm lines 261228-261289)
wxPoint TGItem::GetPositionNextToItem() const {
	TPictureIO *sprite = const_cast<TPictureIO *>(&_sprite);
	wxPoint position = _centeredPosition;

	if (_currentAnimation && _currentAnimation->IsSpriteIndexValid()) {
		sprite = _currentAnimation->GetCurrentSprite();

		if (!sprite)
			return position;
	}

	sprite->RefreshSprite(false);
	position.x += (int)sprite->GetSizedWidth() / 2;
	return position;
}
