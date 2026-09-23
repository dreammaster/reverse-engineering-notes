#include "TStandardPaths.h"

#include "TComposedFileManager.h"

bool TStandardPaths::s_isEditorApp = false;
bool TStandardPaths::s_useLocalDir = false;
wxString TStandardPaths::s_companyName;
wxString TStandardPaths::s_gameName;
wxString TStandardPaths::s_projectName;

TStandardPaths::TStandardPaths() : m_wxPaths(&wxGetAppTraitsStandardPaths()) {
}

TStandardPaths::~TStandardPaths() = default;

void TStandardPaths::SetEditor(bool isEditor) {
    s_isEditorApp = isEditor;
}

void TStandardPaths::SetUseLocalDir(bool useLocalDir) {
    s_useLocalDir = useLocalDir;
}

void TStandardPaths::InitGameAndCompanyName(const wxString& companyName, const wxString& gameName,
                                             const wxString& projectName) {
    s_companyName = companyName;
    s_gameName = gameName;
    s_projectName = projectName;
}

std::wstring TStandardPaths::BuildDir(const wchar_t* editorSuffix, const wchar_t* fallbackPrefix,
                                       const wchar_t* cannotCreateMsg) const {
    std::wstring dir = (s_useLocalDir ? m_wxPaths->GetUserLocalDataDir() : m_wxPaths->GetUserDataDir())
                            .ToStdWstring();

    if (s_isEditorApp) {
        dir += editorSuffix;
    } else if (s_companyName.IsEmpty() && s_gameName.IsEmpty()) {
        // Neither company nor game name configured: fall back to a fixed
        // "VisionaireStudio/<projectName>/" subfolder.
        dir += fallbackPrefix;
        dir += s_projectName.ToStdWstring();
        dir += L"/";
    } else {
        if (!s_companyName.IsEmpty())
            dir += s_companyName.ToStdWstring() + L"/";
        if (!s_gameName.IsEmpty())
            dir += s_gameName.ToStdWstring() + L"/";
    }

    if (!wxDir::Exists(wxString(dir))) {
        if (!wxFileName::Mkdir(wxString(dir), 0777, 0) && wxLog::loglevel >= 0) {
            wxLog::logexpanded(cannotCreateMsg, dir.c_str());
        }
    }
    return dir;
}

std::wstring TStandardPaths::GetLogFileDir() const {
    return BuildDir(L"V/", L"VisionaireStudio/", L"cannot create directory (%s) for logfile");
}

std::wstring TStandardPaths::GetConfigDir() const {
    return BuildDir(L"/Visionaire Editor", L"VisionaireStudio/", L"cannot create directory (%s) for config");
}

std::wstring TStandardPaths::GetSavegamePath() const {
    std::wstring dir = GetConfigDir();
    dir += L"Savegames";
    return dir;
}

wxString TStandardPaths::GetTempDir() const {
    return m_wxPaths->GetTempDir();
}

wxString TStandardPaths::GetExecutablePath() const {
    return m_wxPaths->GetExecutablePath();
}

wxString TStandardPaths::GetUserDataDir() const {
    return m_wxPaths->GetUserDataDir();
}

wxString TStandardPaths::GetResourcesDir(bool checkCompiled) const {
    if (checkCompiled) {
        // Return value is discarded in the original disassembly too - both
        // branches end up returning wxFileName::GetCwd() regardless.
        TComposedFileManager::IsGameCompiled();
    }
    return wxFileName::GetCwd();
}
