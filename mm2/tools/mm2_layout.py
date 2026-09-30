"""
Pure-python parser for the Plink86 overlay layout of MM2.EXE.

Used both by the command line (plink_info.py) and, via sys.path, by the IDA
scripts in ../ida_scripts, so it must not import anything IDA-specific.

Background (see mm2/docs/exe-layout.md for the full story):
  * MM2.EXE is an MS-C program linked with Phoenix Plink86.  The MZ header only
    describes the *resident* image; everything after it (the "trailing data"
    IDA complains about) is the initialised DGROUP, loaded at runtime by the
    Plink86 stub, which reopens MM2.EXE for the purpose.
  * The 14 *.OVL files are Plink86 overlays: raw code blobs that share two
    memory windows inside the resident code segment (cs = load base).
"""

import os
import struct

DEFAULT_GAME_DIR = r"D:\GOG Games\Might and Magic 2"

IDA_BASE_PARA = 0x1000          # IDA's MZ loader relocates the image to 1000h
SEG_TABLE_SEG = 0x6BF           # Plink86 segment table lives in this (relative) paragraph
RECORD_SIZE = 16


class Record:
    """One 16-byte Plink86 segment-table row (plus the next row's first words,
    which the loader also reads: +10h/+12h = file offset, +14h = load size)."""

    def __init__(self, index, name, w, nxt):
        self.index = index                    # 0-based row; thunk index = row + 1
        self.name = name                      # file the segment is read from
        self.flags = w[3]                     # +6: 4000h preload, 8000h resident
        self.reloc_count = w[4]               # +8: number of 4-byte relocation entries
        self.start_para = w[5]                # +Ah: load paragraph (relative to load base)
        self.end_para = w[6]                  # +Ch: end paragraph
        self.name_off = w[7]                  # +Eh: offset of file name in the seg table seg
        self.file_para = nxt[0] | (nxt[1] << 16)   # +10h: paragraph offset of the record in its file
        self.load_paras = nxt[2]              # +14h: paragraphs actually read from the file

    @property
    def mem_paras(self):
        return self.end_para - self.start_para

    @property
    def reloc_paras(self):
        return (self.reloc_count + 3) >> 2    # reloc table is padded to whole paragraphs

    @property
    def file_pos(self):
        return self.file_para * 16

    @property
    def body_pos(self):
        return self.file_pos + self.reloc_paras * 16

    @property
    def body_size(self):
        return min(self.load_paras, self.mem_paras) * 16

    @property
    def ida_start(self):
        return (self.start_para + IDA_BASE_PARA) * 16


class Layout:
    def __init__(self, exe_path=None, game_dir=DEFAULT_GAME_DIR):
        self.game_dir = game_dir
        self.exe_path = exe_path or os.path.join(game_dir, "MM2.EXE")
        with open(self.exe_path, "rb") as f:
            self.data = f.read()
        d = self.data
        (self.sig, self.last_page, self.pages, self.nreloc, self.hdr_paras, self.min_alloc,
         self.max_alloc, self.ss, self.sp, self.checksum, self.ip, self.cs,
         self.reloc_off, self.ovno) = struct.unpack("<14H", d[:28])
        assert self.sig == 0x5A4D, "not an MZ executable"
        self.hdr_size = self.hdr_paras * 16
        self.image_size = (self.pages - 1) * 512 + (self.last_page or 512)
        self.trailing = len(d) - self.image_size
        self.loaded_size = self.image_size - self.hdr_size     # what IDA's MZ loader maps

        # Segment table: rows at (SEG_TABLE_SEG:0).  Row 0 is a header
        # (+0: load-base fixup word, +2: initial ss:sp words).
        tbl = self.hdr_size + SEG_TABLE_SEG * 16
        self.table_pos = tbl
        rows = []
        i = 0
        while True:
            w = struct.unpack("<8H", d[tbl + i * 16: tbl + i * 16 + 16])
            rows.append(w)
            if w[3] == 0xFFFF and i > 0:
                break
            i += 1
        # rows[0..n-1] are records, the last row is the 0xFFFF terminator
        self.records = []
        for k in range(len(rows) - 1):
            w, nxt = rows[k], rows[k + 1]
            name = self._cstr(tbl + w[7])
            self.records.append(Record(k, name, w, nxt))
        self.dgroup = self.records[-1]

    def _cstr(self, pos):
        end = self.data.index(b"\0", pos)
        return self.data[pos:end].decode("ascii")

    # -- overlays ---------------------------------------------------------
    @property
    def overlays(self):
        return [r for r in self.records if r.name.upper().endswith(".OVL")]

    def overlay(self, name):
        name = name.upper()
        if not name.endswith(".OVL"):
            name += ".OVL"
        for r in self.overlays:
            if r.name.upper() == name:
                return r
        raise KeyError(name)

    def read_segment(self, rec):
        """Return (body_bytes, [(offset, seg_para), ...]) for a record, reading
        overlays from their .OVL file and DGROUP from the EXE tail."""
        path = self.exe_path if rec.name.upper() == "MM2.EXE" else os.path.join(self.game_dir, rec.name)
        with open(path, "rb") as f:
            blob = f.read()
        relocs = []
        for i in range(rec.reloc_count):
            off, seg = struct.unpack("<HH", blob[rec.file_pos + i * 4: rec.file_pos + i * 4 + 4])
            relocs.append((off, seg))
        return blob[rec.body_pos: rec.body_pos + rec.body_size], relocs

    # -- thunks -----------------------------------------------------------
    THUNK_TABLE_REL = 0x6D8A          # first thunk, offset in the code segment

    def thunks(self):
        """List of (code_seg_offset, thunk_index, target_offset).  thunk_index
        0x8000 = resident target, 0x8000+n = overlay record n-1."""
        out = []
        p = self.hdr_size + self.THUNK_TABLE_REL
        while self.data[p] == 0x9A:
            idx, = struct.unpack("<H", self.data[p + 5: p + 7])
            assert self.data[p + 7] == 0xEA
            toff, = struct.unpack("<H", self.data[p + 8: p + 10])
            out.append((p - self.hdr_size, idx, toff))
            p += 12
        return out

    def thunk_owner(self, idx):
        n = idx & 0x7FFF
        return None if n == 0 else self.records[n - 1]
