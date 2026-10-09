// Original path confirmed via x_assert() calls: src/datastruct/visionaire.cpp -
// see manifest/source_layout.tsv.
//
// TVisionaire is the root of the game data: the registry of all the tables
// (TTable, one per kind of object), the "main object" (the game itself, an
// object with the fixed id {1, table -1}), the empty and "any" placeholder
// objects, the link table that records which objects link to which (only kept
// in the editor: in player mode AddLink()/RemoveLink() do nothing), the
// id-mapping lists used when objects are pasted or merged from another
// project, and the loading and saving of whole projects and savegames.
// TVisionaireGame (vstables/visionaireGame.h) is the subclass that registers
// the game's own tables.
//
// Confirmed (Deponia_Linux.asm lines 608417-620665, the loading/saving and the
// merge/import editor methods noted below excepted). Layout of the original
// (members are named here): game id (+0x08), project type (+0x14: 0 master,
// 1 slave), revision (+0x18), temporary parent links (+0x28), link table
// (+0x40, sorted by target, owner, field; its stored size +0x58), id mappings
// (+0x60), "has id mapping" (+0x78), project path (+0x80), modified (+0x88),
// shutting down (+0x89), tables by number (+0x90) and in creation order
// (+0xA8), main object (+0xC0), game type group (+0xC8), empty/any objects
// (+0xD0/+0xD8), table-name maps (+0xE0/+0x100/+0x120), table names (+0x140),
// object memory pool (+0x158).
#pragma once

#include <map>
#include <string>
#include <vector>

#include "TSignalSlot.h"
#include "baselib/xmlWriter.h"
#include "datastruct/compareinfo.h"
#include "datastruct/typegrp.h"
#include "datastruct/visenums.h"
#include "datastruct/visobjref.h"
#include "datastruct/vlist.h"
#include "vscommon/scripting/id.h"

class EventHandler;
class TLink;
class TTable;
class TVedFile;
class TVisionaireObject;


// One entry of the link table (12 bytes): the object `from` links to the
// object `to` through its field `field`. Also the shape of the temporary
// parent links the loader collects (from = parent, to = child).
struct TLinkRef {
	TId from;
	TId to;
	int field = -1;
};

// An id remapping from an import: the id in the source project and the id the
// object got here.
struct TMappedId {
	TId from;
	TId to;
};

// The three names a table is known by in the data files.
struct TTableNamesEntry {
	wxString visName;
	wxString singular;
	wxString plural;
};

class TVisionaire {
public:
	TVisionaire(const TTypeGroup &gameTypeGroup, int projectType);
	virtual ~TVisionaire();

	/** Builds the record types for a file-version range (TVisionaireGame
	 *  does; the first virtual of the original). */
	virtual void InitWithVersion(int /*versionLow*/) {
	}

	TVisionaire(const TVisionaire &) = delete;
	TVisionaire &operator=(const TVisionaire &) = delete;

	/** Frees the global name table and field lookup table. */
	static void CleanUp();
	/** Empties every table and the main object. */
	void Clear();
	/** Clears and makes sure there is a main object. */
	bool NewGame();

	void SetModified(bool modified) {
		_modified = modified;
	}
	bool IsModified() const {
		return _modified;
	}
	/** The byte at +0x89 of the original: set while the object tree is torn
	 *  down, when deleting a link record doesn't unregister it. */
	bool IsLinkRemovalSuppressed() const {
		return _shuttingDown;
	}
	void SetDirty() {
		_modified = true;
	}
	bool HasIdMapping() const {
		return _hasIdMapping;
	}
	int GetGameId() const {
		return _gameId;
	}

	/** The main (game) object. */
	TVisObjRef GetGame() const;
	TVisObjRef GetEmptyObject() const;
	TVisObjRef GetAnyObject() const;

