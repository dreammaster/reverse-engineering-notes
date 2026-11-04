#!/usr/bin/env python3
"""
Run an unpacked Cadaver executable in the dosemu harness, feeding keys, and
snapshot VGA frames / full memory.

  tools/emu_run.py <game_dir> <exe> <out_prefix> <total_Minstr> [keys...]

  keys: hex words (scancode<<8|ascii), e.g. 0231 = '1', 011b = Esc.
Writes <out_prefix>_<n>.png every 25M instructions, <out_prefix>_mem.bin at end.
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from dosemu import Emu

game, exe, prefix, total = sys.argv[1], sys.argv[2], sys.argv[3], int(float(sys.argv[4]) * 1e6)
e = Emu(exe, game)
e.pending_keys = [int(k, 16) for k in sys.argv[5:]]
done, n = 0, 0
step = 25_000_000
while done < total and e.exit_code is None:
    if not e.run(max_instr=step):
        break
    done += step
    n += 1
    e.save_frame('%s_%d.png' % (prefix, n))
    print('%dM instr  ip %04x:%04x  ints %s' % (done // 1_000_000, e.r('cs'), e.r('ip'),
          {hex(k): v for k, v in e.int_log.items()}), flush=True)
open(prefix + '_mem.bin', 'wb').write(bytes(e.uc.mem_read(0, 0xA0000)))
open(prefix + '_vram.bin', 'wb').write(b''.join(bytes(p) for p in e.vga.planes))
