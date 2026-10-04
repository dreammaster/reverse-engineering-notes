#include "datastruct/binaryProjectReader.h"

#include <cstring>

#include "Diagnostics.h"
#include <EventHandler.h>
#include "LoadSaveProgressEvent.h"
#include "TCharHolder.h"
#include "TSprite.h"
#include "TTextLanguage.h"
#include "datastruct/data.h"
#include "datastruct/datagrp.h"
#include "datastruct/link.h"
#include "datastruct/table.h"
#include "datastruct/typegrp.h"
#include "datastruct/vedfile.h"
#include "datastruct/visionaire.h"
#include "datastruct/visionaireobject.h"
#include "vstables/fieldIds.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/datastruct/binaryProjectReader.cpp";

TMemoryBuffer membuf;
unsigned long offset = 0;
bool g_binaryReadOverrun = false;
float X_LoadProgress = 0.0f;

// Reads `size` bytes at the cursor (zeros, and the overrun flag, if the buffer
// is too short - the original reads whatever is there).
static bool readBytes(void *dest, unsigned long size) {
	if (offset + size > membuf.GetLen() || !membuf.GetData()) {
		std::memset(dest, 0, size);
		g_binaryReadOverrun = true;
		offset += size;
		return false;
	}

	std::memcpy(dest, membuf.GetData() + offset, size);
	offset += size;
	return true;
}

static int readInt() {
	int value = 0;
	readBytes(&value, 4);
	return value;
}

static unsigned short readUShort() {
	unsigned short value = 0;
	readBytes(&value, 2);
	return value;
}

static unsigned char readByte() {
	unsigned char value = 0;
	readBytes(&value, 1);
	return value;
}

// Confirmed (asm lines 652737-653126): u16 id (at most 0x347), u16 count (at
// most 999), then `count` children.
bool make_structure(bin_structure &out) {
	out.id = readUShort();
	if (out.id > 0x347)
		return false;

	int count = (short)readUShort();
	if (count > 0x3E7)
		return false;

	for (int i = 0; i < count; i++) {
		bin_structure child;
		if (!make_structure(child) || g_binaryReadOverrun)
			return false;
		out.children.push_back(child);
	}
	return true;
}

void load(int &value, std::FILE * /*file*/) {
	value = readInt();
}

// Confirmed (asm lines 649811-649829): only skips 8 bytes, the value is left
// alone.
void load(long & /*value*/, std::FILE * /*file*/) {
	offset += 8;
}

void load(bool &value, std::FILE * /*file*/) {
	value = readByte() != 0;
}

void load(char &value, std::FILE * /*file*/) {
	value = (char)readByte();
}

void load(float &value, std::FILE * /*file*/) {
	readBytes(&value, 4);
}

// Confirmed (asm lines 649906-650015): int length, then that many bytes (the
// length includes the terminator).
void load(TCharHolder &value, std::FILE * /*file*/) {
	long length = readInt();
	if (length < 0 || offset + length > membuf.GetLen()) {
		g_binaryReadOverrun = true;
		value.resize(0);
		return;
	}

	value.resize(length);
	if (length > 0)
		readBytes(const_cast<char *>(value.mb_str()), length);
}

void load(wxPoint &value, std::FILE * /*file*/) {
	value.x = readInt();
	value.y = readInt();
}

void load(wxRect &value, std::FILE * /*file*/) {
	value.x = readInt();
	value.y = readInt();
	value.width = readInt();
	value.height = readInt();
}

// Confirmed (asm lines 650106-650201): path, pause, name, position, then the
// transparency mode and colour.
void load(TSprite &value, std::FILE *file) {
	load(value.GetPathNonConst(), file);
	int pause = readInt();
	load(value.GetNameNonConst(), file);

	wxPoint position;
	position.x = readInt();
	position.y = readInt();
	int mode = readInt();
	int color = readInt();

	value.SetPosition(position, 1.0f);
	value.SetPause(pause);
	value.SetTransparency(static_cast<eTransparencyMode>(mode), (unsigned int)color);
}

