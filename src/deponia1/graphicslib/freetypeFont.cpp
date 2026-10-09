#include "graphicslib/freetypeFont.h"

TFreetypeFont::TFreetypeFont() {
}

TFreetypeFont::~TFreetypeFont() {
}

// TODO: Freetype and the glyph textures (asm 1582405-1589320) are not reconstructed: no font can be made.
bool TFreetypeFont::CreateFont(const wxString &/*path*/, float /*size*/, int /*color*/, bool /*border*/,
                               float /*borderSize*/, int /*borderColor*/, bool /*shadow*/,
                               const wxPoint &/*shadowOffset*/) {
	return false;
}

TFreetypeFont::Character *TFreetypeFont::GetChar(unsigned int /*code*/) {
	return &_unknown;
}

int TFreetypeFont::StringLength(const std::string &/*utf8*/) {
	return 0;
}

void TFreetypeFont::CheckFillRate() {
}
