// Not yet assert-confirmed to a specific file; stays at the top level.
// TPictureIO's base class (confirmed: TPictureIO's ctor/dtor call
// TPictureMEM::TPictureMEM()/~TPictureMEM() directly on `this`, no offset
// adjustment). Owns the decoded pixel buffer and width/height - not
// reversed beyond that.
//
// Inherits TSprite: TPictureIO also calls TSprite::Set/operator==/
// SetImageSize on `this` with no offset adjustment, only explained by this
// base chain (TPictureMEM : public TSprite).
#pragma once

#include "TSprite.h"

class TPictureMEM : public TSprite {
public:
    TPictureMEM() = default;
    virtual ~TPictureMEM() = default;

    void ClearMemData();

protected:
    int m_width = 0;
    int m_height = 0;
};
