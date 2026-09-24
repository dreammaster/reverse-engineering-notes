#pragma once

#include <functional>

// Original path confirmed via an x_assert() call inside
// TComposedFile::GetComposedFileExtension() (not yet reconstructed):
// src/baselib/composedfile.cpp - see manifest/source_layout.tsv.
//
// Real class almost certainly has many more members; only the piece main()
// touches (a retry-load callback hook) is reconstructed here.
class TComposedFile {
public:
    static std::function<void()> onRetryLoad;
};
