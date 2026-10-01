#include "TComposedFileManager.h"

#include <cstring>

#include "baselib/file.h"
#include "baselib/memfile.h"

bool TComposedFileManager::s_bCompiledGame = false;
std::vector<TComposedFile> TComposedFileManager::s_manualContainers;
TComposedFile TComposedFileManager::s_mainContainer;
TComposedFile TComposedFileManager::s_savegameContainer;
TComposedFile TComposedFileManager::s_gameContainer;
std::vector<TComposedFile> TComposedFileManager::s_sceneContainers;
std::vector<TComposedFile> TComposedFileManager::s_characterContainers;
std::vector<TComposedFile> TComposedFileManager::s_interfaceContainers;
std::wstring TComposedFileManager::s_movieComposedFile;

bool TComposedFileManager::IsGameCompiled() {
	return s_bCompiledGame;
}

void TComposedFileManager::Init(const wxFileName &/*file*/, const wxString &password,
                                const std::vector<TCharHolder> &strings) {
	if (!s_bCompiledGame)
		return;

	s_manualContainers.clear();
	s_manualContainers.resize(strings.size());
	for (std::size_t i = 0; i < strings.size(); i++) {
		wxFileName path = strings[i];
		s_manualContainers[i].Init(path, password, -2);
	}
}

void TComposedFileManager::Init(const wxFileName &/*file*/, const wxString &password, int size1,
                                const wxFileName &path1, int size2, const wxFileName &path2, int size3,
                                const wxFileName &path3, int size4, const wxFileName &path4, int size5,
                                const wxFileName &path5) {
	if (!s_bCompiledGame)
		return;

	if (size1 > 0)
		s_gameContainer.Init(path1, password, -1);

	s_sceneContainers.clear();
	s_sceneContainers.resize(size2 > 0 ? static_cast<std::size_t>(size2) : 0);
	if (s_sceneContainers.size() == 1) {
		s_sceneContainers[0].Init(path2, password, -1);
	} else {
		for (std::size_t i = 0; i < s_sceneContainers.size(); i++)
			s_sceneContainers[i].Init(path2, password, static_cast<long>(i));
	}

	s_characterContainers.clear();
	s_characterContainers.resize(size3 > 0 ? static_cast<std::size_t>(size3) : 0);
	if (s_characterContainers.size() == 1) {
		s_characterContainers[0].Init(path3, password, -1);
	} else {
		for (std::size_t i = 0; i < s_characterContainers.size(); i++)
			s_characterContainers[i].Init(path3, password, static_cast<long>(i));
	}

	s_interfaceContainers.clear();
	s_interfaceContainers.resize(size4 > 0 ? static_cast<std::size_t>(size4) : 0);
	if (s_interfaceContainers.size() == 1) {
		s_interfaceContainers[0].Init(path4, password, -1);
	} else {
		for (std::size_t i = 0; i < s_interfaceContainers.size(); i++)
			s_interfaceContainers[i].Init(path4, password, static_cast<long>(i));
	}

	if (size5 > 0)
		s_movieComposedFile = path5.GetFullPath().ToStdWstring();
}

void TComposedFileManager::CleanUp() {
	s_sceneContainers.clear();
	s_characterContainers.clear();
	s_interfaceContainers.clear();
}

TComposedFile *TComposedFileManager::GetMainContainer() {
	return &s_mainContainer;
}

bool TComposedFileManager::InitMainContainer(const wxFileName &file, const wxString &password) {
	s_bCompiledGame = IsComposedFile(file);
	return s_mainContainer.Init(file, password, -1);
}

bool TComposedFileManager::SetSavegameFile(const wxFileName &file, const wxString &name) {
	return s_savegameContainer.Init(file, name, -1);
}

bool TComposedFileManager::IsComposedFile(const wxFileName &file) {
	// Real signature: TFile::OpenReadFromComposedFile(file, "", 0, 4) - this
	// call site always passes offset 0, which is equivalent to a plain open.
	TFile reader;
	if (!reader.OpenRead(file))
		return false;

	TMemoryBuffer buffer;
	reader.ReadToBuf(buffer, 4);
	bool isComposed = buffer.GetLen() == 4 && std::memcmp(buffer.GetData(), "VIS3", 4) == 0;
	reader.Close();
	return isComposed;
}

bool TComposedFileManager::GetComposedFileInfo(const wxFileName &/*file*/, TComposedFileInfo &/*outInfo*/) {
	// Not reversed to the byte level - see this method's own header comment.
	return false;
}

