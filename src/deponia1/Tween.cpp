#include "Tween.h"

void Tween::Update(float /*deltaMs*/) {
}

bool Tween::IsFinished() const {
	return false;
}

bool TVisObjTween::update(double /*deltaMs*/) {
	return true;
}
