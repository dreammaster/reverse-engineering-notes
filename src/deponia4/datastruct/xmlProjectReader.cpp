#include "datastruct/xmlProjectReader.h"

#include <cstring>
#include <string>

#include "Diagnostics.h"
#include <EventHandler.h>
#include "LoadSaveProgressEvent.h"
#include "TMemoryBuffer.h"
#include "TSignalData.h"
#include "TSignalSlot.h"
#include "TXMLNames.h"
#include "baselib/xmlCommon.h"
#include "datastruct/binaryProjectReader.h"
#include "datastruct/data.h"
#include "datastruct/datagrp.h"
#include "datastruct/table.h"
#include "datastruct/typegrp.h"
#include "datastruct/vedfile.h"
#include "datastruct/visionaire.h"
#include "datastruct/visionaireobject.h"
#include "vstables/fieldIds.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/datastruct/xmlProjectReader.cpp";

// The document and its text stay between the passes of a load (loading type 0 keeps them for type 1).
static XmlDocument g_xmlDocument;
static TMemoryBuffer g_xmlBuffer;

// The tags of the savegame's own tables and objects are not in the table of the names: they are recognized by their
// text. (The numbers are the names of the active texts, actions and animations; the tables 0x18, 0x19 and 0x1A.)
struct SaveTag {
	const char *text;
	int name;
	int table;
};

static const SaveTag kSaveTables[] = {
	{"TextsSave", kActiveTexts, 0x18},
	{"ActionsSave", kActiveActions, 0x19},
	{"AnimationsSave", kActiveAnimations, 0x1A},
};

static const SaveTag kSaveObjects[] = {
	{"TextSave", kActiveText, 0x18},
	{"ActionSave", kActiveAction, 0x19},
	{"AnimationSave", kActiveAnimation, 0x1A},
};

static int saveTagName(const SaveTag *tags, const char *text) {
	for (int i = 0; i < 3; i++) {
		if (strcmp(tags[i].text, text) == 0)
			return tags[i].name;
	}
	return -1;
}

UserData::UserData(TVisionaire *visionaire_, bool saveGame_, int rootTag_, TLoadingTypeEnum loadingType_,
                   int *outVersion_, TSignalSlot *slot_, EventHandler *handler_, int tableCount_)
	: visionaire(visionaire_), saveGame(saveGame_), rootTag(rootTag_), loadingType((int)loadingType_),
	  outVersion(outVersion_), tableCount(tableCount_), slot(slot_), handler(handler_) {
	ints.reserve(128);
	floats.reserve(128);
	points.reserve(128);
	rects.reserve(128);
	strings.reserve(128);
	sprites.reserve(128);
	paths.reserve(128);
	links.reserve(128);
	texts.reserve(128);
}

// An error of the file: the prefix and the message (the arguments of the message follow the format).
#define LOG_ERROR(format, ...) 	do { 		if (wxLog::loglevel >= 0) 			wxLog::logexpanded(format, L"Error reading project file. ", ##__VA_ARGS__); 	} while (0)

static wxString utf(const std::string &text) {
	wxString result;

	toUTF(&result, text.c_str());
	return result;
}

static int nameOf(const std::string &name) {
	return TXMLNames::GetNrByUtf8Name(name.c_str());
}

static int intOf(const XmlAttribute *attribute) {
	return (int)dtol(attribute->value.c_str());
}

static bool boolOf(const XmlAttribute *attribute) {
	return ConvertToBool(attribute->value.c_str(), attribute->value.c_str() + attribute->value.size());
}

static const char *valueEnd(const XmlAttribute *attribute) {
	return attribute->value.c_str() + attribute->value.size();
}

static bool isEmptyId(const TId &id) {
	return id.getId() == -1 && id.getTable() == 0xFF;
}

static void sendProgress(UserData *ud, int table) {
	if (ud->handler)
		ud->handler->AddPendingEvent(new LoadSaveProgressEvent(table, true));
}

// A value is put in the place `index` of a list that keeps its elements, or after them.
template<typename T>
static void putInList(std::vector<T> &list, int index, const T &value) {
	if (index < (int)list.size())
		list[index] = value;
	else
		list.push_back(value);
}

