#include "vscommon/fontManager.h"

void TFontManager::SetCurrentFont(const TVisObjRef& /*font*/) {
}

void TFontManager::GetTextDimension(const wxString& /*text*/, wxPoint& outSize) const {
    outSize = wxPoint();
}

void TFontManager::PrintText(const wxString& /*text*/, TextAlignmentEnum /*alignment*/, const wxPoint& /*pos*/,
                              float /*scale*/, GLCharBuffer* /*buffer*/) {
}
