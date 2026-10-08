/* A small 8086/80186 interpreter.  It runs the game's sound driver modules (ADLIB.DRV ...) unchanged: the driver code lives in
 * the same flat memory as the recompiled game (segment:offset addressing as in real mode), port I/O goes to callbacks. */
#ifndef MM3_X86_H
#define MM3_X86_H

#include <stdint.h>

typedef struct X86 X86;
struct X86 {
	uint8_t *mem;       /* 1 MB + 64 KB of flat memory (real-mode addresses are linear = seg*16 + off, no wrap at 1 MB) */
	uint16_t ax, cx, dx, bx, sp, bp, si, di;
	uint16_t es, cs, ss, ds, ip;
	uint16_t flags;     /* bit 0 CF, 2 PF, 4 AF, 6 ZF, 7 SF, 8 TF, 9 IF, 10 DF, 11 OF */
	int halted;
	void *io_user;
	void (*out8)(void *user, uint16_t port, uint8_t value);
	uint8_t (*in8)(void *user, uint16_t port);
	void (*intr)(X86 *cpu, uint8_t vector); /* software interrupt hook (int 21h ...); NULL = vector through the IVT */
	unsigned long steps;
};

/* Runs until the far return address on the stack equals `sentinel_cs:sentinel_ip` is reached (a retf/iret popped it), or
 * `max_steps` instructions have run.  Returns 0 on success, -1 on an unknown opcode / step limit. */
int x86_run(X86 *cpu, uint16_t stop_cs, uint16_t stop_ip, unsigned long max_steps);
/* Far call from the host: pushes `nargs` words (args[0] ends up lowest, like cdecl), a far return address to the stop point, and runs. */
int x86_call_far(X86 *cpu, uint16_t seg, uint16_t off, const uint16_t *args, int nargs, unsigned long max_steps);

#endif
