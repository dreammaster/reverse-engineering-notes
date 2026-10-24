# Initialised data audit (vars.c vs DATA.OVL)

535 non-pointer variables compared; 64 pointer tables (string/pointer arrays) skipped here and checked via the string audit below.

**Result:** 529 identical, 3 differ, 2 zero in C while non-zero in the original.

| var | DS addr | size | finding |
|---|---|---|---|
| D_52ba_vdp | 52ba | 64 | 25/64 bytes differ; first at +0x2 (C 00 vs orig 08) |
| D_52f3_forceHerc | 52f3 | 4 | C uninitialised/zero but DATA.OVL has non-zero data (1 non-zero bytes) |
| D_5304 | 5304 | 4 | 2/4 bytes differ; first at +0x2 (C ff vs orig 18) |
| D_5358 | 5358 | 2 | 2/2 bytes differ; first at +0x0 (C ff vs orig 00) |
| D_6708_map | 6708 | 1024 | C uninitialised/zero but DATA.OVL has non-zero data (206 non-zero bytes) |
| D_b21e | b21e | 3000 | beyond DATA.OVL image |

## String literals (2214 `_TEXT/_STATIC_TEXT` with a DS address)

2214 match DATA.OVL exactly; 0 differ.


## Pointer tables (string/char* arrays in vars.c vs original near pointers)

47 of 47 tables identical to the original pointer values.

| table | DS addr | declared | C entries | findings |
|---|---|---|---|---|
| D_17f6 | 17f6 | 0x30 | 48 | ok |
| D_1856 | 1856 | 0x30 | 48 | ok |
| D_18b6 | 18b6 | 0x30 | 48 | ok |
| D_1916 | 1916 | 0x26 | 38 | ok |
| D_1962 | 1962 | 56 | 56 | ok |
| D_19d2 | 19d2 | 8 | 8 | ok |
| D_19e2 | 19e2 | 48 + 1 | 49 | ok |
| D_1a44 | 1a44 | 10 | 10 | ok |
| D_1a6a | 1a6a | 5 | 5 | ok |
| D_1bfc | 1bfc | 26 | 26 | ok |
| D_1c30 | 1c30 | 0x30 | 48 | ok |
| D_1e3a | 1e3a | 0x28 | 40 | ok |
| D_1f4e | 1f4e | 8 | 8 | ok |
| D_1f5e | 1f5e | 8 | 8 | ok |
| D_24ea | 24ea | 0x80 | 128 | ok |
| D_25ea | 25ea | 30 | 0 | ok |
| D_2652 | 2652 | 4 | 0 | ok |
| D_27dc | 27dc | 3 | 3 | ok |
| D_2e10 | 2e10 | 11 | 11 | ok |
| D_3666 | 3666 | 8 | 8 | ok |
| D_3810 | 3810 | 9 | 9 | ok |
| D_3c20 | 3c20 | 8 | 8 | ok |
| D_3ca6 | 3ca6 | 4 | 4 | ok |
| D_3cae | 3cae | 4 | 4 | ok |
| D_3cb6 | 3cb6 | 4 | 4 | ok |
| D_3cce | 3cce | 0x30 | 48 | ok |
| D_3d2e | 3d2e | 4 | 4 | ok |
| D_3d36 | 3d36 | 4 | 4 | ok |
| D_3d3e | 3d3e | 4 | 4 | ok |
| D_3d46 | 3d46 | 2 | 2 | ok |
| D_3d4a | 3d4a | 4 | 4 | ok |
| D_3d52 | 3d52 | 4 | 4 | ok |
| D_3e0c | 3e0c | 19 | 19 | ok |
| D_3e32 | 3e32 | 8 | 8 | ok |
| D_3e42 | 3e42 | 12 | 12 | ok |
| D_3e72 | 3e72 | 3 | 3 | ok |
| D_419c | 419c | 8 | 8 | ok |
| D_41ac | 41ac | 8 | 8 | ok |
| D_444a | 444a | 3 | 3 | ok |
| D_4502 | 4502 | 8 | 8 | ok |
| D_4aa0 | 4aa0 | 4 | 0 | ok |
| D_4aa8 | 4aa8 | 0x22 + 1 | 35 | ok |
| D_4b3e | 4b3e | 8 | 8 | ok |
| D_4c74 | 4c74 | 26 | 26 | ok |
| D_4ca8 | 4ca8 | 26 | 26 | ok |
| D_4cf6 | 4cf6 | 13 | 13 | ok |
| D_515c | 515c | 4 | 4 | ok |
