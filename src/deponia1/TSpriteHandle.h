// Not yet assert-confirmed to a specific file; stays at the top level.
// A ref-counted handle to a cached GPU sprite/texture; TPictureIO holds one
// while it has a loaded image resident. Confirmed members from usage:
// AddRef/Release/GetRefCount, and fields at +0x2C/+0x30 (image width/height,
// read by LoadSpriteFromCache).
#pragma once

class TSpriteHandle {
public:
    void AddRef();
    void Release();
    int GetRefCount() const;

    int width = 0;
    int height = 0;

private:
    int m_refCount = 0;
};
