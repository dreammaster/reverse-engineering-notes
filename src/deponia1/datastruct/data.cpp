#include "datastruct/data.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <new>
#include <string>

#include "Diagnostics.h"
#include "baselib/xmlCommon.h"
#include "baselib/xmlWriter.h"
#include "datastruct/datagrp.h"
#include "datastruct/visionaire.h"
#include "datastruct/visionaireobject.h"
#include "vstables/fieldIds.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/datastruct/data.cpp";
static const char *const kValueNull = "value != NULL";

// Reports a null value address: logged at verbosity > 1, then asserted.
static void reportNullValue(const wchar_t *message, int line) {
	if (wxLog::loglevel > 1)
		wxLog::logexpanded(message);
	x_assert(false, kValueNull, kSourceFile, line);
}

// Confirmed (asm lines 623914-623988)
int &GetRefInt(const void *value) {
	static int emptyInt;

	if (value)
		return *const_cast<int *>(static_cast<const int *>(value));

	reportNullValue(L"value for int is NULL", 0x1A);
	emptyInt = 0;
	return emptyInt;
}

// Confirmed (asm lines 623989-624063)
float &GetRefFloat(const void *value) {
	static float emptyFloat;

	if (value)
		return *const_cast<float *>(static_cast<const float *>(value));

	reportNullValue(L"value for float is NULL", 0x27);
	emptyFloat = 0;
	return emptyFloat;
}

// Confirmed (asm lines 624064-624138)
bool &GetRefBool(const void *value) {
	static bool emptybool;

	if (value)
		return *const_cast<bool *>(static_cast<const bool *>(value));

	reportNullValue(L"value for bool is NULL", 0x34);
	emptybool = false;
	return emptybool;
}

// Confirmed (asm lines 624139-624243)
TCharHolder &GetRefString(const void *value) {
	static TCharHolder emptyString;

	if (value)
		return *const_cast<TCharHolder *>(static_cast<const TCharHolder *>(value));

	reportNullValue(L"value for string is NULL", 0x41);
	return emptyString;
}

// Confirmed (asm lines 624244-624348)
TCharHolder &GetRefPath(const void *value) {
	static TCharHolder emptyFileName;

	if (value)
		return *const_cast<TCharHolder *>(static_cast<const TCharHolder *>(value));

	reportNullValue(L"value for path is NULL", 0x4E);
	return emptyFileName;
}

// Confirmed (asm lines 624349-624444)
wxRect &GetRefRect(const void *value) {
	static wxRect emptyRect;

	if (value)
		return *const_cast<wxRect *>(static_cast<const wxRect *>(value));

	reportNullValue(L"value for rect is NULL", 0x5B);
	emptyRect = wxRect();
	return emptyRect;
}

// Confirmed (asm lines 624445-624538)
wxPoint &GetRefPoint(const void *value) {
	static wxPoint emptyPoint;

	if (value)
		return *const_cast<wxPoint *>(static_cast<const wxPoint *>(value));

	reportNullValue(L"value for point is NULL", 0x68);
	emptyPoint = wxPoint();
	return emptyPoint;
}

// Confirmed (asm lines 624539-624645)
TSprite &GetRefSprite(const void *value) {
	static TSprite emptySprite;

	if (value)
		return *const_cast<TSprite *>(static_cast<const TSprite *>(value));

	reportNullValue(L"value for sprite is NULL", 0x75);
	emptySprite.Clear();
	return emptySprite;
}

// Confirmed (asm lines 624646-624752)
TLink &GetRefLink(const void *value) {
	static TLink emptyLink;

	if (value)
		return *const_cast<TLink *>(static_cast<const TLink *>(value));

	reportNullValue(L"value for TLink is NULL", 0x82);
	emptyLink.Clear();
	return emptyLink;
}

// The list helpers all work the same way: the shared empty list is cleared
// before it's handed out.
#define GET_REF_LIST(name, type, line, text)                                    \
	type &name(const void *value) {                                              \
		static type emptyList;                                                    \
                                                                                  \
		if (value)                                                                \
			return *const_cast<type *>(static_cast<const type *>(value));         \
                                                                                  \
		reportNullValue(text, line);                                              \
		emptyList.clear();                                                        \
		return emptyList;                                                         \
	}

