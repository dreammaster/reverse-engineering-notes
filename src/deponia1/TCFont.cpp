#include "TCFont.h"

TCFont::TCFont(TVisObjRef &/*font*/, TFontManager */*manager*/) {
}

int TCFont::GetLineHeight() const {
	return 0;
}

void TCFont::GetTextDimension(const wxString &/*text*/, wxPoint &outSize) const {
	outSize = wxPoint();
}

void TCFont::GetTextDimension(const std::list<wxString> &/*lines*/, wxPoint &outSize) const {
	outSize = wxPoint();
}

void TCFont::SplitIntoLines(const wxString &/*text*/, std::list<wxString> &/*outLines*/, int /*maxWidth*/) {
}

void TCFont::PerformAutoLineBreak(const wxString &/*text*/, std::list<wxString> &/*outLines*/) {
}

void TCFont::PrintText(const wxString &/*text*/, TextAlignmentEnum /*alignment*/, const wxPoint &/*pos*/,
                       float /*scale*/, int /*a*/, GLCharBuffer */*buffer*/) {
}

void TCFont::PrintTextLines(const std::list<wxString> &/*lines*/, const std::vector<int> &/*lineWidths*/,
                            TextAlignmentEnum /*alignment*/, const wxPoint &/*pos*/, float /*scale*/, int /*a*/,
                            bool /*b*/, std::vector<GLCharBuffer *> */*buffers*/) {
}
