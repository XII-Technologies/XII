#pragma once

#include <EditorFramework/Preferences/Preferences.h>

class XII_EDITORFRAMEWORK_DLL xiiScenePreferencesUser : public xiiPreferences
{
  XII_ADD_DYNAMIC_REFLECTION(xiiScenePreferencesUser, xiiPreferences);

public:
  xiiScenePreferencesUser();

  void     SetCameraSpeed(xiiInt32 value);
  xiiInt32 GetCameraSpeed() const { return m_iCameraSpeed; }

  void SetShowGrid(bool bShow);
  bool GetShowGrid() const { return m_bShowGrid; }

protected:
  bool m_bShowGrid;
  int  m_iCameraSpeed;
};
