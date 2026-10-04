// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/datastruct/visobjref.cpp - see manifest/source_layout.tsv.
//
// TVisObjRef is the engine's generic handle onto a game-data object: a huge
// number of classes fetch typed field values from one via GetBool/GetInt/
// GetStr/GetLink/GetPoint/GetRect/GetPath, each keyed by an opaque integer
// field id from Visionaire's data schema (e.g. TMasterControl::Draw calls
// GetInt(kGameShaderExclude) to pick a render path - the id numbers are recovered
// faithfully from the disassembly, but what each one *means* requires the
// data-schema/property-table system, which hasn't been reversed yet).
//
// Confirmed in full (Deponia_Linux.asm lines 597612-600388, all 85 manifest-
// listed methods): a counted reference to one TVisionaireObject (a single
// pointer; copying takes a reference, destroying releases it) whose every
// method forwards to the object, answering a default (-1, 0, false, empty...)
// when the reference is empty.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "TCharHolder.h"
#include "WxStub.h"
#include "TTextLanguage.h"
#include "datastruct/eventhandler.h"
#include "vscommon/scripting/id.h"
#include "vstables/fieldIds.h"

// How TDataGroup::SetValue() treats a new value (Deponia_Linux.asm lines
// 601106-601430): kForce stores it unconditionally and notifies the event
// handlers; kSendEvent stores it only if it differs from the current one, and
// notifies; kNoEvent stores it only if it differs, without notifying. Every
// call site reversed so far passes kNoEvent (2). The names are invented.
enum class TSendEventEnum { kForce = 0, kSendEvent = 1, kNoEvent = 2 };

// Confirmed 2 values, 0 and 1 (TGameControl::InitGameActions/
// InitInterfaces, asm lines 458124-458250, 466739-467222+) - real
// meaning/names not resolved.
enum class TypeOrder { kValue0 = 0, kValue1 = 1, kValue3 = 3 };

// How TVisionaire::ChangeOrder() moves an object in its list (Deponia_Linux.asm
// lines 610604-610748; the names are invented).
enum class TMoveOrderEnum { kUp = 0, kDown = 1, kFirst = 2, kLast = 3 };

class TSprite;
class TVisionaireObject;
class TVisionaire;
class TVList;

class TVisObjRef {
public:
	TVisObjRef() : _object(nullptr) {
	}
	explicit TVisObjRef(const TVisionaireObject *object);
	explicit TVisObjRef(const TVisionaireObject &object);
	TVisObjRef(const TVisObjRef &other);
	~TVisObjRef();
	TVisObjRef &operator=(const TVisObjRef &other);

	/** Drops the reference. */
	void Clear();
	/** Removes the object from its visionaire (see TVisionaireObject::Remove())
	 *  and drops the reference. */
	void Remove();
	void Set(TVisionaireObject *object);
	TVisionaireObject *GetObjectPointer() const {
		return _object;
	}
	/** A new reference to the object (null if there is none). */
	TVisionaireObject *GetReference() const;

	/** Two empty references are equal; otherwise TVisionaireObject::operator==. */
	bool operator==(const TVisObjRef &other) const;
	bool operator==(const TVisionaireObject &other) const;

	bool IsEmpty() const;
	bool IsAnyObject() const;
	int GetTypeLink(int fieldId) const;
	int GetTypeField(int fieldId) const;
	TVisObjRef GetParent() const;
	int GetParentField() const;
	/** The object's id as its 4 raw bytes (an empty TId(-1,-1) when empty). */
	const std::uint8_t *GetId() const;
	long GetOrder() const;
	bool ChangeOrder(TMoveOrderEnum move);
	void SetName(const TCharHolder &name);
	TCharHolder GetName() const;
	wxString GetNameWithParents(int levels) const;
	void GetChildren(TVList &outChildren) const;
	bool GetLinkByNameIgnoreCase(const wxString &name, int fieldId, TVisObjRef &outLink);
	bool GetLinkByName(const wxString &name, int fieldId, TVisObjRef &outLink);

