#include "xanth/verbs.h"

namespace Xanth {

static const VerbInfo kVerbs[] = {
#include "xanth/tables/verb_table.inc"
};

static const int kInitialVerbs[] = {
#include "xanth/tables/initial_verbs.inc"
};

const VerbInfo *findVerb(int id) {
	for (int i = 0; i < ARRAYSIZE(kVerbs); i++)
		if (kVerbs[i].id == id)
			return &kVerbs[i];
	return nullptr;
}

std::vector<int> initialVerbs() {
	return std::vector<int>(kInitialVerbs, kInitialVerbs + ARRAYSIZE(kInitialVerbs));
}

} // namespace Xanth
