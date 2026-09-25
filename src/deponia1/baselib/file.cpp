#include "baselib/file.h"

TFile::~TFile() {
    if (m_handle)
        std::fclose(m_handle);
}

bool TFile::OpenRead(const wxFileName& path) {
    m_handle = std::fopen(static_cast<const char*>(path.GetFullPath().mb_str()), "rb");
    return m_handle != nullptr;
}

unsigned long TFile::ReadToBuf(TMemoryBuffer& buffer, unsigned long size) {
    buffer.Init(size);
    if (!m_handle)
        return 0;
    return static_cast<unsigned long>(std::fread(buffer.GetData(), 1, size, m_handle));
}
