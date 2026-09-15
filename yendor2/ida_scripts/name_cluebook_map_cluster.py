"""
Names the F1 "MAPS" clue-book category cluster (world/towns/mines,
per ShowClueBookHelpScreen).

sub_13278 (-> RunClueBookMapCategory): called from ShowClueBook (F1).
Calls LoadClueBookMapEntry then DrawClueBookMapGrid, loops polling
input and hit-testing region table 0x6976 (the same table used by
RunClueBookItemCategory) -- on a hit, calls sub_14122 (a cell-select
handler, not traced), exits on ESC (byte_2E400==0x1B).

sub_1419B (-> LoadClueBookMapEntry): reads the current map id
(word_2E3EE[0]) via FileEntry_Read from WORLD.DAT (bx=0x9043) into a
freshly allocated buffer -- the map-entry loader, mirroring
LoadClueBookMonsterEntry's shape.

sub_13FCF (-> DrawClueBookMapGrid): computes a row/col grid layout
(id-1 divided by 20) from the loaded map id, clears the video buffer
(FillVideoBuffer), and draws location labels via the already-named
BuildClueLocationSuffix -- confirms this renders a map grid with
per-cell location text.

sub_14122 (the cell-click handler) left unnamed -- not traced.

Run via:
    .\run_ida_script.ps1 name_cluebook_map_cluster.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x13278: "RunClueBookMapCategory",
    0x1419B: "LoadClueBookMapEntry",
    0x13FCF: "DrawClueBookMapGrid",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x13278,
    "F1 'MAPS' clue-book category loop (called from ShowClueBook). "
    "LoadClueBookMapEntry + DrawClueBookMapGrid, then polls input and "
    "hit-tests region table 0x6976 for cell clicks (sub_14122, not "
    "traced), until ESC.",
    False,
)
ida_bytes.set_cmt(
    0x1419B,
    "Loads the current map id (word_2E3EE[0]) via FileEntry_Read from "
    "WORLD.DAT into a fresh buffer. Called by RunClueBookMapCategory.",
    False,
)
ida_bytes.set_cmt(
    0x13FCF,
    "Draws the F1 map grid: computes a row/col layout (id-1 / 20) from "
    "the loaded map id, clears the video buffer, and draws per-cell "
    "location labels via BuildClueLocationSuffix.",
    False,
)