// Confirmed (asm lines 624753-625850)
GET_REF_LIST(GetRefLinks, std::vector<TLink>, 0x97, L"value for list is NULL")
GET_REF_LIST(GetRefVRect, std::vector<wxRect>, 0x9C, L"value for list is NULL")
GET_REF_LIST(GetRefVPoint, std::vector<wxPoint>, 0xA1, L"value for list is NULL")
GET_REF_LIST(GetRefVPath, std::vector<TCharHolder>, 0xA6, L"value for list is NULL")
GET_REF_LIST(GetRefVInt, std::vector<int>, 0xAB, L"value for list is NULL")
GET_REF_LIST(GetRefVFloat, std::vector<float>, 0xB0, L"value for list is NULL")
GET_REF_LIST(GetRefVBool, std::vector<bool>, 0xB5, L"value for list is NULL")
GET_REF_LIST(GetRefVString, std::vector<TCharHolder>, 0xBA, L"value for list is NULL")
GET_REF_LIST(GetRefVSprite, std::vector<TSprite>, 0xBF, L"value for list is NULL")
GET_REF_LIST(GetRefVText, std::vector<TTextLanguage>, 0xC4, L"value for list is NULL")

#undef GET_REF_LIST

static int roundUp8(int size) {
	return (size + 7) & ~7;
}

// Confirmed (asm lines 625861-625884), with the sizes of the reconstructed
// types (see the file comment).
int TData::GetDataSize(eTypeData type) {
	switch (type) {
	case eTypeData::kBool:
		return roundUp8(sizeof(bool));
	case eTypeData::kInt:
		return roundUp8(sizeof(int));
	case eTypeData::kString:
	case eTypeData::kPath:
		return roundUp8(sizeof(TCharHolder));
	case eTypeData::kFloat:
		return roundUp8(sizeof(float));
	case eTypeData::kUnused5:
		return 0;
	case eTypeData::kRectList:
		return roundUp8(sizeof(std::vector<wxRect>));
	case eTypeData::kSpriteList:
		return roundUp8(sizeof(std::vector<TSprite>));
	case eTypeData::kPointList:
		return roundUp8(sizeof(std::vector<wxPoint>));
	case eTypeData::kStringList:
	case eTypeData::kPathList:
		return roundUp8(sizeof(std::vector<TCharHolder>));
	case eTypeData::kIntList:
		return roundUp8(sizeof(std::vector<int>));
	case eTypeData::kFloatList:
		return roundUp8(sizeof(std::vector<float>));
	case eTypeData::kPoint:
		return roundUp8(sizeof(wxPoint));
	case eTypeData::kRect:
		return roundUp8(sizeof(wxRect));
	case eTypeData::kSprite:
		return roundUp8(sizeof(TSprite));
	case eTypeData::kLink:
		return roundUp8(sizeof(TLink));
	case eTypeData::kLinkList:
		return roundUp8(sizeof(std::vector<TLink>));
	case eTypeData::kTextList:
		return roundUp8(sizeof(std::vector<TTextLanguage>));
	}
	return 0;
}

