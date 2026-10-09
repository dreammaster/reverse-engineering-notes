#include "graphicslib/spriteCache.h"

#include "Diagnostics.h"
#include "TSpriteHandle.h"
#include "graphicslib/graphics.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/graphicslib/spriteCache.cpp";

TSpriteCache::~TSpriteCache() {
	Clear();
}

// Confirmed (asm lines 749371-749500)
void TSpriteCache::SetCacheSize(int megabytes) {
	if (megabytes <= 19) {
		if (wxLog::loglevel > 0)
			wxLog::logexpanded(L"Invalid cache size. Cache size set to minimum size of 20 MB.");

		_cacheSize = 20LL << 20;
		_cacheLimitFifo = 18LL << 20;
		return;
	}

	if (megabytes > 500) {
		if (wxLog::loglevel > 0)
			wxLog::logexpanded(L"Invalid cache size. Cache size set to maximum size of 500 MB.");

		_cacheSize = 500LL << 20;
		_cacheLimitFifo = 450LL << 20;
		return;
	}

	_cacheSize = static_cast<long long>(megabytes) << 20;

	long long limit = static_cast<int>(static_cast<double>(megabytes) * 0.9);

	if (limit <= 9)
		limit = 10;

	_cacheLimitFifo = limit << 20;
}

// Confirmed (asm lines 749521-749538)
size_t TSpriteCache::GetSpriteCount() const {
	return _sprites.size();
}

// Confirmed (asm lines 749538-749612): the place in the (case-insensitive) order is looked for, and then the name has to be
// the same in the case too.
TSpriteHandle *TSpriteCache::GetSprite(const wxString &name) {
	Map::iterator it = _sprites.lower_bound(name);

	if (it != _sprites.end() && name.Cmp(it->first) == 0)
		return it->second;

	return nullptr;
}

// Confirmed (asm lines 749612-749659)
int TSpriteCache::GetUsedSize() {
	int size = 0;

	for (auto &entry : _sprites) {
		if (entry.second->GetRefCount() > 1)
			size += entry.second->GetMemorySize();
	}

	return size;
}

// Confirmed (asm lines 749659-749716)
int TSpriteCache::GetUnusedSize() {
	int size = 0;

	for (auto &entry : _sprites) {
		if (entry.second->GetRefCount() == 1)
			size += entry.second->GetMemorySize();
	}

	return size;
}

static wxString number(long long value) {
	return wxString(std::to_wstring(value));
}

// Confirmed (asm lines 749716-751056): the sprites that nobody else uses, in the order of the queue, then the ones in use.
void TSpriteCache::PrintCacheContents(std::list<wxString> &lines) {
	int unused = GetUnusedSize();

	lines.push_back(wxString(L"Total number of sprites in cache: ") + number(static_cast<long long>(_sprites.size())));
	lines.push_back(wxString(L""));
	lines.push_back(wxString(L"Sprites which are not used anymore but still in cache:"));
	lines.push_back(wxString(L"Used: ") + number(unused >> 10) + wxString(L" KB, Free: ") +
	                number((_cacheSize - unused) >> 10) + wxString(L" KB"));

	for (Map::iterator entry : _queue) {
		if (entry->second->GetRefCount() == 1)
			lines.push_back(entry->first + wxString(L" (") + number(entry->second->GetMemorySize() >> 10) + wxString(L" KB)"));
	}

	lines.push_back(wxString(L""));
	lines.push_back(wxString(L"Sprites which are currently used:"));

	int usedKb = 0;

	for (Map::iterator entry : _queue) {
		if (entry->second->GetRefCount() > 1)
			usedKb += entry->second->GetMemorySize() >> 10;
	}

	lines.push_back(wxString(L"Used: ") + number(usedKb) + wxString(L" KB"));

	for (Map::iterator entry : _queue) {
		if (entry->second->GetRefCount() > 1)
			lines.push_back(entry->first + wxString(L" (") + number(entry->second->GetMemorySize() >> 10) + wxString(L" KB)"));
	}
}

void TSpriteCache::eraseFromQueue(Map::iterator entry) {
	for (auto it = _queue.begin(); it != _queue.end(); ++it) {
		if (*it == entry) {
			_queue.erase(it);
			return;
		}
	}
}

// Confirmed (asm lines 751056-751175): the sprite loses the reference of the cache (the backend frees it at 0).
void TSpriteCache::removeEntry(Map::iterator entry) {
	TSpriteHandle *sprite = entry->second;

	_usedSize -= sprite->GetMemorySize();
	sprite->Release();

	if (sprite->GetRefCount() == 0)
		graphics->OnSpriteHandleReleased(sprite);

	eraseFromQueue(entry);
	_sprites.erase(entry);
}

// Confirmed (asm lines 751175-751254)
bool TSpriteCache::RemoveFromCache(const wxString &name) {
	Map::iterator it = _sprites.lower_bound(name);

	if (it == _sprites.end() || name.CmpNoCase(it->first) != 0)
		return false;

	removeEntry(it);
	return true;
}

// Confirmed (asm lines 751254-751613). Nothing is done while the unused sprites take no more than the cache. The queue is
// walked from the oldest: a sprite that nobody else uses is thrown out (the backend frees it), one that is used is passed, until
// the unused ones are no more than the limit or the queue is through.
bool TSpriteCache::UpdateCache() {
	long long unused = GetUnusedSize();

	if (!(_cacheSize < unused))
		return false;

	x_assert(!_queue.empty(), "!m_vFifo.empty()", kSourceFile, 0xB3);

	if (_queue.empty())
		return false;

	int skipped = 1;

	while (unused > _cacheLimitFifo) {
		Map::iterator entry = _queue[_queue.size() - skipped];
		TSpriteHandle *sprite = entry->second;

		if (!sprite) {
			x_assert(false, "false", kSourceFile, 0xCD);

			if (wxLog::loglevel > 0)
				wxLog::logexpanded(L"Cache contains empty entry!");

			eraseFromQueue(entry);
			_sprites.erase(entry);
		} else if (sprite->GetRefCount() == 1) {
			_usedSize -= sprite->GetMemorySize();
			unused -= sprite->GetMemorySize();
			graphics->OnSpriteHandleReleased(sprite);
			eraseFromQueue(entry);
			_sprites.erase(entry);
		} else {
			skipped++;
		}

		if (skipped >= static_cast<int>(_queue.size())) {
			x_assert(_cacheLimitFifo >= unused, "cacheSizeUnusedSprites <= m_cacheLimitFull", kSourceFile, 0xD6);
			return true;
		}
	}

	return true;
}

// Confirmed (asm lines 751613-751713)
void TSpriteCache::Clear() {
	_cleared = true;

	if (!_sprites.empty())
		graphics->FinishDraw();

	for (auto &entry : _sprites) {
		if (entry.second)
			graphics->OnSpriteHandleReleased(entry.second);
	}

	_sprites.clear();
	_queue.clear();
}

// Confirmed (asm lines 751855-752058)
void TSpriteCache::AddToCache(const wxString &name, TSpriteHandle *sprite) {
	Map::iterator it = _sprites.lower_bound(name);

	if (it != _sprites.end() && name.Cmp(it->first) == 0)
		return;

	sprite->AddRef();

	std::pair<Map::iterator, bool> inserted = _sprites.insert(std::make_pair(name, sprite));

	if (inserted.second) {
		_queue.push_front(inserted.first);
		_usedSize += sprite->GetMemorySize();
	}
}
