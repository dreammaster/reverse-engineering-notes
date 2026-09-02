#include "wiz/ucsd_volume.h"

#include <algorithm>
#include <cctype>
#include <cstdio>

namespace wiz {

namespace {
constexpr int kDirStartBlock = 2;
constexpr int kEntrySize = 26;

std::string upper(std::string s) {
    for (char &c : s) c = char(std::toupper((unsigned char)c));
    return s;
}
} // namespace

bool UcsdVolume::load(const std::string &path) {
    FILE *f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    long sz = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (sz < kDirStartBlock * long(kBlock) + 4) { std::fclose(f); return false; }
    data_.resize(size_t(sz));
    size_t got = std::fread(data_.data(), 1, data_.size(), f);
    std::fclose(f);
    if (got != data_.size()) return false;

    const u8 *dir = data_.data() + kDirStartBlock * kBlock;

    // Volume header (entry 0).
    // +0 firstBlk, +2 lastBlk, +4 kind, +6 len + 7-char name, +14 eovBlocks,
    // +16 numFiles, ...
    u8 nlen = dir[6];
    if (nlen > 7) nlen = 7;
    volName_.assign(reinterpret_cast<const char *>(dir + 7), nlen);
    eovBlocks_ = rd16(dir + 14);
    u16 numFiles = rd16(dir + 16);

    entries_.clear();
    for (int i = 1; i <= int(numFiles); ++i) {
        const u8 *e = dir + i * kEntrySize;
        if (e + kEntrySize > data_.data() + data_.size()) break;
        u8 fnlen = e[6];
        if (fnlen == 0 || fnlen > 15) continue;   // skip a bogus entry
        Entry ent;
        ent.firstBlock = rd16(e + 0);
        ent.lastBlock = rd16(e + 2);
        ent.kind = Kind(rd16(e + 4) & 0x0F);
        ent.name.assign(reinterpret_cast<const char *>(e + 7), fnlen);
        ent.lastByte = rd16(e + 22);
        ent.index = i;
        entries_.push_back(std::move(ent));
    }
    return true;
}

const UcsdVolume::Entry *UcsdVolume::find(const std::string &name) const {
    std::string up = upper(name);
    for (const Entry &e : entries_)
        if (upper(e.name) == up) return &e;
    return nullptr;
}

Bytes UcsdVolume::block(size_t n, size_t count) const {
    size_t off = n * kBlock;
    size_t len = count * kBlock;
    if (off + len > data_.size()) len = data_.size() > off ? data_.size() - off : 0;
    return {data_.data() + off, len};
}

std::vector<u8> UcsdVolume::fileBytes(const Entry &e) const {
    Bytes b = block(e.firstBlock, e.nblocks());
    size_t want = std::min(e.size(), b.size());
    return {b.p, b.p + want};
}

} // namespace wiz