// Confirmed (asm lines 625885-626160)
void TData::CreateDataInstance(eTypeData type, void *value) {
	switch (type) {
	case eTypeData::kBool:
		GetRefBool(value) = false;
		break;
	case eTypeData::kInt:
		GetRefInt(value) = -1;
		break;
	case eTypeData::kFloat:
		GetRefFloat(value) = -1.0f;
		break;
	default:
		break;
	}

	if (!value)
		return;

	switch (type) {
	case eTypeData::kString:
	case eTypeData::kPath:
		new (value) TCharHolder();
		break;
	case eTypeData::kRectList:
		new (value) std::vector<wxRect>();
		break;
	case eTypeData::kSpriteList:
		new (value) std::vector<TSprite>();
		break;
	case eTypeData::kPointList:
		new (value) std::vector<wxPoint>();
		break;
	case eTypeData::kStringList:
	case eTypeData::kPathList:
		new (value) std::vector<TCharHolder>();
		break;
	case eTypeData::kIntList:
		new (value) std::vector<int>();
		break;
	case eTypeData::kFloatList:
		new (value) std::vector<float>();
		break;
	case eTypeData::kPoint:
		*static_cast<wxPoint *>(value) = wxPoint();
		break;
	case eTypeData::kRect:
		*static_cast<wxRect *>(value) = wxRect();
		break;
	case eTypeData::kSprite:
		new (value) TSprite();
		break;
	case eTypeData::kLink:
		new (value) TLink();
		break;
	case eTypeData::kLinkList:
		new (value) std::vector<TLink>();
		break;
	case eTypeData::kTextList:
		new (value) std::vector<TTextLanguage>();
		break;
	default:
		break;
	}
}

// The visionaire this record belongs to, or null. (The original reads
// group->_owner->_visionaire.)
static TVisionaire *groupVisionaire(TDataGroup *group) {
	if (!group || !group->GetOwner())
		return nullptr;
	return group->GetOwner()->GetVisionaire();
}

// Unregisters a link record's target (not for parent links or empty ones).
static void unregisterLink(const TLink &link, TDataGroup *group) {
	if (link.GetId().getId() == -1 && link.GetId().getTable() == 0xFF)
		return;
	if (link.IsParentLink())
		return;

	TVisionaire *visionaire = groupVisionaire(group);
	visionaire->RemoveLink(group->GetOwner()->GetTId(), link.GetId(), link.GetField());
}

// Confirmed (asm lines 626161-626520)
void TData::DeleteDataInstance(eTypeData type, void *value, TDataGroup *group) {
	if (!value)
		return;

	switch (type) {
	case eTypeData::kString:
	case eTypeData::kPath:
		static_cast<TCharHolder *>(value)->~TCharHolder();
		break;
	case eTypeData::kRectList:
		static_cast<std::vector<wxRect> *>(value)->~vector();
		break;
	case eTypeData::kSpriteList:
		static_cast<std::vector<TSprite> *>(value)->~vector();
		break;
	case eTypeData::kPointList:
		static_cast<std::vector<wxPoint> *>(value)->~vector();
		break;
	case eTypeData::kStringList:
	case eTypeData::kPathList:
		static_cast<std::vector<TCharHolder> *>(value)->~vector();
		break;
	case eTypeData::kIntList:
		static_cast<std::vector<int> *>(value)->~vector();
		break;
	case eTypeData::kFloatList:
		static_cast<std::vector<float> *>(value)->~vector();
		break;
	case eTypeData::kSprite:
		static_cast<TSprite *>(value)->~TSprite();
		break;
	case eTypeData::kLink: {
		TLink *link = static_cast<TLink *>(value);
		TVisionaire *visionaire = groupVisionaire(group);
		if (visionaire && !visionaire->IsLinkRemovalSuppressed())
			unregisterLink(*link, group);
		link->~TLink();
		break;
	}
	case eTypeData::kLinkList: {
		std::vector<TLink> *links = static_cast<std::vector<TLink> *>(value);
		TVisionaire *visionaire = groupVisionaire(group);
		if (visionaire && !visionaire->IsLinkRemovalSuppressed()) {
			for (const TLink &link : *links) {
				x_assert(!(link.GetId().getId() == -1 && link.GetId().getTable() == 0xFF), "!it->GetId().IsEmpty()",
				         kSourceFile, 0x154);
				unregisterLink(link, group);
			}
		}
		links->~vector();
		break;
	}
	case eTypeData::kTextList:
		static_cast<std::vector<TTextLanguage> *>(value)->~vector();
		break;
	default:
		break;
	}
}

