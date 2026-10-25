// KANJIREA:13 (+14, 15) -- the title screen: the Wizardry logo bitmap, the
// "WELCOME TO THE WORLD OF WIZARDRY!" line, the S)TART / M)AKE dialog, and the
// wandering monster portraits that appear and dissolve until a key is pressed.
#pragma once
#include "wiz/dos.h"
#include "wiz/rng.h"

namespace wiz {

enum class TitleResult { Start, MakeDisk, Quit };

// `titleFile` = the raw 200.TITLE bytes.  Returns the player's choice.  M)AKE
// SCENARIO DISK has no meaning for a standalone engine, so callers treat it as
// "stay on the title" (see runTitle's `allowMake`).
TitleResult runTitle(Dos &d, Rng &rng, const std::vector<u8> &titleFile);

} // namespace wiz