// The root element: the version of the file (and of a project the type and the revision), then the data of that version.
// False if the element is not the one expected (the load stops).
static bool openRoot(XmlNode *node, int nr, UserData *ud) {
	if (nr != ud->rootTag) {
		LOG_ERROR(L"%lsExpected tag %ls but found %ls", TXMLNames::GetString(ud->rootTag).wc_str(), utf(node->name).wc_str());
		ud->error = true;
		return false;
	}

	int version = -1;
	int revision = -1;
	int projectType = -1;

	for (XmlAttribute *attribute = node->firstAttribute; attribute; attribute = attribute->next) {
		int attributeName = nameOf(attribute->name);

		if (attributeName == kProjectType)
			projectType = intOf(attribute);
		else if (attributeName == kRevision)
			revision = intOf(attribute);
		else if (attributeName == kVersion)
			version = intOf(attribute);
	}

	if (version == -1) {
		LOG_ERROR(L"%lsMissing file version.");
		x_assert(false, "false", kSourceFile, 0xD6);
		ud->error = true;
		return true;
	}

	if (ud->outVersion)
		*ud->outVersion = version;

	TVisionaire *visionaire = ud->visionaire;

	if (version != 0xBA) {
		visionaire->InitWithVersion(version);
		visionaire->GetMainObject()->GetData()->CreateTempData();

		if (ud->saveGame)
			visionaire->CreateTempData();
	}
	if (!ud->saveGame) {
		visionaire->SetProjectType(projectType);
		visionaire->SetRevision(revision);
	}
	if (TXMLNames::IsScrambled())
		visionaire->SetScrambled();

	ud->state = 2;
	return true;
}

// The "Game" element, the main object: its attributes are its fields (a second pass of a load leaves it alone).
static void openGame(XmlNode *node, int nr, UserData *ud) {
	sendProgress(ud, -1);

	TVisionaireObject *mainObject = ud->visionaire->GetMainObject();
	TTypeGroup *typeGroup = mainObject->GetData()->GetTypeGroupPtrNonConst();

	if (!ud->skip) {
		if (!typeGroup) {
			x_assert(false, "false", kSourceFile, 0x12F);
			LOG_ERROR(L"%lsCould not find type group for game object.");
			ud->error = true;
		} else {
			int lastModified = -1;

			for (XmlAttribute *attribute = node->firstAttribute; attribute; attribute = attribute->next) {
				int attributeName = nameOf(attribute->name);

				if (attributeName == kLastModified) {
					lastModified = intOf(attribute);
				} else if (attributeName != -1 && attributeName != kName && attributeName != kId && attributeName != kOrder) {
					const TTypeData &typeData = typeGroup->GetTypeData(attributeName);
					void *data = mainObject->GetData()->GetData(typeData);

					if (data)
						TData::SerializeParam(attribute->value.c_str(), valueEnd(attribute), typeData.GetType(), data);
				}
			}

			if (ud->loadingType == 0 || ud->loadingType == 2) {
				static TCharHolder empty;

				mainObject->GetData()->Serialize(1, 1, lastModified, empty);
			}
		}
	}

	ud->objectTag = nr;
	ud->state = 4;
	ud->object = mainObject;
	ud->typeGroup = typeGroup;
}

// A table element (Characters ...): the table is found by its description; whether it is read depends on the load.
static void openTable(XmlNode *node, int nr, UserData *ud) {
	TVisionaire *visionaire = ud->visionaire;
	int table = visionaire->GetTableIdentifierByDescription(nr, true);

	if (table == -2 && ud->saveGame) {
		// (the tables of the active texts, actions and animations of a savegame)
		for (int i = 0; i < 3; i++) {
			if (strcmp(kSaveTables[i].text, node->name.c_str()) == 0) {
				table = kSaveTables[i].table;
				nr = kSaveTables[i].name;
			}
		}
	}

	sendProgress(ud, table);

	ud->tableTag = nr;

	int previous = ud->tablesRead;

	ud->tablesRead = previous == -1 ? 0 : previous + 1;

	// (the first table has to be the Loadings, any other is read over; a second pass leaves the Loadings alone)
	bool readTable;

	if (previous == -1)
		readTable = table == 0x15;
	else
		readTable = !(ud->loadingType == 1 && table == 0x15);

	if (!readTable) {
		ud->state = 3;
		ud->skip = true;
		return;
	}

	if (!visionaire->GetTable(table, &ud->table)) {
		x_assert(false, "false", kSourceFile, 0x1A8);
		LOG_ERROR(L"%lsFailed to get table %ls (identifier %d).", utf(node->name).wc_str(), table);
		ud->error = true;
		return;
	}

	long versionedId = -1;
	int tableId = -2;

	for (XmlAttribute *attribute = node->firstAttribute; attribute; attribute = attribute->next) {
		int attributeName = nameOf(attribute->name);

		if (attributeName == kTableId)
			tableId = intOf(attribute);
		else if (attributeName == kVersionedId)
			versionedId = intOf(attribute);
		// (kNewId is read over)
	}

	if (ud->table->GetIdentifier() != tableId) {
		x_assert(false, "false", kSourceFile, 0x1A0);
		LOG_ERROR(L"%lsTable %ls has invalid identifier. Expected identifier %d but found %d.", ud->table->GetTableName().wc_str(),
		          ud->table->GetIdentifier(), tableId);
		ud->error = true;
		return;
	}

	if (!ud->saveGame)
		ud->table->SetVersionedId(versionedId);

	ud->typeGroup = ud->table->GetTypeGroup();
	if (!ud->typeGroup) {
		x_assert(false, "false", kSourceFile, 0x199);
		LOG_ERROR(L"%lsCould not find type group for table %ls", ud->table->GetTableName().wc_str());
		ud->error = true;
		return;
	}
	ud->state = 3;
}

