#include "TStandardPaths.h"

std::wstring TStandardPaths::GetLogFileDir() const {
    return L".";
}

std::wstring TStandardPaths::GetConfigDir() const {
    return L"./";
}
