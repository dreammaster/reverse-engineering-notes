// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/graphicslib/graphics.cpp - see manifest/source_layout.tsv.
//
// The global rendering-backend singleton (`cs:graphics` in the
// disassembly). Only the surface TPictureIO/TMasterControl touch is
// stubbed: GetPreloadedPicManager(), GetSpriteFromCache(), and a virtual
// method at vtable slot 0x90 (called whenever a TSpriteHandle's refcount
// hits zero, to let the backend release the GPU resource - real name not
// recovered).
#pragma once

#include "TSpriteHandle.h"
#include "WxStub.h"

class TPreloadedPicManager;

class TGraphicsInterface {
public:
    virtual ~TGraphicsInterface() = default;

    TPreloadedPicManager* GetPreloadedPicManager();
    TSpriteHandle* GetSpriteFromCache(const wxString& name);

    // vtable slot 0x90 in the original; called with a TSpriteHandle* whose
    // refcount just reached zero.
    virtual void OnSpriteHandleReleased(TSpriteHandle* handle);

    // Called from TMasterControl::Draw/Signal/PlayAVI (vtable slots
    // 0x30/0x38/0x178 there); real parameter meaning not recovered.
    virtual void SetMatrixMode(bool a, bool b);
    virtual void ResetMatrix(bool a, bool b);
    virtual void Flip();

private:
    TPreloadedPicManager* m_preloadedPicManager = nullptr;
};

extern TGraphicsInterface* graphics;
