#include "vstables/records.h"

#include <vector>

#include "TCharHolder.h"
#include "TSprite.h"
#include "vstables/fieldIds.h"

// Confirmed (asm lines 1472440-1472628)
bool TTAnimation::IsModelAnimation(const TVisObjRef &animation) {
	TVisObjRef parent = animation.GetParent();

	if (parent.GetId()[3] != 0x11)
		return false;
	return !parent.GetPath(kOutfitModel).GetName().IsEmpty();
}

// Confirmed (asm lines 1472629-1472760)
bool TTAnimation::IsBonesAnimation(const TVisObjRef &animation) {
	TVisObjRef parent = animation.GetParent();

	if (parent.GetId()[3] == 0x11) {
		std::vector<TCharHolder> paths;

		parent.GetPaths(kOutfitModelFiles, paths);
		return !paths.empty();
	}
	if (parent.GetId()[3] == 6)
		return !parent.GetPath(kObjectModel).GetName().IsEmpty();
	return false;
}

// Confirmed (asm lines 1472334-1472440): sets or clears the mirrored flag of
// every sprite of the animation's sprite list.
void TTAnimation::SetMirrored(bool mirrored) {
	std::vector<TSprite> sprites;

	GetSprites(kAnimationSprites, sprites);
	for (TSprite &sprite : sprites)
		sprite.SetMirrored(mirrored);
	SetValue(kAnimationSprites, sprites, TSendEventEnum::kSendEvent);
}
