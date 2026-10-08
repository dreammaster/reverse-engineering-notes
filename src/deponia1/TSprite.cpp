#include "TSprite.h"

#include <cwchar>

#include "vscommon/scripting/luaConversion.h"
#include "vscommon/scripting/visLua.h"

TSprite::TSprite(const TSprite &other) {
	Set(other);
}

TSprite::TSprite(const wxFileName &path) {
	_path = path;
}

void TSprite::Set(const TSprite &other) {
	_position = other._position;
	_scale = other._scale;
	_path = other._path;
	_transparentColor = other._transparentColor;
	_transparencyMode = other._transparencyMode;
	_flags = other._flags;
	_pause = other._pause;
	_name = other._name;
}

bool TSprite::operator==(const TSprite &other) const {
	if (_transparencyMode != other._transparencyMode)
		return false;
	if (_transparencyMode == eTransparencyMode::kColorKey && _transparentColor != other._transparentColor)
		return false;
	if (_pause != other._pause)
		return false;
	if (!(_path == other._path))
		return false;
	if (_position != other._position)
		return false;
	if (IsMirrored() != other.IsMirrored())
		return false;
	return _name == other._name;
}

bool TSprite::CmpReloadNeeded(const TSprite &other) const {
	wxFileName thisPath = _path;
	wxFileName otherPath = other._path;
	if (thisPath.GetFullPath().Cmp(otherPath.GetFullPath()) != 0)
		return true;
	if (_transparencyMode != other._transparencyMode)
		return true;
	if (_transparencyMode == eTransparencyMode::kColorKey && _transparentColor != other._transparentColor)
		return true;
	return false;
}

void TSprite::Clear() {
	_position = wxPoint();
	_scale = 100.0f;
	_path = TCharHolder();
	_transparencyMode = eTransparencyMode::kAlpha;
	_transparentColor = 0;
	_flags = 0;
	_pause = -1;
	_name = "";
}

// Confirmed (asm lines 583664-583929): the sprite as a Lua table constructor, which SetFromLuaString() reads back.
wxString TSprite::ToLuaString() const {
	const wchar_t *transparency = L"eTransparencyUndefined";

	switch (_transparencyMode) {
	case eTransparencyMode::kAlpha:
		transparency = L"eTransparencyAlpha";
		break;
	case eTransparencyMode::kNone:
		transparency = L"eTransparencyNone";
		break;
	case eTransparencyMode::kColorKey:
		transparency = L"eTransparencyColorKey";
		break;
	default:
		break;
	}

	wxString path = _path.GetFullPath(1);
	wchar_t text[2048];

	std::swprintf(text, sizeof(text) / sizeof(text[0]),
	              L"{path='%ls',position={x=%d,y=%d},transparency=%ls,transpcolor=%d,pause=%d}", path.wc_str(),
	              _position.x, _position.y, transparency, static_cast<int>(_transparentColor), static_cast<int>(_pause));
	return wxString(text);
}

// Confirmed (asm lines 583939-584050): the text is run as Lua (a table constructor, "return " first is its
// business) and the table it gives is read as the sprite. The answer is always true. (The table is given to
// ConvertFromLua() as index -1, but that function counts after it has pushed the name of the first key, so it
// looks at the wrong value and the sprite stays as it was - the same in the original. Only the editor calls this.)
bool TSprite::SetFromLuaString(const wxString &value) {
	LuaDoString(std::string(value.mb_str()), std::string());
	ConvertFromLua(*this, -1);
	lua_settop(L, -2);
	return true;
}

bool TSprite::IsTransparencyEqual(const TSprite &other) const {
	if (other._transparencyMode == eTransparencyMode::kAny)
		return true;
	if (other._transparencyMode == eTransparencyMode::kColorKey)
		return _transparentColor == other._transparentColor;
	return _transparencyMode == other._transparencyMode;
}
