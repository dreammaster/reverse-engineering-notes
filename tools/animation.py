#!/usr/bin/env python3
"""
Sprite animation stepper = CAD.EXE sub_13BC5 (1000:3BC5) + the command table at 1000:3C77.  docs/formats.md 6.12.

State (the 10 bytes at [position record + record[+0Eh]], initial values are the thing record's tail from +16):
    delay (b0)  frame (b1)  dir (b2)  pos (b3)  dx (w4)  dy (w6)  cnt (b8)  cad (b9)
One step per game loop iteration (every 2 timer ticks = 25 Hz).  The script is a list of u16be words: low byte = frame number
(when the high byte is 0) -- more precisely high byte = command (0 = plain frame, 0xFF-0xF3 = command), low byte = argument/frame.
"""
import struct


def step(st, script, offsets, events=None):
    """st: dict(delay, frame, dir, pos, cnt, cad); script: list of (cmd, arg) pairs; offsets: [(dx, dy)] per frame.
    Returns nothing; updates st (and st['dx'], st['dy']).  Event commands are appended to `events` as (cmd, arg)."""
    n = len(script)
    if n == 0:
        return
    pos = st['pos']
    for _ in range(4 * n + 8):
        if not 0 <= pos < n:
            pos = 0
        if st['delay'] != 0:
            break
        cmd, arg = script[pos]
        if cmd == 0:
            break
        r = _command(st, cmd, arg, pos, events)
        pos = st['_pos'] & 0xFF
        st['pos'] = pos
        if r != 0:
            break
    if not 0 <= pos < n:
        pos = 0
    delay = st['delay']
    advance = False
    if delay == 0:
        st['frame'] = script[pos][1]
        advance = True
    elif delay < 0x80:
        st['delay'] = delay - 1
        advance = st['delay'] == 0
    # delay >= 0x80 (FE/FF): frozen, no advance
    if advance:
        if st['dir'] == 0:
            pos += 1
        else:
            pos -= 1
            if pos == 0:
                st['dir'] = 0
        pos &= 0xFF
        if st['cad'] == 0:
            st['pos'] = pos
        else:
            st['cnt'] -= 1
            if st['cnt'] < 0:
                st['cnt'] = st['cad']
                st['pos'] = pos
    st['dx'], st['dy'] = offsets[st['frame']] if st['frame'] < len(offsets) else (0, 0)


def _command(st, cmd, arg, pos, events):
    """The handlers of the table at 1000:3C77 (index 0xFF-cmd).  Sets st['_pos']; returns AL (0 = go on, else stop)."""
    st['_pos'] = pos
    idx = 0xFF - cmd
    if idx == 0:                      # FF: freeze
        st['delay'] = 0xFF
        return 1
    if idx == 1:                      # FE: wait `arg` steps
        st['delay'] = arg
        return 1
    if idx == 2:                      # FD: loop to the start
        st['_pos'] = 0
        return 0
    if idx == 3:                      # FC: jump to `arg`
        st['_pos'] = arg
        return 0
    if idx == 4:                      # FB: run backwards
        st['dir'] = 1
        st['_pos'] = pos - 2
        return 0
    if idx == 5:                      # FA: turn round
        if st['dir']:
            st['_pos'] = pos + 1
        st['_pos'] += 1
        st['dir'] = 0
        return 0
    if idx == 6:                      # F9: freeze (0xFE) and fire a trigger
        st['delay'] = 0xFE
        if events is not None:
            events.append((cmd, arg))
        return 1
    # F8.. : event commands, skipped here
    if events is not None:
        events.append((cmd, arg))
    st['_pos'] = pos + 1
    return 0
