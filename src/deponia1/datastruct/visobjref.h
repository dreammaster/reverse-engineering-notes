// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/datastruct/visobjref.cpp - see manifest/source_layout.tsv.
//
// TVisObjRef is the engine's generic handle onto a game-data object: a huge
// number of classes fetch typed field values from one via GetBool/GetInt/
// GetStr/GetLink/GetPoint/GetRect/GetPath, each keyed by an opaque integer
// field id from Visionaire's data schema (e.g. TMasterControl::Draw calls
// GetInt(0x313) to pick a render path - the id numbers are recovered
// faithfully from the disassembly, but what each one *means* requires the
// data-schema/property-table system, which hasn't been reversed yet).
//
// This stub only provides the method surface TMasterControl needs; it does
// not implement real field storage/lookup.
#pragma once

#include <cstdint>
#include <string>

#include "WxStub.h"

class TVisObjRef {
public:
    TVisObjRef() = default;
    TVisObjRef(const TVisObjRef&) = default;
    TVisObjRef& operator=(const TVisObjRef&) = default;
    ~TVisObjRef() = default;

    bool IsEmpty() const { return true; }

    bool GetBool(int fieldId) const;
    int GetInt(int fieldId) const;
    wxString GetStr(int fieldId) const;
    std::wstring GetPath(int fieldId) const;
    TVisObjRef GetLink(int fieldId) const;
    const wxPoint* GetPoint(int fieldId) const;
    const wxRect* GetRect(int fieldId) const;

    // Real GetId() returns a small (likely 4-byte) identifier the caller
    // treats as 3-4 individual bytes (see TMasterControl::PlayAVI packing
    // it into a 32-bit value) - exact meaning not resolved.
    const std::uint8_t* GetId() const;

private:
    wxPoint m_point{};
    wxRect m_rect{};
    std::uint8_t m_id[4]{};
};
