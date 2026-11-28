// Original path confirmed via x_assert() calls: src/datastruct/visionaireobject.cpp -
// see manifest/source_layout.tsv.
//
// Confirmed (Deponia_Linux.asm lines 586479-595389 and 587318-595389): the
// engine's game-data object - one scene, character, action, ... - as a
// reference-counted wrapper around its record (a TDataGroup). The wrapper owns
// the object's identity (id and table, parent id and field, order), a Lua
// handle and the reference count; everything about the fields is forwarded to
// the TDataGroup (see datastruct/datagrp.h), after checking that the object is
// "valid" (not removed, with an id and with a record) - an invalid object
// answers every getter with a default and ignores every setter.
//
// Layout of the original (the members' names are invented): vptr (+0x00),
// id (+0x08), parent id (+0x0C), order (+0x10, 24 bits), flags (+0x13: bit 0
// "any object", bit 1 "record stored in the same memory block", bit 2
// "removed"), record (+0x18), TVisionaire (+0x20), reference count (+0x28,
// starts at 1), parent field (+0x2C), Lua handle (+0x30).
//
// TVList holds a reference to each of its objects: GetReference()/Release()
// are how (Release() deletes the object when the last one goes).
#pragma once

#include <cstdint>
#include <vector>

#include "TCharHolder.h"
#include "TTextLanguage.h"
#include "datastruct/data.h"
#include "datastruct/datagrp.h"
#include "datastruct/eventhandler.h"
#include "datastruct/typegrp.h"
#include "datastruct/visobjref.h"
#include "datastruct/vlist.h"
#include "vscommon/scripting/id.h"

// Packs a TVisObjRef::GetId()/TVisionaireObject::GetId() 3-byte id into a
// 32-bit value the same way every confirmed hash-lookup site does it
// (TGameControl::StartDialog/EndDialog/GetCharacter, TFontManager::
// GetFont/SetCurrentFont/Initialize, and others): byte0 | (byte1<<8) |
// (sign-extended byte2<<16) - the sign extension of the third byte is
// confirmed (an `and 0xFF000000` masking a `sar 0x1F`-derived sign mask in
// the disassembly), its purpose is not.
inline int PackVisId(const std::uint8_t *id) {
	return id[0] | (id[1] << 8) | (static_cast<int>(static_cast<std::int8_t>(id[2])) << 16);
}

class TVisionaire;
class TTable;

class TVisionaireObject {
public:
	explicit TVisionaireObject(TVisionaire *visionaire);
	TVisionaireObject(bool anyObject, TVisionaire *visionaire);
	TVisionaireObject(int id, int order, int table, TVisionaire *visionaire, const TTypeGroup *typeGroup,
	                  bool inPlace);
	virtual ~TVisionaireObject();

	TVisionaireObject(const TVisionaireObject &) = delete;
	/** The original's assignment operators are stubs that assert (false). */
	TVisionaireObject &operator=(const TVisionaireObject &other);
	TVisionaireObject &operator=(const TVisionaireObject *other);
	bool operator==(const TVisionaireObject &other) const;

	/** What new objects are called until they are named (shared by all). */
	static void SetNameNewObject(const TCharHolder &name);
	static TCharHolder &GetNameNewObject();

	/** Records the object's parent and the field it hangs under, and links the
	 *  parent to it with a parent link. */
	void Init(TVisionaireObject *parent, int field);

	void SetLuaObject(int handle) {
		_luaObject = handle;
	}
	int GetLuaObject() const {
		return _luaObject;
	}

	/** Reference counting: the object starts with one reference. */
	TVisionaireObject *GetReference();
	void Release();

	/** Detaches the object: optionally from its visionaire, unlinks it from the
	 *  objects linking to it, drops its record and marks it removed; then
	 *  releases the caller's reference. */
	void Remove(bool fromVisionaire, bool unlink);

