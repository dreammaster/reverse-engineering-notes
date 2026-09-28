#include "TSpriteHandle.h"

void TSpriteHandle::AddRef() {
	++_refCount;
}

void TSpriteHandle::Release() {
	if (_refCount > 0)
		--_refCount;
}

int TSpriteHandle::GetRefCount() const {
	return _refCount;
}