// An object element (Character ...) in a table: its header (name, id, order, last modification), then the fields that are
// attributes of it.
static void openObject(XmlNode *node, int nr, UserData *ud) {
	if (ud->skip)
		return;

	if (nr == -1 && ud->saveGame)
		nr = saveTagName(kSaveObjects, node->name.c_str());

	x_assert(ud->object == nullptr, "ud->Object == NULL", kSourceFile, 0x1BF);
	if (ud->object)
		return;

	ud->objectTag = nr;

	TCharHolder name;
	int id = -1;
	int order = -1;
	int lastModified = -1;

	for (XmlAttribute *attribute = node->firstAttribute; attribute; attribute = attribute->next) {
		int attributeName = nameOf(attribute->name);

		if (attributeName == kName)
			name.Assign(attribute->value.c_str(), valueEnd(attribute));
		else if (attributeName == kId)
			id = intOf(attribute);
		else if (attributeName == kOrder)
			order = intOf(attribute);
		else if (attributeName == kLastModified)
			lastModified = intOf(attribute);
	}

	if (!ud->saveGame || ud->table->IsActiveTable()) {
		static TCharHolder empty;

		ud->object = ud->table->CreateObjectLoad(id);
		ud->object->GetData()->Serialize(id, order, lastModified, empty);
		ud->object->GetData()->GetNameNonConst().exchange(name);
	} else {
		ud->object = ud->visionaire->GetObjectById(TId(id, ud->table->GetIdentifier()));
	}

	if (ud->object) {
		for (XmlAttribute *attribute = node->firstAttribute; attribute; attribute = attribute->next) {
			int attributeName = nameOf(attribute->name);

			if (attributeName == kId || attributeName == kOrder || attributeName == kLastModified || attributeName == kName)
				continue;

			const TTypeData &typeData = ud->typeGroup->GetTypeData(attributeName);
			void *data = ud->object->GetData()->GetData(typeData);

			if (data)
				TData::SerializeParam(attribute->value.c_str(), valueEnd(attribute), typeData.GetType(), data);
		}
	}

	ud->state = 4;
}

// The attributes of a point, a rect or a link are read by name: this is the number in an attribute for a name.
static int attributeInt(XmlNode *node, int name, int fallback) {
	int result = fallback;

	for (XmlAttribute *attribute = node->firstAttribute; attribute; attribute = attribute->next) {
		if (nameOf(attribute->name) == name)
			result = intOf(attribute);
	}
	return result;
}

static void readPoint(XmlNode *node, UserData *ud) {
	wxPoint point;

	point.x = attributeInt(node, kX, -1);
	point.y = attributeInt(node, kY, -1);

	if (ud->fieldType == (int)eTypeData::kPoint) {
		GetRefPoint(ud->fieldData) = point;
	} else {
		putInList(ud->points, ud->index, point);
		ud->index++;
	}
}

static void readRect(XmlNode *node, UserData *ud) {
	int top = attributeInt(node, kT, 0);
	int left = attributeInt(node, kLeft, 0);
	int bottom = attributeInt(node, kBottom, 0);
	int right = attributeInt(node, kRight, 0);

	if (ud->fieldType == (int)eTypeData::kRect) {
		wxRect &rect = GetRefRect(ud->fieldData);

		rect.x = left;
		rect.y = top;
		if (left < right)
			rect.SetRight(right);
		if (bottom > top)
			rect.SetBottom(bottom);
		return;
	}

	wxRect rect;

	rect.x = left;
	rect.y = top;
	rect.width = 0;
	rect.height = 0;
	if (left < right)
		rect.SetRight(right);
	else
		rect.width = 0;
	if (bottom > top)
		rect.SetBottom(bottom);
	else
		rect.height = 0;

	putInList(ud->rects, ud->index, rect);
	ud->index++;
}

