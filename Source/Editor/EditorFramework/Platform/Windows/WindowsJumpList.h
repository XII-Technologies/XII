/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Basics/Platform/Windows/MinWindows.h>
#  include <QString>

class xiiRecentFilesList;
struct ICustomDestinationList;
struct IShellLinkW;

/// Native Shell presentation of the Editor's authoritative project history.
/// Update requires the taskbar controller's COM apartment and the Qt GUI thread.
class XII_EDITORFRAMEWORK_DLL xiiWindowsJumpList
{
public:
  static constexpr const wchar_t* GetAppId() { return L"XII.Technologies.Editor"; }

  /// Call once from the Editor executable, before Qt or any application UI is created.
  static xiiResult ConfigureAppId();

  /// Removed destinations are pruned only after CommitList succeeds. The owner must persist changed history.
  static xiiResult Update(xiiRecentFilesList& recentProjects, bool& out_bHistoryChanged);

private:
  friend struct xiiWindowsJumpListTest;

  static QString                BuildProjectArguments(const QString& sProject);
  static xiiMinWindows::HRESULT CreateLink(const QString& sExecutable, const QString& sArguments, const QString& sTitle, IShellLinkW*& out_pLink);
  static xiiMinWindows::HRESULT BuildList(ICustomDestinationList& destinations, const QString& sExecutable, xiiRecentFilesList& recentProjects, bool& out_bHistoryChanged);
};

#endif
