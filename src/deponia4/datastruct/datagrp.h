// Original path confirmed via x_assert() calls: src/datastruct/datagrp.cpp -
// see manifest/source_layout.tsv.
//
// Confirmed in full (Deponia_Linux.asm lines 600389-607108, all 41
// manifest-listed methods; the compiler's constant-propagated clone of
// ForEachCall at 604508 is the same function). A TDataGroup is one game-data
// record's field storage: two raw blocks laid out by its TTypeGroup - the
// persistent fields (_data) and the temporary ones (_tempData, created on
// demand), each field at TTypeData::GetOffset() - together with the record's
// name, its last-modified stamp and flags, the event handlers registered on
// it, and the TVisionaireObject that owns it (which in turn knows the
// TVisionaire, whose "modified" flag every change raises).
//
// A field is read and written through TData (datastruct/data.h), which knows
// each kind of value. Links (kLink/kLinkList fields) are kept in step with the
// visionaire's link tables: setting, clearing and removing them also adds and
// removes the entries there (TVisionaire::AddLink/RemoveLink), and removing a
// parent link removes the object it pointed to. The members' names are
// invented (the binary carries none); the layout notes give the original
// offsets.
#pragma once

#include <vector>

#include "TCharHolder.h"
#include "datastruct/compareinfo.h"
#include "datastruct/data.h"
#include "datastruct/eventhandler.h"
#include "datastruct/typegrp.h"
#include "datastruct/vlist.h"
#include "datastruct/visobjref.h"
#include "vscommon/scripting/id.h"

class TProjectFileWriter;
class TVisionaire;
class TVisionaireObject;

/** What ForEachCall() does with each field of the record (values 1, 2, 3 and
 *  5 are the ones the switch handles; the names are invented). */
enum class eActivity : int {
	kSerializeContent = 1, // `a` is a TProjectFileWriter: write the list/point/rect/sprite/link fields as elements
	kSerializeParams = 2,  // `a` is a TProjectFileWriter: write the plain fields as attributes
	kClear = 3,            // reset every field
	kCompare = 5           // `a` is the TDataGroup to compare with, `b` a TVisObjectCompareInfo
};

class TDataGroup {
public:
	TDataGroup(const TTypeGroup *typeGroup, TVisionaireObject *owner);
	~TDataGroup();

	TDataGroup(const TDataGroup &) = delete;
	TDataGroup &operator=(const TDataGroup &) = delete;

	/** Creates the storage of the temporary fields (the record must not have
	 *  any yet) / destroys it. */
	void CreateTempData();
	void RemoveTempData();

	const TTypeGroup *GetTypeGroupPtr() const {
		return _typeGroup;
	}
	TTypeGroup *GetTypeGroupPtrNonConst() {
		return const_cast<TTypeGroup *>(_typeGroup);
	}

	/** Tells the handlers registered for `event` that field `field` changed;
	 *  `linked`, when it isn't empty, names the object the change concerns. */
	void NotifyEvent(int field, TEventEnum event, TId *linked);
	void RegisterEventHandler(TEventHandlerInterface *handler, TEventEnum event);
	void UnRegisterEventHandler(TEventHandlerInterface *handler);

	/** The address of a field's value, or null if it has none (no storage, or
	 *  its block doesn't exist). */
	void *GetData(const TTypeData &type) const;
	/** The address of a field's value of the given kind; null (and logged) if
	 *  the record has no such field or it is of another kind. */
	void *GetValue(int field, eTypeData type) const;

	/** Stores a value into a field (see TSendEventEnum for when). A string
	 *  value may be stored in a path field and vice versa; ValueInt and
	 *  ValueFloat mirror each other. False if the field's kind doesn't match. */
	bool SetValue(int field, const void *value, eTypeData type, TSendEventEnum event);

	/** Links. A link field points to one object; a link-list field to many (an
	 *  entry is a parent link if the other object is this one's child). */
	bool SetLink(int field, const TId &id, bool parent, bool notify);
	bool SetLink(int field, const TId &id, bool notify);
	bool SetParentLink(int field, const TId &id, bool notify);
	bool ClearLink(int field, bool notify);
	bool SetLinkAnyObject(int field, bool notify);
	/** Removes the link to `id` from a link/link-list field; false if there
	 *  was none or `id` is empty. */
	bool RemoveLink(int field, const TId &id, bool notify);
	/** Removes every link of a link-list field (the flag is not used). */
	void ClearLinks(int field, bool notify);
	/** After an id remapping: re-points the links to the mapped ids and drops
	 *  those to objects that don't exist. */
	void ValidateAndAdaptLinks();
	/** The objects a link-list field points to that exist, ordered; false if
	 *  the list can't be read. */
	bool GetList(int field, TVList &out) const;
	unsigned long GetListSize(int field) const;

	/** Runs an activity over every needed field (see eActivity); false if
	 *  one of them failed. */
	bool ForEachCall(eActivity activity, void *a, void *b);
	void Clear();
	/** Copies the fields (and the name) of another record, except those listed
	 *  in `skip`; links are re-registered, mapped to new ids when the
	 *  visionaire has an id mapping. Parent links are only copied if `copyParents`. */
	void CopyContent(const TDataGroup &source, bool copyParents, std::vector<int> *skip);

	/** The record's own data: its id and order (stored in the owner) and its
	 *  last-modified stamp (24 bits) and name. */
	void Serialize(int id, int order, int lastModified, const TCharHolder &name);
	/** Writes the whole record as an element. */
	bool Serialize(TProjectFileWriter &writer);

	bool SetName(const TCharHolder &name);
	const TCharHolder &GetName() const {
		return _name;
	}
	TCharHolder &GetNameNonConst() {
		return _name;
	}
	TId GetId() const;
	long GetLastModified() const {
		return _lastModified;
	}
	unsigned long GetSizeMemory(bool deep) const;

	bool IsDirty() const {
		return (_flags & kDirty) != 0;
	}
	void SetDirty() {
		_flags |= kDirty;
	}
	bool IsTemporary() const {
		return (_flags & kTemporary) != 0;
	}
	void SetTemporary(bool temporary) {
		_flags = temporary ? (_flags | kTemporary) : (_flags & ~kTemporary);
	}
	bool IsOrderContainsPosition() const {
		return (_flags & kOrderContainsPosition) != 0;
	}
	void SetOrderContainsPosition(bool value) {
		_flags = value ? (_flags | kOrderContainsPosition) : (_flags & ~kOrderContainsPosition);
	}

	TVisionaireObject *GetOwner() const {
		return _owner;
	}

private:
	enum {
		kDirty = 1,
		kTemporary = 2,
		kOrderContainsPosition = 4
	};

	/** What an event handler registered for (+0 the handler, +8 the event). */
	struct TEventHandlerAndField {
		TEventHandlerInterface *handler;
		TEventEnum event;
	};

	/** The shared part of RemoveLink() (the original's compiler clone). */
	bool removeLinkOf(int field, const TId &id, bool notify);
	/** The visionaire the owner belongs to. */
	TVisionaire *visionaire() const;
	/** Raises the visionaire's "modified" flag and this record's. */
	void markModified();

	TCharHolder _name;                                    // +0x00
	char *_data;                                          // +0x10
	char *_tempData;                                      // +0x18
	TVisionaireObject *_owner;                            // +0x20
	std::vector<TEventHandlerAndField *> _handlers;       // +0x28
	const TTypeGroup *_typeGroup;                         // +0x40
	int _lastModified;                                    // +0x48 (24 bits, signed)
	unsigned char _flags;                                 // +0x4B
};
