#include "TPointSize.h"

#include <algorithm>

#include "datastruct/vlist.h"
#include "datastruct/visionaireobject.h"
#include "vstables/fieldIds.h"

// Confirmed (asm lines 1390459-1390501)
TPointSize::TPointSize() {
}

TPointSize::~TPointSize() {
}

// Confirmed (asm lines 1390502-1390519)
void TPointSize::ClearSizeSystem() {
	_sizes.clear();
}

// Confirmed (asm lines 1390538-1390655)
float TPointSize::GetCalculatedSize(const wxPoint &position) const {
	size_t count = _sizes.size();

	if (count == 0)
		return 100.0f;
	if (count == 1)
		return (float)_sizes[0].second;

	// the two entries the position is between (the first or the last two when it
	// is above or below all of them)
	size_t upper = 1;

	if (position.y > _sizes[0].first) {
		while (upper < count - 1 && position.y > _sizes[upper].first)
			upper++;
	}
	size_t lower = upper - 1;

	float span = 1.0f;

	if (_sizes[upper].first - _sizes[lower].first != 0)
		span = (float)(_sizes[upper].first - _sizes[lower].first);

	float t = (float)(position.y - _sizes[lower].first) / span;

	return t * ((float)_sizes[upper].second - (float)_sizes[lower].second) + (float)_sizes[lower].second;
}

static bool pointsorter(const std::pair<int, short> &first, const std::pair<int, short> &second) {
	return first.first < second.first;
}

// Confirmed (asm lines 1390856-1391160)
void TPointSize::UpdateSizeSystem(TVList &points) {
	_sizes.clear();

	for (TVisionaireObject *point : points) {
		int size = point->GetInt(kPointSize);

		if (size == -1)
			continue;
		_sizes.push_back(std::make_pair(point->GetPoint(kPointPosition)->y, (short)size));
	}

	std::sort(_sizes.begin(), _sizes.end(), pointsorter);
}