// Confirmed (asm lines 650202-650274): id, table, then the two flag bytes
// (the first is the "any" flag, the second the "parent" flag); `field` is the
// description of the field the link is stored in.
void load(TLink &value, int field, std::FILE * /*file*/) {
	int id = readInt();
	int table = readInt();
	bool any = readByte() != 0;
	bool parent = readByte() != 0;

	value.Serialize(id, table, field, parent, any);
}

// The vector loaders: an int count, then that many elements.
static unsigned long readCount() {
	int count = readInt();
	if (count < 0 || (unsigned long)count > membuf.GetLen() - (offset < membuf.GetLen() ? offset : membuf.GetLen())) {
		// every element is at least a byte: more than that is certainly corrupt
		g_binaryReadOverrun = true;
		return 0;
	}
	return (unsigned long)count;
}

void load(std::vector<TLink> &values, int field, std::FILE *file) {
	unsigned long count = readCount();

	values.clear();
	values.resize(count);
	for (unsigned long i = 0; i < count; i++)
		load(values[i], field, file);
}

void load(std::vector<wxRect> &values, std::FILE *file) {
	unsigned long count = readCount();

	values.clear();
	values.resize(count);
	for (unsigned long i = 0; i < count; i++)
		load(values[i], file);
}

void load(std::vector<wxPoint> &values, std::FILE *file) {
	unsigned long count = readCount();

	values.clear();
	values.resize(count);
	for (unsigned long i = 0; i < count; i++)
		load(values[i], file);
}

void load(std::vector<TSprite> &values, std::FILE *file) {
	unsigned long count = readCount();

	values.clear();
	values.resize(count);
	for (unsigned long i = 0; i < count; i++)
		load(values[i], file);
}

void load(std::vector<TCharHolder> &values, std::FILE *file) {
	unsigned long count = readCount();

	values.clear();
	values.resize(count);
	for (unsigned long i = 0; i < count; i++)
		load(values[i], file);
}

void load(std::vector<float> &values, std::FILE *file) {
	unsigned long count = readCount();

	values.clear();
	values.resize(count);
	for (unsigned long i = 0; i < count; i++)
		load(values[i], file);
}

void load(std::vector<int> &values, std::FILE *file) {
	unsigned long count = readCount();

	values.clear();
	values.resize(count);
	for (unsigned long i = 0; i < count; i++)
		load(values[i], file);
}

// Confirmed (asm lines 651632-651921): text, audio file, language id.
void load(std::vector<TTextLanguage> &values, std::FILE *file) {
	unsigned long count = readCount();

	values.clear();
	values.resize(count);
	for (unsigned long i = 0; i < count; i++) {
		load(values[i].text, file);
		load(values[i].audioFile, file);
		values[i].languageId = readInt();
	}
}

// Confirmed (asm lines 651922-652736): a switch over the 19 kinds; kind 5 (and
// anything above 18) is asserted against.
void loadObject(std::FILE *file, void *data, int type, int field) {
	switch (type) {
	case 0:
		GetRefBool(data) = readByte() != 0;
		break;
	case 1:
		GetRefInt(data) = readInt();
		break;
	case 2:
		load(GetRefString(data), file);
		break;
	case 3:
		load(GetRefPath(data), file);
		break;
	case 4:
		readBytes(&GetRefFloat(data), 4);
		break;
	case 6:
		load(GetRefVRect(data), file);
		break;
	case 7:
		load(GetRefVSprite(data), file);
		break;
	case 8:
		load(GetRefVPoint(data), file);
		break;
	case 9:
		load(GetRefVString(data), file);
		break;
	case 10:
		load(GetRefVInt(data), file);
		break;
	case 11:
		load(GetRefVPath(data), file);
		break;
	case 12:
		load(GetRefVFloat(data), file);
		break;
	case 13:
		load(GetRefPoint(data), file);
		break;
	case 14:
		load(GetRefRect(data), file);
		break;
	case 15:
		load(GetRefSprite(data), file);
		break;
	case 16:
		load(GetRefLink(data), field, file);
		break;
	case 17:
		load(GetRefLinks(data), field, file);
		break;
	case 18:
		load(GetRefVText(data), file);
		break;
	default:
		x_assert(false, "false", kSourceFile, 0x24F);
		break;
	}
}

