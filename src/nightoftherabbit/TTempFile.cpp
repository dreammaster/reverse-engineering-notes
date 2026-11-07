#include "TTempFile.h"

#include "TStandardPaths.h"

std::vector<std::wstring> TTempFile::s_tempFiles;

// Confirmed (asm lines 553545-553700). The original concatenates the temp
// directory straight onto the name; wxWidgets' GetTempDir() has no trailing
// separator, so one is added here when it's missing (the real binary's
// files evidently land inside the temp directory, not beside it).
wxFileName TTempFile::AddTempFile(const wxString &name, const wxString &ext) {
	TStandardPaths paths;
	std::wstring path = paths.GetTempDir().ToStdWstring();
	if (!path.empty() && path.back() != L'/' && path.back() != L'\\')
		path += L'/';
	path += name.ToStdWstring();
	path += ext.ToStdWstring();

	wxFileName file(path);
	file.NormalizePath();
	s_tempFiles.push_back(file.GetFullPath().ToStdWstring());
	return file;
}

// Confirmed (asm lines 553440-553545).
void TTempFile::DeleteTempFiles() {
	for (const std::wstring &path : s_tempFiles)
		wxRemoveFile(wxString(path));
	s_tempFiles.clear();
}
