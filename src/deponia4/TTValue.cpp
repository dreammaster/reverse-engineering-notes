#include "vstables/records.h"

#include <cstdlib>

#include "vstables/fieldIds.h"

// Confirmed (asm lines 1476759-1476801): a random integer in [minimum, maximum]
// (rand() modulo the span, so not evenly distributed) becomes the value.
void TTValue::SetRandomValue(int minimum, int maximum) {
	int span = maximum - minimum;

	SetValue(kValueInt, rand() % (span + 1) + minimum, TSendEventEnum::kSendEvent);
}
