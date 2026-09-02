// Boltac's Trading Post -- shop stock + price rules.
//
// Ports BOLTAC (P01020A) from the Apple source, cross-checked against DOS
// SHOPS procs 14-23.  The DOS game keeps per-object stock counts (BOLTACXX)
// in SCENARIO.DATA and rewrites them as you trade; the standalone engine
// keeps them in its own file so it never touches the user's scenario.
// See docs/town.md.
#pragma once
#include "wiz/scenario.h"
#include <string>
#include <vector>

namespace wiz {

class Shop {
public:
    // Seed the stock table from the scenario's ZOBJECT records.
    void seedFrom(const Scenario &sc);
    bool load(const std::string &path, const Scenario &sc);   // false if absent
    bool save(const std::string &path) const;

    int  count() const { return int(stock_.size()); }
    int  stock(int obj) const { return (obj >= 0 && obj < count()) ? stock_[obj] : 0; }
    void setStock(int obj, int n) { if (obj >= 0 && obj < count()) stock_[obj] = i16(n); }

    // BOLTAC's stock filter: an item is on the shelf when BOLTACXX != 0 and it
    // is not cursed (DOS also guards BOLTACXX > -2).
    bool onShelf(const Scenario &sc, int obj) const {
        int n = stock(obj);
        return n != 0 && n > -2 && !ObjectRec{sc.record(Scenario::Object, obj)}.cursed();
    }

private:
    std::vector<i16> stock_;
};

// HALFPRIC = 2: Boltac buys / uncurses / identifies at half the list price;
// an unidentified item is only worth 1 gp.
inline int64_t sellValue(const WizLong &listPrice, bool identified) {
    return identified ? listPrice.value() / 2 : 1;
}
inline int64_t serviceFee(const WizLong &listPrice) {   // uncurse / identify
    return listPrice.value() / 2;
}

} // namespace wiz