// Confirmed (asm lines 626521-627456)
void TData::ClearDataInstance(eTypeData type, void *value) {
	switch (type) {
	case eTypeData::kBool:
		GetRefBool(value) = false;
		break;
	case eTypeData::kInt:
		GetRefInt(value) = -1;
		break;
	case eTypeData::kString:
		GetRefString(value) = "";
		break;
	case eTypeData::kPath:
		GetRefPath(value) = "";
		break;
	case eTypeData::kFloat:
		GetRefFloat(value) = -1.0f;
		break;
	case eTypeData::kRectList:
		GetRefVRect(value).clear();
		break;
	case eTypeData::kSpriteList:
		GetRefVSprite(value).clear();
		break;
	case eTypeData::kPointList:
		GetRefVPoint(value).clear();
		break;
	case eTypeData::kStringList:
		GetRefVString(value).clear();
		break;
	case eTypeData::kIntList:
		GetRefVInt(value).clear();
		break;
	case eTypeData::kPathList:
		GetRefVPath(value).clear();
		break;
	case eTypeData::kFloatList:
		GetRefVFloat(value).clear();
		break;
	case eTypeData::kPoint: {
		wxPoint &point = GetRefPoint(value);
		point.x = -1;
		point.y = -1;
		break;
	}
	case eTypeData::kRect: {
		wxRect &rect = GetRefRect(value);
		rect.x = -1;
		rect.y = -1;
		rect.width = -1;
		rect.height = -1;
		break;
	}
	case eTypeData::kSprite:
		GetRefSprite(value).Clear();
		break;
	case eTypeData::kLink:
		GetRefLink(value).Clear();
		break;
	case eTypeData::kLinkList:
		GetRefLinks(value).clear();
		break;
	case eTypeData::kTextList:
		GetRefVText(value).clear();
		break;
	default:
		break;
	}
}

// Confirmed (asm lines 627457-627877)
void TData::SerializeParam(const char *begin, const char *end, eTypeData type, void *value) {
	switch (type) {
	case eTypeData::kBool:
		GetRefBool(value) = ConvertToBool(begin, end);
		break;
	case eTypeData::kInt: {
		std::string text(begin, end);
		GetRefInt(value) = (int)dtol(text.c_str());
		break;
	}
	case eTypeData::kString:
		GetRefString(value).Assign(begin, end);
		break;
	case eTypeData::kPath: {
		TCharHolder &path = GetRefPath(value);
		path.Assign(begin, end);
		if (path.size() != 0)
			normalizepath(const_cast<char *>(path.mb_str()));
		break;
	}
	case eTypeData::kFloat:
		GetRefFloat(value) = ConvertToFloat(begin, end);
		break;
	default:
		break;
	}
}

// Confirmed (asm lines 627878-628147)
void TData::SerializeParam(TProjectFileWriter &writer, eTypeData type, int name, void *value) {
	switch (type) {
	case eTypeData::kBool:
		writer.AddAttribute(name, GetRefBool(value));
		break;
	case eTypeData::kInt:
		writer.AddAttribute(name, GetRefInt(value));
		break;
	case eTypeData::kString:
		writer.AddAttributeS(name, GetRefString(value));
		break;
	case eTypeData::kPath:
		writer.AddAttributeS(name, GetRefPath(value));
		break;
	case eTypeData::kFloat:
		writer.AddAttribute(name, GetRefFloat(value));
		break;
	default:
		break;
	}
}

// Confirmed (asm lines 628148-628809)
bool TData::SerializeContent(TProjectFileWriter &writer, eTypeData type, int name, void *value) {
	switch (type) {
	case eTypeData::kRectList:
		writer.Serialize(name, GetRefVRect(value));
		break;
	case eTypeData::kSpriteList:
		writer.Serialize(name, GetRefVSprite(value));
		break;
	case eTypeData::kPointList:
		writer.Serialize(name, GetRefVPoint(value));
		break;
	case eTypeData::kStringList:
	case eTypeData::kPathList:
		writer.Serialize(name, GetRefVString(value));
		break;
	case eTypeData::kIntList:
		writer.Serialize(name, GetRefVInt(value));
		break;
	case eTypeData::kFloatList:
		writer.Serialize(name, GetRefVFloat(value));
		break;
	case eTypeData::kPoint:
		writer.Serialize(name, GetRefPoint(value));
		break;
	case eTypeData::kRect:
		writer.Serialize(name, GetRefRect(value));
		break;
	case eTypeData::kSprite:
		writer.Serialize(name, GetRefSprite(value));
		break;
	case eTypeData::kLink:
		GetRefLink(value).Serialize(writer, name);
		break;
	case eTypeData::kLinkList:
		TLink::Serialize(writer, name, GetRefLinks(value));
		break;
	case eTypeData::kTextList:
		writer.Serialize(name, GetRefVText(value));
		break;
	default:
		break;
	}
	return true;
}

