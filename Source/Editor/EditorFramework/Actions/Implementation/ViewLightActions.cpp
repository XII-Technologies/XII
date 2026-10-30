/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/ViewLightActions.h>
#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Settings/EditorPreferences.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewLightButtonAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewLightSliderAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiActionDescriptorHandle xiiViewLightActions::s_hLightMenu;
xiiActionDescriptorHandle xiiViewLightActions::s_hDirLight;
xiiActionDescriptorHandle xiiViewLightActions::s_hDirLightAngle;
xiiActionDescriptorHandle xiiViewLightActions::s_hDirLightShadows;
xiiActionDescriptorHandle xiiViewLightActions::s_hDirLightIntensity;
xiiActionDescriptorHandle xiiViewLightActions::s_hSetAsDefault;

void xiiViewLightActions::RegisterActions()
{
  s_hLightMenu = XII_REGISTER_MENU_WITH_ICON("View.LightMenu", ":/EditorFramework/Icons/ViewLightMenu.svg");

  s_hDirLight          = XII_REGISTER_ACTION_1("View.DirectionalLight", xiiActionScope::Document, "View", "", xiiViewLightButtonAction, xiiEngineViewLightSettingsEvent::Type::DirectionalLightChanged);
  s_hDirLightAngle     = XII_REGISTER_ACTION_1("View.DirLightAngle", xiiActionScope::Document, "View", "", xiiViewLightSliderAction, xiiEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged);
  s_hDirLightShadows   = XII_REGISTER_ACTION_1("View.DirectionalLightShadows", xiiActionScope::Document, "View", "", xiiViewLightButtonAction, xiiEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged);
  s_hDirLightIntensity = XII_REGISTER_ACTION_1("View.DirLightIntensity", xiiActionScope::Document, "View", "", xiiViewLightSliderAction, xiiEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged);
  s_hSetAsDefault      = XII_REGISTER_ACTION_1("View.SetAsDefault", xiiActionScope::Document, "View", "", xiiViewLightButtonAction, xiiEngineViewLightSettingsEvent::Type::DefaultValuesChanged);
}

void xiiViewLightActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hLightMenu);
  xiiActionManager::UnregisterAction(s_hDirLight);
  xiiActionManager::UnregisterAction(s_hDirLightAngle);
  xiiActionManager::UnregisterAction(s_hDirLightShadows);
  xiiActionManager::UnregisterAction(s_hDirLightIntensity);
  xiiActionManager::UnregisterAction(s_hSetAsDefault);
}

void xiiViewLightActions::MapToolbarActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hLightMenu, "", 2.5f);
  const xiiStringView sSubPath = "View.LightMenu";
  pMap->MapAction(s_hDirLight, sSubPath, 4.0f);
  pMap->MapAction(s_hDirLightAngle, sSubPath, 5.0f);
  pMap->MapAction(s_hDirLightShadows, sSubPath, 6.0f);
  pMap->MapAction(s_hDirLightIntensity, sSubPath, 7.0f);
  pMap->MapAction(s_hSetAsDefault, sSubPath, 9.0f);
}

//////////////////////////////////////////////////////////////////////////

