/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Settings/EditorSettings.h>
#include <Foundation/Strings/String.h>

/// Stores project specific preferences for the current user
class XII_EDITORFRAMEWORK_DLL xiiProjectPreferencesUser : public xiiEditorSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProjectPreferencesUser, xiiEditorSettings);

public:
  xiiProjectPreferencesUser();

  // which apps to launch as external 'Players' (other than xiiPlayer.exe)
  xiiDynamicArray<xiiString> m_PlayerApps;

  // the directory where the project should be exported to
  xiiString m_sExportFolder;
};
