#include "wiz/roster.h"
#include "wiz/scenario.h"

#include <cstdio>
#include <cstring>

namespace wiz {

bool Roster::load(const std::string &path) {
    FILE *f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    u8 buf[Character::kRecordBytes];
    for (int i = 0; i < kSlots; ++i) {
        if (std::fread(buf, 1, sizeof buf, f) != sizeof buf) { std::fclose(f); return false; }
        chars_[i].read({buf, sizeof buf});
    }
    std::fclose(f);
    return true;
}

void Roster::seedFrom(const Scenario &sc) {
    for (int i = 0; i < kSlots; ++i) {
        Bytes rec = sc.record(Scenario::Char, i);
        if (rec.size() >= Character::kRecordBytes)
            chars_[i].read(rec);
        else {
            chars_[i] = Character{};
            chars_[i].status = Status::Lost;
        }
    }
}

bool Roster::save(const std::string &path) const {
    FILE *f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    for (int i = 0; i < kSlots; ++i) {
        auto rec = chars_[i].write();
        std::fwrite(rec.data(), 1, rec.size(), f);
    }
    std::fclose(f);
    return true;
}

int Roster::findLost() const {
    for (int i = 0; i < kSlots; ++i)
        if (chars_[i].status == Status::Lost) return i;
    return -1;
}

int Roster::findByName(const std::string &name) const {
    for (int i = 0; i < kSlots; ++i)
        if (chars_[i].status != Status::Lost && chars_[i].name == name) return i;
    return -1;
}

int Roster::liveCount() const {
    int n = 0;
    for (int i = 0; i < kSlots; ++i)
        if (chars_[i].status != Status::Lost) ++n;
    return n;
}

} // namespace wiz
