#include "wiz/shop.h"

#include <cstdio>

namespace wiz {

void Shop::seedFrom(const Scenario &sc) {
    int n = sc.count(Scenario::Object);
    stock_.assign(n, 0);
    for (int i = 0; i < n; ++i)
        stock_[i] = i16(ObjectRec{sc.record(Scenario::Object, i)}.boltacXX());
}

bool Shop::load(const std::string &path, const Scenario &sc) {
    int n = sc.count(Scenario::Object);
    FILE *f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    std::vector<i16> in(n);
    bool ok = std::fread(in.data(), sizeof(i16), n, f) == size_t(n);
    std::fclose(f);
    if (!ok) return false;
    stock_ = std::move(in);
    return true;
}

bool Shop::save(const std::string &path) const {
    FILE *f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    std::fwrite(stock_.data(), sizeof(i16), stock_.size(), f);
    std::fclose(f);
    return true;
}

} // namespace wiz