	TVisionaire *GetVisionaire() const {
		return _visionaire;
	}
	const TTypeGroup *GetTypeGroup();
	TVisionaireObject *GetParent() const;
	/** Sets the parent id and field directly (used when an object is pasted). */
	void SetParent(const TId &parentId, int field) {
		_parentId = parentId;
		_parentField = (short)field;
	}
	TId GetParentId() const {
		return _parentId;
	}
	int GetParentField() const {
		return _parentField;
	}
	bool IsMemoryBlocked() const {
		return (_flags & kMemoryBlocked) != 0;
	}
	bool IsAnyObject() const {
		return (_flags & kAnyObject) != 0;
	}
	/** True for a removed object, one without an id, or one without a record. */
	bool IsEmpty() const;
	bool IsTemporary() const;
	void SetTemporary(bool temporary);

	/** The id as its 4 raw bytes (the same shape as TVisObjRef::GetId()). */
	const std::uint8_t *GetId() const {
		return reinterpret_cast<const std::uint8_t *>(&_id);
	}
	TId GetTId() const {
		return _id;
	}
	/** The 24-bit id and order. */
	void SetId24(int id) {
		_id = TId(id, _id.getTable());
	}
	int GetId24() const {
		return _id.getId();
	}
	void SetOrder(int order) {
		_order = order & 0xFFFFFF;
		if (_order & 0x800000)
			_order -= 0x1000000;
	}
	void SetOrder24(int order) {
		SetOrder(order);
	}
	int GetOrder() const {
		return _order;
	}
	int GetOrder24() const {
		return _order;
	}
	bool ChangeOrder(TMoveOrderEnum move);

	int GetTypeLink(int field) const;
	int GetTypeField(int field) const;

	bool SetName(const TCharHolder &name);
	const TCharHolder &GetName() const;
	/** The name preceded by those of up to `depth` parents ("parent: name"). */
	wxString GetNameWithParents(int depth) const;
	/** The name the editor lists the object under. */
	const wxString &GetNameInList() const;

	long GetLastModified() const;
	void SetLastModified(long stamp);
	void SetLastModified();

	// Field access. Getters answer a default (-1, 0, false, empty...) when the
	// object isn't valid or the field isn't there.
	bool SetValue(int field, const void *value, eTypeData type, TSendEventEnum event);
	bool IsFieldEmpty(int field) const;
	void *GetValue(int field, eTypeData type) const;
	void GetTexts(int field, std::vector<TTextLanguage> **out) const;
	void GetPaths(int field, std::vector<TCharHolder> **out) const;
	int GetInt(int field) const;
	float GetFloat(int field) const;
	bool GetBool(int field) const;
	wxString GetStr(int field) const;
	const TCharHolder &GetStrHolder(int field) const;
	wxFileName GetPath(int field) const;
	const wxPoint *GetPoint(int field) const;
	const wxRect *GetRect(int field) const;
	const TSprite &GetSprite(int field) const;
	/** The object a link field points to, or null (also for an "any" link). */
	TVisionaireObject *GetLink(int field) const;
	const TLink &GetLinkId(int field) const;
	TVisionaireObject *GetLinkByNameIgnoreCase(const wxString &name, int field);
	TVisionaireObject *GetLinkByName(const wxString &name, int field);
	/** Fills `out` with the objects a link-list field points to. */
	bool GetList(int field, TVList &out) const;
	/** The objects of a link-list field; sorted by order if `order` is 1. */
	bool GetLinks(int field, TypeOrder order, TVList &out) const;
	unsigned long GetLinksListSize(int field) const;
	int GetLinksHighestOrder(int field) const;

