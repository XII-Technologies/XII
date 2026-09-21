/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Strings/String.h>

/// Stores project specific preferences for the current user
class XII_EDITORFRAMEWORK_DLL xiiProjectPreferencesUser : public xiiPreferences
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProjectPreferencesUser, xiiPreferences);

public:
  xiiProjectPreferencesUser();

  // which apps to launch as external 'Players' (other than xiiPlayer.exe)
  xiiDynamicArray<xiiString> m_PlayerApps;

  // the directory where the project should be exported to
  xiiString m_sExportFolder;
};
