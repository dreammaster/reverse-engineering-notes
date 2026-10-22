#pragma once
// Wrappers for routines of the original program that are not ported yet. A ported routine calls them like ordinary C++
// functions; when the routine is ported its wrapper is replaced by the real implementation. Every wrapper names the
// address it stands for and the registers it takes and returns.
#include "wasteland1/machine.h"

namespace wl {
namespace legacy {

inline void call(Machine &m, uint16_t address, X86Regs &r) {
	if (!m.callLegacy) throw std::runtime_error("legacy routine called without the recompiled program");
	m.callLegacy(address, r);
}

} // namespace legacy
} // namespace wl
