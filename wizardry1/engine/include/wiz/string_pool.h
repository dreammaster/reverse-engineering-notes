// Decoder for the DOS Wizardry string pool (ASCII.KRN).
// Algorithm recovered from SYSTEM.PASCAL (WIZARDRY procs 38 GETSTR / 82 loader);
// see docs/file-formats.md and docs/strings.txt.
#pragma once
#include "wiz/types.h"

namespace wiz {

class StringPool {
public:
    // Accepts the raw ASCII.KRN bytes (from UcsdVolume::fileBytes).
    bool load(std::vector<u8> asciiKrn);

    // Returns the deciphered string for key `kn`, or "" if the key is not in
    // the tree.  `ok` (optional) reports whether the key was found.
    std::string get(int kn, bool *ok = nullptr) const;

    // Positional key helpers (keys are not stored in the scenario records).
    static int monsterNameKey(int idx, int field = 2) { return 13000 + 4 * idx + field; }
    static int objectNameKey(int idx, int field = 1)  { return 14000 + 2 * idx + field; }
    static int spellNameKey(int idx)                  { return 5001 + idx; }
    static int messageKey(int msgNo, int line)        { return 20000 + 50 * msgNo + line; }

    int keyLo() const { return keyLo_; }
    int keyHi() const { return keyHi_; }

private:
    struct Node { u16 startIdx, endIdx, indexOff, left, right; };

    std::vector<u8> data_;
    std::vector<u16> offsets_;
    std::vector<Node> tree_;
    u16 root_ = 0;
    int keyLo_ = 0, keyHi_ = 0;

    int rawSlot(int kn) const;   // -> word offset into data_, or -1
};

} // namespace wiz
