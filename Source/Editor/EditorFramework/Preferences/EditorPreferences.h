/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Strings/String.h>

class xiiEngineViewLightSettings;

/// Stores editor specific preferences for the current user
class XII_EDITORFRAMEWORK_DLL xiiEditorPreferencesUser : public xiiPreferences
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditorPreferencesUser, xiiPreferences);

public:
  xiiEditorPreferencesUser();
  ~xiiEditorPreferencesUser();

  void ApplyDefaultValues(xiiEngineViewLightSettings& ref_settings);
  void SetAsDefaultValues(const xiiEngineViewLightSettings& settings);

  float     m_fPerspectiveFieldOfView = 70.0f;
  xiiAngle  m_RotationSnapValue       = xiiAngle::MakeFromDegree(15.0f);
  float     m_fScaleSnapValue         = 0.125f;
  float     m_fTranslationSnapValue   = 0.25f;
  bool      m_bUsePrecompiledTools    = true;
  xiiString m_sCustomPrecompiledToolsFolder;
  bool      m_bLoadLastProjectAtStartup          = true;
  bool      m_bShowSplashScreen                  = true;
  bool      m_bExpandSceneTreeOnSelection        = true;
  bool      m_bBackgroundAssetProcessing         = false;
  bool      m_bHighlightUntranslatedUI           = false;
  bool      m_bAssetBrowserShowItemsInSubFolders = true;

  // Auto-save interval in minutes. 0 = off.
  xiiUInt32 m_uiAutoSaveMinutes = 5;

  bool      m_bSkyBox                      = true;
  bool      m_bSkyLight                    = true;
  xiiString m_sSkyLightCubeMap             = "{ 0b202e08-a64f-465d-b38e-15b81d161822 }";
  float     m_fSkyLightIntensity           = 1.0f;
  bool      m_bDirectionalLight            = true;
  xiiAngle  m_DirectionalLightAngle        = xiiAngle::MakeFromDegree(70.0f);
  bool      m_bDirectionalLightShadows     = false;
  float     m_fDirectionalLightIntensity   = 10.0f;
  bool      m_bFog                         = false;
  bool      m_bClearEditorLogsOnPlay       = true;
  bool      m_bCombinedEditorAndEngineLogs = true;

  void SetShowInDevelopmentFeatures(bool b);
  bool GetShowInDevelopmentFeatures() const
  {
    return m_bShowInDevelopmentFeatures;
  }

  void SetHighlightUntranslatedUI(bool b);
  bool GetHighlightUntranslatedUI() const
  {
    return m_bHighlightUntranslatedUI;
  }

  void  SetGizmoSize(float f);
  float GetGizmoSize() const { return m_fGizmoSize; }

  void      SetMaxFramerate(xiiUInt16 uiFPS);
  xiiUInt16 GetMaxFramerate() const { return m_uiMaxFramerate; }

  void      SetMaxFramerateWhenUnfocused(xiiUInt16 uiFPS);
  xiiUInt16 GetMaxFramerateWhenUnfocused() const { return m_uiMaxFramerateWhenUnfocused; }

  xiiHybridArray<xiiString, 8> m_RecentlyCreatedTypes;

private:
  void SyncGlobalSettings();

  float     m_fGizmoSize                  = 1.5f;
  bool      m_bShowInDevelopmentFeatures  = false;
  xiiUInt16 m_uiMaxFramerate              = 60;
  xiiUInt16 m_uiMaxFramerateWhenUnfocused = 15U;
};
