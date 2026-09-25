#include "TMemoryBuffer.h"

void TMemoryBuffer::Init(unsigned long size) {
    m_data.assign(size, 0);
}

unsigned char* TMemoryBuffer::GetData() {
    return m_data.data();
}

const unsigned char* TMemoryBuffer::GetData() const {
    return m_data.data();
}

unsigned long TMemoryBuffer::GetSize() const {
    return static_cast<unsigned long>(m_data.size());
}

bool TMemoryBuffer::Decrypt(const wxString& /*key*/, unsigned long* /*outValue*/, unsigned long /*flags*/) {
    // Real cipher not reversed; stub reports success without modifying data.
    return true;
}
