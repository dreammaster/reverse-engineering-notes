#include "datastruct/binaryProjectReader.h"

#include <cstring>
#include <string>

#include "Diagnostics.h"
#include <EventHandler.h>
#include "LoadSaveProgressEvent.h"
#include "TCharHolder.h"
#include "TSprite.h"
#include "baselib/xmlCommon.h"
#include "baselib/xmlWriter.h"
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

	value.SetPosition(position, -1.0f);
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

			// (the fields of a table are the children of its first child, the node of the type; the "Game" object
			// has them directly)
			if (!fileTable.children.empty())
				loadFields(this, object, typeGroup, fileTable.children[0], saveGame, true);

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

// ---------------------------------------------------------------------------------------------------------------------
// The writing side (asm lines 647165-650879 and 655305-657820). Everything is appended to the global buffer `membuf`;
// `offset` follows the length.

static void append(const void *data, unsigned long size) {
	membuf.AppendData(data, size);
	offset += size;
}

/** UTF-8 of a string (what wxString::mb_str() gives on the original's Linux). */
static std::string narrow(const wxString &text) {
	std::string result;

	for (wchar_t c : text.ToStdWstring()) {
		unsigned int code = static_cast<unsigned int>(c);

		if (code < 0x80) {
			result += static_cast<char>(code);
		} else if (code < 0x800) {
			result += static_cast<char>(0xC0 | (code >> 6));
			result += static_cast<char>(0x80 | (code & 0x3F));
		} else if (code < 0x10000) {
			result += static_cast<char>(0xE0 | (code >> 12));
			result += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
			result += static_cast<char>(0x80 | (code & 0x3F));
		} else {
			result += static_cast<char>(0xF0 | (code >> 18));
			result += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
			result += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
			result += static_cast<char>(0x80 | (code & 0x3F));
		}
	}

	return result;
}

// Confirmed (asm lines 647317-647720): the id, the number of children and the children.
void writeStructure(bin_structure &structure, std::FILE *file) {
	unsigned short id = structure.id;
	unsigned short count = static_cast<unsigned short>(structure.children.size());

	append(&id, 2);
	append(&count, 2);

	for (bin_structure &child : structure.children)
		writeStructure(child, file);
}

// Confirmed (asm lines 647720-647782): the bytes of the string, without a length.
void savec(const wxString &value, std::FILE * /*file*/) {
	std::string text = narrow(value);

	append(text.data(), text.size());
}

void save(int value, std::FILE * /*file*/) {
	append(&value, 4);
}

void save(long value, std::FILE * /*file*/) {
	long long wide = value;

	append(&wide, 8);
}

void save(char value, std::FILE * /*file*/) {
	append(&value, 1);
}

void save(bool value, std::FILE * /*file*/) {
	char byte = value ? 1 : 0;

	append(&byte, 1);
}

void save(float value, std::FILE * /*file*/) {
	append(&value, 4);
}

// Confirmed (asm lines 647907-647979): the length of the narrow string (without a terminator), then its bytes.
void save(const wxString &value, std::FILE * /*file*/) {
	std::string text = narrow(value);
	int length = static_cast<int>(text.size());

	append(&length, 4);
	append(text.data(), text.size());
}

// Confirmed (asm lines 647979-648021): the size (it includes the terminator) and the bytes.
void save(TCharHolder &value, std::FILE * /*file*/) {
	int length = static_cast<int>(value.size());

	append(&length, 4);

	if (length > 0)
		append(value.mb_str(), static_cast<unsigned long>(length));
}

// Confirmed (asm lines 648021-648136): the full path as a string.
void save(wxFileName &value, std::FILE *file) {
	save(value.GetFullPath(), file);
}

void save(wxFileName value, std::FILE *file, int /*byValue*/) {
	save(value.GetFullPath(), file);
}

void save(const wxPoint &value, std::FILE * /*file*/) {
	append(&value.x, 4);
	append(&value.y, 4);
}