static void readSprite(XmlNode *node, int nr, UserData *ud) {
	TCharHolder name;
	TCharHolder path;
	int pause = 0;
	int transparency = -1;
	unsigned int color = 0;

	for (XmlAttribute *attribute = node->firstAttribute; attribute; attribute = attribute->next) {
		switch (nameOf(attribute->name)) {
		case kPath: {
			path.Assign(attribute->value.c_str(), valueEnd(attribute));

			// (the backslashes of a path from Windows)
			if (path.size() != 0)
				normalizepath(const_cast<char *>(path.mb_str()));
			break;
		}
		case kName:
			name.Assign(attribute->value.c_str(), valueEnd(attribute));
			break;
		case kPause:
			pause = intOf(attribute);
			break;
		case kTransparency:
			transparency = intOf(attribute);
			break;
		case kTranspColor:
			color = (unsigned int)(long)intOf(attribute);
			break;
		default:
			break;
		}
	}

	bool isList = ud->fieldType == (int)eTypeData::kSpriteList;
	TSprite *target;

	if (isList && ud->index < (int)ud->sprites.size()) {
		target = &ud->sprites[ud->index];
		target->GetNameNonConst().exchange(name);
		target->GetPathNonConst().exchange(path);
		target->SetPause(pause);
		target->SetTransparency((eTransparencyMode)transparency, color);
	} else {
		TSprite local;

		local.GetNameNonConst().exchange(name);
		local.GetPathNonConst().exchange(path);
		local.SetPause(pause);
		local.SetTransparency((eTransparencyMode)transparency, color);

		if (ud->fieldType == (int)eTypeData::kSprite) {
			TSprite &ref = GetRefSprite(ud->fieldData);

			ref = local;
			target = &ref;
		} else {
			ud->sprites.push_back(local);
			target = &ud->sprites[ud->index];
		}
	}

	ud->sprite = target;
	if (isList)
		ud->index++;
	ud->itemTag = nr;
	ud->state = 5;
}

// A link element: the table, the id and the flags (parent, any) of a link, kept in the field (a link) or in the list.
static void readLink(XmlNode *node, int nr, UserData *ud) {
	int table = -2;
	int id = -1;
	bool any = false;
	bool parent = false;

	for (XmlAttribute *attribute = node->firstAttribute; attribute; attribute = attribute->next) {
		switch (nameOf(attribute->name)) {
		case kTableId:
			table = intOf(attribute);
			break;
		case kId:
			id = intOf(attribute);
			break;
		case kParentLink:
			parent = boolOf(attribute);
			break;
		case kLinkAny:
			any = boolOf(attribute);
			break;
		default:
			break;
		}
	}

	if (ud->fieldType != (int)eTypeData::kLink) {
		// a link of a list
		if (ud->index < (int)ud->links.size())
			ud->links[ud->index].Serialize(id, table, ud->fieldTag, parent, any);
		else if (any)
			ud->links.push_back(TLink(true));
		else
			ud->links.push_back(TLink(TId(id, table), ud->fieldTag, parent));
		ud->index++;
		return;
	}

	TLink &link = GetRefLink(ud->fieldData);

	if (ud->saveGame && !isEmptyId(link.GetId()) && !link.IsParentLink())
		ud->visionaire->RemoveLink(ud->object->GetTId(), link.GetId(), link.GetField());

	link.Serialize(id, table, nr, parent, any);
	if (any)
		return;
	if (parent)
		ud->visionaire->AddParentLink(ud->object->GetTId(), link.GetId(), link.GetField());
	else
		ud->visionaire->AddLink(ud->object->GetTId(), link.GetId(), link.GetField(), false);
}

// The attribute of a float or an int list's element; `name` is the attribute (the element's own name for the value).
static void readFloat(XmlNode *node, UserData *ud) {
	for (XmlAttribute *attribute = node->firstAttribute; attribute; attribute = attribute->next) {
		if (nameOf(attribute->name) == kFloat) {
			float value = ConvertToFloat(attribute->value.c_str(), valueEnd(attribute));

			putInList(ud->floats, ud->index, value);
			ud->index++;
			return;
		}
	}
}

static void readInt(XmlNode *node, UserData *ud) {
	for (XmlAttribute *attribute = node->firstAttribute; attribute; attribute = attribute->next) {
		if (attribute->name == "int") {
			int value = intOf(attribute);

			putInList(ud->ints, ud->index, value);
			ud->index++;
			return;
		}
	}
}

