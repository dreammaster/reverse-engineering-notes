/* The verb table of the original (89 verbs: id, name, the preposition that follows the first object, the number of objects) and the verbs the interface starts with. */
#ifndef XANTH_VERBS_H
#define XANTH_VERBS_H

#include "common/types.h"

namespace Xanth {

struct VerbInfo {
	int id;
	const char *name;
	const char *preposition;       // nullptr: none
	int objects;                   // 1 or 2
};

/** The verb with that id, or nullptr. */
const VerbInfo *findVerb(int id);
/** The ids of the verb list at the start of the game, in the order they are shown. */
std::vector<int> initialVerbs();

} // namespace Xanth

#endif
