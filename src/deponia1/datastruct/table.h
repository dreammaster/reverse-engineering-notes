// Original path confirmed via x_assert() calls: src/datastruct/table.cpp -
// see manifest/source_layout.tsv.
//
// Confirmed (Deponia_Linux.asm lines 660697-668540; the editor-side methods
// noted below are only partly read). A TTable keeps all the objects of one
// table of a TVisionaire (scenes, characters, interfaces, ...): they are
// created in id order, so the list is sorted by id and is searched through an
// id -> position hash index (kept here as a std::unordered_map instead of the
// original's 0x1000-bucket chained table, and rebuilt if the sizes ever
// disagree, as the original does). A second list, sorted by name, is built the
// first time a lookup by name is made and then maintained.
//
// A table also hands out new ids (the next one is kept in 24 bits), knows the
// highest id the project file had when it was last saved (the "versioned id",
// which is how new/deleted/changed objects are told apart for merging) and, for
// the tables of an "active" (savegame) type, which field of an active object
// links back to the static object it was created from.
//
// Not read in detail (implemented from their names and the call sites only):
// GetChangedObjects(), GetDeletedObjects(), GetConflictedObjectsCount(),
// ImportNewObjects(), MergeObjects(), FixOrderOfImportedObject(), BeginPaste()
// /EndPaste(); they matter to the editor, not to the player.
#pragma once

#include <unordered_map>
#include <vector>

#include "WxStub.h"
#include "datastruct/compareinfo.h"
#include "datastruct/typegrp.h"
#include "datastruct/visobjref.h"
#include "datastruct/vlist.h"
#include "vscommon/scripting/id.h"

class TProjectFileWriter;
class TVisionaire;
class TVisionaireObject;

class TTable {
public:
	/** `description` is the XML name id of the table's elements, `identifier`
	 *  the table number objects carry in their ids; `activeLinkField` is the
	 *  field of an active object that points back to the object it was made
	 *  from (-1 if the table isn't an active one). */
	TTable(TVisionaire *visionaire, int description, int identifier, const wxString &name, int unknown,
	       TTypeGroup *typeGroup, int activeLinkField, bool flag);
	~TTable();

	int GetIdentifier() const {
		return _identifier;
	}
	int GetDescription() const {
		return _description;
	}
	const wxString &GetTableName() const {
		return _name;
	}
	bool GetTypeGroup(int &description, const TTypeGroup **typeGroup) const {
		description = _description;
		*typeGroup = _typeGroup;
		return true;
	}
	TTypeGroup *GetTypeGroup() {
		return _typeGroup;
	}
	unsigned long GetCount() const {
		return _objects.size();
	}
	/** The object at a position of the id-ordered list. */
	bool GetObjectAtPosition(TVisionaireObject **object, int position) const;

	TVisionaireObject *GetObject(const TId &id) const;
	bool GetObject(const TId &id, TVisObjRef &out, bool warn) const;
	TVisionaireObject *GetObject(const wxString &name) const;
	bool GetObjectPosition(const TId &id, unsigned long &position) const;
	bool GetObjectPosition(const wxString &name, const TId &id, unsigned long &position) const;
	bool GetByName(const wxString &name, TVisObjRef &out);
	/** All the objects (references), by order if `sortByOrder`. */
	bool GetList(TVList &out, bool sortByOrder) const;
	/** The active object made from the object with the given id (null if
	 *  none; logged if this isn't an active table). */
	TVisionaireObject *GetActiveObject(const TId &id);
	TVisionaireObject *CreateActiveObject(const TId &id);

	/** Creates the object while a project or savegame is read. */
	TVisionaireObject *CreateObjectLoad(int id);
	/** Creates a new object with the next id, placed after the last sibling
	 *  under `parent`'s list `field`. */
	TVisionaireObject *CreateObject(TVisionaireObject *parent, int field, bool flag);
	/** Creates an object with a given id (which must not exist). */
	TVisionaireObject *CreateObjectWithId(const TId &id, int order);
	/** Copies an object into this table under a new id. */
	TVisionaireObject *PasteObject(const TVisionaireObject *source, const TId &parentId, int parentField);
	void BeginPaste();
	void EndPaste();

	/** Removes an object from the table (dropping the table's reference). */
	bool RemoveObject(TVisionaireObject *object);
	void Clear();
	/** Called when an object was renamed; `oldPosition` is its position in the
	 *  name-ordered list before. */
	void ObjectNameChanged(const TVisionaireObject *object, int oldPosition);
	/** Reports (logs) objects whose parent doesn't exist. */
	void ValidateParentLinks();

	/** The order moves swap the orders of an object and its sibling(s) in the
	 *  parent's list; false if the object has no parent or is already there. */
	bool MoveOrderFirst(const TVisionaireObject *object);
	bool MoveOrderLast(const TVisionaireObject *object);
	bool MoveOrderUp(const TVisionaireObject *object);
	bool MoveOrderDown(const TVisionaireObject *object);

	unsigned long GetSizeMemory() const;
	unsigned long GetSizeMemory(const TVisionaireObject &object, bool deep) const;
	bool Serialize(TProjectFileWriter &writer);

	/** Tables of the savegame type 1 hold active objects. */
	bool IsActiveTable() const;
	void ResetActiveData();
	void CreateTempData();
	void RemoveTempData();

	/** Keeps the next-id counter ahead of every id in use. */
	void NotifyNewId(int id);
	void InitVersionedId();
	void SetVersionedId(long id) {
		_versionedId = (int)id;
	}

	void GetNewObjects(TVList &out) const;
	void GetDeletedObjects(const TTable *other, TVList &out) const;
	void GetChangedObjects(TVList &out) const;
	int GetConflictedObjectsCount(const TTable *other) const;
	void ImportNewObjects(const TVisionaire &source, TTable *sourceTable);
	void MergeObjects(const TTable *other, TVisObjectCompareInfo &info);
	void FixOrderOfImportedObject(TVisionaireObject *object);

private:
	/** Adds a created object to the lists and indexes. */
	void addObject(TVisionaireObject *object, bool atEnd);
	void rebuildIndex();
	void ensureNameIndex() const;
	int nameInsertPosition(const wxString &name) const;
	int allocateId();

	int _description;                                  // +0x24
	int _identifier;                                   // +0x20
	wxString _name;                                    // +0x28
	TTypeGroup *_typeGroup;                            // +0x30
	std::vector<TVisionaireObject *> _objects;         // +0x38, ordered by id
	std::unordered_map<int, int> _index;               // +0x00/+0x10/+0x18, id -> position
	mutable std::vector<TVisionaireObject *> _byName;  // +0x50, ordered by name
	mutable bool _nameIndexBuilt;                      // +0x80
	int _activeLinkField;                              // +0x84
	bool _flag;                                        // +0x88
	int _unknown;                                      // +0x8C
	int _versionedId;                                  // +0x90
	int _pasteBase;                                    // +0x94
	int _nextId;                                       // +0x98 (24 bits)
	TVisionaire *_visionaire;                          // +0xA0
};