static bool isEmptyId(const TId &id) {
	return id.getId() == -1 && id.getTable() == 0xFF;
}

// What has to happen to the link table around reading one link / link list
// into an object that may already hold values (a savegame is loaded over an
// existing project): the old links are dropped from the table first ...
static void removeOldLinks(TVisionaire *visionaire, TVisionaireObject *object, int type, void *data) {
	if (type == 16) {
		TLink &link = GetRefLink(data);

		if (!isEmptyId(link.GetId()) && !link.IsParentLink())
			visionaire->RemoveLink(object->GetTId(), link.GetId(), link.GetField());
	} else if (type == 17) {
		std::vector<TLink> &links = GetRefLinks(data);

		for (const TLink &link : links) {
			x_assert(!isEmptyId(link.GetId()), "!it->GetId().IsEmpty()", kSourceFile, 0x44C);
			if (!link.IsParentLink())
				visionaire->RemoveLink(object->GetTId(), link.GetId(), link.GetField());
		}
		links.clear();
	}
}

// ... and the new ones are entered afterwards (parent links wait in the
// temporary list until the whole file is read).
static void addNewLinks(TVisionaire *visionaire, TVisionaireObject *object, int type, void *data) {
	if (type == 16) {
		TLink &link = GetRefLink(data);

		if (link.IsAnyLink())
			return;
		if (link.IsParentLink())
			visionaire->AddParentLink(object->GetTId(), link.GetId(), link.GetField());
		else
			visionaire->AddLink(object->GetTId(), link.GetId(), link.GetField(), false);
	} else if (type == 17) {
		std::vector<TLink> &links = GetRefLinks(data);

		for (const TLink &link : links) {
			if (link.IsParentLink())
				visionaire->AddParentLink(object->GetTId(), link.GetId(), link.GetField());
			else
				visionaire->AddLink(object->GetTId(), link.GetId(), link.GetField(), false);
		}
	}
}

// The fields every record has in its own header instead (name, id, order,
// lastModified): not read as ordinary fields.
static bool isHeaderField(int description) {
	return description == kName || description == kId || description == kOrder || description == kLastModified;
}

// Reads the fields the file stores for one record, in the file's order.
// `unloadLinks` is whether links already in the object are taken out of the
// link table before they are overwritten; `alwaysAddLinks` whether the new
// ones are entered whatever that was.
static void loadFields(TVisionaire *visionaire, TVisionaireObject *object, TTypeGroup *typeGroup,
                       const bin_structure &fileTable, bool unloadLinks, bool alwaysAddLinks) {
	float length = (float)membuf.GetLen();

	for (const bin_structure &fileField : fileTable.children) {
		X_LoadProgress = (float)offset / length;

		if (isHeaderField(fileField.id))
			continue;

		const TTypeData &typeData = typeGroup->GetTypeData(fileField.id);
		void *data = object->GetData()->GetData(typeData);
		int type = (int)typeData.GetType();

		if (unloadLinks)
			removeOldLinks(visionaire, object, type, data);

		loadObject(nullptr, data, type, fileField.id);

		if (alwaysAddLinks || unloadLinks)
			addNewLinks(visionaire, object, type, data);
	}
}

