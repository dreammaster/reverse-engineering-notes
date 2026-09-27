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

// Confirmed to have at least one value, 2, used at every SetValue() call
// site seen so far (TGameControl::SetOnScrollDestination, Deponia_Linux.asm
// lines 460720-460821) - real meaning/other values not resolved.
enum class TSendEventEnum { SendEvent = 2 };

// Confirmed 2 values, 0 and 1 (TGameControl::InitGameActions/
// InitInterfaces, asm lines 458124-458250, 466739-467222+) - real
// meaning/names not resolved.
enum class eTypeOrder { Value0 = 0, Value1 = 1 };

class TVisionaireObject;
class TVList;

class TVisObjRef {
public:
    TVisObjRef() = default;
    TVisObjRef(const TVisObjRef&) = default;
    TVisObjRef& operator=(const TVisObjRef&) = default;
    ~TVisObjRef() = default;
    // Confirmed (TGameControl::InitInterfaces, asm line 458174): TVList
    // elements are TVisionaireObject*, each converted through this before
    // use - so TVisObjRef is a lightweight handle onto the heavier
    // TVisionaireObject data record.
    explicit TVisObjRef(const TVisionaireObject& object);

    bool IsEmpty() const { return true; }
    // Confirmed (TGameControl::GetInterface/IsTextActive/IsTalking): compares
    // the 3-4 byte id, per GetId()'s comment above.
    bool operator==(const TVisObjRef& other) const;

    bool GetBool(int fieldId) const;
    int GetInt(int fieldId) const;
    wxString GetStr(int fieldId) const;
    std::wstring GetPath(int fieldId) const;
    TVisObjRef GetLink(int fieldId) const;
    // Confirmed called (TGameControl::ResetState, Deponia_Linux.asm line
    // 460932) with a field id and a bool - not reversed beyond that call
    // shape.
    void ClearLink(int fieldId, bool flag);
    // Confirmed called (TGameControl::StartDialog, asm line 461050) with a
    // field id, a linked TVisObjRef, and a bool - not reversed beyond that
    // call shape.
    void SetLink(int fieldId, const TVisObjRef& value, bool flag);
    // Confirmed 3 overloads (TGameControl::SetOnScrollDestination/
    // CenterScene, asm lines 460533-460821) - not reversed beyond their
    // call shapes.
    void SetValue(int fieldId, const wxPoint& value, TSendEventEnum event);
    void SetValue(int fieldId, bool value, TSendEventEnum event);
    void SetValue(int fieldId, int value, TSendEventEnum event);
    void SetValue(int fieldId, const wxString& value, TSendEventEnum event);
    const wxPoint* GetPoint(int fieldId) const;
    const wxRect* GetRect(int fieldId) const;
    // Confirmed call shape only (TGameControl::InitInterfaces/
    // InitGameActions, asm lines 458124-458250, 466739-467222+) - fills
    // outLinks with TVisionaireObject* elements; not reversed beyond that.
    void GetLinks(int fieldId, eTypeOrder order, TVList& outLinks) const;

    // Real GetId() returns a small (likely 4-byte) identifier the caller
    // treats as 3-4 individual bytes (see TMasterControl::PlayAVI packing
    // it into a 32-bit value) - exact meaning not resolved.
    const std::uint8_t* GetId() const;

private:
    wxPoint m_point{};
    wxRect m_rect{};
    std::uint8_t m_id[4]{};
};