static void readPath(XmlNode *node, UserData *ud) {
	for (XmlAttribute *attribute = node->firstAttribute; attribute; attribute = attribute->next) {
		if (nameOf(attribute->name) == kPath) {
			wxString text = utf(attribute->value);

			text.Replace(L"\\", L"/", true);

			TCharHolder path;

			path.Assign(text);
			putInList(ud->paths, ud->index, path);
			ud->index++;
			return;
		}
	}
}

static void readString(XmlNode *node, UserData *ud) {
	for (XmlAttribute *attribute = node->firstAttribute; attribute; attribute = attribute->next) {
		int attributeName = nameOf(attribute->name);

		if (attributeName == kPath || attributeName == kText) {
			TCharHolder text(attribute->value.c_str(), valueEnd(attribute));

			putInList(ud->strings, ud->index, text);
			ud->index++;
			return;
		}
	}
}

// One text of a list: the text, the speech and the language.
static void readText(XmlNode *node, UserData *ud) {
	ud->index++;
	if (ud->index >= (int)ud->texts.size()) {
		TTextLanguage empty;

		empty.languageId = -1;
		ud->texts.push_back(empty);
	}

	TTextLanguage &text = ud->texts[ud->index - 1];

	for (XmlAttribute *attribute = node->firstAttribute; attribute; attribute = attribute->next) {
		int attributeName = nameOf(attribute->name);

		if (attributeName == kInt)
			text.languageId = intOf(attribute);
		else if (attributeName == kPath)
			text.audioFile = TCharHolder(attribute->value.c_str(), valueEnd(attribute));
		else if (attributeName == kText_0x3A || attributeName == kText_0xD1)
			text.text = TCharHolder(attribute->value.c_str(), valueEnd(attribute));
	}
}

// An element in an object: the field of that name (the first one) or one of its elements.
static void openField(XmlNode *node, int nr, UserData *ud) {
	if (ud->skip)
		return;

	x_assert(ud->object != nullptr || ud->saveGame, "ud->Object != NULL || ud->SaveGame", kSourceFile, 0x212);

	if (!ud->object) {
		// (a savegame with an object that is not there any more: the elements of its fields are read over)
		if (ud->fieldTag == -1) {
			ud->fieldTag = nr;
			ud->fieldType = -1;
		}
		return;
	}

	bool opened = false;

	if (ud->fieldTag == -1) {
		const TTypeData &typeData = ud->typeGroup->GetTypeData(nr);

		ud->fieldTag = nr;
		ud->fieldType = (int)typeData.GetType();
		ud->fieldData = ud->object->GetData()->GetData(typeData);
		ud->index = 0;
		opened = true;
	}

	if (!ud->fieldData)
		return;

	switch ((eTypeData)ud->fieldType) {
	case eTypeData::kRect:
		readRect(node, ud);
		break;
	case eTypeData::kRectList:
		if (!opened)
			readRect(node, ud);
		break;
	case eTypeData::kPoint:
		readPoint(node, ud);
		break;
	case eTypeData::kPointList:
		if (!opened)
			readPoint(node, ud);
		break;
	case eTypeData::kSprite:
		readSprite(node, nr, ud);
		break;
	case eTypeData::kSpriteList:
		if (!opened)
			readSprite(node, nr, ud);
		break;
	case eTypeData::kFloatList:
		if (!opened)
			readFloat(node, ud);
		break;
	case eTypeData::kIntList:
		if (!opened)
			readInt(node, ud);
		break;
	case eTypeData::kPathList:
		if (!opened)
			readPath(node, ud);
		break;
	case eTypeData::kStringList:
		if (!opened)
			readString(node, ud);
		break;
	case eTypeData::kTextList:
		if (!opened)
			readText(node, ud);
		break;
	case eTypeData::kLink:
		readLink(node, nr, ud);
		break;
	case eTypeData::kLinkList:
		if (!opened)
			readLink(node, nr, ud);
		break;
	default:
		break;
	}
}

// An element in a sprite: its position (the "cutting" and "transparent" are not used).
static void openSpriteItem(XmlNode *node, int nr, UserData *ud) {
	if (ud->skip)
		return;

	x_assert(ud->sprite != nullptr, "ud->Sprite != NULL", kSourceFile, 0x3D5);

	if (nr == kPosition) {
		wxPoint position;

		position.x = attributeInt(node, kX, 0);
		position.y = attributeInt(node, kY, 0);
		ud->sprite->SetPosition(position, -1.0f);
	} else if (nr != kCutting && nr != kTransparent) {
		x_assert(false, "false", kSourceFile, 0x3F0);
		LOG_ERROR(L"%lsFailed to read sprite. Expected tag %ls but found %ls.", TXMLNames::GetString(kPosition).wc_str(),
		          utf(node->name).wc_str());
		ud->error = true;
	}
}

