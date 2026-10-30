/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Settings/ScenePreferences.h>
#include <Foundation/Serialization/GraphPatch.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScenePreferencesUser, 2, xiiRTTIDefaultAllocator<xiiScenePreferencesUser>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiSettingsScopeAttribute(xiiSettingsScope::Document),
    new xiiDocumentSettingsAttribute("xiiGameObjectDocument"),
    new xiiCategoryAttribute("Scene"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ShowGrid", m_bShowGrid)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("CameraSpeed", m_iCameraSpeed)->AddAttributes(new xiiDefaultValueAttribute(9), new xiiClampValueAttribute(0, 24)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  class xiiScenePreferencesPatch_1_2 : public xiiGraphPatch
  {
  public:
    xiiScenePreferencesPatch_1_2() : xiiGraphPatch("xiiScenePreferencesUser", 2) {}

    void Patch(xiiGraphPatchContext&, xiiAbstractObjectGraph*, xiiAbstractObjectNode* pNode) const override
    {
      if (const auto* pSpeed = pNode->FindProperty("CameraSpeed"))
      {
        xiiResult      result = XII_FAILURE;
        const xiiInt32 iSpeed = pSpeed->m_Value.ConvertTo<xiiInt32>(&result);
        // The old UI allowed 1..30; camera movement and the public setter have always used 0..24.
        if (result.Succeeded() && iSpeed >= 0 && iSpeed <= 30)
          pNode->ChangeProperty("CameraSpeed", xiiMath::Clamp(iSpeed, 0, 24));
      }
    }
  };

  xiiScenePreferencesPatch_1_2 g_ScenePreferencesPatch_1_2;
} // namespace

xiiScenePreferencesUser::xiiScenePreferencesUser() :
  xiiEditorSettings(Domain::Document, "Scene")
{
  m_bShowGrid    = true;
  m_iCameraSpeed = 9;
}

void xiiScenePreferencesUser::SetCameraSpeed(xiiInt32 value)
{
  m_iCameraSpeed = xiiMath::Clamp(value, 0, 24);

  NotifyChanged();
}

void xiiScenePreferencesUser::SetShowGrid(bool bShow)
{
  m_bShowGrid = bShow;

  NotifyChanged();
}
