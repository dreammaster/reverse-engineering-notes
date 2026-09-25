#include "TSprite.h"

bool TSprite::operator==(const TSprite& other) const {
    return m_path.GetFullPath().ToStdWstring() == other.m_path.GetFullPath().ToStdWstring() && m_id == other.m_id &&
           m_type == other.m_type;
}

void TSprite::Set(const TSprite& other) {
    m_path = other.m_path;
    m_id = other.m_id;
    m_type = other.m_type;
}

void TSprite::SetImageSize(int width, int height) {
    m_imageWidth = width;
    m_imageHeight = height;
}
