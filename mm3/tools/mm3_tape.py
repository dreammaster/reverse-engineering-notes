"""Reader for the lockstep tapes written by src/tape.c (see there for the format)."""
import struct


class Tape:
    def __init__(self, path):
        self.data = open(path, "rb").read()
        assert self.data[:8] == b"MM3TAPE1", "not a tape"
        (self.memsize,) = struct.unpack_from("<I", self.data, 8)
        self.mem0 = self.data[12:12 + self.memsize]
        self.pos = 12 + self.memsize

    def events(self):
        try:
            yield from self._events()
        except (struct.error, IndexError):
            return                        # damaged tail

    def _events(self):
        d, p = self.data, self.pos
        n = len(d)
        while p < n:
            t = chr(d[p]); p += 1
            ev = {"t": t}
            if t == "X":                       # the recompiled routine was aborted (timeout): the rest of its call is not on the tape
                ev["delta"] = []
                yield ev
                continue
            if t == "F":
                ln = d[p]; p += 1
                ev["name"] = d[p:p + ln].decode(); p += ln
                r = struct.unpack_from("<10H", d, p); p += 20
                ev["regs"] = dict(zip(("ax", "bx", "cx", "dx", "si", "di", "bp", "es", "ds", "sp"), r))
                ev["delta"] = []
                yield ev
                continue
            if t in "TH":
                ln = d[p]; p += 1
                ev["name"] = d[p:p + ln].decode(); p += ln
                if t == "H":
                    ev["nargs"], ev["ret"] = d[p], d[p + 1]; p += 2
                r = struct.unpack_from("<10H", d, p); p += 20
                ev["regs"] = dict(zip(("ax", "bx", "cx", "dx", "si", "di", "bp", "es", "ds", "sp"), r))
                if t == "H":
                    nw = d[p]; p += 1
                    ev["stack"] = struct.unpack_from("<%dH" % nw, d, p); p += 2 * nw
            elif t == "h":
                ev["ax"], ev["dx"], ev["es"], ev["sp"] = struct.unpack_from("<4H", d, p); p += 8
            elif t == "U":
                ev["ax"], ev["dx"] = struct.unpack_from("<2H", d, p); p += 4
            else:       # a damaged tail (the recorder was killed in the middle of an event): the tape ends here
                return
            if p + 4 > n:
                return
            (cnt,) = struct.unpack_from("<I", d, p); p += 4
            runs = []
            for _ in range(cnt):
                if p + 8 > n:
                    return
                off, ln = struct.unpack_from("<II", d, p); p += 8
                if p + ln > n:
                    return
                runs.append((off, d[p:p + ln])); p += ln
            ev["delta"] = runs
            yield ev


if __name__ == "__main__":
    import sys
    from collections import Counter
    c = Counter()
    n = 0
    for ev in Tape(sys.argv[1]).events():
        n += 1
        if ev["t"] in "TH":
            c[ev["t"] + " " + ev["name"]] += 1
    print(n, "events")
    for k, v in c.most_common():
        print("%8d  %s" % (v, k))