// The end of a field's element: the elements it collected go to the field.
static void closeField(UserData *ud) {
	if (ud->skip)
		return;

	void *data = ud->fieldData;

	if (!data) {
		ud->fieldTag = -1;
		return;
	}

	switch ((eTypeData)ud->fieldType) {
	case eTypeData::kRectList: {
		std::vector<wxRect> &target = GetRefVRect(data);

		target.reserve(ud->index);
		for (int i = 0; i < ud->index; i++)
			target.push_back(ud->rects[i]);
		break;
	}
	case eTypeData::kSpriteList: {
		std::vector<TSprite> &target = GetRefVSprite(data);

		target.reserve(ud->index);
		for (int i = 0; i < ud->index; i++)
			target.push_back(ud->sprites[i]);
		ud->sprite = nullptr;
		break;
	}
	case eTypeData::kSprite:
		ud->sprite = nullptr;
		break;
	case eTypeData::kPointList: {
		std::vector<wxPoint> &target = GetRefVPoint(data);

		target.reserve(ud->index);
		for (int i = 0; i < ud->index; i++)
			target.push_back(ud->points[i]);
		break;
	}
	case eTypeData::kStringList: {
		std::vector<TCharHolder> &target = GetRefVString(data);

		target.reserve(ud->index);
		for (int i = 0; i < ud->index; i++)
			target.push_back(ud->strings[i]);
		break;
	}
	case eTypeData::kIntList: {
		std::vector<int> &target = GetRefVInt(data);

		target.reserve(ud->index);
		for (int i = 0; i < ud->index; i++)
			target.push_back(ud->ints[i]);
		break;
	}
	case eTypeData::kPathList: {
		std::vector<TCharHolder> &target = GetRefVPath(data);

		target.reserve(ud->index);
		for (int i = 0; i < ud->index; i++)
			target.push_back(ud->paths[i]);
		break;
	}
	case eTypeData::kFloatList: {
		std::vector<float> &target = GetRefVFloat(data);

		target.reserve(ud->index);
		for (int i = 0; i < ud->index; i++)
			target.push_back(ud->floats[i]);
		break;
	}
	case eTypeData::kTextList: {
		std::vector<TTextLanguage> &target = GetRefVText(data);

		target.reserve(ud->index);
		for (int i = 0; i < ud->index; i++)
			target.push_back(ud->texts[i]);
		break;
	}
	case eTypeData::kLinkList: {
		std::vector<TLink> &target = GetRefLinks(data);

		// (a savegame is loaded over the data: the links that are there are taken out of the table first)
		if (ud->saveGame) {
			for (const TLink &link : target) {
				x_assert(!isEmptyId(link.GetId()), "!it->GetId().IsEmpty()", kSourceFile, 0x4B2);
				if (!link.IsParentLink())
					ud->visionaire->RemoveLink(ud->object->GetTId(), link.GetId(), link.GetField());
			}
			target.clear();
		}

		target.reserve(ud->index);
		for (int i = 0; i < ud->index; i++)
			target.push_back(ud->links[i]);

		for (int i = 0; i < ud->index; i++) {
			const TLink &link = ud->links[i];

			if (link.IsParentLink())
				ud->visionaire->AddParentLink(ud->object->GetTId(), link.GetId(), link.GetField());
			else
				ud->visionaire->AddLink(ud->object->GetTId(), link.GetId(), link.GetField(), false);
		}
		break;
	}
	default:
		break;
	}

	ud->fieldTag = -1;
	ud->fieldData = nullptr;
}

