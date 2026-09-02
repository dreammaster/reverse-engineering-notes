// Reader for a UCSD p-System volume image (the DOS Wizardry WIZ*.DSK / SAVE*.DSK
// files are linear 512-byte-block images of one). See docs/overview.md.
#pragma once
#include "wiz/types.h"

namespace wiz {

class UcsdVolume {
public:
    static constexpr size_t kBlock = 512;

    enum class Kind : u8 {
        None = 0, BadBlocks, Code, Text, Info, Data, Graf, Foto, SecureDir
    };

    struct Entry {
        u16 firstBlock = 0;   // first block of the file
        u16 lastBlock = 0;    // first block past the file
        Kind kind = Kind::None;
        std::string name;
        u16 lastByte = 0;     // bytes used in the file's last block
        int index = 0;

        u16 nblocks() const { return u16(lastBlock - firstBlock); }
        size_t size() const {
            return nblocks() == 0 ? 0 : size_t(nblocks() - 1) * kBlock + lastByte;
        }
    };

    // Loads the whole image into memory. Returns false on I/O or parse error.
    bool load(const std::string &path);

    const std::string &volumeName() const { return volName_; }
    size_t totalBlocks() const { return data_.size() / kBlock; }
    const std::vector<Entry> &entries() const { return entries_; }

    const Entry *find(const std::string &name) const;   // case-insensitive
    Bytes block(size_t n, size_t count = 1) const;
    std::vector<u8> fileBytes(const Entry &e) const;

private:
    std::vector<u8> data_;
    std::string volName_;
    u16 eovBlocks_ = 0;
    std::vector<Entry> entries_;
};

} // namespace wiz
