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
#include "datastruct/visobjref.h"

class TPreloadedPicManager;
struct TPictureMemBlock;

// Confirmed 2 raw-int parameters read straight from game data (field ids
// 0x224/0x225), never a named constant - real values/meaning not resolved
// (TGameControl::LoadAndInitGame, Deponia_Linux.asm lines 467710-467720).
enum class TInterpolationEnum : int {};

class TGraphicsInterface {
public:
	virtual ~TGraphicsInterface() = default;

	TPreloadedPicManager *GetPreloadedPicManager();
	TSpriteHandle *GetSpriteFromCache(const wxString &name);
	// Confirmed call shape only (TGScene::SetCurrentLightmap(), Deponia_
	// Linux.asm line 168383+) - the shared memory block a scene's lightmap
	// picture decodes into; not reversed beyond that call shape.
	TPictureMemBlock *GetLightMapMemBlock();

	// Confirmed call shapes only (TGameControl::LoadAndInitGame, asm lines
	// 467717-467726, 467884-467886) - not reversed beyond that.
	void SetFilters(TInterpolationEnum a, TInterpolationEnum b);
	void PreallocateTextures(int count);
	void SetCacheSize(int size);

	// Confirmed call shapes only (TGameControl::Update, Deponia_Linux.asm
	// lines 469849, 469854) - checked once per frame; when the count has
	// changed, UpdateCache() is called to react to it. Not reversed beyond
	// that call shape.
	int GetCacheSpriteCount() const;
	void UpdateCache();

	// vtable slot 0x90 in the original; called with a TSpriteHandle* whose
	// refcount just reached zero.
	virtual void OnSpriteHandleReleased(TSpriteHandle *handle);

	// Called from TMasterControl::Draw/Signal/PlayAVI (vtable slots
	// 0x30/0x38/0x178 there); real parameter meaning not recovered.
	virtual void SetMatrixMode(bool a, bool b);
	virtual void ResetMatrix(bool a, bool b);
	virtual void Flip();

private:
	TPreloadedPicManager *_preloadedPicManager = nullptr;
};

extern TGraphicsInterface *graphics;

// Confirmed real globals (TGScene::Draw(), Deponia_Linux.asm lines 166568-
// 166997): the shader every picture draws with when its data record names
// none (id -1), and the callback that activates a given shader for the
// picture about to be drawn. Neither is reversed - the real shader system
// belongs with the (unmodeled) GL backend behind `graphics`.
extern int defaultShader;
void ShaderCallback(int shader, TVisObjRef *ref);