// Confirmed (asm lines 636270-641661). The walk: each element is opened according to the state, its children are read
// (the same function), then it is closed according to the state it has then. The name of an element that is unknown ends
// the load, except the savegame's own (see kSaveTables). The cases of the state and the order of the checks follow the
// original; the messages of the errors are in its words.
void TXMLProjectReader::parseNodes(XmlNode *node, UserData *ud) {
	if (!node)
		return;

	if (ud->textLength)
		X_LoadProgress = (float)node->namePos / (float)ud->textLength;

	if (ud->skip)
		return;

	for (; node; node = node->next) {
		int nr = nameOf(node->name);

		// open
		switch (ud->state) {
		case 0:
			if (!openRoot(node, nr, ud))
				return;
			break;
		case 1:
			openGame(node, nr, ud);
			break;
		case 2:
			if (nr == kClipboard || nr == kGame) {
				ud->state = 1;
				if (ud->loadingType == 1)
					ud->skip = true;
				openGame(node, nr, ud);
			} else {
				openTable(node, nr, ud);
			}
			break;
		case 3:
			openObject(node, nr, ud);
			break;
		case 4:
			openField(node, nr, ud);
			break;
		case 5:
			openSpriteItem(node, nr, ud);
			break;
		default:
			x_assert(false, "false", kSourceFile, 0x3F9);
			LOG_ERROR(L"%lsInvalid parser state.");
			ud->error = true;
			break;
		}

		parseNodes(node->firstChild, ud);

		if (nr == -1) {
			// (a tag of the savegame's own tables and objects: it is named by its text)
			int mapped = -1;

			if (ud->saveGame) {
				mapped = saveTagName(kSaveTables, node->name.c_str());
				if (mapped == -1)
					mapped = saveTagName(kSaveObjects, node->name.c_str());
			}
			if (mapped == -1) {
				LOG_ERROR(L"%lsFound unknown tag: %ls.", utf(node->name).wc_str());
				ud->error = true;
				return;
			}
			nr = mapped;
		}

		// close
		switch (ud->state) {
		case 4:
			if (ud->fieldTag == nr) {
				closeField(ud);
			} else if (ud->objectTag == nr) {
				x_assert(ud->object != nullptr || ud->saveGame, "ud->Object != NULL || ud->SaveGame", kSourceFile, 0x4D2);
				x_assert(ud->typeGroup != nullptr, "ud->TypeGroup != NULL", kSourceFile, 0x4D3);

				if (ud->object && !ud->saveGame)
					ud->typeGroup->OnInit(ud->object);

				if (!ud->table) {
					// (the main object)
					ud->state = 2;
					ud->skip = false;
				} else {
					if (ud->object)
						ud->table->NotifyNewId(ud->object->GetId24());
					ud->state = 3;
				}
				ud->object = nullptr;
				ud->objectTag = -1;
			}
			break;
		case 5:
			if (ud->skip)
				break;
			if (ud->itemTag == nr) {
				ud->sprite = nullptr;
				ud->itemTag = -1;
				ud->state = 4;
				if (nr == ud->fieldTag)
					ud->fieldTag = -1;
			} else if (nr != kPosition && nr != kCutting) {
				x_assert(false, "false", kSourceFile, 0x4FA);
				LOG_ERROR(L"%lsFound invalid end tag. Excepted %ls (id: %d) or %ls (id: %d)", TXMLNames::GetString(ud->itemTag).wc_str(),
				          ud->itemTag, TXMLNames::GetString(kPosition).wc_str(), kPosition);
				ud->error = true;
			}
			break;
		case 3:
			if (ud->tableTag == nr) {
				if (ud->object) {
					x_assert(false, "false", kSourceFile, 0x425);
					ud->error = true;
				} else {
					ud->state = 2;
					ud->skip = false;
					ud->table = nullptr;
					ud->typeGroup = nullptr;
					ud->tableTag = -1;
					if (nr == kLoadings)
						ud->loadingDone = true;
				}
			} else if (!ud->skip) {
				int expected = ud->objectTag == -1 ? ud->tableTag : ud->objectTag;

				x_assert(false, "false", kSourceFile, 0x439);
				LOG_ERROR(L"%lsFound invalid end tag. Expected %ls (id: %d) but found %ls (id: %d)",
				          TXMLNames::GetString(expected).wc_str(), expected, utf(node->name).wc_str(), nr);
				ud->error = true;
			}
			break;
		default:
			break;
		}
	}
}

// Confirmed (asm lines 641669-642068): clipboard text, read as a whole project with the root "VisionaireAdventure".
bool TXMLProjectReader::ReadXMLDoc(const wxString &text) {
	UserData ud(_visionaire, false, kVisionaireAdventure, TLoadingTypeEnum::kValue2, nullptr, nullptr, nullptr, -1);
	std::string utf8(text.mb_str());
	XmlDocument document;

	try {
		document.Parse(utf8.c_str());
	} catch (const XmlParseError &error) {
		LOG_ERROR(L"%lsXML parse error: %s at %ld", error.what, (long)error.position);
		return false;
	}

	ud.text = utf8.c_str();
	ud.textLength = utf8.size();
	parseNodes(document.GetFirstNode(), &ud);
	return !ud.error;
}

