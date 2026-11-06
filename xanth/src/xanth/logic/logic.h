/* Common header of the generated game logic (the .cpp files in this folder are written by reversing/scripts/gen_logic.py from the original program).
 *
 * Every function takes the Game and the arguments the original pushed: handlers get the phase, exit functions the direction, conversation callbacks the
 * conversation, line and option. They return what the original returns in ax (handlers: non-zero = the command was handled).
 */
#ifndef XANTH_LOGIC_H
#define XANTH_LOGIC_H

#include "xanth/game.h"

namespace Xanth {

typedef int (*HandlerFn)(Game &g, int arg);

struct VerbHandler {
	int verb;
	HandlerFn fn;
};

/** Objects whose name depends on the game state: the callback gets phase 0x10 and the object and returns the name (or nothing, for the name in the object record). */
struct NameCallback {
	int obj;
	TextRef (*fn)(Game &g, Word phase, Word obj);
};
extern const NameCallback kNameCallbacks[];
extern const unsigned kNameCallbackCount;

extern const HandlerFn kObjectHandlers[448];
extern const HandlerFn kExitFunctions[];
extern const HandlerFn kEventFunctions[];
extern const unsigned kExitFunctionCount, kEventFunctionCount;
extern const VerbHandler kVerbHandlers[];
extern const unsigned kVerbHandlerCount;

/** Text of the original's data segment that the logic prints (engine_strings.inc is generated locally from the original program and is not part of the repository). */
char *engineString(unsigned index);
#define ES(n) (::Xanth::engineString(n))
#define ES_UNKNOWN(n) (::Xanth::engineString(0xFFFF))
#define DSEG(off) (::Xanth::engineString(0xFFFF))

/** Address of a routine passed as a callback. */
#define FN(f) (&(f))

// Constructs the decompiler could not turn into plain C++ yet (the functions that use them are listed in manual_review.txt); they compile but do not work.
#define objectType(o) (g.state.objects.type(o))
#define DS 0x84FC
#define SEG(x) 0
#define sext(x) ((int16)(x))
#define hi(x) 0
#define mem8(seg, off) (g.exe8((seg), (off)))
#define mem16(seg, off) (g.exe16((seg), (off)))
#define mem32(seg, off) (g.exe16((seg), (off)))
#define callIndirect(...) (g.other("callIndirect"))

#include "xanth/logic/functions.inc"

} // namespace Xanth

#endif
