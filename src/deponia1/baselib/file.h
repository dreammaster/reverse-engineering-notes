// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/baselib/file.cpp - see manifest/source_layout.tsv.
#pragma once

#include <cstdio>

#include "TMemoryBuffer.h"
#include "WxStub.h"

class TFile {
public:
    TFile() = default;
    ~TFile();

    bool OpenRead(const wxFileName& path);
    unsigned long ReadToBuf(TMemoryBuffer& buffer, unsigned long size);

private:
    std::FILE* m_handle = nullptr;
};
