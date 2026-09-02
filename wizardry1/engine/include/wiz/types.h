// Core value types for the DOS Wizardry data layer.
// Layouts recovered in docs/ (pmachine.md, file-formats.md, globals.md).
#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace wiz {

using u8 = uint8_t;
using u16 = uint16_t;
using i16 = int16_t;
using u32 = uint32_t;

// A non-owning view over bytes (std::span is C++20).
struct Bytes {
    const u8 *p = nullptr;
    size_t n = 0;
    const u8 &operator[](size_t i) const { return p[i]; }
    size_t size() const { return n; }
    bool empty() const { return n == 0; }
    Bytes sub(size_t off, size_t len) const { return {p + off, len}; }
};

inline u16 rd16(const u8 *p) { return u16(p[0] | (p[1] << 8)); }              // little-endian
inline i16 rd16s(const u8 *p) { return i16(rd16(p)); }
inline u16 rd16(Bytes b, size_t o) { return rd16(b.p + o); }

// TWIZLONG: three i16 limbs, base 10000.  value = low + mid*1e4 + high*1e8.
struct WizLong {
    i16 low = 0, mid = 0, high = 0;
    int64_t value() const {
        return int64_t(low) + int64_t(mid) * 10000 + int64_t(high) * 100000000LL;
    }
    static WizLong read(const u8 *p) {
        return {rd16s(p), rd16s(p + 2), rd16s(p + 4)};
    }
};

enum class Race  : u8 { NoRace, Human, Elf, Dwarf, Gnome, Hobbit };
enum class Class : u8 { Fighter, Mage, Priest, Thief, Bishop, Samurai, Lord, Ninja };
enum class Align : u8 { Unalign, Good, Neutral, Evil };
enum class Status: u8 { OK, Afraid, Asleep, Paralyzed, Stoned, Dead, Ashes, Lost };
enum class ObjType : u8 { Weapon, Armor, Shield, Helmet, Gauntlet, Special, Misc };

// Maze wall state per edge (2 bits packed on disk).
enum class Wall : u8 { Open, Wall, Door, HiddenDoor };

// Special-square kinds (TSQUARE; DOS renames noted in file-formats.md).
enum class Square : u8 {
    Normal, Stairs, Pit, Chute, TurnRandom, Darkness, Teleport,
    Damage, Buttons, RockWater, Fizzle, ScnMsg, Encounter
};

} // namespace wiz
