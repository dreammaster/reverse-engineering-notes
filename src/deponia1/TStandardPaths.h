// Reconstructed from Deponia_Linux.asm, TStandardPaths methods at asm lines
// 552187-553394 (address range 0x651240-0x651E40). See NOTES.md for the
// reasoning behind non-obvious pieces.
//
// TStandardPaths is Visionnaire's own wrapper around wxWidgets'
// wxStandardPathsBase: it holds a pointer to the real wx object (m_wxPaths)
// and either forwards to it directly (GetTempDir, GetExecutablePath,
// GetUserDataDir) or builds on top of it with company/game/project-name
// subfolders and editor-mode variants (GetLogFileDir, GetConfigDir).
#pragma once

#include <string>

#include "WxStub.h"

class TStandardPaths {
public:
    TStandardPaths();
    virtual ~TStandardPaths();

    static void SetEditor(bool isEditor);
    static void SetUseLocalDir(bool useLocalDir);
    static void InitGameAndCompanyName(const wxString& companyName, const wxString& gameName,
                                        const wxString& projectName);

    // These build a path from scratch (wx dir + company/game/project name
    // subfolders) and create the directory if it doesn't exist yet, so they
    // return std::wstring rather than forwarding a wxString from wx.
    std::wstring GetLogFileDir() const;
    std::wstring GetConfigDir() const;

    // Appends "Savegames" straight onto GetConfigDir() with no separator.
    // In non-editor mode GetConfigDir() always ends in "/" so this is fine;
    // in editor mode it doesn't (its editor-suffix literal is
    // "/Visionaire Editor", no trailing slash), which would concatenate
    // into ".../Visionaire EditorSavegames" - a real quirk in the original,
    // reproduced as-is rather than "fixed", and presumably harmless since
    // savegames are a player-only concept the editor wouldn't call this
    // for.
    std::wstring GetSavegamePath() const;

    // Thin forwarders to the underlying wxStandardPathsBase.
    wxString GetTempDir() const;
    wxString GetExecutablePath() const;
    wxString GetUserDataDir() const;

    // `checkCompiled` argument's return value is discarded in the original
    // (TComposedFileManager::IsGameCompiled() is called but not tested) -
    // both branches return wxFileName::GetCwd(). Reproduced as observed
    // rather than guessing what it was meant to do.
    wxString GetResourcesDir(bool checkCompiled) const;

private:
    // GetLogFileDir and GetConfigDir are ~95% identical in the original
    // (same wx-dir/company/game/project-name/editor-mode logic, differing
    // only in three literals); factored into one helper rather than
    // reproducing that duplication.
    std::wstring BuildDir(const wchar_t* editorSuffix, const wchar_t* fallbackPrefix,
                          const wchar_t* cannotCreateMsg) const;

    wxStandardPathsBase* m_wxPaths;

    static bool s_isEditorApp;
    static bool s_useLocalDir;
    static wxString s_companyName;
    static wxString s_gameName;
    static wxString s_projectName;
};
