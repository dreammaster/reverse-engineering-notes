#include "vstables/records.h"

#include "TSprite.h"
#include "vstables/fieldIds.h"

// Confirmed (asm lines 1453570-1453607): moves the sprite stored in the record.
void TTSprite::SetPosition(const wxPoint &position) {
	TSprite sprite(GetSprite(kSpriteSprite));

	sprite.SetPosition(position, -1.0f);
	SetValue(kSpriteSprite, sprite, TSendEventEnum::kSendEvent);
}
