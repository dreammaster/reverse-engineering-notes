#include "TComposedFileManager.h"

#include "baselib/composedfile.h"

bool TComposedFileManager::IsGameCompiled() {
	return false;
}

bool TComposedFileManager::InitMainContainer(const wxFileName &/*file*/, const wxString &/*password*/) {
	return false;
}

TComposedFile *TComposedFileManager::GetMainContainer() {
	static TComposedFile file;
	return &file;
}

void TComposedFileManager::Init(const wxFileName &/*file*/, const wxString &/*password*/,
                                const std::vector<TCharHolder> &/*strings*/) {
}

void TComposedFileManager::Init(const wxFileName &/*file*/, const wxString &/*password*/, int /*size1*/,
                                const wxFileName &/*path1*/, int /*size2*/, const wxFileName &/*path2*/,
                                int /*size3*/, const wxFileName &/*path3*/, int /*size4*/,
                                const wxFileName &/*path4*/, int /*size5*/, const wxFileName &/*path5*/) {
}