TComposedFile *TComposedFileManager::GetComposedFile(const TComposedFileInfo &info) {
	switch (info.containerType) {
	case TContainerTypeEnum::kType0:
		return &s_mainContainer;
	case TContainerTypeEnum::kType1:
		if (info.index == -1)
			return s_sceneContainers.size() == 1 ? &s_sceneContainers[0] : nullptr;
		if (info.index < 0 || static_cast<std::size_t>(info.index) >= s_sceneContainers.size())
			return nullptr;
		return &s_sceneContainers[static_cast<std::size_t>(info.index)];
	case TContainerTypeEnum::kType2:
		if (info.index == -1)
			return s_characterContainers.size() == 1 ? &s_characterContainers[0] : nullptr;
		if (info.index < 0 || static_cast<std::size_t>(info.index) >= s_characterContainers.size())
			return nullptr;
		return &s_characterContainers[static_cast<std::size_t>(info.index)];
	case TContainerTypeEnum::kType4:
		return &s_savegameContainer;
	case TContainerTypeEnum::kType5:
		if (info.index == -1)
			return s_interfaceContainers.size() == 1 ? &s_interfaceContainers[0] : nullptr;
		if (info.index < 0 || static_cast<std::size_t>(info.index) >= s_interfaceContainers.size())
			return nullptr;
		return &s_interfaceContainers[static_cast<std::size_t>(info.index)];
	case TContainerTypeEnum::kType6:
		return &s_gameContainer;
	case TContainerTypeEnum::kEmbeddedInExecutable:
	case TContainerTypeEnum::kEmbeddedInExecutableAlt:
		if (s_manualContainers.empty() || info.index <= 0 ||
		        static_cast<std::size_t>(info.index) > s_manualContainers.size())
			return nullptr;
		return &s_manualContainers[static_cast<std::size_t>(info.index) - 1];
	default:
		return nullptr;
	}
}

bool TComposedFileManager::GetMemoryFile(TMemoryFile &outFile, const wxFileName &file) {
	TComposedFileInfo info;
	if (GetComposedFileInfo(file, info)) {
		TComposedFile *composedFile = GetComposedFile(info);
		if (!composedFile) {
			if (wxLog::loglevel >= 0) {
				wxString fmt;
				toUTF(&fmt, "C");
				wxLog::logexpanded(fmt.wc_str(), file.GetFullPath().wc_str());
			}
			return false;
		}
		bool ok = composedFile->GetMemoryFile(outFile, file, info.index);
		if (!ok && wxLog::loglevel >= 0) {
			wxString fmt;
			toUTF(&fmt, "Could not open file '%s' in container '%s'");
			wxLog::logexpanded(fmt.wc_str(), composedFile->GetComposedFilePath().c_str(),
			                   file.GetFullPath().wc_str());
		}
		return ok;
	}

	wxFile reader;
	if (!reader.Open(file.GetFullPath(), 1))
		return false;
	long length = reader.Length();
	if (!outFile.Reserve(length)) {
		reader.Close();
		return false;
	}
	unsigned long got = reader.Read(reinterpret_cast<char *>(outFile.GetBuffer()), static_cast<unsigned long>(length));
	reader.Close();
	return got == static_cast<unsigned long>(length);
}

bool TComposedFileManager::Open(TFile &outFile, const wxFileName &file) {
	TComposedFileInfo info;
	if (GetComposedFileInfo(file, info)) {
		TComposedFile *composedFile = GetComposedFile(info);
		if (!composedFile)
			return false;
		return composedFile->Open(outFile, file, info.index);
	}

	if (!file.IsOk())
		return false;
	outFile.SetOriginalPath(file);
	return outFile.OpenRead(file);
}

bool TComposedFileManager::Export(const wxFileName &file, const wxFileName &/*outPath*/, wxFileName &outName) {
	TComposedFileInfo info;
	if (!GetComposedFileInfo(file, info))
		return false;
	TComposedFile *composedFile = GetComposedFile(info);
	if (!composedFile)
		return false;
	return composedFile->Export(info.index, outName, file.GetExt(), file.GetName());
}

bool TComposedFileManager::DecryptComposedFile(const wxFileName &file, wxFileName &outFile,
        const wxString &password) {
	TComposedFileInfo info;
	if (!GetComposedFileInfo(file, info) || info.containerType != TContainerTypeEnum::kType3 || info.index < 0)
		return false;

	outFile.Assign(wxString(s_movieComposedFile));
	wxString ext;
	toUTF(&ext, "v");
	outFile.SetExt(wxString(ext.ToStdWstring() + std::to_wstring(info.index)));
	return TFile().DecryptHeader(outFile, password);
}

bool TComposedFileManager::EncryptComposedFile(const wxFileName &file, const wxString &password) {
	wxFileName outFile;
	return DecryptComposedFile(file, outFile, password);
}

bool TComposedFileManager::GetComposedMovieFileName(const wxFileName &file, wxFileName &outName) {
	TComposedFileInfo info;
	if (!GetComposedFileInfo(file, info))
		return false;

	if (info.containerType == TContainerTypeEnum::kEmbeddedInExecutableAlt) {
		if (info.index <= 0 || static_cast<std::size_t>(info.index) > s_manualContainers.size())
			return false;
		outName.Assign(wxString(s_manualContainers[static_cast<std::size_t>(info.index) - 1].GetComposedFilePath()));
		return true;
	}
	if (info.containerType == TContainerTypeEnum::kType3) {
		if (info.index < 0)
			return false;
		outName.Assign(wxString(s_movieComposedFile));
		wxString ext;
		toUTF(&ext, "v");
		outName.SetExt(wxString(ext.ToStdWstring() + std::to_wstring(info.index)));
		return true;
	}
	return false;
}

bool TComposedFileManager::FileExists(const wxFileName &file) {
	if (!s_bCompiledGame)
		return file.Exists();
	wxString hash;
	toUTF(&hash, "#");
	return file.GetFullPath().Find(hash) != -1;
}