void save(wxRect &value, std::FILE * /*file*/) {
	append(&value.x, 4);
	append(&value.y, 4);
	append(&value.width, 4);
	append(&value.height, 4);
}

// Confirmed (asm lines 648226-648339): path, pause, name, position, transparency mode and colour (as load()).
void save(TSprite &value, std::FILE *file) {
	save(value.GetPathNonConst(), file);
	save(value.GetPause(), file);
	save(value.GetNameNonConst(), file);

	wxPoint position = value.GetPosition();

	save(position.x, file);
	save(position.y, file);
	save(static_cast<int>(value.GetTransparency()), file);
	save(static_cast<int>(value.GetTransparentColor()), file);
}

// Confirmed (asm lines 648339-648409): id (24 bits, signed), table (a signed byte), the "any" and "parent" flags.
void save(TLink &value, std::FILE *file) {
	const TId &id = value.GetId();

	save(id.getId(), file);
	save(static_cast<int>(static_cast<signed char>(id.getTable())), file);
	save(value.IsAnyLink(), file);
	save(value.IsParentLink(), file);
}

// The vector savers: an int count, then the elements.
void save(std::vector<TLink> &values, std::FILE *file) {
	save(static_cast<int>(values.size()), file);

	for (TLink &link : values)
		save(link, file);
}

void save(std::vector<wxRect> &values, std::FILE *file) {
	save(static_cast<int>(values.size()), file);

	for (wxRect &rect : values)
		save(rect, file);
}

void save(std::vector<wxPoint> &values, std::FILE *file) {
	save(static_cast<int>(values.size()), file);

	for (wxPoint &point : values)
		save(point, file);
}

void save(std::vector<TSprite> &values, std::FILE *file) {
	save(static_cast<int>(values.size()), file);

	for (TSprite &sprite : values)
		save(sprite, file);
}

void save(std::vector<TCharHolder> &values, std::FILE *file) {
	save(static_cast<int>(values.size()), file);

	for (TCharHolder &text : values)
		save(text, file);
}

void save(std::vector<float> &values, std::FILE *file) {
	save(static_cast<int>(values.size()), file);

	for (float value : values)
		save(value, file);
}

void save(std::vector<int> &values, std::FILE *file) {
	save(static_cast<int>(values.size()), file);

	for (int value : values)
		save(value, file);
}

void save(std::vector<wxString> &values, std::FILE *file) {
	save(static_cast<int>(values.size()), file);

	for (wxString &text : values)
		save(text, file);
}

// Confirmed (asm lines 648960-649075): text, audio file, language id (as load()).
void save(std::vector<TTextLanguage> &values, std::FILE *file) {
	save(static_cast<int>(values.size()), file);

	for (TTextLanguage &entry : values) {
		save(entry.text, file);
		save(entry.audioFile, file);
		save(entry.languageId, file);
	}
}

// Confirmed (asm lines 650275-650867): as loadObject(); a path is normalised (the slashes) before it is written; the kind 5
// and anything above 18 are asserted against.
void saveObject(std::FILE *file, void *data, int type) {
	switch (type) {
	case 0:
		save(GetRefBool(data), file);
		break;
	case 1:
		save(GetRefInt(data), file);
		break;
	case 2:
		save(GetRefString(data), file);
		break;
	case 3:
		if (GetRefPath(data).size() != 0)
			normalizepath(const_cast<char *>(GetRefPath(data).mb_str()));

		save(GetRefPath(data), file);
		break;
	case 4:
		save(GetRefFloat(data), file);
		break;
	case 6:
		save(GetRefVRect(data), file);
		break;
	case 7:
		save(GetRefVSprite(data), file);
		break;
	case 8:
		save(GetRefVPoint(data), file);
		break;
	case 9:
		save(GetRefVString(data), file);
		break;
	case 10:
		save(GetRefVInt(data), file);
		break;
	case 11:
		save(GetRefVPath(data), file);
		break;
	case 12:
		save(GetRefVFloat(data), file);
		break;
	case 13:
		save(GetRefPoint(data), file);
		break;
	case 14:
		save(GetRefRect(data), file);
		break;
	case 15:
		save(GetRefSprite(data), file);
		break;
	case 16:
		save(GetRefLink(data), file);
		break;
	case 17:
		save(GetRefLinks(data), file);
		break;
	case 18:
		save(GetRefVText(data), file);
		break;
	default:
		x_assert(false, "false", kSourceFile, 0x226);
		break;
	}
}

