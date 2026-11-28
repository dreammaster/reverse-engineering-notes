#include "TSpriteHandle.h"

TSpriteHandle::~TSpriteHandle() {
	for (TSpritePartHandle *part : parts)
		delete part;

	delete[] transparencyBitmap;
}

void TSpriteHandle::AddRef() {
	++_refCount;
}

void TSpriteHandle::Release() {
	--_refCount;
}

int TSpriteHandle::GetRefCount() const {
	return _refCount;
}