xiiViewLightButtonAction::xiiViewLightButtonAction(const xiiActionContext& context, const char* szName, xiiEngineViewLightSettingsEvent::Type button) :
  xiiButtonAction(context, szName, false, "")
{
  m_ButtonType                 = button;
  xiiQtEngineViewWidget* pView = qobject_cast<xiiQtEngineViewWidget*>(m_Context.m_pWindow);
  m_pSettings                  = static_cast<xiiEngineViewLightSettings*>(pView->GetDocumentWindow()->GetDocument()->FindSyncObject(xiiEngineViewLightSettings::GetStaticRTTI()));
  XII_ASSERT_DEV(m_pSettings != nullptr, "The asset document does not have a xiiEngineViewLightSettings sync object.");
  m_SettingsID = m_pSettings->m_EngineViewLightSettingsEvents.AddEventHandler(xiiMakeDelegate(&xiiViewLightButtonAction::LightSettingsEventHandler, this));

  switch (m_ButtonType)
  {
    case xiiEngineViewLightSettingsEvent::Type::DirectionalLightChanged:
      SetCheckable(true);
      SetIconPath(":/TypeIcons/xiiDirectionalLightComponent.svg");
      break;
    case xiiEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged:
      SetCheckable(true);
      SetIconPath(":/TypeIcons/xiiDirectionalLightComponent.svg");
      break;
    case xiiEngineViewLightSettingsEvent::Type::DefaultValuesChanged:
      SetCheckable(false);
      SetIconPath(":/EditorFramework/Icons/ViewLightMenu.svg");
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  UpdateAction();
}

xiiViewLightButtonAction::~xiiViewLightButtonAction()
{
  m_pSettings->m_EngineViewLightSettingsEvents.RemoveEventHandler(m_SettingsID);
}

void xiiViewLightButtonAction::Execute(const xiiVariant& value)
{
  switch (m_ButtonType)
  {
    case xiiEngineViewLightSettingsEvent::Type::DirectionalLightChanged:
    {
      m_pSettings->SetDirectionalLight(value.ConvertTo<bool>());
    }
    break;
    case xiiEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged:
    {
      m_pSettings->SetDirectionalLightShadows(value.ConvertTo<bool>());
    }
    break;
    case xiiEngineViewLightSettingsEvent::Type::DefaultValuesChanged:
    {
      if (xiiQtUiServices::MessageBoxQuestion("Do you want to make the current light settings the global default?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) == QMessageBox::StandardButton::Yes)
      {
        xiiEditorPreferencesUser* pPreferences = xiiEditorSettings::QuerySettings<xiiEditorPreferencesUser>();
        pPreferences->SetAsDefaultValues(*m_pSettings);
      }
    }
    break;
    default:
      break;
  }
}

void xiiViewLightButtonAction::LightSettingsEventHandler(const xiiEngineViewLightSettingsEvent& e)
{
  if (m_ButtonType == e.m_Type)
  {
    UpdateAction();
  }
}

void xiiViewLightButtonAction::UpdateAction()
{
  switch (m_ButtonType)
  {
    case xiiEngineViewLightSettingsEvent::Type::DirectionalLightChanged:
    {
      SetChecked(m_pSettings->GetDirectionalLight());
    }
    break;
    case xiiEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged:
    {
      SetChecked(m_pSettings->GetDirectionalLightShadows());
    }
    break;
    default:
      break;
  }
}
//////////////////////////////////////////////////////////////////////////

xiiViewLightSliderAction::xiiViewLightSliderAction(const xiiActionContext& context, const char* szName, xiiEngineViewLightSettingsEvent::Type button) :
  xiiSliderAction(context, szName)
{
  m_ButtonType                 = button;
  xiiQtEngineViewWidget* pView = qobject_cast<xiiQtEngineViewWidget*>(m_Context.m_pWindow);
  m_pSettings                  = static_cast<xiiEngineViewLightSettings*>(pView->GetDocumentWindow()->GetDocument()->FindSyncObject(xiiEngineViewLightSettings::GetStaticRTTI()));
  XII_ASSERT_DEV(m_pSettings != nullptr, "The asset document does not have a xiiEngineViewLightSettings sync object.");
  m_SettingsID = m_pSettings->m_EngineViewLightSettingsEvents.AddEventHandler(xiiMakeDelegate(&xiiViewLightSliderAction::LightSettingsEventHandler, this));

  switch (m_ButtonType)
  {
    case xiiEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged:
      SetIconPath(":/TypeIcons/xiiDirectionalLightComponent.svg");
      SetRange(0, 360);
      break;
    case xiiEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged:
      SetIconPath(":/TypeIcons/xiiDirectionalLightComponent.svg");
      SetRange(0, 200);
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  UpdateAction();
}

xiiViewLightSliderAction::~xiiViewLightSliderAction()
{
  m_pSettings->m_EngineViewLightSettingsEvents.RemoveEventHandler(m_SettingsID);
}

void xiiViewLightSliderAction::Execute(const xiiVariant& value)
{
  switch (m_ButtonType)
  {
    case xiiEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged:
    {
      m_pSettings->SetDirectionalLightAngle(xiiAngle::MakeFromDegree(value.ConvertTo<float>()));
    }
    break;
    case xiiEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged:
    {
      m_pSettings->SetDirectionalLightIntensity(value.ConvertTo<float>() / 10.0f);
    }
    break;
    default:
      break;
  }
}

void xiiViewLightSliderAction::LightSettingsEventHandler(const xiiEngineViewLightSettingsEvent& e)
{
  if (m_ButtonType == e.m_Type)
  {
    UpdateAction();
  }
}

void xiiViewLightSliderAction::UpdateAction()
{
  switch (m_ButtonType)
  {
    case xiiEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged:
    {
      SetValue(xiiMath::Clamp((xiiInt32)(m_pSettings->GetDirectionalLightAngle().GetDegree()), 0, 360));
    }
    break;
    case xiiEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged:
    {
      SetValue(xiiMath::Clamp((xiiInt32)(m_pSettings->GetDirectionalLightIntensity() * 10.0f), 1, 200));
    }
    break;
    default:
      break;
  }
}
