#!/usr/bin/env python3
"""Run ROLAND.BIN (the Roland MT-32 / LAPC-1 driver of Cadaver 2) on tools/x86.py and write what it sends to the MPU-401 as a Standard MIDI File.

The driver is called like the other sound drivers: entry 0 = tick (50 Hz), entry 4 = init, command area at offsets 8..0xC ([8] music request,
[9] music playing, [0Ah] effect request).  It writes MIDI bytes to port 330h, preceded by the MPU-401 'send data to track n' commands D0h..D7h on port 331h.
Every tick is 20 ms: the file uses 25 ticks per quarter note at 500000 us per quarter.

    python roland_midi.py music N out.mid [seconds]     # song N (the driver's [8] request), default 60 s
    python roland_midi.py effect N out.mid [seconds]    # sound effect N ([0Ah] request)
"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import x86

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SEG = 0x3000
RET = (0x0100, 0x0000)


class Roland:
    def __init__(self, path=None):
        path = path or os.path.join(ROOT, 'unpacked', 'cadaver2', 'ROLAND.BIN')
        self.cpu = x86.CPU()
        d = open(path, 'rb').read()
        self.cpu.mem[SEG << 4:(SEG << 4) + len(d)] = d
        self.out = []                      # MIDI bytes of the current tick
        self.cpu.io_out = self.io_out
        self.cpu.io_in = lambda port, w: 0xFE if port == 0x330 else 0       # MPU-401: status 0 = ready, data 0FEh = ACK
        self.far(4)

    def io_out(self, port, v, w):
        if port == 0x330:
            self.out.append(v & 0xFF)

    def far(self, off):
        c = self.cpu
        c.s['cs'] = SEG; c.s['ds'] = 0x1F17; c.s['ss'] = 0x0200; c.r['sp'] = 0xF000
        c.push(RET[0]); c.push(RET[1]); c.ip = off
        c.run(500000, stop=RET)
        return c.r['ax'] & 0xFFFF

    def poke(self, off, v):
        self.cpu.mem[(SEG << 4) + off] = v & 0xFF

    def peek(self, off):
        return self.cpu.mem[(SEG << 4) + off]

    def tick(self):
        self.out = []
        self.far(0)
        return bytes(self.out)


def split_events(buf):
    """MIDI bytes of one tick -> list of messages (running status is not used by the driver, but it is handled)."""
    msgs, i, status = [], 0, 0
    n_data = {0x80: 2, 0x90: 2, 0xA0: 2, 0xB0: 2, 0xC0: 1, 0xD0: 1, 0xE0: 2}
    while i < len(buf):
        b = buf[i]
        if b >= 0x80:
            status = b; i += 1
        k = n_data.get(status & 0xF0)
        if k is None:
            i += 1; continue
        msgs.append(bytes([status]) + bytes(buf[i:i + k])); i += k
    return msgs


def vlq(v):
    out = [v & 0x7F]
    v >>= 7
    while v:
        out.append((v & 0x7F) | 0x80); v >>= 7
    return bytes(reversed(out))


def write_midi(path, events):
    """events: list of (tick, message bytes)."""
    trk = bytearray()
    trk += vlq(0) + b'\xFF\x51\x03' + (500000).to_bytes(3, 'big')
    last = 0
    for t, m in events:
        trk += vlq(t - last) + m
        last = t
    trk += vlq(0) + b'\xFF\x2F\x00'
    with open(path, 'wb') as f:
        f.write(b'MThd' + (6).to_bytes(4, 'big') + (0).to_bytes(2, 'big') + (1).to_bytes(2, 'big') + (25).to_bytes(2, 'big'))
        f.write(b'MTrk' + len(trk).to_bytes(4, 'big') + bytes(trk))


def run(kind, n, seconds=60):
    r = Roland()
    ev, quiet = [], 0
    for t in range(seconds * 50):
        if t == 1:
            r.poke(8 if kind == 'music' else 0xA, n)
        for m in split_events(r.tick()):
            ev.append((t, m))
        if t > 5 and kind == 'effect' and not r.peek(0xA) and not r.out:
            quiet += 1
            if quiet > 100:
                break
    return ev


if __name__ == '__main__':
    if len(sys.argv) < 4:
        print(__doc__); sys.exit(1)
    ev = run(sys.argv[1], int(sys.argv[2]), int(sys.argv[4]) if len(sys.argv) > 4 else 60)
    write_midi(sys.argv[3], ev)
    print('%d MIDI messages, %.1f s' % (len(ev), (ev[-1][0] if ev else 0) / 50.0))
