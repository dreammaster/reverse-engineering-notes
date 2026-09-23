#pragma once

#include <functional>

// Real class almost certainly has many more members; only the piece main()
// touches (a retry-load callback hook) is reconstructed here.
class TComposedFile {
public:
    static std::function<void()> onRetryLoad;
};
