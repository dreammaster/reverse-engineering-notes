# Reading TVisionaireGame::UpdateVersion

`TVisionaireGame::UpdateVersion` (asm lines 1508677-1523438) is a chain of `if (version <= N) fix` that
GCC threaded into jump chains on `cmp [rsp+0C78h+var_C54], imm`. These scripts cut it back into the fixes:

- `uvdisp.py` - the instructions of the function and `entry(V)`: where the code of a file of version V starts.
- `uvpaths.py` - `reach(V)`: the instructions a file of version V runs (the version compares are decided).
- `uvdump.py <hex version>` - the instructions only version V (not V+1) runs, i.e. the fix numbered V, with the
  field names of `fieldIds.h` next to the constants.

The result is in `src/deponia1/vstables/visionaireGameUpgrade.cpp`. Not read: the fix of version 0x71 (1358
instructions of the dump) and the step of 0xB3 (the lists of the container build rules).
