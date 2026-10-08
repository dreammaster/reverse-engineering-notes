#include "vscommon/objAccess.h"

#include <cwchar>

#include "Diagnostics.h"
#include "TSText.h"
#include "TTAction.h"
#include "TTButton.h"
#include "TTScene.h"
#include "TTText.h"
#include "TXMLNames.h"
#include "datastruct/typegrp.h"
#include "datastruct/visionaire.h"
#include "datastruct/visionaireobject.h"
#include "vstables/records.h"
#include "vstables/visionaireGame.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/vscommon/objAccess.cpp";

static TVisionaireGame *s_game = nullptr;

// Confirmed (asm lines 1385285-1385295)
void InitObjectAccess(TVisionaireGame *game) {
	s_game = game;
}

// Confirmed (asm lines 1385312-1385577): the type group of the table (the order of the tables of
// TVisionaireGame::TVisionaireGame()).
TTypeGroup *GetTypeGroup(int table) {
	switch (table) {
	case -1:
		return &TTGame::GetTypeGroup();
	case 0:
		return &TTCharacter::GetTypeGroup();
	case 1:
		return &TTInterface::GetTypeGroup();
	case 2:
		return &TTButton::GetTypeGroup();
	case 3:
		return &TTFont::GetTypeGroup();
	case 4:
		return &TTScene::GetTypeGroup();
	case 5:
		return &TTPoint::GetTypeGroup();
	case 6:
		return &TTObject::GetTypeGroup();
	case 7:
		return &TTAction::GetTypeGroup();
	case 8:
		return &TTActionPart::GetTypeGroup();
	case 9:
		return &TTAnimation::GetTypeGroup();
	case 10:
		return &TTCondition::GetTypeGroup();
	case 11:
		return &TTDialog::GetTypeGroup();
	case 12:
		return &TTDialogPart::GetTypeGroup();
	case 13:
		return &TTSprite::GetTypeGroup();
	case 14:
		return &TTText::GetTypeGroup();
	case 15:
		return &TTCursor::GetTypeGroup();
	case 16:
		return &TTCommentSet::GetTypeGroup();
	case 17:
		return &TTOutfit::GetTypeGroup();
	case 18:
		return &TTLanguage::GetTypeGroup();
	case 19:
		return &TTTextLanguage::GetTypeGroup();
	case 20:
		return &TTValue::GetTypeGroup();
	case 21:
		return &TTLoading::GetTypeGroup();
	case 22:
		return &TTParticles::GetTypeGroup();
	case 23:
		return &TTParticleContainer::GetTypeGroup();
	case 24:
		return &TSText::GetTypeGroup();
	case 25:
		return &TSAction::GetTypeGroup();
	case 26:
		return &TSAnimation::GetTypeGroup();
	case 27:
		return &TTCommentSetEntry::GetTypeGroup();
	case 28:
		return &TTAnimationFrame::GetTypeGroup();
	case 29:
		return &TTWaySystem::GetTypeGroup();
	case 30:
		return &TTScript::GetTypeGroup();
	case 31:
		return &TTInterfaceClass::GetTypeGroup();
	case 32:
		return &TTActionArea::GetTypeGroup();
	case 33:
		return &TTAreaAction::GetTypeGroup();
	case 34:
		return &TTScriptVariable::GetTypeGroup();
	case 35:
		return &TTModel::GetTypeGroup();
	case 36:
		return &TTShader::GetTypeGroup();
	case 37:
		return &TTAudioBus::GetTypeGroup();
	case 38:
		return &TTEvent::GetTypeGroup();
	default:
		x_assert(false, "0", kSourceFile, 0x73);
		return nullptr;
	}
}

// Confirmed (asm lines 1385586-1385787 and 1385787-1385944): "(%d,%d)" with the table and the id. (The
// original keeps the last 8 strings it made in a ring and gives out a pointer into it.)
static std::wstring idString(int table, int id) {
	return L"(" + std::to_wstring(table) + L"," + std::to_wstring(id) + L")";
}

std::string IdStrStd(const TId &id) {
	std::wstring text = idString(static_cast<signed char>(id.getTable()), id.getId());

	return std::string(wxString(text).mb_str());
}

