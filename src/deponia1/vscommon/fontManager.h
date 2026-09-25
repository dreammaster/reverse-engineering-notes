// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/vscommon/fontManager.cpp - see manifest/source_layout.tsv.
#pragma once

#include "WxStub.h"
#include "datastruct/visobjref.h"

enum class TextAlignmentEnum { Left, Center, Right };
class GLCharBuffer;

class TFontManager {
public:
    void SetCurrentFont(const TVisObjRef& font);
    void GetTextDimension(const wxString& text, wxPoint& outSize) const;
    void PrintText(const wxString& text, TextAlignmentEnum alignment, const wxPoint& pos, float scale,
                   GLCharBuffer* buffer = nullptr);
};
