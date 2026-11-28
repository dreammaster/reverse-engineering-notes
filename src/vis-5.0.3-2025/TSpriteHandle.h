// Not yet assert-confirmed to a specific file; stays at the top level.
// A ref-counted handle to a cached GPU sprite/texture; TPictureIO holds one
// while it has a loaded image resident. Confirmed members from usage:
// AddRef/Release/GetRefCount, and fields at +0x2C/+0x30 (image width/height,
// read by LoadSpriteFromCache).
//
// AddRef()/Release()/GetRefCount() (Deponia_Linux.asm lines 772739-772780)
// are confirmed to operate on a refcount at the real object's +0x78 -
// Release() decrements UNCONDITIONALLY, with no zero-guard (this stub had
// added one; removed to match). GetMemorySize() (asm lines 772788-772797)
// reads a plain int at +0x00, confirmed but not previously implemented.
//
// The destructor (asm lines 772635-772731) reveals this class is
// considerably bigger and more complex than modeled here: it removes
// itself from a static global registry (`TSpriteHandle::s_all`, a
// vector-like structure paired with a global end-pointer), frees a
// dynamically-sized collection of individually-owned pointers at +0x08
// (matching AddSpritePart(TSpritePartHandle const&) from the manifest - a
// not-yet-modeled companion type), a separate owned buffer at +0x20, and a
// separate owned object at +0x60, before freeing its own +0x08 buffer.
// AddSpritePart() itself (manifest line ~775729) wasn't reached this pass -
// this needs its own dedicated pass (TSpritePartHandle's own shape,
// AddSpritePart()'s logic, and the ctor/static-registry bookkeeping) rather
// than being guessed at here.
#pragma once

#include <vector>

/** One part of the texture of a sprite (the sprites are cut up when a texture cannot be as big as they are): the
 *  rectangle of the part in the picture. The rest of it - the texture and its coordinates - is the backend's. */
struct TSpritePartHandle {
	int left = 0;
	int top = 0;
	int right = 0;
	int bottom = 0;
};

class TSpriteHandle {
public:
	void AddRef();
	void Release();
	int GetRefCount() const;
	int GetMemorySize() const {
		return _memorySize;
	}
	void SetMemorySize(int size) {
		_memorySize = size;
	}

	~TSpriteHandle();

	/** +0x20: a bit for every 4th pixel of every 4th line, set where the picture is transparent (see
	 *  TPictureMEM::CreateTransparencyBitmap()); `bitmapRowBytes` (+0x28) bytes to a line. Null: no hit testing by pixel. */
	unsigned char *transparencyBitmap = nullptr;
	int bitmapRowBytes = 0;
	int width = 0;   // +0x2C
	int height = 0;  // +0x30
	/** +0x08: the parts (owned by the handle in the original; a backend that cuts sprites up fills this). */
	std::vector<TSpritePartHandle *> parts;

private:
	int _memorySize = 0;
	int _refCount = 1;  // +0x78 (the constructor makes it 1: the reference of whoever made the handle)
};
