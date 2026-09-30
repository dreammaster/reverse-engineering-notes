#include "TSprite.h"

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

wxString TSprite::ToLuaString() const {
	// Not reversed - see this method's own header comment.
	return wxString();
}

bool TSprite::SetFromLuaString(const wxString &/*value*/) {
	// Not reversed - see this method's own header comment.
	return true;
}

bool TSprite::IsTransparencyEqual(const TSprite &other) const {
	if (other._transparencyMode == eTransparencyMode::kAny)
		return true;
	if (other._transparencyMode == eTransparencyMode::kColorKey)
		return _transparentColor == other._transparentColor;
	return _transparencyMode == other._transparencyMode;
}
