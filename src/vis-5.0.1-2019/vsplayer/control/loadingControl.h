// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/vsplayer/control/loadingControl.cpp - see manifest/source_layout.tsv.
//
// Confirmed layout (ctor/dtor/Draw/UpdateStatus/EndLoading, Deponia_Linux.asm
// lines 483185-483771): two embedded TPictureIO objects (a background image,
// drawn full-surface, and a progress-bar image, drawn through a moving/
// resizing source rect), a pair of wxRect fields (the progress bar's full
// bounds and its currently-filled portion), a wxPoint (an alternate mode's
// base position), a bool (which of two progress-fill visual modes is
// active), and a wxFileName (a loading-sound path, played once in
// EndLoading() if never played during loading).
//
// Init() itself is still NOT reversed: its disassembly calls TSprite::
// GetPath()/GetWidth()/GetHeight()/SetPosition() directly on its
// SLoadingScreen& parameter's image fields. TSprite has since been fully
// reversed (see TSprite.h) and its real offsets (_path @0x30, _imageWidth/
// _imageHeight @0x18/0x1C) now match what Init()'s disassembly showed all
// along - the earlier-suspected "contradiction" with this project's TSprite
// model is resolved, TSprite's model was simply wrong before. What's still
// unresolved is SLoadingScreen's own structure: TMasterControl::
// SetLoadingScreen's memberwise copy suggests each SLoadingScreen image
// field is 16 bytes wide, and Init() calling TSprite's own methods directly
// on them suggests they may be full TSprite objects (or at least
// TSprite-compatible) rather than bare TCharHolder fields - but SLoadingScreen
// and Init() haven't had their own dedicated pass yet, so this stays an
// honest gap rather than guessed at.
#pragma once

#include "TPaintControl.h"
#include "TSoundInterface.h"
#include "WxStub.h"
#include "graphicslib/picture.h"

struct SLoadingScreen;

class TLoadingControl : public TPaintControl {
public:
	// Confirmed call shape only (TMasterControl::ShowLoadingScreen,
	// Deponia_Linux.asm line 488349) - not reversed, see the class comment
	// above.
	void Init(SLoadingScreen &screen, TSoundInterface *soundManager);
	void UpdateStatus(int current, int total);
	// Confirmed call shape only (TGameControl::LoadAndInitGame,
	// Deponia_Linux.asm lines 468227-468230) - not reversed beyond that.
	void EndLoading(TSoundInterface *soundManager);
	void Draw() override;

private:
	// Confirmed present (asm lines 483191-483200, 483670-483704): the "full"
	// bounds of the progress bar area, and the sub-rect (within the same
	// picture) currently drawn to represent progress - resized each
	// UpdateStatus() call. Real names not recovered.
	wxRect _totalRect;
	wxRect _fillRect;
	// Confirmed present (asm lines 483691-483699): a base position used by
	// one of the two progress-fill visual modes below; real purpose beyond
	// that isn't resolved.
	wxPoint _fillOrigin;
	// Confirmed present (asm line 483668, set from SLoadingScreen's own
	// fieldA0 in the still-unreversed Init()): picks between two progress-
	// bar visual modes - sliding a mask over a static image (false) vs.
	// growing a filled rect from _fillOrigin (true). Real meaning/name not
	// resolved.
	bool _progressFillsForward = false;
	// Confirmed embedded TPictureIO objects, not pointers (ctor calls
	// TPictureIO::TPictureIO() directly on `this`-relative addresses with no
	// null-pointer branch, dtor likewise) - the progress bar image (drawn
	// via DrawWithSrcRect(_fillRect, ...) in Draw()) and the background
	// image (drawn full-surface).
	TPictureIO _progressBarPic;
	TPictureIO _backgroundPic;
	// Confirmed present (EndLoading's wxFileName::IsOk() check, Init()'s own
	// wxFileName::IsOk() and std::wstring::assign() calls) - the loading
	// sound's path, played once in EndLoading() if Init() never managed to
	// play it directly (see that method's own not-yet-reversed comment).
	wxFileName _soundPath;
};
