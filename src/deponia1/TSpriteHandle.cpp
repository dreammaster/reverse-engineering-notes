#include "TSpriteHandle.h"

void TSpriteHandle::AddRef() {
    ++m_refCount;
}

void TSpriteHandle::Release() {
    if (m_refCount > 0)
        --m_refCount;
}

int TSpriteHandle::GetRefCount() const {
    return m_refCount;
}
