// Not yet assert-confirmed to a specific file; stays at the top level.
//
// TPictureMEM's base class (inferred: TPictureIO's ctor/dtor call
// TPictureMEM::TPictureMEM()/~TPictureMEM() on `this` with no offset
// adjustment, and TPictureIO also calls TSprite::Set/operator==/
// SetImageSize on `this` with no offset adjustment - only explained by
// TPictureMEM : public TSprite, chaining through to TPictureIO). Fields
// below are inferred from TPictureIO::GetSpriteName's usage (a path, an id,
// and a "type" checked against 1) - real names/full field set unconfirmed.
#pragma once

#include "TCharHolder.h"

class TSprite {
public:
    TSprite() = default;

    bool operator==(const TSprite& other) const;
    void Set(const TSprite& other);
    void SetImageSize(int width, int height);

    TCharHolder m_path;
    int m_id = 0;
    int m_type = 0;
    int m_imageWidth = 0;
    int m_imageHeight = 0;
};
