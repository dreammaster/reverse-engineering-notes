#!/usr/bin/env python3
"""Run BEEPER.BIN (the PC-speaker music/effects driver of Cadaver) on tools/x86.py and record what it does to the speaker.

The game loads the BIN at offset 0 of a segment and calls it through far calls: entry 0 = tick (called from the timer ISR, 50 Hz),
entry 4 = init (returns AL=1).  The command area is at offsets 8..0xC: [8] music request, [9] music playing, [0xA] sound effect request (word),
[0xC] 'effect finished' marker.  The driver sets the speaker with out 43h,B6h / out 42h (divisor lo, hi&1F) / out 61h (bits 0,1 = gate + speaker).
speaker_state() reports (frequency Hz or 0) after every tick."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import x86

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SEG = 0x3000
RET = (0x0100, 0x0000)


class Beeper:
    def __init__(self, path=None):
        path = path or os.path.join(ROOT, 'cadaver1', 'BEEPER.BIN')
        self.cpu = x86.CPU()
        data = open(path, 'rb').read()
        base = SEG << 4
        self.cpu.mem[base:base + len(data)] = data
        self.port61 = 0
        self.pit = []              # bytes written to port 42h since the last 43h
        self.divisor = 0
        self.log = []              # (port, value) for the tick in progress
        self.cpu.io_out = self.io_out
        self.cpu.io_in = self.io_in
        self.far(4)                # init

    def io_in(self, port, width):
        return self.port61 if port == 0x61 else 0

    def io_out(self, port, v, width):
        self.log.append((port, v))
        if port == 0x61:
            self.port61 = v & 0xFF
        elif port == 0x43:
            self.pit = []
        elif port == 0x42:
            self.pit.append(v & 0xFF)
            if len(self.pit) == 2:
                self.divisor = self.pit[0] | (self.pit[1] << 8)

    def far(self, off):
        c = self.cpu
        c.s['cs'] = SEG
        c.s['ds'] = 0x1F17
        c.s['ss'] = 0x0200
        c.r['sp'] = 0xF000
        c.push(RET[0])
        c.push(RET[1])
        c.ip = off
        c.run(200000, stop=RET)
        # RETF pops ip then cs; our sentinel is 0100:0000
        return c.r['ax'] & 0xFF

    def tick(self):
        self.log = []
        c = self.cpu
        c.s['cs'] = SEG
        c.ip = 0
        c.s['ss'] = 0x0200
        c.r['sp'] = 0xF000
        c.push(RET[0])
        c.push(RET[1])
        c.run(200000, stop=RET)
        return self.speaker()

    def speaker(self):
        """Frequency in Hz that is currently sounding (0 = silent)."""
        if (self.port61 & 3) != 3 or self.divisor == 0:
            return 0
        return 1193182.0 / self.divisor

    def poke(self, off, v):
        self.cpu.mem[(SEG << 4) + off] = v & 0xFF

    def peek(self, off):
        return self.cpu.mem[(SEG << 4) + off]

    # -- 1000:156C / 1000:1613 -----------------------------------------------------------
    def play(self, sid):
        """Start sound `sid` (CAD.EXE 1000:156C with the Beeper configuration)."""
        if not hasattr(self, 'flags'):
            self.flags = [0] * 0x80
        e = SNDTAB[sid] if sid < len(SNDTAB) else 0
        if e & 0x80:
            e &= 0x7F
            self.flags[e] = e
        elif self.peek(0xA) | (self.peek(0xB) << 8):
            return
        self.poke(0xA, e)

    def music(self, on):
        """INTRO.EXE 10F78 / 10F93: start the music unless it is playing ([9] == 1), or stop it ([8] = FFh)."""
        if on:
            if self.peek(9) != 1:
                self.poke(8, 1)
        else:
            self.poke(8, 0xFF)

    def stop(self, sid):
        """Stop sound `sid` (1000:1613 + 1000:1685)."""
        if not hasattr(self, 'flags'):
            self.flags = [0] * 0x80
        e = (sid + 1) & 0xFF
        self.flags[e & 0x7F] = 0
        self.poke(0xC, e)
        for i in range(0x80):
            if self.flags[i]:
                self.play(self.flags[i] - 1)


def _sndtab():
    d = open(os.path.join(ROOT, 'unpacked', 'cadaver1', 'CAD.EXE'), 'rb').read()
    b = 0x5C130 + 0x4064              # ds:4064 (file offset of ds:0 = 0x5C130)
    return list(d[b:b + 0x8C])


SNDTAB = _sndtab()


def write_wav(path, what, seconds=10, rate=22050):
    """Render sound id `what` (int) or the intro tune ('music') to a 16-bit mono WAV (one square wave per 20 ms tick)."""
    import struct, wave
    b = Beeper()
    if what == 'music':
        b.music(True)
    else:
        b.play(int(what, 0))
    out = bytearray()
    phase = 0.0
    per = rate // 50
    for _ in range(int(seconds * 50)):
        f = b.tick()
        if what == 'music':
            b.music(True)
        for _ in range(per):
            if f:
                phase = (phase + f / rate) % 1.0
                out += struct.pack('<h', 6000 if phase < 0.5 else -6000)
            else:
                out += b'\0\0'
    with wave.open(path, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(bytes(out))


if __name__ == '__main__':
    if len(sys.argv) > 3 and sys.argv[1] == 'wav':        # beeper.py wav ID|music OUT.wav [seconds]
        write_wav(sys.argv[3], sys.argv[2], float(sys.argv[4]) if len(sys.argv) > 4 else 10)
        sys.exit(0)
    b = Beeper()
    eff = int(sys.argv[1], 0) if len(sys.argv) > 1 else 1
    n = int(sys.argv[2]) if len(sys.argv) > 2 else 100
    b.poke(0xA, eff)
    last = None
    for t in range(n):
        f = b.tick()
        key = round(f)
        if key != last:
            print(t, key, b.log)
            last = key
