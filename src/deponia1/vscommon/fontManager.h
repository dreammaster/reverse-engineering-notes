// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/vscommon/fontManager.cpp - see manifest/source_layout.tsv.
#pragma once

#include "WxStub.h"
#include "datastruct/visobjref.h"
#include "datastruct/vlist.h"

enum class TextAlignmentEnum { Left, Center, Right };
class GLCharBuffer;

class TFontManager {
public:
    // Confirmed called with a list of font objects fetched from
    // TVisionaire::GetList (TGameControl::InitFonts, asm lines 458293-458322)
    // - not reversed beyond that call shape.
    void Initialize(TVList& fonts);
    void SetCurrentFont(const TVisObjRef& font);
    void GetTextDimension(const wxString& text, wxPoint& outSize) const;
    void PrintText(const wxString& text, TextAlignmentEnum alignment, const wxPoint& pos, float scale,
                   GLCharBuffer* buffer = nullptr);
};
