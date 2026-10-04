#include "vstables/records.h"

#include "TSprite.h"
#include "vstables/fieldIds.h"

// Confirmed (asm lines 1472334-1472440): sets or clears the mirrored flag of
// every sprite of the animation's sprite list.
void TTAnimation::SetMirrored(bool mirrored) {
	std::vector<TSprite> sprites;

	GetSprites(kAnimationSprites, sprites);
	for (TSprite &sprite : sprites)
		sprite.SetMirrored(mirrored);
	SetValue(kAnimationSprites, sprites, TSendEventEnum::kSendEvent);
}
