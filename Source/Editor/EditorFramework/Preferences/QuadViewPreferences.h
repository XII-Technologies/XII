#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/Preferences/Preferences.h>

struct XII_EDITORFRAMEWORK_DLL xiiEngineViewPreferences
{
  xiiVec3                       m_vCamPos         = xiiVec3::ZeroVector();
  xiiVec3                       m_vCamDir         = xiiVec3::UnitXAxis();
  xiiVec3                       m_vCamUp          = xiiVec3::UnitZAxis();
  xiiSceneViewPerspective::Enum m_PerspectiveMode = xiiSceneViewPerspective::Perspective;
  xiiViewRenderMode::Enum       m_RenderMode      = xiiViewRenderMode::Default;
  float                         m_fFov            = 70.0f;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_EDITORFRAMEWORK_DLL, xiiEngineViewPreferences);

class XII_EDITORFRAMEWORK_DLL xiiQuadViewPreferencesUser : public xiiPreferences
{
  XII_ADD_DYNAMIC_REFLECTION(xiiQuadViewPreferencesUser, xiiPreferences);

public:
  xiiQuadViewPreferencesUser();

  bool                     m_bQuadView;
  xiiEngineViewPreferences m_ViewSingle;
  xiiEngineViewPreferences m_ViewQuad0;
  xiiEngineViewPreferences m_ViewQuad1;
  xiiEngineViewPreferences m_ViewQuad2;
  xiiEngineViewPreferences m_ViewQuad3;

  xiiUInt32                FavCams_GetCount() const { return 10; }
  xiiEngineViewPreferences FavCams_GetCam(xiiUInt32 i) const { return m_FavoriteCamera[i]; }
  void                     FavCams_SetCam(xiiUInt32 i, xiiEngineViewPreferences cam) { m_FavoriteCamera[i] = cam; }
  void                     FavCams_Insert(xiiUInt32 uiIndex, xiiEngineViewPreferences cam) {}
  void                     FavCams_Remove(xiiUInt32 uiIndex) {}

  xiiEngineViewPreferences m_FavoriteCamera[10];
};
