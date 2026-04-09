#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)

class xiiRecentFilesList;

/// Helper class for managing Windows taskbar jump lists
class XII_EDITORFRAMEWORK_DLL xiiWindowsJumpList
{
public:
  /// Updates the Windows taskbar jump list with recent projects
  static void UpdateJumpList(const xiiRecentFilesList& recentProjects);
};

#endif
