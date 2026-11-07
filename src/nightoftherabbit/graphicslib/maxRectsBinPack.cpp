#include "graphicslib/maxRectsBinPack.h"

#include <climits>

// Confirmed (asm lines 775803-775857)
void MaxRectsBinPack::Init(int width, int height, bool allowFlip) {
	_binWidth = width;
	_binHeight = height;
	_allowFlip = allowFlip;

	_used.clear();
	_free.clear();
	_free.push_back(wxRect{0, 0, width, height});
}

// Confirmed (asm lines 775426-775445)
void MaxRectsBinPack::Clear() {
	_used.clear();
	_free.clear();
}

// Confirmed (asm lines 773548-773671)
float MaxRectsBinPack::Occupancy() {
	unsigned long long usedSurfaceArea = 0;

	for (const wxRect &rect : _used)
		usedSurfaceArea += static_cast<unsigned long long>(static_cast<unsigned>(rect.width * rect.height));

	return static_cast<float>(usedSurfaceArea) / static_cast<float>(_binWidth * _binHeight);
}

// Confirmed (asm lines 773671-773846)
wxRect MaxRectsBinPack::FindPositionForNewNodeBottomLeft(int width, int height, int &bestY, int &bestX) {
	wxRect bestNode;

	bestY = INT_MAX;

	for (const wxRect &freeRect : _free) {
		// try to place the rectangle upright
		if (freeRect.width >= width && freeRect.height >= height) {
			int topSideY = freeRect.y + height;

			if (topSideY < bestY || (topSideY == bestY && freeRect.x < bestX)) {
				bestNode.x = freeRect.x;
				bestNode.y = freeRect.y;
				bestNode.width = width;
				bestNode.height = height;
				bestY = topSideY;
				bestX = freeRect.x;
			}
		}

		// then turned
		if (_allowFlip && freeRect.width >= height && freeRect.height >= width) {
			int topSideY = freeRect.y + width;

			if (topSideY < bestY || (topSideY == bestY && freeRect.x < bestX)) {
				bestNode.x = freeRect.x;
				bestNode.y = freeRect.y;
				bestNode.width = height;
				bestNode.height = width;
				bestY = topSideY;
				bestX = freeRect.x;
			}
		}
	}

	return bestNode;
}

// Confirmed (asm lines 777028-777148)
void MaxRectsBinPack::PlaceRect(wxRect node) {
	size_t numRectanglesToProcess = _free.size();

	for (size_t i = 0; i < numRectanglesToProcess; i++) {
		if (SplitFreeNode(_free[i], node)) {
			_free.erase(_free.begin() + i);
			i--;
			numRectanglesToProcess--;
		}
	}

	PruneFreeList();
	_used.push_back(node);
}

// Confirmed (asm lines 776789-777028)
bool MaxRectsBinPack::SplitFreeNode(wxRect freeNode, wxRect &usedNode) {
	// the rectangles have to touch
	if (usedNode.x >= freeNode.x + freeNode.width || usedNode.x + usedNode.width <= freeNode.x ||
	    usedNode.y >= freeNode.y + freeNode.height || usedNode.y + usedNode.height <= freeNode.y)
		return false;

	if (usedNode.x < freeNode.x + freeNode.width && usedNode.x + usedNode.width > freeNode.x) {
		// a new node at the top side of the used node
		if (usedNode.y > freeNode.y && usedNode.y < freeNode.y + freeNode.height) {
			wxRect newNode = freeNode;

			newNode.height = usedNode.y - newNode.y;
			_free.push_back(newNode);
		}

		// a new node at the bottom side of the used node
		if (usedNode.y + usedNode.height < freeNode.y + freeNode.height) {
			wxRect newNode = freeNode;

			newNode.y = usedNode.y + usedNode.height;
			newNode.height = freeNode.y + freeNode.height - (usedNode.y + usedNode.height);
			_free.push_back(newNode);
		}
	}

	if (usedNode.y < freeNode.y + freeNode.height && usedNode.y + usedNode.height > freeNode.y) {
		// a new node at the left side of the used node
		if (usedNode.x > freeNode.x && usedNode.x < freeNode.x + freeNode.width) {
			wxRect newNode = freeNode;

			newNode.width = usedNode.x - newNode.x;
			_free.push_back(newNode);
		}

		// a new node at the right side of the used node
		if (usedNode.x + usedNode.width < freeNode.x + freeNode.width) {
			wxRect newNode = freeNode;

			newNode.x = usedNode.x + usedNode.width;
			newNode.width = freeNode.x + freeNode.width - (usedNode.x + usedNode.width);
			_free.push_back(newNode);
		}
	}

	return true;
}

// Confirmed (asm lines 775445-775609)
void MaxRectsBinPack::PruneFreeList() {
	for (size_t i = 0; i < _free.size(); i++) {
		for (size_t j = i + 1; j < _free.size(); j++) {
			if (IsContainedIn(_free[i], _free[j])) {
				_free.erase(_free.begin() + i);
				i--;
				break;
			}

			if (IsContainedIn(_free[j], _free[i])) {
				_free.erase(_free.begin() + j);
				j--;
			}
		}
	}
}

// Confirmed (asm lines 775609-775645)
bool MaxRectsBinPack::IsContainedIn(const wxRect &a, const wxRect &b) {
	return a.x >= b.x && a.y >= b.y && a.x + a.width <= b.x + b.width && a.y + a.height <= b.y + b.height;
}
