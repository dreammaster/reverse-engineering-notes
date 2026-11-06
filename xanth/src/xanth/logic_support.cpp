#include "xanth/logic/logic.h"

namespace Xanth {

// Text of the original program's data segment that the logic prints. The table is generated from the original executable by reversing/scripts/gen_logic.py into
// xanth/logic/engine_strings.inc; that file contains text of the game and is therefore kept out of the repository (see .gitignore).
static const char *const kEngineStrings[] = {
#include "xanth/logic/engine_strings.inc"
    ""};

char *engineString(unsigned index) {
	return const_cast<char *>(index < ARRAYSIZE(kEngineStrings) - 1 ? kEngineStrings[index] : "");
}

} // namespace Xanth
