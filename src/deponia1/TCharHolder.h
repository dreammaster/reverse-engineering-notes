// Not yet assert-confirmed to a specific file; stays at the top level.
// A lightweight string handle used throughout the engine (SLoadingScreen's
// image-path fields, TTempFile's tracked paths). Modeled as a thin wxString
// wrapper; real internal representation (possibly an interned/ref-counted
// string, going by patterns seen elsewhere - see NOTES.md) not reversed.
#pragma once

#include "WxStub.h"

class TCharHolder {
public:
    TCharHolder() = default;
    TCharHolder(const TCharHolder&) = default;
    TCharHolder& operator=(const TCharHolder&) = default;

    operator wxString() const { return m_value; }
    operator wxFileName() const { return wxFileName(m_value.ToStdWstring()); }
    wxString GetFullPath() const { return m_value; }

private:
    wxString m_value;
};
