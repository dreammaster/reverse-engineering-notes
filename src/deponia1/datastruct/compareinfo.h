// Not yet assert-confirmed to a specific file; stays alongside datagrp.h.
//
// What TDataGroup::ForEachCall(kCompare, ...) reports: one entry per field in
// which two records differ (Deponia_Linux.asm lines 603378-603500; TDataCompareInfo
// is the symbol of the vector element type, 0x18 bytes - the field id and the
// two values as text, the id being -1 for a difference in the record's name).
// Member names are invented.
//
// TVisObjectCompareInfo is the result holder the caller (TVisionaire::
// MergeObjects, TTable::MergeObjects) passes in; ForEachCall() only touches
// the list at +0x18. The rest of the struct isn't reversed yet.
#pragma once

#include <vector>

#include "WxStub.h"

struct TDataCompareInfo {
	int description = -1;
	wxString thisText;
	wxString otherText;
};

struct TVisObjectCompareInfo {
	std::vector<TDataCompareInfo> differences;
};
