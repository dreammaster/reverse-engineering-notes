// Reconstructed from Deponia_Linux.asm (MaxRectsBinPack, asm 773548-777492 and 775426-775857: 20 methods): Jukka Jylanki's
// MaxRects bin packer (public domain) in the version that the game has - rectangles are wxRect. A bin of a width and a height
// is filled with rectangles one after the other; `_free` are the rectangles that are still free (they overlap), `_used`
// the ones that were placed.
//
// Reconstructed: Init(), Clear(), Occupancy(), PlaceRect(), SplitFreeNode(), PruneFreeList(), IsContainedIn() and
// FindPositionForNewNodeBottomLeft() - what the game uses (the letters of a font are packed with the bottom-left rule).
// Not reconstructed: Insert() and the other rules (best short side, best long side, best area, contact point), FreeRect()
// with the joining of the free rectangles (Adjacent/Combine/ExtendJoin: the game's own addition), and FindGreatestRect().
#pragma once

#include <vector>

#include "WxStub.h"

class MaxRectsBinPack {
public:
	MaxRectsBinPack() = default;

	/** The bin is `width` x `height`, with the rectangles that may be turned (`allowFlip`); it is empty. */
	void Init(int width, int height, bool allowFlip);
	/** Takes all of the rectangles out. */
	void Clear();
	/** How much of the bin the placed rectangles fill (0 to 1). */
	float Occupancy();

	/** The place for a `width` x `height` rectangle that has the lowest top side (and then the lowest left side); `bestY`
	 *  is the top side (the y + height), `bestX` the left side. The rectangle (empty if there is no room) is returned. */
	wxRect FindPositionForNewNodeBottomLeft(int width, int height, int &bestY, int &bestX);
	/** The rectangle is taken from the free ones, it is a placed one. */
	void PlaceRect(wxRect node);

	int _binWidth = 0;                 // +0x00
	int _binHeight = 0;                // +0x04
	bool _allowFlip = false;           // +0x08
	std::vector<wxRect> _used;         // +0x10
	std::vector<wxRect> _free;         // +0x28

private:
	/** The free rectangle `freeNode` is cut up round `usedNode` (the pieces are added to _free); false if they do not touch. */
	bool SplitFreeNode(wxRect freeNode, wxRect &usedNode);
	/** Takes away the free rectangles that are in other free ones. */
	void PruneFreeList();
	static bool IsContainedIn(const wxRect &a, const wxRect &b);
};