static bool sameRects(const std::vector<wxRect> &a, const std::vector<wxRect> &b) {
	if (a.size() != b.size())
		return false;
	for (size_t i = 0; i < a.size(); i++) {
		if (a[i].x != b[i].x || a[i].y != b[i].y || a[i].width != b[i].width || a[i].height != b[i].height)
			return false;
	}
	return true;
}

template<typename T>
static bool sameElements(const std::vector<T> &a, const std::vector<T> &b) {
	if (a.size() != b.size())
		return false;
	for (size_t i = 0; i < a.size(); i++) {
		if (!(a[i] == b[i]))
			return false;
	}
	return true;
}

static bool sameStrings(const std::vector<TCharHolder> &a, const std::vector<TCharHolder> &b) {
	if (a.size() != b.size())
		return false;
	for (size_t i = 0; i < a.size(); i++) {
		if (!a[i].SameAs(b[i]))
			return false;
	}
	return true;
}

// Every link of `a` has to be somewhere in `b`, and the lists have to be the
// same length.
static bool sameLinks(const std::vector<TLink> &a, const std::vector<TLink> &b) {
	if (a.size() != b.size())
		return false;
	for (const TLink &link : a) {
		bool found = false;
		for (const TLink &other : b) {
			if (link == other) {
				found = true;
				break;
			}
		}
		if (!found)
			return false;
	}
	return true;
}

// Confirmed (asm lines 628810-630420 and 630421-631473, which are the same
// switch with the second value taken as is rather than null-checked). Kinds 5
// and 18 are never equal.
static int compareValues(eTypeData type, const void *a, const void *b) {
	bool equal;

	switch (type) {
	case eTypeData::kBool:
		equal = GetRefBool(a) == GetRefBool(b);
		break;
	case eTypeData::kInt:
		equal = GetRefInt(a) == GetRefInt(b);
		break;
	case eTypeData::kString:
		equal = GetRefString(a).SameAs(GetRefString(b));
		break;
	case eTypeData::kPath:
		equal = GetRefPath(a).SameAs(GetRefPath(b));
		break;
	case eTypeData::kFloat:
		equal = GetRefFloat(a) == GetRefFloat(b);
		break;
	case eTypeData::kRectList:
		equal = sameRects(GetRefVRect(a), GetRefVRect(b));
		break;
	case eTypeData::kSpriteList:
		equal = sameElements(GetRefVSprite(a), GetRefVSprite(b));
		break;
	case eTypeData::kPointList:
		equal = sameElements(GetRefVPoint(a), GetRefVPoint(b));
		break;
	case eTypeData::kStringList:
		equal = sameStrings(GetRefVString(a), GetRefVString(b));
		break;
	case eTypeData::kIntList:
		equal = sameElements(GetRefVInt(a), GetRefVInt(b));
		break;
	case eTypeData::kPathList:
		equal = sameStrings(GetRefVPath(a), GetRefVPath(b));
		break;
	case eTypeData::kFloatList:
		equal = sameElements(GetRefVFloat(a), GetRefVFloat(b));
		break;
	case eTypeData::kPoint:
		equal = GetRefPoint(a) == GetRefPoint(b);
		break;
	case eTypeData::kRect:
		equal = sameRects({GetRefRect(a)}, {GetRefRect(b)});
		break;
	case eTypeData::kSprite:
		equal = GetRefSprite(a) == GetRefSprite(b);
		break;
	case eTypeData::kLink:
		equal = GetRefLink(a) == GetRefLink(b);
		break;
	case eTypeData::kLinkList:
		equal = sameLinks(GetRefLinks(a), GetRefLinks(b));
		break;
	default:
		equal = false;
		break;
	}
	return equal ? 0 : -1;
}

