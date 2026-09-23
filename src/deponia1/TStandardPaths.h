#pragma once

#include <string>

// TStandardPaths::GetLogFileDir / GetConfigDir are called as ordinary
// instance methods on the global `standardPaths` object in main().
class TStandardPaths {
public:
    std::wstring GetLogFileDir() const;
    std::wstring GetConfigDir() const;
};
