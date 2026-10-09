// Reconstructed from Deponia_Linux.asm (TSpriteCache, asm 749371-752058: 13 methods; path of the original, from the x_assert()s:
// src/graphicslib/...). The cache of the sprites (the textures of the pictures) that the graphics backend keeps, in the base
// class TGraphicsInterface (+0x08). A sprite is found by the name of the picture (TPictureIO::GetSpriteName(): the path and the
// settings of the transparency). The cache holds one reference of each sprite it has; a sprite with no other reference is
// "unused" and may be thrown out when the unused ones take more than the cache allows (UpdateCache(), called every frame): the
// oldest are thrown out first until what is left is no more than `_cacheLimitFifo`.
//
// Layout: +0x08 the map (a case-insensitive order, `cmpPathStr`), +0x30 the queue of the entries (the newest first),
// +0x80 how many bytes the sprites in the cache take, +0x88 the size of the cache, +0x90 what is left of the unused ones when it
// is cleaned up, +0x98 set by Clear().
#pragma once

#include <deque>
#include <list>
#include <map>

#include "WxStub.h"

class TSpriteHandle;

class TSpriteCache {
public:
	TSpriteCache() = default;
	~TSpriteCache();

	/** The size of the cache in megabytes (20 to 500; the sizes outside are logged and the nearest one is used). The limit that
	 *  is left when it is cleaned up is 90% of it (at least 10 MB). */
	void SetCacheSize(int megabytes);
	/** How many sprites are in the cache. */
	size_t GetSpriteCount() const;
	/** The sprite with this name (null: none; the name has to be the same in the case of the letters too). */
	TSpriteHandle *GetSprite(const wxString &name);
	/** How many bytes the sprites that others use take, and the ones that nobody else uses. */
	int GetUsedSize();
	int GetUnusedSize();
	/** The text lines of the debugger's "cache contents". */
	void PrintCacheContents(std::list<wxString> &lines);
	/** Takes the sprite out (it is let go of; at 0 references the backend frees it). */
	bool RemoveFromCache(const wxString &name);
	/** Throws out the oldest unused sprites when they take more than the cache; true if it did. */
	bool UpdateCache();
	/** Lets go of all of the sprites. */
	void Clear();
	/** Puts the sprite in, with one reference more (nothing if a sprite of the name is there). */
	void AddToCache(const wxString &name, TSpriteHandle *sprite);

private:
	struct PathLess {
		bool operator()(const wxString &a, const wxString &b) const {
			return a.CmpNoCase(b) < 0;
		}
	};
	typedef std::map<wxString, TSpriteHandle *, PathLess> Map;

	void removeEntry(Map::iterator entry);
	void eraseFromQueue(Map::iterator entry);

	Map _sprites;                           // +0x08
	std::deque<Map::iterator> _queue;       // +0x30, the newest first
	long long _usedSize = 0;                // +0x80
	long long _cacheSize = 0x2800000;       // +0x88, 40 MB
	long long _cacheLimitFifo = 0x1E00000;  // +0x90, 30 MB
	bool _cleared = false;                  // +0x98
};
