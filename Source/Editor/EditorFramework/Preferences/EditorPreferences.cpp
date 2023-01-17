#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Profiling/Profiling.h>
#include <GuiFoundation/PropertyGrid/Implementation/AddSubElementButton.moc.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorPreferencesUser, 1, xiiRTTIDefaultAllocator<xiiEditorPreferencesUser>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RestoreProjectOnStartup", m_bLoadLastProjectAtStartup)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("ShowSplashscreen", m_bShowSplashscreen)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("BackgroundAssetProcessing", m_bBackgroundAssetProcessing)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("FieldOfView", m_fPerspectiveFieldOfView)->AddAttributes(new xiiDefaultValueAttribute(70.0f), new xiiClampValueAttribute(10.0f, 150.0f)),
    XII_ACCESSOR_PROPERTY("GizmoSize", GetGizmoSize, SetGizmoSize)->AddAttributes(new xiiDefaultValueAttribute(1.5f), new xiiClampValueAttribute(0.2f, 5.0f)),
    XII_MEMBER_PROPERTY("UseOldGizmos", m_bOldGizmos),
    XII_ACCESSOR_PROPERTY("ShowInDevelopmentFeatures", GetShowInDevelopmentFeatures, SetShowInDevelopmentFeatures),
    XII_MEMBER_PROPERTY("RotationSnap", m_RotationSnapValue)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::Degree(15.0f))),
    XII_MEMBER_PROPERTY("ScaleSnap", m_fScaleSnapValue)->AddAttributes(new xiiDefaultValueAttribute(0.125f)),
    XII_MEMBER_PROPERTY("TranslationSnap", m_fTranslationSnapValue)->AddAttributes(new xiiDefaultValueAttribute(0.25f)),
    XII_MEMBER_PROPERTY("UsePrecompiledTools", m_bUsePrecompiledTools)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("ExpandSceneTreeOnSelection", m_bExpandSceneTreeOnSelection)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("AssetFilterCombobox", m_bAssetFilterCombobox)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("ClearEditorLogsOnPlay", m_bClearEditorLogsOnPlay)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ACCESSOR_PROPERTY("HighlightUntranslatedUI", GetHighlightUntranslatedUI, SetHighlightUntranslatedUI),

    // START GROUP Engine View Light Settings
    XII_MEMBER_PROPERTY("SkyBox", m_bSkyBox)->AddAttributes(new xiiDefaultValueAttribute(true), new xiiGroupAttribute("Engine View Light Settings")),
    XII_MEMBER_PROPERTY("SkyLight", m_bSkyLight)->AddAttributes(new xiiDefaultValueAttribute(true), new xiiClampValueAttribute(0.0f, 2.0f)),
    XII_MEMBER_PROPERTY("SkyLightCubeMap", m_sSkyLightCubeMap)->AddAttributes(new xiiDefaultValueAttribute(xiiStringView("{ 0b202e08-a64f-465d-b38e-15b81d161822 }")), new xiiAssetBrowserAttribute("CompatibleAsset_Texture_Cube")),
    XII_MEMBER_PROPERTY("SkyLightIntensity", m_fSkyLightIntensity)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 20.0f)),
    XII_MEMBER_PROPERTY("DirectionalLight", m_bDirectionalLight)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("DirectionalLightAngle", m_DirectionalLightAngle)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::Degree(30.0f)), new xiiClampValueAttribute(xiiAngle::Degree(-90.0f), xiiAngle::Degree(90.0f))),
    XII_MEMBER_PROPERTY("DirectionalLightShadows", m_bDirectionalLightShadows),
    XII_MEMBER_PROPERTY("DirectionalLightIntensity", m_fDirectionalLightIntensity)->AddAttributes(new xiiDefaultValueAttribute(10.0f)),
    XII_MEMBER_PROPERTY("Fog", m_bFog),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiEditorPreferencesUser::xiiEditorPreferencesUser() :
  xiiPreferences(Domain::Application, "General")
{
}

xiiEditorPreferencesUser::~xiiEditorPreferencesUser() = default;

void xiiEditorPreferencesUser::ApplyDefaultValues(xiiEngineViewLightSettings& settings)
{
  settings.SetSkyBox(m_bSkyBox);
  settings.SetSkyLight(m_bSkyLight);
  settings.SetSkyLightCubeMap(m_sSkyLightCubeMap);
  settings.SetSkyLightIntensity(m_fSkyLightIntensity);
  settings.SetDirectionalLight(m_bDirectionalLight);
  settings.SetDirectionalLightAngle(m_DirectionalLightAngle);
  settings.SetDirectionalLightShadows(m_bDirectionalLightShadows);
  settings.SetDirectionalLightIntensity(m_fDirectionalLightIntensity);
  settings.SetFog(m_bFog);
}

void xiiEditorPreferencesUser::SetAsDefaultValues(const xiiEngineViewLightSettings& settings)
{
  m_bSkyBox                    = settings.GetSkyBox();
  m_bSkyLight                  = settings.GetSkyLight();
  m_sSkyLightCubeMap           = settings.GetSkyLightCubeMap();
  m_fSkyLightIntensity         = settings.GetSkyLightIntensity();
  m_bDirectionalLight          = settings.GetDirectionalLight();
  m_DirectionalLightAngle      = settings.GetDirectionalLightAngle();
  m_bDirectionalLightShadows   = settings.GetDirectionalLightShadows();
  m_fDirectionalLightIntensity = settings.GetDirectionalLightIntensity();
  m_bFog                       = settings.GetFog();
  TriggerPreferencesChangedEvent();
}

void xiiEditorPreferencesUser::SetShowInDevelopmentFeatures(bool b)
{
  m_bShowInDevelopmentFeatures = b;

  xiiQtAddSubElementButton::s_bShowInDevelopmentFeatures = b;
}

void xiiEditorPreferencesUser::SetHighlightUntranslatedUI(bool b)
{
  m_bHighlightUntranslatedUI = b;

  xiiTranslator::HighlightUntranslated(m_bHighlightUntranslatedUI);
}

void xiiEditorPreferencesUser::SetGizmoSize(float f)
{
  m_fGizmoSize = f;
  SyncGlobalSettings();
}

void xiiEditorPreferencesUser::SyncGlobalSettings()
{
  xiiGlobalSettingsMsgToEngine msg;
  msg.m_fGizmoScale = m_fGizmoSize;

  xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void xiiQtEditorApp::LoadEditorPreferences()
{
  XII_PROFILE_SCOPE("Preferences");
  xiiPreferences::QueryPreferences<xiiEditorPreferencesUser>();
}
