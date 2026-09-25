// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/baselib/memfile.cpp - see manifest/source_layout.tsv.
#pragma once

#include "TMemoryBuffer.h"

class TMemoryFile {
public:
    TMemoryBuffer& GetBuffer();

private:
    TMemoryBuffer m_buffer;
};