std::string IdStrStd(const std::uint8_t *id) {
	int value = id[0] + (id[1] << 8) + (static_cast<signed char>(id[2]) << 16);

	return std::string(wxString(idString(static_cast<signed char>(id[3]), value)).mb_str());
}

wxString IdStr(const TId &id) {
	return wxString(idString(static_cast<signed char>(id.getTable()), id.getId()));
}

// Confirmed (asm lines 1385944-1386006): the "any object" has its own id.
wxString IdStr(const TVisionaireObject &object) {
	if (object.IsAnyObject())
		return IdStr(AnyId);

	const std::uint8_t *id = object.GetId();

	return IdStr(TId(id[0] + (id[1] << 8) + (static_cast<signed char>(id[2]) << 16), id[3]));
}

wxString IdStr(const TVisObjRef &object) {
	if (object.IsAnyObject())
		return IdStr(AnyId);

	const std::uint8_t *id = object.GetId();

	return IdStr(TId(id[0] + (id[1] << 8) + (static_cast<signed char>(id[2]) << 16), id[3]));
}

/** The character at `index` of the text (0 at its end). */
static wchar_t charAt(const std::wstring &text, size_t index) {
	return index < text.size() ? text[index] : 0;
}

/** A piece of the text, as it is with the characters cut to a byte (what the original's narrow string
 *  from the range does). */
static std::string narrow(const std::wstring &text, size_t first, size_t last) {
	std::string result;

	for (size_t i = first; i < last; i++)
		result += static_cast<char>(text[i]);

	return result;
}

// Confirmed (asm lines 1386006-1386989)
bool FindObjectByNameRelative(const wxString &path, TVisObjRef &object, bool warn) {
	const std::wstring text = path.ToStdWstring();
	size_t pos = 0;

	for (;;) {
		if (charAt(text, pos) == 0)
			return true;

		const std::uint8_t *id = object.GetId();
		int table = static_cast<signed char>(id[3]);
		TTypeGroup *group = GetTypeGroup(table);

		if (charAt(text, pos) != L'.') {
			if (wxLog::loglevel >= 0)
				wxLog::logexpanded(L"Expecting '.' before next field-name in path.");
			return false;
		}

		// the name of the field: after a `V` that is followed by a capital it is the name
		// without the V; it goes to the next '.' or '[' (or the end)
		size_t first = pos + 1;
		size_t last = first;

		if (charAt(text, first) == L'V') {
			wchar_t next = charAt(text, first + 1);

			if (next > L'@' && next <= L'Z')
				first++;
		}

		if (charAt(text, last) != L'[' && charAt(text, last) != 0 && charAt(text, last) != L'.') {
			last++;

			while (charAt(text, last) != L'[' && charAt(text, last) != 0 && charAt(text, last) != L'.')
				last++;
		}

		std::string name = narrow(text, first, last);
		std::wstring wideName = text.substr(first, last - first);

		// the field is first looked for with the name of the table before it (`SceneName`)
		std::string withTable = std::string(object.GetVisionaire()->GetTableNameSingular(table, true).mb_str()) + name;
		int field = TXMLNames::GetNrByUtf8Name(withTable.c_str());

		if (field == -1 || group->GetType(field, false) == static_cast<eTypeData>(-1))
			field = TXMLNames::GetNrByUtf8Name(name.c_str());

		TTypeData *typeData = group->GetTypeDataPtr(field);

		if (!typeData) {
			if (wxLog::loglevel >= 0)
				wxLog::logexpanded(L"Unknown data-field \"%s\".", wideName.c_str());
			return false;
		}

		switch (typeData->GetType()) {
		case eTypeData::kLink:
			if (charAt(text, last) == L'[') {
				if (wxLog::loglevel >= 0)
					wxLog::logexpanded(L"Field \"%s\" must not be indexed.", wideName.c_str());
				return false;
			}

			object = object.GetLink(field);
			pos = last;
			break;
		case eTypeData::kLinkList: {
			if (charAt(text, last) != L'[') {
				if (wxLog::loglevel >= 0)
					wxLog::logexpanded(L"Field is missing \"index\". You must give an object-name or a number in square-brackets.");
				return false;
			}

			size_t indexFirst = last + 1;
			size_t indexLast = indexFirst;

			while (charAt(text, indexLast) != L']' && charAt(text, indexLast) != 0)
				indexLast++;

			if (charAt(text, indexLast) != L']') {
				if (wxLog::loglevel >= 0)
					wxLog::logexpanded(L"Missing closing bracket.");
				return false;
			}

			wxString index(text.substr(indexFirst, indexLast - indexFirst));
			TVisObjRef from(object);

			if (!from.GetLinkByName(index, field, object)) {
				if (warn && wxLog::loglevel > 0)
					wxLog::logexpanded(L"Can't find object \"%s\" in field \"%s\".", index.wc_str(), wideName.c_str());
				return false;
			}

			pos = indexLast + 1;
			break;
		}
		default:
			if (wxLog::loglevel >= 0)
				wxLog::logexpanded(L"Field \"%s\" is not a link-field.", wideName.c_str());
			return false;
		}
	}
}