int TData::Compare(eTypeData type, const void *a, const void *b) {
	return compareValues(type, a, b);
}

int TData::CompareDataWithValue(eTypeData type, const void *data, const void *value) {
	return compareValues(type, data, value);
}

static wxString formatText(const wchar_t *format, ...) {
	wchar_t buffer[128];
	va_list args;

	va_start(args, format);
	vswprintf(buffer, 128, format, args);
	va_end(args);
	return wxString(buffer);
}

// Joins the printed elements with the separator.
template<typename T, typename F>
static wxString joinList(const std::vector<T> &list, const wchar_t *separator, F toText) {
	wxString result;
	bool first = true;

	for (const T &element : list) {
		if (!first)
			result += wxString(separator);
		first = false;
		result += toText(element);
	}
	return result;
}

static wxString linkText(const TLink &link, TVisionaire *visionaire) {
	if (link.GetId().getId() == -1 && link.GetId().getTable() == 0xFF)
		return wxString(L"[empty]");

	TVisionaireObject *object = visionaire->GetObjectById(link.GetId());
	if (!object)
		return wxString();
	return wxString(object->GetName());
}

// Confirmed (asm lines 631474-633417)
wxString TData::ToString(eTypeData type, void *value, TVisionaire *visionaire) {
	switch (type) {
	case eTypeData::kBool:
		return wxString(GetRefBool(value) ? L"true" : L"false");
	case eTypeData::kInt:
		return formatText(L"%d", GetRefInt(value));
	case eTypeData::kString:
		return wxString(GetRefString(value));
	case eTypeData::kPath:
		return GetRefPath(value).GetFullPath();
	case eTypeData::kFloat:
		return formatText(L"%f", (double)GetRefFloat(value));
	case eTypeData::kRectList:
		return joinList(GetRefVRect(value), L", ", [](const wxRect &rect) {
			return formatText(L"(%d,%d)-(%d,%d)", rect.GetLeft(), rect.GetTop(), rect.GetRight(), rect.GetBottom());
		});
	case eTypeData::kSpriteList:
		return joinList(GetRefVSprite(value), L", ", [](const TSprite &sprite) {
			return sprite.ToLuaString();
		});
	case eTypeData::kPointList:
		return joinList(GetRefVPoint(value), L", ", [](const wxPoint &point) {
			return formatText(L"(%d,%d)", point.x, point.y);
		});
	case eTypeData::kStringList:
		return joinList(GetRefVString(value), L", ", [](const TCharHolder &text) {
			return wxString(text);
		});
	case eTypeData::kIntList:
		return joinList(GetRefVInt(value), L", ", [](int number) {
			return CONVTOSTR(number);
		});
	case eTypeData::kPathList:
		return joinList(GetRefVPath(value), L", ", [](const TCharHolder &path) {
			wxString text;
			toUTF(&text, path.mb_str());
			return text;
		});
	case eTypeData::kFloatList:
		return joinList(GetRefVFloat(value), L", ", [](float number) {
			return formatText(L"%.2f", (double)number);
		});
	case eTypeData::kPoint: {
		const wxPoint &point = GetRefPoint(value);
		return formatText(L"%d,%d", point.x, point.y);
	}
	case eTypeData::kRect: {
		const wxRect &rect = GetRefRect(value);
		return formatText(L"%d,%d-%d,%d", rect.GetLeft(), rect.GetTop(), rect.GetRight(), rect.GetBottom());
	}
	case eTypeData::kSprite:
		return GetRefSprite(value).ToLuaString();
	case eTypeData::kLink: {
		const TLink &link = GetRefLink(value);
		if (link.IsAnyLink())
			return wxString(L"[any]");
		return linkText(link, visionaire);
	}
	case eTypeData::kLinkList:
		return joinList(GetRefLinks(value), L"\n", [visionaire](const TLink &link) {
			return linkText(link, visionaire);
		});
	default:
		return wxString();
	}
}

