#include "wiz/scenario.h"

namespace wiz {

namespace {
std::string cleanPStr(const u8 *p) {   // UCSD STRING: len byte then chars
    int len = p[0];
    std::string s;
    for (int i = 1; i <= len; ++i)
        if (p[i] >= 32 && p[i] < 127) s.push_back(char(p[i]));
    return s;
}

std::vector<std::string> readStrArray(const u8 *&p, int n, int width) {
    std::vector<std::string> out;
    for (int i = 0; i < n; ++i) out.push_back(cleanPStr(p + i * width));
    p += n * width;
    return out;
}
} // namespace

bool Scenario::load(std::vector<u8> scenarioData) {
    data_ = std::move(scenarioData);
    if (data_.size() < 0x6A + 26 * 10) return false;
    const u8 *d = data_.data();

    gameName_ = cleanPStr(d);
    for (int t = 0; t < TypeCount; ++t) {
        recPer2b_[t] = rd16(d + 0x2A + 2 * t);
        count_[t]    = rd16(d + 0x3A + 2 * t);
        recSize_[t]  = rd16(d + 0x4A + 2 * t);
        bloff_[t]    = rd16(d + 0x5A + 2 * t);
    }

    const u8 *p = d + 0x6A;
    race_   = readStrArray(p, 6, 10);   // STRING[9]  -> 10 bytes
    class_  = readStrArray(p, 8, 10);
    status_ = readStrArray(p, 8, 9);    // STRING[8]  -> 9 bytes
    align_  = readStrArray(p, 4, 10);
    return true;
}

Bytes Scenario::record(Type t, int r) const {
    if (t < 0 || t >= TypeCount || r < 0 || r >= count_[t] || recPer2b_[t] == 0)
        return {};
    size_t blk = size_t(bloff_[t]) + 2 * (r / recPer2b_[t]);
    size_t off = blk * 512 + size_t(recSize_[t]) * (r % recPer2b_[t]);
    if (off + recSize_[t] > data_.size()) return {};
    return {data_.data() + off, recSize_[t]};
}

const char *Scenario::typeName(Type t) {
    static const char *n[] = {"zero", "maze", "monster", "reward",
                              "object", "char", "spcchr", "exp"};
    return (t >= 0 && t < TypeCount) ? n[t] : "?";
}

} // namespace wiz
