#include "TTempFile.h"

void TTempFile::DeleteTempFiles() {
}

wxString TTempFile::AddTempFile(const wxString &name, const wxString &ext) {
	return name + wxString(L".") + ext;
}
