#include "TSprite.h"

bool TSprite::operator==(const TSprite &other) const {
	return _path.GetFullPath().ToStdWstring() == other._path.GetFullPath().ToStdWstring() && _id == other._id &&
	       _type == other._type;
}

void TSprite::Set(const TSprite &other) {
	_path = other._path;
	_id = other._id;
	_type = other._type;
}

void TSprite::SetImageSize(int width, int height) {
	_imageWidth = width;
	_imageHeight = height;
}