	// Tables.
	/** Sizes the table registry (tables are numbered 0..count-1). */
	void CreateTables(int count);
	/** Registers table number `identifier`. False (and logged) if it exists. */
	bool AddTable(int description, int identifier, const wxString &visName, const wxString &singular,
	              const wxString &plural, const TTypeGroup &typeGroup, int activeLinkField, bool flag);
	bool GetTable(int identifier, TTable **outTable) const;
	bool GetTableConst(int identifier, const TTable **outTable) const;
	const TTypeGroup *GetTypeGroup(int identifier, int &description) const;
	/** All the tables exist. */
	void CompleteTables() const;
	const wxString &GetVisTableName(int identifier, bool warn) const;
	const wxString &GetTableNameSingular(int identifier, bool warn) const;
	const wxString &GetTableNamePlural(int identifier, bool warn) const;
	/** The table number for a name (-1 for the game itself, -2 if unknown). */
	int GetTableIdentifierByVisName(const wxString &name, bool warn) const;
	int GetTableIdentifierBySingularName(const wxString &name, bool warn) const;
	int GetTableIdentifierByPluralName(const wxString &name, bool warn) const;
	int GetTableIdentifierByDescription(int description, bool warn) const;

	// Objects.
	TVisionaireObject *GetObjectById(const TId &id) const;
	bool GetObjectById(const TId &id, TVisObjRef &out, bool warn) const;
	bool GetObjectByName(const wxString &name, int table, TVisObjRef &out);
	TVisObjRef CreateObject(int table, TVisObjRef &parent, int field);
	TVisionaireObject *CreateObjectWithId(int table, const TId &id, int order);
	TVisObjRef CloneObject(const TVisObjRef &source, bool flag);
	TVisionaireObject *CopyObject(const TVisionaireObject *source, TVisionaireObject *parent, int field, bool flag);
	TVisObjRef CopyObject(const TVisObjRef &source, TVisObjRef &parent, int field);
	TVisObjRef CreateActiveObject(int table, const TVisObjRef &source);
	TVisObjRef GetActiveObject(int table, const TId &id);
	void ResetActiveData();
	void ResetActiveData(eVisionaireTable table);
	bool RemoveObjectWithoutChildren(TVisObjRef &object);
	/** Removes an object and, recursively, the objects hanging under it. */
	bool RemoveObjectByParent(TVisionaireObject *object);
	bool RemoveObject(TVisionaireObject *object);
	bool RemoveParentLinkedObject(const TLink &link);
	bool GetList(int table, TVList &out, bool sortByOrder) const;
	long GetListSize(int table) const;
	bool ChangeOrder(const TVisObjRef &object, TMoveOrderEnum move);
	unsigned long GetSizeMemory(int table) const;
	void RemoveTempData();
	void CreateTempData();
	/** Reserves memory for `count` objects (a pool in the original; objects
	 *  are allocated individually here, so these only keep the numbers). */
	void InitMemory(int count);
	void *Reserve();

	// Project kind and versioning.
	bool IsMasterProject() const {
		return _projectType == 0;
	}
	bool IsSlaveProject() const {
		return _projectType == 1;
	}
	bool InitAsMasterProject();
	void InitAsSlaveProject();
	int GetRevision() const {
		return _revision;
	}
	void SetRevision(int revision) {
		_revision = revision;
	}
	void SetProjectType(int type) {
		_projectType = type;
	}
	void IncreaseRevision();
	void InitVersionedIds();
	/** The value TVisionaireObject::SetLastModified() stamps. */
	int GetModifiedStamp() const {
		return _revision;
	}
	bool HasPath() const;
	const wxFileName &GetPath() const {
		return _path;
	}

	// Links between objects (kept only outside player mode).
	void AddLink(const TId &from, const TId &to, int field, bool sorted);
	bool RemoveLink(const TId &from, const TId &to, int field);
	void AddParentLink(const TId &parent, const TId &child, int field);
	void GetObjectsLinkedTo(const TId &id, std::vector<TLinkRef> &out) const;
	void StoreLinksCount();
	/** Gives the loaded objects the parents the temporary links recorded. */
	void SetupParents();
	void SortLinks();