	bool SetValue(int field, bool value, TSendEventEnum event);
	bool SetValue(int field, int value, TSendEventEnum event);
	bool SetValue(int field, float value, TSendEventEnum event);
	bool SetValue(int field, const TCharHolder &value, TSendEventEnum event);
	bool SetValue(int field, const wxPoint &value, TSendEventEnum event);
	bool SetValue(int field, const wxRect &value, TSendEventEnum event);
	bool SetValue(int field, const wxFileName &value, TSendEventEnum event);
	bool SetValue(int field, const wxString &value, TSendEventEnum event);
	bool SetValue(int field, const TSprite &value, TSendEventEnum event);
	bool SetValue(int field, const std::vector<wxRect> &value, TSendEventEnum event);
	bool SetValue(int field, const std::vector<TSprite> &value, TSendEventEnum event);
	bool SetValue(int field, const std::vector<wxPoint> &value, TSendEventEnum event);
	bool SetValue(int field, const std::vector<TCharHolder> &value, TSendEventEnum event);
	bool SetPathList(int field, const std::vector<TCharHolder> &value, TSendEventEnum event);
	bool SetValue(int field, const std::vector<int> &value, TSendEventEnum event);
	bool SetValue(int field, const std::vector<float> &value, TSendEventEnum event);
	bool SetValue(int field, const std::vector<TTextLanguage> &value, TSendEventEnum event);
	/** Replaces a link-list field by the objects of the list. */
	bool SetValue(int field, const TVList &objects, bool notify);

	bool ClearLink(int field, bool notify);
	bool SetLinkAnyObject(int field, bool notify);
	bool SetLink(int field, const TId &id, bool notify);
	bool RemoveLink(int field, const TId &id, bool notify);
	/** Whether a link or link-list field points to `other`. */
	bool IsLinked(int field, const TVisionaireObject *other) const;

	void CopyContent(const TVisionaireObject *source, bool copyParents, std::vector<int> *skip);
	unsigned long GetSizeMemory(bool deep, bool unused) const;
	unsigned long GetLinksSizeMemory(int field) const;

	void RegisterEventHandler(TEventHandlerInterface *handler, TEventEnum event);
	void UnRegisterEventHandler(TEventHandlerInterface *handler);

	void GetRects(int field, std::vector<wxRect> &out) const;
	void GetPoints(int field, std::vector<wxPoint> &out) const;
	void GetSprites(int field, std::vector<TSprite> &out) const;
	void GetPaths(int field, std::vector<TCharHolder> &out) const;
	void GetStrings(int field, std::vector<TCharHolder> &out) const;
	void GetInts(int field, std::vector<int> &out) const;
	void GetFloats(int field, std::vector<float> &out) const;
	void GetTextsCopy(int field, std::vector<TTextLanguage> &out) const;

	/** The objects linking to this one and the fields they do it through. */
	void GetObjectsLinkedTo(TVList &out, std::vector<int> &fields) const;
	/** The objects hanging under this one (through its parent-link fields). */
	void GetChildren(TVList &out) const;
	/** The fields of this object that link to `other`, and for each whether
	 *  it is a link list and whether it is a parent link. */
	bool ContainsLinkTo(const TVisionaireObject &other, std::vector<int> &fields, std::vector<bool> &isList,
	                    std::vector<bool> &isParent);

	/** The record, or null. */
	TDataGroup *GetData() const {
		return _data;
	}

private:
	enum {
		kAnyObject = 1,
		kMemoryBlocked = 2,
		kRemoved = 4
	};

	/** The "valid" test every forwarding method starts with. */
	bool isValid() const {
		return !(_flags & kRemoved) && _id.getId() != -1 && _data != nullptr;
	}
	/** The unlinking part of Remove() and the destructor. */
	void unlinkFromObjectsLinkedTo();

	TId _id;                  // +0x08
	TId _parentId;            // +0x0C
	int _order;               // +0x10 (24 bits, signed)
	unsigned char _flags;     // +0x13
	TDataGroup *_data;        // +0x18
	TVisionaire *_visionaire; // +0x20
	int _refCount;            // +0x28
	short _parentField;       // +0x2C
	int _luaObject;           // +0x30
};

/** Ordering by object order / by id (asm lines 586479-586512, 587152+). */
bool cmpOrder(const TVisionaireObject *a, const TVisionaireObject *b);
bool cmpId(const TVisionaireObject *a, const TVisionaireObject *b);
