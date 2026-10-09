// Not yet assert-confirmed to a specific file; stays at the top level.
// A single loaded font, owned by TFontManager (manifest: todo, 16 methods,
// Deponia_Linux.asm lines 1391446-1397049 - not reversed at all yet). Only
// the call shapes TFontManager::Initialize()/GetFont()/SetCurrentFont() and
// its various delegating accessors need are declared here, all confirmed
// from TFontManager's own (fully reversed) disassembly rather than TCFont's
// own - see fontManager.cpp.
#pragma once

#include <list>
#include <vector>

#include "WxStub.h"
#include "datastruct/visobjref.h"
#include "vscommon/fontManager.h"

class TFontManager;

// Confirmed call shape only (TGText, Deponia_Linux.asm lines 213821-213827): the glyph buffer
// of a printed text, kept by the text that printed it and deleted by it. Not reversed.
class GLCharBuffer {
public:
	~GLCharBuffer();
};

class TCFont {
public:
	/** Set while a text is printed that is not tied to a character (TGText::Draw()): the text
	 *  is zoomed with the scene. */
	static bool ZoomText;

	TCFont(TVisObjRef &font, TFontManager *manager);

	/** Called every frame by ShowFrame (asm 497798): makes the Freetype font again when it has to be (TODO: the
	 *  Freetype fonts, TFreetypeFont, are not reconstructed, nothing is done). */
	void CheckFreetypeFont();

	int GetLineHeight() const;
	void GetTextDimension(const wxString &text, wxPoint &outSize) const;
	void GetTextDimension(const std::list<wxString> &lines, wxPoint &outSize) const;
	void SplitIntoLines(const wxString &text, std::list<wxString> &outLines, int maxWidth);
	void PerformAutoLineBreak(const wxString &text, std::list<wxString> &outLines);
	void PrintText(const wxString &text, TextAlignmentEnum alignment, const wxPoint &pos, float scale, int a,
	               GLCharBuffer *buffer);
	void PrintTextLines(const std::list<wxString> &lines, const std::vector<int> &lineWidths,
	                    TextAlignmentEnum alignment, const wxPoint &pos, float scale, int a, bool b,
	                    std::vector<GLCharBuffer *> *buffers);
};
