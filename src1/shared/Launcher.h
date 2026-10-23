/* Shared::Launcher -- how one program starts another. Where the original EXECs a program and reads its exit code, a ported program
 * calls a Launcher (SW.EXE's shell implements it; PROG3 uses it to start PROG5 and PROG6, and so on). It lives in Shared so that
 * no program has to know another program's namespace. */

#ifndef YENDOR1_SHARED_LAUNCHER_H
#define YENDOR1_SHARED_LAUNCHER_H

#include "shared/Params.h"
#include "shared/Runtime.h"

namespace Yendor1 {
namespace Shared {

/** Runs one of the game's programs (PROG0..PROG18) and returns its exit code. The parameter block is the child's own copy of the 127
 *  byte command tail; what the child writes into it is not seen by the parent, as with DOS EXEC. */
class Launcher {
public:
	enum { kNotImplemented = -1 };
	virtual ~Launcher() {}
	virtual int run(int program, Params &params, Runtime &rt) = 0;
};

} // namespace Shared
} // namespace Yendor1

#endif
