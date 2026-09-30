#include "vscommon/scripting/argument.h"

#include <cwchar>

#include "datastruct/visionaire.h"
#include "datastruct/visionaireobject.h"
#include "vscommon/scripting/id.h"
#include "vscommon/scripting/lua.h"

namespace {
// Confirmed byte-for-byte (TArgument::SetPath/AddPath, Deponia_Linux.asm
// line 3139804) - a custom URI-like scheme prefix for Visionaire's own path
// references, stripped when present.
const wxString kPathPrefix(L"vispath:");
}  // namespace

void TArgument::Clear() {
	_value = std::monostate{};
	_type = TArgType::kNone;
}

void TArgument::SetType(TArgType type) {
	// Confirmed (Deponia_Linux.asm lines 1435752-1435837): default-
	// initializes the new type's payload. Types kNone and kObjectList hit an
	// x_assert(false) in the original when set this way directly - skipped
	// rather than reproduced, matching this project's usual treatment of
	// "impossible per the caller" assertions.
	switch (type) {
	case TArgType::kNone:
		_value = std::monostate{};
		break;
	case TArgType::kBool:
		_value = false;
		break;
	case TArgType::kInt:
		_value = 0;
		break;
	case TArgType::kFloat:
		_value = 0.0;
		break;
	case TArgType::kPoint:
		_value = wxPoint();
		break;
	case TArgType::kRect:
		_value = wxRect();
		break;
	case TArgType::kString:
	case TArgType::kPath:
		_value = wxString();
		break;
	case TArgType::kSprite:
		_value = TSprite();
		break;
	case TArgType::kObject:
		_value = TVisObjRef();
		break;
	case TArgType::kText:
		_value = TTextLanguage();
		break;
	case TArgType::kIntList:
		_value = std::vector<int>();
		break;
	case TArgType::kFloatList:
		_value = std::vector<float>();
		break;
	case TArgType::kPointList:
		_value = std::vector<wxPoint>();
		break;
	case TArgType::kRectList:
		_value = std::vector<wxRect>();
		break;
	case TArgType::kStringList:
	case TArgType::kPathList:
		_value = std::vector<TCharHolder>();
		break;
	case TArgType::kSpriteList:
		_value = std::vector<TSprite>();
		break;
	case TArgType::kObjectList:
		_value = TVList();
		break;
	case TArgType::kTextList:
		_value = std::vector<TTextLanguage>();
		break;
	}
	_type = type;
}

void TArgument::Set(bool value) {
	_value = value;
	_type = TArgType::kBool;
}

void TArgument::Set(int value) {
	_value = value;
	_type = TArgType::kInt;
}

void TArgument::Set(double value) {
	_value = value;
	_type = TArgType::kFloat;
}

void TArgument::Set(const wxPoint &value) {
	_value = value;
	_type = TArgType::kPoint;
}

void TArgument::Set(const wxRect &value) {
	_value = value;
	_type = TArgType::kRect;
}

void TArgument::Set(const wxString &value) {
	_value = value;
	_type = TArgType::kString;
}

void TArgument::SetPath(const wxString &value) {
	_value = value.StartsWith(kPathPrefix) ? value.Mid(8, -1) : value;
	_type = TArgType::kPath;
}

void TArgument::Set(const TSprite &value) {
	_value = value;
	_type = TArgType::kSprite;
}

void TArgument::Set(const TTextLanguage &value) {
	_value = value;
	_type = TArgType::kText;
}

void TArgument::Set(const TVisObjRef &value) {
	_value = value;
	_type = TArgType::kObject;
}

void TArgument::Set(TVisionaireObject *value) {
	_value = TVisObjRef(value);
	_type = TArgType::kObject;
}

void TArgument::Set(const TVList &value) {
	_value = value;
	_type = TArgType::kObjectList;
}

void TArgument::Set(const std::vector<int> &value) {
	_value = value;
	_type = TArgType::kIntList;
}

void TArgument::Set(const std::vector<float> &value) {
	_value = value;
	_type = TArgType::kFloatList;
}

void TArgument::Set(const std::vector<wxPoint> &value) {
	_value = value;
	_type = TArgType::kPointList;
}

void TArgument::Set(const std::vector<wxRect> &value) {
	_value = value;
	_type = TArgType::kRectList;
}

void TArgument::Set(const std::vector<TSprite> &value) {
	_value = value;
	_type = TArgType::kSpriteList;
}

void TArgument::Set(const std::vector<TCharHolder> &value) {
	_value = value;
	_type = TArgType::kStringList;
}

void TArgument::SetPaths(const std::vector<TCharHolder> &value) {
	_value = value;
	_type = TArgType::kPathList;
}

void TArgument::Set(const std::vector<TTextLanguage> &value) {
	_value = value;
	_type = TArgType::kTextList;
}

void TArgument::Add(const TVisObjRef &value) {
	if (_type != TArgType::kObjectList)
		SetType(TArgType::kObjectList);
	std::get<TVList>(_value).push_back(value);
}

void TArgument::Add(int value) {
	if (_type != TArgType::kIntList)
		SetType(TArgType::kIntList);
	std::get<std::vector<int>>(_value).push_back(value);
}

void TArgument::Add(float value) {
	if (_type != TArgType::kFloatList)
		SetType(TArgType::kFloatList);
	std::get<std::vector<float>>(_value).push_back(value);
}

void TArgument::Add(const wxPoint &value) {
	if (_type != TArgType::kPointList)
		SetType(TArgType::kPointList);
	std::get<std::vector<wxPoint>>(_value).push_back(value);
}

void TArgument::Add(const wxRect &value) {
	if (_type != TArgType::kRectList)
		SetType(TArgType::kRectList);
	std::get<std::vector<wxRect>>(_value).push_back(value);
}

