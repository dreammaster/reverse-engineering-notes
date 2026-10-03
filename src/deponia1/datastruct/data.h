// Original path not yet confirmed; stays alongside type.h in datastruct/.
//
// TData is the engine's typed-value storage helper. Only the static byte-
// size lookup is a confirmed call shape (TTypeGroup::AddType(), Deponia_
// Linux.asm line 585963+); not reversed beyond that.
#pragma once

#include "datastruct/type.h"

class TData {
public:
	// The number of bytes a field of the given data type occupies in a
	// record's storage.
	static int GetDataSize(eTypeData type);
};
