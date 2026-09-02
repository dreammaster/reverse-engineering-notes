#include "wiz/string_pool.h"

#include <limits>

namespace wiz {

bool StringPool::load(std::vector<u8> asciiKrn) {
    data_ = std::move(asciiKrn);
    if (data_.size() < 8) return false;

    u16 offBlk = rd16(data_.data() + 0);
    u16 offLen = rd16(data_.data() + 2);
    u16 treeBlk = rd16(data_.data() + 4);
    u16 treeLen = rd16(data_.data() + 6);

    size_t o = size_t(offBlk) * 512;
    size_t t = size_t(treeBlk) * 512;
    if (o + offLen > data_.size() || t + treeLen > data_.size()) return false;

    offsets_.resize(offLen / 2);
    for (size_t i = 0; i < offsets_.size(); ++i)
        offsets_[i] = rd16(data_.data() + o + 2 * i);

    tree_.resize(treeLen / 10);
    for (size_t i = 0; i < tree_.size(); ++i) {
        const u8 *n = data_.data() + t + 10 * i;
        tree_[i] = {rd16(n), rd16(n + 2), rd16(n + 4), rd16(n + 6), rd16(n + 8)};
    }
    if (tree_.empty()) return false;
    root_ = tree_[0].right;   // node 0 is a header; word[4] is the root

    keyLo_ = std::numeric_limits<int>::max();
    keyHi_ = 0;
    for (size_t i = 1; i < tree_.size(); ++i) {
        const Node &nd = tree_[i];
        if (nd.startIdx == 0 && nd.endIdx == 0 && nd.indexOff == 0) continue;
        keyLo_ = std::min<int>(keyLo_, nd.startIdx);
        keyHi_ = std::max<int>(keyHi_, nd.endIdx);
    }
    return true;
}

int StringPool::rawSlot(int kn) const {
    u16 node = root_;
    for (int guard = 0; node != 0 && guard < 200; ++guard) {
        const Node &nd = tree_[node];
        if (kn < nd.startIdx) node = nd.left;
        else if (kn > nd.endIdx) node = nd.right;
        else {
            size_t idx = size_t(nd.indexOff) + size_t(kn - nd.startIdx);
            return idx < offsets_.size() ? int(offsets_[idx]) : -1;
        }
    }
    return -1;
}

std::string StringPool::get(int kn, bool *ok) const {
    int sval = rawSlot(kn);
    if (sval < 0) { if (ok) *ok = false; return {}; }
    if (ok) *ok = true;
    size_t p = size_t(sval) * 2;
    if (p >= data_.size()) return {};
    int len = data_[p];
    if (p + 1 + len > data_.size()) len = int(data_.size() - p - 1);

    int seed = 67 * (kn % 51);
    std::string s;
    s.reserve(len);
    for (int k = 1; k <= len; ++k)
        s.push_back(char((data_[p + k] - seed - 23 * k) & 0xFF));
    return s;
}

} // namespace wiz
