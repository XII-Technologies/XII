/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Settings/EditorPreferences.h>
#include <Foundation/Profiling/Profiling.h>
#include <GuiFoundation/PropertyGrid/Implementation/AddSubElementButton.moc.h>
#include <GuiFoundation/Widgets/SearchableTypeMenu.moc.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorPreferencesUser, 1, xiiRTTIDefaultAllocator<xiiEditorPreferencesUser>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiSettingsScopeAttribute(xiiSettingsScope::Application),
    new xiiCategoryAttribute("Editor"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RestoreProjectOnStartup", m_bLoadLastProjectAtStartup)->AddAttributes(new xiiDefaultValueAttribute(true), new xiiSettingsRestartAttribute()),
    XII_MEMBER_PROPERTY("ShowSplashScreen", m_bShowSplashScreen)->AddAttributes(new xiiDefaultValueAttribute(true), new xiiSettingsRestartAttribute()),
    XII_MEMBER_PROPERTY("BackgroundAssetProcessing", m_bBackgroundAssetProcessing)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("FieldOfView", m_fPerspectiveFieldOfView)->AddAttributes(new xiiDefaultValueAttribute(70.0f), new xiiClampValueAttribute(10.0f, 150.0f)),
    XII_MEMBER_PROPERTY("MaxFramerate", m_uiMaxFramerate)->AddAttributes(new xiiDefaultValueAttribute(60)),
    XII_MEMBER_PROPERTY("MaxFramerateWhenUnfocused", m_uiMaxFramerateWhenUnfocused)->AddAttributes(new xiiDefaultValueAttribute(15)),
    XII_ACCESSOR_PROPERTY("GizmoSize", GetGizmoSize, SetGizmoSize)->AddAttributes(new xiiDefaultValueAttribute(1.5f), new xiiClampValueAttribute(0.2f, 5.0f)),
    XII_ACCESSOR_PROPERTY("ShowInDevelopmentFeatures", GetShowInDevelopmentFeatures, SetShowInDevelopmentFeatures),
    XII_MEMBER_PROPERTY("RotationSnap", m_RotationSnapValue)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::MakeFromDegree(15.0f)), new xiiGroupAttribute("Snapping")),
    XII_MEMBER_PROPERTY("ScaleSnap", m_fScaleSnapValue)->AddAttributes(new xiiDefaultValueAttribute(0.125f), new xiiGroupAttribute("Snapping")),
    XII_MEMBER_PROPERTY("TranslationSnap", m_fTranslationSnapValue)->AddAttributes(new xiiDefaultValueAttribute(0.25f), new xiiGroupAttribute("Snapping")),
    XII_MEMBER_PROPERTY("UsePrecompiledTools", m_bUsePrecompiledTools)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("CustomPrecompiledToolsFolder", m_sCustomPrecompiledToolsFolder),
    XII_MEMBER_PROPERTY("ExpandSceneTreeOnSelection", m_bExpandSceneTreeOnSelection)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("ClearEditorLogsOnPlay", m_bClearEditorLogsOnPlay)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("CombinedEditorAndEngineLogs", m_bCombinedEditorAndEngineLogs)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ACCESSOR_PROPERTY("HighlightUntranslatedUI", GetHighlightUntranslatedUI, SetHighlightUntranslatedUI),
    XII_MEMBER_PROPERTY("AssetBrowserShowItemsInSubFolders", m_bAssetBrowserShowItemsInSubFolders)->AddAttributes(new xiiDefaultValueAttribute(true), new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("AutoSaveMinutes", m_uiAutoSaveMinutes)->AddAttributes(new xiiDefaultValueAttribute(5), new xiiClampValueAttribute(0, 24 * 60)),

    // START GROUP Engine View Light Settings
    XII_MEMBER_PROPERTY("DirectionalLight", m_bDirectionalLight)->AddAttributes(new xiiDefaultValueAttribute(true), new xiiGroupAttribute("Engine View Light Settings")),
    XII_MEMBER_PROPERTY("DirectionalLightAngle", m_DirectionalLightAngle)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::MakeFromDegree(70.0f)), new xiiClampValueAttribute(xiiAngle::MakeFromDegree(0.0f), xiiAngle::MakeFromDegree(360.0f))),
    XII_MEMBER_PROPERTY("DirectionalLightShadows", m_bDirectionalLightShadows),
    XII_MEMBER_PROPERTY("DirectionalLightIntensity", m_fDirectionalLightIntensity)->AddAttributes(new xiiDefaultValueAttribute(10.0f)),
    XII_ARRAY_MEMBER_PROPERTY("RecentTypes", m_RecentlyCreatedTypes)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiEditorPreferencesUser::xiiEditorPreferencesUser() :
  xiiEditorSettings(Domain::Application, "General")
{
}

xiiEditorPreferencesUser::~xiiEditorPreferencesUser()
{
  if (xiiQtTypeMenu::s_pRecentList == &m_RecentlyCreatedTypes)
    xiiQtTypeMenu::s_pRecentList = nullptr;
}

void xiiEditorPreferencesUser::ApplyDefaultValues(xiiEngineViewLightSettings& ref_settings)
{
  ref_settings.SetDirectionalLight(m_bDirectionalLight);
  ref_settings.SetDirectionalLightAngle(m_DirectionalLightAngle);
  ref_settings.SetDirectionalLightShadows(m_bDirectionalLightShadows);
  ref_settings.SetDirectionalLightIntensity(m_fDirectionalLightIntensity);
}

void xiiEditorPreferencesUser::SetAsDefaultValues(const xiiEngineViewLightSettings& settings)
{
  m_bDirectionalLight          = settings.GetDirectionalLight();
  m_DirectionalLightAngle      = settings.GetDirectionalLightAngle();
  m_bDirectionalLightShadows   = settings.GetDirectionalLightShadows();
  m_fDirectionalLightIntensity = settings.GetDirectionalLightIntensity();
  NotifyChanged();
}

void xiiEditorPreferencesUser::SetShowInDevelopmentFeatures(bool b)
{
  m_bShowInDevelopmentFeatures = b;
}

void xiiEditorPreferencesUser::SetHighlightUntranslatedUI(bool b)
{
  m_bHighlightUntranslatedUI = b;
}

void xiiEditorPreferencesUser::SetGizmoSize(float f)
{
  m_fGizmoSize = f;
}

void xiiEditorPreferencesUser::OnChanged()
{
  xiiQtTypeMenu::s_pRecentList                = &m_RecentlyCreatedTypes;
  xiiQtTypeMenu::s_bShowInDevelopmentFeatures = m_bShowInDevelopmentFeatures;
  xiiTranslator::HighlightUntranslated(m_bHighlightUntranslatedUI);
  if (xiiEditorEngineProcessConnection::GetSingleton() != nullptr)
    SyncGlobalSettings();
}

void xiiEditorPreferencesUser::SetMaxFramerate(xiiUInt16 uiFPS)
{
  if (m_uiMaxFramerate == uiFPS)
    return;

  m_uiMaxFramerate = uiFPS;
}

void xiiEditorPreferencesUser::SetMaxFramerateWhenUnfocused(xiiUInt16 uiFPS)
{
  if (m_uiMaxFramerateWhenUnfocused == uiFPS)
    return;

  m_uiMaxFramerateWhenUnfocused = uiFPS;
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
  xiiEditorSettings::QuerySettings<xiiEditorPreferencesUser>();
}
