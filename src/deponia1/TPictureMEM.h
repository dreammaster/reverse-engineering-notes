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

struct TPictureMemBlock;

class TPictureMEM : public TSprite {
public:
	TPictureMEM() = default;
	virtual ~TPictureMEM() = default;

	void ClearMemData();

	// Confirmed call shapes only (TGScene::GetTint()/SetCurrentLightmap(),
	// Deponia_Linux.asm lines 168383-171104) - a lightmap's pixel lookup (the
	// float is the scene's brightness) and its backing memory block; not
	// reversed beyond those call shapes.
	unsigned int GetPixel(const wxPoint &pos, float brightness) const;
	void SetMemoryBlock(TPictureMemBlock *block);
	// Confirmed call shape only (TMSavegame::SaveSnapShot(), Deponia_Linux.asm
	// line 162236+) - rescales `source` to a new size into this picture (the
	// two bools' meaning is unresolved); not reversed beyond that.
	void ResizeImage(const TPictureMEM &source, const wxPoint &size, bool flag1, bool flag2);

protected:
	int _width = 0;
	int _height = 0;
};
