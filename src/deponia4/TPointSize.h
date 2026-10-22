// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed in full (Deponia_Linux.asm lines 1390459-1391161, all 5 methods): the
// way system's size scaling. A character looks smaller the further up the scene
// it stands, so the way points that have a size (kPointSize) give a list of
// (y position, size in percent) sorted by y, and the size at any position is
// interpolated linearly between the two neighbouring entries (extrapolated past
// the first or last one; 100 when there are none, the one size when there is
// one). TGWaySystem derives from this and forwards to it.
#pragma once

#include <utility>
#include <vector>

#include "WxStub.h"

class TVList;

class TPointSize {
public:
	TPointSize();
	~TPointSize();

	/** Forgets all sizes. */
	void ClearSizeSystem();
	/** The size (percent) at the position's y. */
	float GetCalculatedSize(const wxPoint &position) const;
	/** Rebuilds the sizes from the way points (those that have a size). */
	void UpdateSizeSystem(TVList &points);

private:
	std::vector<std::pair<int, short> > _sizes;  // (y, size), sorted by y
};
