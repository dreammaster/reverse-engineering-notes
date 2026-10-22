#pragma once
// Adapters between the register calling convention of the recompiled code and the hand written ports.
//
// A port replaces one generated function (listed in tools/recomp/ports.txt). It receives the machine and the register
// set exactly as the original function would see them and leaves the results in the registers the callers read; the
// return address is handled by the generated wrapper. Registers and flags the original leaves undefined are listed as
// `junk` in ports.txt and not compared by wlrctest.
#include "wasteland1/recomp_rt.h"

#define PORT(name) void port_##name(::wl::Machine &m, ::wl::rc::Regs &r)
