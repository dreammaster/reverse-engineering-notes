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
#include <vector>

#include "TCharHolder.h"
#include "WxStub.h"

// Confirmed to have at least one value, 2, used at every SetValue() call
// site seen so far (TGameControl::SetOnScrollDestination, Deponia_Linux.asm
// lines 460720-460821) - real meaning/other values not resolved.
enum class TSendEventEnum { kSendEvent = 2 };

// Confirmed 2 values, 0 and 1 (TGameControl::InitGameActions/
// InitInterfaces, asm lines 458124-458250, 466739-467222+) - real
// meaning/names not resolved.
enum class TypeOrder { kValue0 = 0, kValue1 = 1 };

class TSprite;
class TVisionaireObject;
class TVisionaire;
class TVList;

class TVisObjRef {
public:
	TVisObjRef() = default;
	TVisObjRef(const TVisObjRef &) = default;
	TVisObjRef &operator=(const TVisObjRef &) = default;
	~TVisObjRef() = default;
	// Confirmed (TGameControl::InitInterfaces, asm line 458174): TVList
	// elements are TVisionaireObject*, each converted through this before
	// use - so TVisObjRef is a lightweight handle onto the heavier
	// TVisionaireObject data record.
	explicit TVisObjRef(const TVisionaireObject &object);
	// A second, pointer-taking overload (TGameControl::InitCharacters, asm
	// line 466302) - functionally the same conversion, just dereferencing
	// first.
	explicit TVisObjRef(const TVisionaireObject *object);

	bool IsEmpty() const {
		return true;
	}
	// Confirmed (TGameControl::GetInterface/IsTextActive/IsTalking): compares
	// the 3-4 byte id, per GetId()'s comment above.
	bool operator==(const TVisObjRef &other) const;
	// A second overload comparing directly against a raw TVisionaireObject
	// (TGameControl::Load, Deponia_Linux.asm line 476802) - presumably the
	// same id-based comparison as above, just against the other side's own
	// id instead of another TVisObjRef's; not reversed beyond that call
	// shape.
	bool operator==(const TVisionaireObject &other) const;

	bool GetBool(int fieldId) const;
	int GetInt(int fieldId) const;
	// Confirmed call shape only (TGameControl::UpdateWalkingSounds,
	// Deponia_Linux.asm line 463580) - not reversed beyond that.
	float GetFloat(int fieldId) const;
	wxString GetStr(int fieldId) const;
	// Confirmed call shape only (TGameControl::PreLoad, Deponia_Linux.asm
	// line 464274) - fills outStrings with TCharHolder elements (16 bytes
	// each, per the iterator-distance-based size check there); not
	// reversed beyond that.
	void GetStrings(int fieldId, std::vector<TCharHolder> &outStrings) const;
	// Confirmed call shape only (TGameControl::ReplaceGame, Deponia_Linux.asm
	// line 468762): unlike GetStr/GetInt/etc., takes no field id - "the
	// name" is a property of the object itself, not a per-field lookup.
	TCharHolder GetName() const;
	std::wstring GetPath(int fieldId) const;
	TVisObjRef GetLink(int fieldId) const;
	// Confirmed call shape only (TGameControl::InitCharacters, asm line
	// 466313) - not reversed beyond that.
	TVisObjRef GetParent() const;
	// Confirmed call shape only (TManagedObject::SetPolygon, Deponia_Linux.
	// asm line 191690) - a name including parent-object context (depth
	// given by `levels`), distinct from the plain GetName() above; not
	// reversed beyond that call shape.
	wxString GetNameWithParents(int levels) const;
	// Confirmed called (TGameControl::ResetState, Deponia_Linux.asm line
	// 460932) with a field id and a bool - not reversed beyond that call
	// shape.
	void ClearLink(int fieldId, bool flag);
	// Confirmed called (TGameControl::StartDialog, asm line 461050) with a
	// field id, a linked TVisObjRef, and a bool - not reversed beyond that
	// call shape.
	void SetLink(int fieldId, const TVisObjRef &value, bool flag);
	// Confirmed 3 overloads (TGameControl::SetOnScrollDestination/
	// CenterScene, asm lines 460533-460821) - not reversed beyond their
	// call shapes.
	void SetValue(int fieldId, const wxPoint &value, TSendEventEnum event);
	void SetValue(int fieldId, bool value, TSendEventEnum event);
	void SetValue(int fieldId, int value, TSendEventEnum event);
	void SetValue(int fieldId, const wxString &value, TSendEventEnum event);
	const wxPoint *GetPoint(int fieldId) const;
	const wxRect *GetRect(int fieldId) const;
	// Confirmed call shapes only (TGScene::SetScene()/InitialiseBackground()/
	// BeginScene(), Deponia_Linux.asm lines 168670-172880) - the sprite-typed,
	// rect-list-typed and string-holder-typed counterparts of GetRect()/
	// GetStr() above (GetSprite()/GetStrHolder() return a reference into the
	// data record's own storage rather than a copy); not reversed beyond
	// those call shapes.
	const TSprite &GetSprite(int fieldId) const;
	void GetRects(int fieldId, std::vector<wxRect> &outRects) const;
	const TCharHolder &GetStrHolder(int fieldId) const;
	// Confirmed call shape only (TGameControl::InitInterfaces/
	// InitGameActions, asm lines 458124-458250, 466739-467222+) - fills
	// outLinks with TVisionaireObject* elements; not reversed beyond that.
	void GetLinks(int fieldId, TypeOrder order, TVList &outLinks) const;
	// A second, order-less list accessor - distinct from TVisionaire::GetList
	// (datastruct/visionaire.h), which takes an extra bool. Confirmed call
	// shape only (TGameControl::InitGameActions, asm line 467032).
	void GetList(int fieldId, TVList &outList) const;

	// Real GetId() returns a small (likely 4-byte) identifier the caller
	// treats as 3-4 individual bytes (see TMasterControl::PlayAVI packing
	// it into a 32-bit value) - exact meaning not resolved.
	const std::uint8_t *GetId() const;

	// Confirmed call shape only (TGObjectManager::MouseMove, Deponia_Linux.
	// asm lines 600326-600356): the real body reads a TVisionaireObject*
	// this project's own stub doesn't model, then forwards to its own
	// GetVisionaire() (x_assert on a non-null result skipped, per this
	// project's usual treatment); not reversed beyond that call shape.
	TVisionaire *GetVisionaire() const;

	// Confirmed call shape only (TManagedObject::ExecuteMatchingAction,
	// Deponia_Linux.asm lines 598006-598021): the real body reads the same
	// TVisionaireObject* field GetVisionaire() does, then forwards to
	// TVisionaireObject::IsAnyObject() - not reversed beyond that, and this
	// project's own TVisObjRef stub has no such field to forward from.
	bool IsAnyObject() const;

private:
	wxPoint _point{};
	wxRect _rect{};
	std::uint8_t _id[4] {};
};