	// Pasting and merging objects from another project.
	void BeginPaste();
	void EndPaste();
	void EndMerge();
	/** The id an imported object got, or the empty id. */
	TId GetMappedId(const TId &id) const;
	TVisionaireObject *PasteObject(const TVisionaireObject *source, const TId &parentId, int parentField);
	TVisObjRef PasteObject(const TVisObjRef &source, const TId &parentId, int parentField);
	void GetNewObjects(TVList &out) const;
	void GetDeletedObjects(const TVisionaire &other, TVList &out) const;
	void GetChangedObjects(TVList &out) const;
	int GetConflictedObjectsCount(const TVisionaire &other) const;
	void ImportNewObjects(const TVisionaire &source);
	void MergeObjects(const TVisionaire &other, TVisObjectCompareInfo &info);

	/** Whether the program is the player (no link table, no editing). */
	static void SetVisPlayerMode(bool playerMode);
	static bool IsVisPlayerMode;
	static int ActiveInstances;
	static wxString s_tableNameInvalid;

	// Loading and saving (still to be reconstructed from the project readers
	// and writers; see NOTES.md).
	/** Writes the project or a savegame to `writer` (`forceXml`: also a savegame as XML). */
	bool SaveData(TProjectFileWriter &writer, EventHandler *handler, bool forceXml);
	bool SaveSaveGame(TProjectFileWriter &writer);
	/** Puts the tables and the fields in the scrambled order (see SaveData()). */
	void SetScrambled();
	/** Called before the game data are saved (the editor's TVisionaireGame sorts things); false stops the saving. */
	virtual bool BeforeSave();
	/** The game data as XML text. */
	wxString SaveDataGameToString();

	bool Load(const wxFileName &file, const wxString &extra, eSaveGame saveGame, TLoadingTypeEnum type,
	          int *outFlag, TSignalSlot *slot, EventHandler *handler);
	/** Reads a binary (VBIN) project or savegame (datastruct/binaryProjectReader.cpp). */
	bool BinaryLoad(TVedFile &file, TLoadingTypeEnum type, TSignalSlot *slot, int *outVersion,
	                EventHandler *handler);
	/** Writes the project (`saveGame` false) or a savegame (true) in the binary format that BinaryLoad() reads
	 *  (datastruct/binaryProjectReader.cpp). With `writer` (the savegame writer of the game) the file goes into its
	 *  buffer, else into `file` (compressed). */
	bool BinarySave(const wxFileName &file, EventHandler *handler, bool saveGame, TProjectFileWriter *writer);

protected:
	int tableIndex(const TId &id) const;

	int _gameId;                                 // +0x08
	int _version;                                // +0x0C
	int _projectType;                            // +0x14
	int _revision;                               // +0x18
	std::vector<TLinkRef> _tempParentLinks;      // +0x28
	std::vector<TLinkRef> _links;                // +0x40
	long _linksCount;                            // +0x58
	std::vector<TMappedId> _mappedIds;           // +0x60
	bool _hasIdMapping;                          // +0x78
	wxFileName _loadedFile;                      // +0x20, the file last loaded
	wxFileName _path;                            // +0x80
	bool _modified;                              // +0x88
	bool _shuttingDown;                          // +0x89
	bool _destroying;                            // +0x8A
	std::vector<TTable *> _tables;               // +0x90, by table number
	std::vector<TTable *> _tableList;            // +0xA8, in creation order
	TVisionaireObject *_mainObject;              // +0xC0
	const TTypeGroup *_gameTypeGroup;            // +0xC8
	TVisionaireObject *_emptyObject;             // +0xD0
	TVisionaireObject *_anyObject;               // +0xD8
	std::map<std::wstring, int> _visNameIds;     // +0xE0
	std::map<std::wstring, int> _singularIds;    // +0x100
	std::map<std::wstring, int> _pluralIds;      // +0x120
	std::vector<TTableNamesEntry *> _tableNames; // +0x140
	int _poolSize;                               // +0x164
};
