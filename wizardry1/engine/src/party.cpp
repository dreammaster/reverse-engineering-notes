#include "wiz/party.h"
#include "wiz/roster.h"

#include <cstdio>
#include <utility>

namespace wiz {

Align Party::align() const {
    Align a = Align::Neutral;                       // GETALIGN default
    for (int i = 0; i < count_; ++i)
        if (chars_[i].align != Align::Neutral) a = chars_[i].align;
    return a;
}

void Party::add(Roster &roster, int slot) {
    if (full() || slot < 0 || slot >= roster.count()) return;
    Character &src = roster.slot(slot);
    src.inMaze = true;                              // flag the roster copy
    chars_[count_] = src;
    disk_[count_] = slot;
    ++count_;
}

void Party::remove(Roster &roster, int i) {
    if (i < 0 || i >= count_) return;
    chars_[i].inMaze = false;
    if (disk_[i] >= 0 && disk_[i] < roster.count())
        roster.slot(disk_[i]) = chars_[i];
    for (int j = i + 1; j < count_; ++j) {
        chars_[j - 1] = chars_[j];
        disk_[j - 1] = disk_[j];
    }
    --count_;
    disk_[count_] = -1;
}

void Party::swapMembers(int a, int b) {
    if (a < 0 || b < 0 || a >= count_ || b >= count_ || a == b) return;
    std::swap(chars_[a], chars_[b]);
    std::swap(disk_[a], disk_[b]);
}

void Party::disband(Roster &roster) {
    for (int i = 0; i < count_; ++i) {
        chars_[i].inMaze = false;
        if (disk_[i] >= 0 && disk_[i] < roster.count())
            roster.slot(disk_[i]) = chars_[i];
    }
    count_ = 0;
    for (int &d : disk_) d = -1;
}

void Party::resyncFromRoster(const Roster &roster) {
    for (int i = 0; i < count_; ++i) {
        if (disk_[i] < 0 || disk_[i] >= roster.count()) continue;
        int64_t gold = chars_[i].gold.v;
        bool inMaze = chars_[i].inMaze;
        chars_[i] = roster.slot(disk_[i]);
        chars_[i].gold.v = gold;
        chars_[i].inMaze = inMaze;
    }
}

bool Party::load(const std::string &path, const Roster &roster) {
    FILE *f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    int n = 0;
    if (std::fread(&n, sizeof n, 1, f) != 1 || n < 0 || n > kMax) { std::fclose(f); return false; }
    count_ = 0;
    for (int i = 0; i < n; ++i) {
        int slot = -1;
        if (std::fread(&slot, sizeof slot, 1, f) != 1) { std::fclose(f); return false; }
        if (slot < 0 || slot >= roster.count()) continue;
        chars_[count_] = roster.slot(slot);
        disk_[count_] = slot;
        ++count_;
    }
    std::fclose(f);
    return true;
}

bool Party::save(const std::string &path) const {
    FILE *f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    std::fwrite(&count_, sizeof count_, 1, f);
    for (int i = 0; i < count_; ++i) std::fwrite(&disk_[i], sizeof disk_[i], 1, f);
    std::fclose(f);
    return true;
}

} // namespace wiz