// Confirmed (asm lines 642068-643450). Loading types 0 and 2 read the file, 1 goes on with the document that type 0 left;
// a file with the flag of the chunks is a series of (sizes, compressed block) to be put together. Type 0 and 2 end with the
// document dropped (type 0 keeps it for the second pass, and takes the temporary data of the main object away).
bool TXMLProjectReader::ReadXMLDoc(TVedFile &file, const wxString &password, int rootTag, TLoadingTypeEnum type,
                                   int *outVersion, TSignalSlot *slot, EventHandler *handler, int tableCount) {
	int loadType = (int)type;

	auto signalProgress = [&](UserData &ud) {
		if (ud.slot) {
			TSignalData signal;
			TSignalData result;

			signal.type = kSignalLoadingProgress;
			signal.loadingCurrent = 0;
			signal.loadingTotal = 12;
			ud.slot->Signal(signal, result);
		}
	};
	auto release = [&]() {
		g_xmlDocument.Clear();
		g_xmlBuffer.ReleaseMemory();
	};

	if (loadType == 1) {
		UserData ud(_visionaire, file.IsSaveGame(), rootTag, type, outVersion, slot, handler, tableCount);

		signalProgress(ud);
		ud.slot = nullptr;
		parseNodes(g_xmlDocument.GetFirstNode(), &ud);
		release();
		return !ud.error;
	}

	if (loadType != 0 && loadType != 2)
		return false;

	if (file.IsContentCompressedChunks()) {
		// (the file is a series of blocks: sizes in 8 bytes (big endian), then the block)
		TMemoryBuffer block;
		unsigned long bytesRead;

		while ((bytesRead = file.ReadToBuf(block, 8)) != 0) {
			x_assert(bytesRead == 8, "bytesRead == 8", kSourceFile, 0x546);
			if (bytesRead < 8) {
				if (wxLog::loglevel >= 0)
					wxLog::logexpanded(L"Invalid chunk header");
				return false;
			}

			const unsigned char *header = block.GetData();
			unsigned long uncompressedSize = ((unsigned long)header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];
			unsigned long storedSize = ((unsigned long)header[4] << 24) | (header[5] << 16) | (header[6] << 8) | header[7];

			if (uncompressedSize == 0xFFEEFFEE)
				break;
			if (storedSize > 0x200000) {
				if (wxLog::loglevel >= 0)
					wxLog::logexpanded(L"Chunk too big");
				return false;
			}

			block.ClearMemory();
			if (file.ReadToBuf(block, storedSize) != storedSize) {
				if (wxLog::loglevel >= 0)
					wxLog::logexpanded(L"Could not read chunk");
				return false;
			}
			if (file.IsContentEncrypted() && !block.Decrypt(password, nullptr, 0)) {
				if (wxLog::loglevel >= 0)
					wxLog::logexpanded(L"Could not decrypt chunk");
				return false;
			}
			if (!block.Uncompress((long)uncompressedSize)) {
				if (wxLog::loglevel >= 0)
					wxLog::logexpanded(L"Could not uncompress chunk");
				return false;
			}
			g_xmlBuffer.AppendData(block.GetData(), block.GetLen());
		}
		g_xmlBuffer.AppendByte(0);
	} else {
		file.ReadToBuf(g_xmlBuffer, 0);
		if (file.IsContentEncrypted() && !g_xmlBuffer.Decrypt(password, nullptr, 0)) {
			if (wxLog::loglevel >= 0)
				wxLog::logexpanded(L"Could not decrypt file");
			return false;
		}
		g_xmlBuffer.AppendByte(0);
	}

	const char *text = reinterpret_cast<const char *>(g_xmlBuffer.GetData());

	try {
		g_xmlDocument.Parse(text);
	} catch (const XmlParseError &error) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"XML parse error: %s at %ld", error.what, (long)error.position);
		release();
		return false;
	}

	if (!g_xmlDocument.GetFirstNode()) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"Empty XML document");
		release();
		return false;
	}

	UserData ud(_visionaire, file.IsSaveGame(), rootTag, type, outVersion, slot, handler, tableCount);

	signalProgress(ud);
	ud.slot = nullptr;
	ud.text = text;
	ud.textLength = g_xmlBuffer.GetLen();
	parseNodes(g_xmlDocument.GetFirstNode(), &ud);

	if (loadType == 0) {
		TVisionaireObject *mainObject = _visionaire->GetMainObject();

		if (mainObject && mainObject->GetData())
			mainObject->GetData()->RemoveTempData();
		else
			x_assert(false, "false", kSourceFile, 0x5AD);
	} else {
		release();
	}
	return !ud.error;
}
