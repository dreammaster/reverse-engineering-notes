// Not yet assert-confirmed to a specific file; stays at the top level.
#pragma once

#include <cstdint>

class TTimer {
public:
    void SetTime();
    std::int64_t GetTime() const;

private:
    std::int64_t m_setAt = 0;
};