void TArgument::Add(const TSprite &value) {
	if (_type != TArgType::kSpriteList)
		SetType(TArgType::kSpriteList);
	std::get<std::vector<TSprite>>(_value).push_back(value);
}

void TArgument::Add(const wxString &value) {
	if (_type != TArgType::kStringList)
		SetType(TArgType::kStringList);
	std::get<std::vector<TCharHolder>>(_value).push_back(TCharHolder(value));
}

void TArgument::AddPath(const wxString &value) {
	if (_type != TArgType::kPathList)
		SetType(TArgType::kPathList);
	wxString stripped = value.StartsWith(kPathPrefix) ? value.Mid(8, -1) : value;
	std::get<std::vector<TCharHolder>>(_value).push_back(TCharHolder(stripped));
}

bool TArgument::GetBool() const {
	return GetOr(TArgType::kBool, false);
}

int TArgument::GetInt() const {
	return GetOr(TArgType::kInt, 0);
}

float TArgument::GetFloat() const {
	return static_cast<float>(GetOr(TArgType::kFloat, 0.0));
}

const wxPoint &TArgument::GetPoint() const {
	static const wxPoint kDefault;
	return GetOr(TArgType::kPoint, kDefault);
}

const wxRect &TArgument::GetRect() const {
	static const wxRect kDefault;
	return GetOr(TArgType::kRect, kDefault);
}

const wxString &TArgument::GetString() const {
	static const wxString kDefault;
	return GetOr(TArgType::kString, kDefault);
}

const wxString &TArgument::GetPath() const {
	static const wxString kDefault;
	return GetOr(TArgType::kPath, kDefault);
}

const TSprite &TArgument::GetSprite() const {
	static const TSprite kDefault;
	return GetOr(TArgType::kSprite, kDefault);
}

const TTextLanguage &TArgument::GetText() const {
	static const TTextLanguage kDefault;
	return GetOr(TArgType::kText, kDefault);
}

TVisObjRef TArgument::GetObject() const {
	return _type == TArgType::kObject ? std::get<TVisObjRef>(_value) : TVisObjRef();
}

const std::vector<int> &TArgument::GetIntList() const {
	static const std::vector<int> kDefault;
	return GetOr(TArgType::kIntList, kDefault);
}

const std::vector<float> &TArgument::GetFloatList() const {
	static const std::vector<float> kDefault;
	return GetOr(TArgType::kFloatList, kDefault);
}

const std::vector<wxPoint> &TArgument::GetPointList() const {
	static const std::vector<wxPoint> kDefault;
	return GetOr(TArgType::kPointList, kDefault);
}

const std::vector<wxRect> &TArgument::GetRectList() const {
	static const std::vector<wxRect> kDefault;
	return GetOr(TArgType::kRectList, kDefault);
}

const std::vector<TCharHolder> &TArgument::GetStringList() const {
	static const std::vector<TCharHolder> kDefault;
	return GetOr(TArgType::kStringList, kDefault);
}

const std::vector<TCharHolder> &TArgument::GetPathList() const {
	static const std::vector<TCharHolder> kDefault;
	return GetOr(TArgType::kPathList, kDefault);
}

const std::vector<TSprite> &TArgument::GetSpriteList() const {
	static const std::vector<TSprite> kDefault;
	return GetOr(TArgType::kSpriteList, kDefault);
}

const TVList &TArgument::GetObjectList() const {
	static const TVList kDefault;
	return GetOr(TArgType::kObjectList, kDefault);
}

const std::vector<TTextLanguage> &TArgument::GetTextList() const {
	static const std::vector<TTextLanguage> kDefault;
	return GetOr(TArgType::kTextList, kDefault);
}

bool TArgument::ConvertToInt() {
	if (_type != TArgType::kFloat)
		return false;
	Set(static_cast<int>(std::get<double>(_value)));
	return true;
}

bool TArgument::ConvertToIntList() {
	if (_type == TArgType::kFloatList) {
		const auto &floats = std::get<std::vector<float>>(_value);
		std::vector<int> ints;
		ints.reserve(floats.size());
		for (float f : floats)
			ints.push_back(static_cast<int>(f));
		Set(ints);
		return true;
	}
	if (_type == TArgType::kNone) {
		Set(std::vector<int>());
		return true;
	}
	return false;
}

bool TArgument::ConvertToObject() {
	// Confirmed (Deponia_Linux.asm lines 1437440-1437665).
	if (_type == TArgType::kNone) {
		Set(TVisObjRef());
		return true;
	}
	if (_type != TArgType::kString)
		return false;

	const wxString &str = std::get<wxString>(_value);
	int a = 0, b = 0;
	std::swscanf(str.c_str(), L"%d,%d", &a, &b);
	if (TId(a, b) == AnyId) {
		Set(GetLuaGame()->GetAnyObject());
		return true;
	}
	TVisObjRef found;
	if (!FindObjectByNameOrId(str, found, true))
		return false;
	Set(found);
	return true;
}

bool TArgument::ConvertToObjectList() {
	// Confirmed (Deponia_Linux.asm lines 1437671-1437912).
	if (_type == TArgType::kNone) {
		Set(TVList());
		return true;
	}
	if (_type != TArgType::kStringList)
		return false;

	TVList result;
	for (const TCharHolder &name : std::get<std::vector<TCharHolder>>(_value)) {
		TVisObjRef found;
		if (!FindObjectByNameOrId(name.GetFullPath(), found, true))
			return false;
		result.push_back(found);
	}
	Set(result);
	return true;
}

void TArgument::ToLua() const {
	// Not reversed - see this method's own header comment.
}