// Confirmed in shape (asm lines 653127-655295). The first part reads the file
// into `membuf` (loading types 0 and 2; type 1 carries on with what a type 0
// pass left behind), then the schema, then every table. Type 0 only loads the
// Loading table (so a loading screen can be shown) and keeps the buffer; the
// other types free it at the end.
//
// Not reconstructed: the progress signals to the editor's TSignalSlot at the
// start (editor only).
bool TVisionaire::BinaryLoad(TVedFile &file, TLoadingTypeEnum type, TSignalSlot * /*slot*/, int *outVersion,
                             EventHandler *handler) {
	int loadType = (int)type;
	bool readFile = loadType == 0 || loadType == 2;
	bool loadingTablePass = loadType == 0;

	g_binaryReadOverrun = false;

	if (readFile) {
		file.Seek(4, 1);

		int header0 = 0, uncompressedSize = 0, storedSize = 0;
		file.ReadMem(&header0, 4);
		file.ReadMem(&uncompressedSize, 4);
		file.ReadMem(&storedSize, 4);

		membuf.Init(storedSize);
		file.ReadMem(membuf.GetData(), storedSize);
		// the original only records the length if the data is compressed
		membuf.SetLen(storedSize);
		if (storedSize != uncompressedSize)
			membuf.Uncompress(uncompressedSize);
	}

	offset = 0;

	bin_structure root;
	root.id = readUShort();
	if (root.id > 0x347)
		return false;

	int tableCount = (short)readUShort();
	if (tableCount > 0x3E7)
		return false;

	for (int i = 0; i < tableCount; i++) {
		bin_structure child;
		if (!make_structure(child))
			return false;
		root.children.push_back(child);
	}

	bool saveGame = file.IsSaveGame();

	int projectType = readInt();
	_revision = readInt();
	int version = readInt();
	if (outVersion)
		*outVersion = version;

	if (version != 0xBA) {
		InitWithVersion(version);
		CreateTempData();
	}
	SetProjectType(projectType);
	SetRevision(_revision);

	for (const bin_structure &fileTable : root.children) {
		int description = (short)fileTable.id;

		// the file's own header fields and the "Game" object are not tables
		if (description == kProjectType || description == kRevision || description == kVersion)
			continue;

		if (description == kGame) {
			if (handler)
				handler->AddPendingEvent(new LoadSaveProgressEvent(-1, true));

			TVisionaireObject *main = _mainObject;
			TTypeGroup *typeGroup = main->GetData()->GetTypeGroupPtrNonConst();
			if (!typeGroup)
				continue;

			// (the original tests the file-reading flag, not the savegame flag, here)
			loadFields(this, main, typeGroup, fileTable, readFile, false);
			main->GetData()->Serialize(1, 1, 0, TCharHolder());
			if (!saveGame)
				typeGroup->OnInit(main);
			continue;
		}

		int identifier = GetTableIdentifierByDescription(description, true);
		if (identifier != 0x15 && loadingTablePass)
			continue;

		if (handler)
			handler->AddPendingEvent(new LoadSaveProgressEvent(identifier, true));

		offset += 4;

		TTable *table = nullptr;
		if (!GetTable(identifier, &table))
			continue;

		if (!saveGame)
			table->SetVersionedId(-1);

		TTypeGroup *typeGroup = table->GetTypeGroup();
		x_assert(typeGroup != nullptr, "false", kSourceFile, 0x3F7);
		if (!typeGroup)
			continue;

		int elements = readInt();
		for (int element = 0; element < elements && !g_binaryReadOverrun; element++) {
			TCharHolder name;
			load(name, nullptr);

			int id = readInt();
			int order = readInt();
			int lastModified = readInt();

			TVisionaireObject *object = nullptr;
			if (saveGame && !table->IsActiveTable())
				object = GetObjectById(TId(id, table->GetIdentifier()));

			if (!object) {
				static TCharHolder emptyName;

				object = table->CreateObjectLoad(id);
				object->GetData()->Serialize(id, order, lastModified, emptyName);
				object->GetData()->GetNameNonConst().exchange(name);
			}

			loadFields(this, object, typeGroup, fileTable, saveGame, true);

			if (!saveGame)
				typeGroup->OnInit(object);

			table->NotifyNewId(object->GetId24());
		}
	}

	if (loadingTablePass) {
		x_assert(_mainObject && _mainObject->GetData(), "false", kSourceFile, 0x491);
		if (_mainObject && _mainObject->GetData())
			_mainObject->GetData()->RemoveTempData();
	} else {
		membuf.ReleaseMemory();
	}

	return !g_binaryReadOverrun;
}
