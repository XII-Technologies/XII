#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Preferences/ScenePreferences.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScenePreferencesUser, 1, xiiRTTIDefaultAllocator<xiiScenePreferencesUser>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ShowGrid", m_bShowGrid),
    XII_MEMBER_PROPERTY("CameraSpeed", m_iCameraSpeed)->AddAttributes(new xiiDefaultValueAttribute(10), new xiiClampValueAttribute(1, 30)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiScenePreferencesUser::xiiScenePreferencesUser() :
  xiiPreferences(Domain::Document, "Scene")
{
  m_iCameraSpeed = 9;
}

void xiiScenePreferencesUser::SetCameraSpeed(xiiInt32 value)
{
  m_iCameraSpeed = xiiMath::Clamp(value, 0, 24);

  TriggerPreferencesChangedEvent();
}

void xiiScenePreferencesUser::SetShowGrid(bool bShow)
{
  m_bShowGrid = bShow;

  TriggerPreferencesChangedEvent();
}