// Confirmed (asm lines 633418-634701). The element sizes are the original
// layout's (see the file comment), since this is a statistic about the data
// rather than about this implementation.
unsigned long TData::GetSizeMemory(eTypeData type, void *value, bool /*deep*/, TVisionaire * /*visionaire*/) {
	unsigned long size = 0;

	switch (type) {
	case eTypeData::kString:
		return GetRefString(value).size();
	case eTypeData::kPath:
		return GetRefPath(value).size();
	case eTypeData::kRectList:
		return GetRefVRect(value).size() * 16;
	case eTypeData::kSpriteList:
		for (TSprite &sprite : GetRefVSprite(value))
			size += 0x50 + sprite.GetPathNonConst().size() + sprite.GetNameNonConst().size();
		return size;
	case eTypeData::kPointList:
		return GetRefVPoint(value).size() * 8;
	case eTypeData::kStringList:
		for (const TCharHolder &text : GetRefVString(value))
			size += 0x10 + text.size();
		return size;
	case eTypeData::kIntList:
		return GetRefVInt(value).size() * 4;
	case eTypeData::kPathList:
		for (const TCharHolder &path : GetRefVPath(value))
			size += 0x10 + path.size();
		return size;
	case eTypeData::kFloatList:
		return GetRefVFloat(value).size() * 4;
	case eTypeData::kSprite: {
		TSprite &sprite = GetRefSprite(value);
		return sprite.GetPathNonConst().size() + sprite.GetNameNonConst().size();
	}
	case eTypeData::kLinkList:
		return GetRefLinks(value).size() * 8;
	case eTypeData::kTextList:
		for (TTextLanguage &text : GetRefVText(value))
			size += text.text.size() + text.audioFile.size() + 0x28;
		return size;
	default:
		return 0;
	}
}

// Confirmed (asm lines 634702-634716)
unsigned long TData::SizePath(const TCharHolder &path) {
	return path.size();
}

// Confirmed (asm lines 634717-635587)
void TData::Assign(eTypeData type, void *dest, const void *source) {
	switch (type) {
	case eTypeData::kBool:
		GetRefBool(dest) = *static_cast<const bool *>(source);
		break;
	case eTypeData::kInt:
		GetRefInt(dest) = *static_cast<const int *>(source);
		break;
	case eTypeData::kString:
		GetRefString(dest) = *static_cast<const TCharHolder *>(source);
		break;
	case eTypeData::kPath:
		GetRefPath(dest) = *static_cast<const TCharHolder *>(source);
		break;
	case eTypeData::kFloat:
		GetRefFloat(dest) = *static_cast<const float *>(source);
		break;
	case eTypeData::kRectList:
		GetRefVRect(dest) = *static_cast<const std::vector<wxRect> *>(source);
		break;
	case eTypeData::kSpriteList:
		GetRefVSprite(dest) = *static_cast<const std::vector<TSprite> *>(source);
		break;
	case eTypeData::kPointList:
		GetRefVPoint(dest) = *static_cast<const std::vector<wxPoint> *>(source);
		break;
	case eTypeData::kStringList:
		GetRefVString(dest) = *static_cast<const std::vector<TCharHolder> *>(source);
		break;
	case eTypeData::kIntList:
		GetRefVInt(dest) = *static_cast<const std::vector<int> *>(source);
		break;
	case eTypeData::kPathList:
		GetRefVPath(dest) = *static_cast<const std::vector<TCharHolder> *>(source);
		break;
	case eTypeData::kFloatList:
		GetRefVFloat(dest) = *static_cast<const std::vector<float> *>(source);
		break;
	case eTypeData::kPoint:
		GetRefPoint(dest) = *static_cast<const wxPoint *>(source);
		break;
	case eTypeData::kRect:
		GetRefRect(dest) = *static_cast<const wxRect *>(source);
		break;
	case eTypeData::kSprite:
		GetRefSprite(dest) = *static_cast<const TSprite *>(source);
		break;
	case eTypeData::kTextList:
		GetRefVText(dest) = *static_cast<const std::vector<TTextLanguage> *>(source);
		break;
	default:
		break;
	}
}