	int GetInt(int fieldId) const;
	float GetFloat(int fieldId) const;
	bool GetBool(int fieldId) const;
	wxString GetStr(int fieldId) const;
	const TCharHolder &GetStrHolder(int fieldId) const;
	wxFileName GetPath(int fieldId) const;
	const wxPoint *GetPoint(int fieldId) const;
	const wxRect *GetRect(int fieldId) const;
	const TSprite &GetSprite(int fieldId) const;
	TVisObjRef GetLink(int fieldId) const;
	void GetLinks(int fieldId, TypeOrder order, TVList &outLinks) const;
	void GetPoints(int fieldId, std::vector<wxPoint> &outPoints) const;
	void GetRects(int fieldId, std::vector<wxRect> &outRects) const;
	void GetSprites(int fieldId, std::vector<TSprite> &outSprites) const;
	void GetStrings(int fieldId, std::vector<TCharHolder> &outStrings) const;
	void GetPaths(int fieldId, std::vector<TCharHolder> **outPaths) const;
	void GetPaths(int fieldId, std::vector<TCharHolder> &outPaths) const;
	void GetInts(int fieldId, std::vector<int> &outInts) const;
	void GetFloats(int fieldId, std::vector<float> &outFloats) const;
	void GetTexts(int fieldId, std::vector<TTextLanguage> **outTexts) const;
	void GetTextsCopy(int fieldId, std::vector<TTextLanguage> &outTexts) const;
	void GetList(int fieldId, TVList &outList) const;
	unsigned long GetLinksListSize(int fieldId) const;
	int GetLinksHighestOrder(int fieldId) const;

	void SetValue(int fieldId, bool value, TSendEventEnum event);
	void SetValue(int fieldId, int value, TSendEventEnum event);
	void SetValue(int fieldId, float value, TSendEventEnum event);
	void SetValue(int fieldId, const TCharHolder &value, TSendEventEnum event);
	void SetValue(int fieldId, const wxPoint &value, TSendEventEnum event);
	void SetValue(int fieldId, const wxRect &value, TSendEventEnum event);
	void SetValue(int fieldId, const wxFileName &value, TSendEventEnum event);
	void SetValue(int fieldId, const wxString &value, TSendEventEnum event);
	void SetValue(int fieldId, const TSprite &value, TSendEventEnum event);
	void SetValue(int fieldId, const TVList &value, bool notify);
	void SetValue(int fieldId, const std::vector<wxRect> &value, TSendEventEnum event);
	void SetValue(int fieldId, const std::vector<TSprite> &value, TSendEventEnum event);
	void SetValue(int fieldId, const std::vector<wxPoint> &value, TSendEventEnum event);
	void SetValue(int fieldId, const std::vector<TCharHolder> &value, TSendEventEnum event);
	void SetPathList(int fieldId, const std::vector<TCharHolder> &value, TSendEventEnum event);
	void SetValue(int fieldId, const std::vector<int> &value, TSendEventEnum event);
	void SetValue(int fieldId, const std::vector<float> &value, TSendEventEnum event);
	void SetValue(int fieldId, const std::vector<TTextLanguage> &value, TSendEventEnum event);

	void SetLinkAnyObject(int fieldId, bool notify);
	void SetLink(int fieldId, const TVisObjRef &value, bool notify);
	void RemoveLink(int fieldId, const TId &id, bool notify);
	void ClearLink(int fieldId, bool notify);
	void GetObjectsLinkedTo(TVList &out, std::vector<int> &fields) const;
	bool ContainsLinkTo(const TVisObjRef &other, std::vector<int> &fields, std::vector<bool> &isList,
	                    std::vector<bool> &isParent);
	unsigned long GetSizeMemory(bool deep, bool unused) const;
	void CopyContent(const TVisObjRef &source, bool copyParents, std::vector<int> *skip);
	void SetTemporary(bool temporary);
	long GetLastModified() const;
	void SetLastModified(long stamp);
	void SetLastModified();
	void RegisterEventHandler(TEventHandlerInterface *handler, TEventEnum event);
	void UnRegisterEventHandler(TEventHandlerInterface *handler);
	TVisionaire *GetVisionaire() const;

private:
	TVisionaireObject *_object;
};
