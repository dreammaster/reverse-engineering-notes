// Not yet assert-confirmed to a specific file; stays at the top level.
// TComposedFile::InitEntries decrypts the entry table via
// TMemoryBuffer::Decrypt(const wxString& key, ulong*, ulong) - the real
// cipher isn't reversed, so this is a no-op passthrough.
#pragma once

#include <vector>

#include "WxStub.h"

class TMemoryBuffer {
public:
    void Init(unsigned long size);
    unsigned char* GetData();
    const unsigned char* GetData() const;
    unsigned long GetSize() const;
    bool Decrypt(const wxString& key, unsigned long* outValue, unsigned long flags);

private:
    std::vector<unsigned char> m_data;
};