/** A schema node with these children. */
static bin_structure makeNode(unsigned short id) {
	bin_structure node;

	node.id = id;
	return node;
}

/** The sign-extended 24 bits (what the original writes of an id, an order and a time stamp). */
static int signExtend24(int value) {
	value &= 0xFFFFFF;

	if (value & 0x800000)
		value -= 0x1000000;

	return value;
}

// Confirmed (asm lines 655305-657820). Writes the project (saveGame false) or the savegame (true) into the buffer in the
// binary format that BinaryLoad() reads:
//   the schema (writeStructure() of: the root 0x64 with the children 0x35, 0x36, 0x11, the Game object 0x74 with the ids
//   of its fields and, for every table whose type fits a savegame (or a project), the table's description with one child,
//   the type's description, with the children 0x0C, 0x0D, 0x0E, 0x37 and the ids of the fields of the type), the project
//   type, the revision and the version, the fields of the Game object, and for every such table the number 0x06054AB5, the
//   number of its records that are not temporary and have data, and each record: its name, id, order, last-modified stamp
//   and the fields. The buffer is then the stored file: with a `writer` (the savegame writer of the game) "VBIN", the
//   number of records, the length and again the length (the data is not compressed), and the data are put in the
//   buffer of the writer; else they go to the file (`file`) with the data compressed: "VBIN", the number of records, the
//   length, the length of the compressed data and the data.
// `handler` gets the progress events of the editor.
bool TVisionaire::BinarySave(const wxFileName &file, EventHandler *handler, bool saveGame, TProjectFileWriter *writer) {
	InitWithVersion(_version);
	membuf.Init(0x1E00000);
	offset = 0;

	int recordCount = 0;

	x_assert(!_mainObject->IsEmpty(), "MainObject->IsEmpty() == false", kSourceFile, 0x26C);

	bin_structure root = makeNode(kVisionaireAdventure);

	root.children.push_back(makeNode(kProjectType));
	root.children.push_back(makeNode(kRevision));
	root.children.push_back(makeNode(kVersion));

	if (_mainObject->IsEmpty()) {
		x_assert(false, "false", kSourceFile, 0x285);
		return false;
	}

	TTypeGroup *gameTypeGroup = _mainObject->GetData()->GetTypeGroupPtrNonConst();

	if (!gameTypeGroup) {
		x_assert(false, "false", kSourceFile, 0x285);
		return false;
	}

	if (handler)
		handler->AddPendingEvent(new LoadSaveProgressEvent(-1, true));

	gameTypeGroup->SetupNeededTypes(saveGame, false, true);

	bin_structure game = makeNode(kGame);

	for (TTypeData *type : gameTypeGroup->GetNeededTypes())
		game.children.push_back(makeNode(static_cast<unsigned short>(type->GetDescription())));

	root.children.push_back(game);

	for (TTable *table : _tableList) {
		TTypeGroup *typeGroup = table->GetTypeGroup();

		if (!typeGroup) {
			x_assert(false, "false", kSourceFile, 0x352);
			continue;
		}

		typeGroup->SetupNeededTypes(saveGame, false, true);

		if (!typeGroup->IsFittingSaveGameType(saveGame))
			continue;

		bin_structure type = makeNode(static_cast<unsigned short>(typeGroup->GetDescription()));

		type.children.push_back(makeNode(kName));
		type.children.push_back(makeNode(kId));
		type.children.push_back(makeNode(kOrder));
		type.children.push_back(makeNode(kLastModified));

		for (TTypeData *field : typeGroup->GetNeededTypes())
			type.children.push_back(makeNode(static_cast<unsigned short>(field->GetDescription())));

		bin_structure tableNode = makeNode(static_cast<unsigned short>(table->GetDescription()));

		tableNode.children.push_back(type);
		root.children.push_back(tableNode);
	}

	writeStructure(root, nullptr);

	save(_projectType, nullptr);
	save(_revision, nullptr);
	save(_version, nullptr);

	// the Game object
	if (!_mainObject->IsEmpty()) {
		TTypeGroup *typeGroup = _mainObject->GetData()->GetTypeGroupPtrNonConst();

		if (typeGroup) {
			typeGroup->SetupNeededTypes(saveGame, false, true);

			for (TTypeData *field : typeGroup->GetNeededTypes())
				saveObject(nullptr, _mainObject->GetData()->GetData(*field), static_cast<int>(field->GetType()));
		}
	}

	// the tables
	for (TTable *table : _tableList) {
		if (handler)
			handler->AddPendingEvent(new LoadSaveProgressEvent(table->GetIdentifier(), true));

		TTypeGroup *typeGroup = table->GetTypeGroup();

		if (!typeGroup) {
			x_assert(false, "false", kSourceFile, 0x352);
			continue;
		}

		typeGroup->SetupNeededTypes(saveGame, false, true);

		if (!typeGroup->IsFittingSaveGameType(saveGame))
			continue;

		save(0x06054AB5, nullptr);

		int count = 0;

		for (TVisionaireObject *object : table->GetObjects()) {
			if (!object->IsTemporary() && object->GetData()) {
				recordCount++;
				count++;
			}
		}

		save(count, nullptr);

		for (TVisionaireObject *object : table->GetObjects()) {
			if (object->IsTemporary() || !object->GetData())
				continue;

			TDataGroup *record = object->GetData();

			save(record->GetNameNonConst(), nullptr);
			save(signExtend24(object->GetId24()), nullptr);
			save(signExtend24(object->GetOrder()), nullptr);
			save(signExtend24(static_cast<int>(record->GetLastModified())), nullptr);

			for (TTypeData *field : typeGroup->GetNeededTypes())
				saveObject(nullptr, record->GetData(*field), static_cast<int>(field->GetType()));
		}

		if (handler)
			handler->AddPendingEvent(new LoadSaveProgressEvent(table->GetIdentifier(), true));
	}

	if (handler)
		handler->AddPendingEvent(new LoadSaveProgressEvent(wxString(L"Saving")));

	if (writer) {
		// The savegame: into the buffer of the writer (the original calls slot 0xC8 of the vtable of the writer it is given,
		// the buffer of TXMLStringWriter).
		TBufferedProjectFileWriter *buffered = dynamic_cast<TBufferedProjectFileWriter *>(writer);

		if (buffered) {
			unsigned long length = membuf.GetLen();
			int number = recordCount;
			int size = static_cast<int>(length);
			TMemoryBuffer &out = buffered->GetBufferNonConst();

			out.ClearMemory();
			out.AppendData("VBIN", 4);
			out.AppendData(&number, 4);
			out.AppendData(&size, 4);
			out.AppendData(&size, 4);
			out.AppendData(membuf.GetData(), length);
		}
	} else {
		wxFile output;

		if (output.Open(file.GetFullPath(), wxString(L"wb"))) {
			int number = recordCount;
			int size = static_cast<int>(membuf.GetLen());

			output.Write("VBIN", 4);
			output.Write(&number, 4);
			output.Write(&size, 4);
			membuf.Compress();
			size = static_cast<int>(membuf.GetLen());
			output.Write(&size, 4);
			output.Write(membuf.GetData(), membuf.GetLen());
			output.Close();
		} else if (wxLog::loglevel >= 0) {
			wxLog::logexpanded(L"can't open %s", file.GetFullPath().wc_str());
		}
	}

	membuf.ClearMemory();
	membuf.ReleaseMemory();
	return true;
}