// Confirmed (asm lines 1386989-1387514)
bool FindObjectByName(const wxString &path, TVisObjRef &object, bool warn) {
	const std::wstring text = path.ToStdWstring();

	// the name of the table: up to the first '[' or '.'
	size_t end = 0;

	if (charAt(text, 0) != L'[' && charAt(text, 0) != 0 && charAt(text, 0) != L'.') {
		end = 1;

		while (charAt(text, end) != L'[' && charAt(text, end) != 0 && charAt(text, end) != L'.')
			end++;
	}

	wxString tableName(text.substr(0, end));
	int table = s_game->GetTableIdentifierByPluralName(tableName, true);

	if (table == -2) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"Path must start with a valid table-name.");
		return false;
	}

	if (table == -1) {
		if (charAt(text, end) == L'[') {
			if (wxLog::loglevel >= 0)
				wxLog::logexpanded(L"Table \"Game\" must not be indexed. There is only one game-object.");
			return false;
		}

		object = s_game->GetGame();
	} else {
		if (charAt(text, end) != L'[') {
			if (wxLog::loglevel >= 0)
				wxLog::logexpanded(L"Table is missing \"index\". You must give an object-name or a number in square-brackets.");
			return false;
		}

		size_t nameFirst = end + 1;

		end = nameFirst;

		while (charAt(text, end) != L']' && charAt(text, end) != 0)
			end++;

		if (charAt(text, end) != L']') {
			if (wxLog::loglevel >= 0)
				wxLog::logexpanded(L"Missing closing bracket.");
			return false;
		}

		wxString name(text.substr(nameFirst, end - nameFirst));

		if (!s_game->GetObjectByName(name, table, object)) {
			if (warn && wxLog::loglevel > 0)
				wxLog::logexpanded(L"Can't find object \"%s\" in table \"%s\".", name.wc_str(), tableName.wc_str());
			return false;
		}

		end++;
	}

	return FindObjectByNameRelative(wxString(text.substr(end)), object, warn);
}

// Confirmed (asm lines 1387514-1387666): "(table,id)" is an id (a table of -1 is the game itself), anything else
// is a path.
bool FindObjectByNameOrId(const wxString &path, TVisObjRef &object, bool warn) {
	if (path.IsEmpty())
		return false;

	const std::wstring text = path.ToStdWstring();

	if (text.front() == L'(' && text.back() == L')') {
		int table = -1;
		int id = -1;

		std::swscanf(text.c_str(), L"(%d,%d)", &table, &id);

		TId objectId(id, table);
		bool isGame = static_cast<unsigned char>(table) == 0xFF;

		// (-1,-1) is no object at all: it is found, and the reference is left as it is
		if (isGame && objectId.getId() == -1)
			return true;

		if (isGame) {
			object = s_game->GetGame();
			return true;
		}

		if (!s_game->GetObjectById(objectId, object, warn)) {
			object.Clear();
			return false;
		}

		return true;
	}

	if (!FindObjectByName(path, object, warn)) {
		object.Clear();
		return false;
	}

	return true;
}

// Confirmed (asm lines 1388740-1388803)
int GetFieldId(const wxString &name) {
	std::wstring text = name.ToStdWstring();

	return TXMLNames::GetNr(wxString(text.size() > 1 ? text.substr(1) : std::wstring()));
}
