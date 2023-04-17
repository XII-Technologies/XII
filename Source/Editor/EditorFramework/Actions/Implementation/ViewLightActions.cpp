#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/ViewLightActions.h>
#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewLightButtonAction, 1, xiiRTTINoAllocator);
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewLightSliderAction, 1, xiiRTTINoAllocator);
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiActionDescriptorHandle xiiViewLightActions::s_hLightMenu;
xiiActionDescriptorHandle xiiViewLightActions::s_hSkyBox;
xiiActionDescriptorHandle xiiViewLightActions::s_hSkyLight;
xiiActionDescriptorHandle xiiViewLightActions::s_hSkyLightCubeMap;
xiiActionDescriptorHandle xiiViewLightActions::s_hSkyLightIntensity;
xiiActionDescriptorHandle xiiViewLightActions::s_hDirLight;
xiiActionDescriptorHandle xiiViewLightActions::s_hDirLightAngle;
xiiActionDescriptorHandle xiiViewLightActions::s_hDirLightShadows;
xiiActionDescriptorHandle xiiViewLightActions::s_hDirLightIntensity;
xiiActionDescriptorHandle xiiViewLightActions::s_hFog;
xiiActionDescriptorHandle xiiViewLightActions::s_hSetAsDefault;

void xiiViewLightActions::RegisterActions()
{
  s_hLightMenu         = XII_REGISTER_MENU_WITH_ICON("View.LightMenu", ":/EditorFramework/Icons/ViewLightMenu.png");
  s_hSkyBox            = XII_REGISTER_ACTION_1("View.SkyBox", xiiActionScope::Document, "View", "", xiiViewLightButtonAction, xiiEngineViewLightSettingsEvent::Type::SkyBoxChanged);
  s_hSkyLight          = XII_REGISTER_ACTION_1("View.SkyLight", xiiActionScope::Document, "View", "", xiiViewLightButtonAction, xiiEngineViewLightSettingsEvent::Type::SkyLightChanged);
  s_hSkyLightCubeMap   = XII_REGISTER_ACTION_1("View.SkyLightCubeMap", xiiActionScope::Document, "View", "", xiiViewLightButtonAction, xiiEngineViewLightSettingsEvent::Type::SkyLightCubeMapChanged);
  s_hSkyLightIntensity = XII_REGISTER_ACTION_1("View.SkyLightIntensity", xiiActionScope::Document, "View", "", xiiViewLightSliderAction, xiiEngineViewLightSettingsEvent::Type::SkyLightIntensityChanged);

  s_hDirLight          = XII_REGISTER_ACTION_1("View.DirectionalLight", xiiActionScope::Document, "View", "", xiiViewLightButtonAction, xiiEngineViewLightSettingsEvent::Type::DirectionalLightChanged);
  s_hDirLightAngle     = XII_REGISTER_ACTION_1("View.DirLightAngle", xiiActionScope::Document, "View", "", xiiViewLightSliderAction, xiiEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged);
  s_hDirLightShadows   = XII_REGISTER_ACTION_1("View.DirectionalLightShadows", xiiActionScope::Document, "View", "", xiiViewLightButtonAction, xiiEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged);
  s_hDirLightIntensity = XII_REGISTER_ACTION_1("View.DirLightIntensity", xiiActionScope::Document, "View", "", xiiViewLightSliderAction, xiiEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged);
  s_hFog               = XII_REGISTER_ACTION_1("View.Fog", xiiActionScope::Document, "View", "", xiiViewLightButtonAction, xiiEngineViewLightSettingsEvent::Type::FogChanged);
  s_hSetAsDefault      = XII_REGISTER_ACTION_1("View.SetAsDefault", xiiActionScope::Document, "View", "", xiiViewLightButtonAction, xiiEngineViewLightSettingsEvent::Type::DefaultValuesChanged);
}

void xiiViewLightActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hLightMenu);
  xiiActionManager::UnregisterAction(s_hSkyBox);
  xiiActionManager::UnregisterAction(s_hSkyLight);
  xiiActionManager::UnregisterAction(s_hSkyLightCubeMap);
  xiiActionManager::UnregisterAction(s_hSkyLightIntensity);
  xiiActionManager::UnregisterAction(s_hDirLight);
  xiiActionManager::UnregisterAction(s_hDirLightAngle);
  xiiActionManager::UnregisterAction(s_hDirLightShadows);
  xiiActionManager::UnregisterAction(s_hDirLightIntensity);
  xiiActionManager::UnregisterAction(s_hFog);
  xiiActionManager::UnregisterAction(s_hSetAsDefault);
}

void xiiViewLightActions::MapActions(const char* szMapping, const char* szPath)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(szMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hLightMenu, szPath, 2.5f);
  xiiStringBuilder sSubPath(szPath, "/View.LightMenu");
  pMap->MapAction(s_hSkyBox, sSubPath, 1.0f);
  pMap->MapAction(s_hSkyLight, sSubPath, 1.0f);
  pMap->MapAction(s_hSkyLightCubeMap, sSubPath, 2.0f);
  pMap->MapAction(s_hSkyLightIntensity, sSubPath, 3.0f);
  pMap->MapAction(s_hDirLight, sSubPath, 4.0f);
  pMap->MapAction(s_hDirLightAngle, sSubPath, 5.0f);
  pMap->MapAction(s_hDirLightShadows, sSubPath, 6.0f);
  pMap->MapAction(s_hDirLightIntensity, sSubPath, 7.0f);
  pMap->MapAction(s_hFog, sSubPath, 8.0f);
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
    case xiiEngineViewLightSettingsEvent::Type::SkyBoxChanged:
      SetCheckable(true);
      SetIconPath(":/TypeIcons/xiiSkyBoxComponent.png");
      break;
    case xiiEngineViewLightSettingsEvent::Type::SkyLightChanged:
      SetCheckable(true);
      SetIconPath(":/TypeIcons/xiiSkyLightComponent.png");
      break;
    case xiiEngineViewLightSettingsEvent::Type::SkyLightCubeMapChanged:
      SetIconPath(":/TypeIcons/xiiSkyLightComponent.png");
      break;
    case xiiEngineViewLightSettingsEvent::Type::DirectionalLightChanged:
      SetCheckable(true);
      SetIconPath(":/TypeIcons/xiiDirectionalLightComponent.png");
      break;
    case xiiEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged:
      SetCheckable(true);
      SetIconPath(":/TypeIcons/xiiDirectionalLightComponent.png");
      break;
    case xiiEngineViewLightSettingsEvent::Type::FogChanged:
      SetCheckable(true);
      SetIconPath(":/TypeIcons/xiiFogComponent.png");
      break;
    case xiiEngineViewLightSettingsEvent::Type::DefaultValuesChanged:
      SetCheckable(false);
      SetIconPath(":/EditorFramework/Icons/ViewLightMenu.png");
      break;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  UpdateAction();
}

xiiViewLightButtonAction::~xiiViewLightButtonAction()
{
  m_pSettings->m_EngineViewLightSettingsEvents.RemoveEventHandler(m_SettingsID);
}

void xiiViewLightButtonAction::Execute(const xiiVariant& value)
{
  xiiQtEngineViewWidget* pView = qobject_cast<xiiQtEngineViewWidget*>(m_Context.m_pWindow);

  switch (m_ButtonType)
  {
    case xiiEngineViewLightSettingsEvent::Type::SkyBoxChanged:
    {
      m_pSettings->SetSkyBox(value.ConvertTo<bool>());
    }
    break;
    case xiiEngineViewLightSettingsEvent::Type::SkyLightChanged:
    {
      m_pSettings->SetSkyLight(value.ConvertTo<bool>());
    }
    break;
    case xiiEngineViewLightSettingsEvent::Type::SkyLightCubeMapChanged:
    {
      xiiStringBuilder sFile     = m_pSettings->GetSkyLightCubeMap();
      xiiUuid          assetGuid = xiiConversionUtils::ConvertStringToUuid(sFile);

      xiiQtAssetBrowserDlg dlg(pView, assetGuid, "CompatibleAsset_Texture_Cube");
      if (dlg.exec() == 0)
        return;

      assetGuid = dlg.GetSelectedAssetGuid();
      if (assetGuid.IsValid())
        xiiConversionUtils::ToString(assetGuid, sFile);

      if (sFile.IsEmpty())
      {
        sFile = dlg.GetSelectedAssetPathRelative();

        if (sFile.IsEmpty())
        {
          sFile = dlg.GetSelectedAssetPathAbsolute();

          xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sFile);
        }
      }

      if (sFile.IsEmpty())
        return;

      m_pSettings->SetSkyLightCubeMap(sFile);
    }
    break;
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
    case xiiEngineViewLightSettingsEvent::Type::FogChanged:
    {
      m_pSettings->SetFog(value.ConvertTo<bool>());
    }
    break;
    case xiiEngineViewLightSettingsEvent::Type::DefaultValuesChanged:
    {
      if (xiiQtUiServices::MessageBoxQuestion("Do you want to make the current light settings the global default?",
                                              QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) == QMessageBox::StandardButton::Yes)
      {
        xiiEditorPreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiEditorPreferencesUser>();
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
    case xiiEngineViewLightSettingsEvent::Type::SkyBoxChanged:
    {
      SetChecked(m_pSettings->GetSkyBox());
    }
    break;
    case xiiEngineViewLightSettingsEvent::Type::SkyLightChanged:
    {
      SetChecked(m_pSettings->GetSkyLight());
    }
    break;
    case xiiEngineViewLightSettingsEvent::Type::SkyLightCubeMapChanged:
    {
    }
    break;
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
    case xiiEngineViewLightSettingsEvent::Type::FogChanged:
    {
      SetChecked(m_pSettings->GetFog());
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
    case xiiEngineViewLightSettingsEvent::Type::SkyLightIntensityChanged:
      SetIconPath(":/TypeIcons/xiiSkyLightComponent.png");
      SetRange(0, 20);
      break;
    case xiiEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged:
      SetIconPath(":/TypeIcons/xiiDirectionalLightComponent.png");
      SetRange(-90, 90);
      break;
    case xiiEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged:
      SetIconPath(":/TypeIcons/xiiDirectionalLightComponent.png");
      SetRange(0, 200);
      break;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  UpdateAction();
}

xiiViewLightSliderAction::~xiiViewLightSliderAction()
{
  m_pSettings->m_EngineViewLightSettingsEvents.RemoveEventHandler(m_SettingsID);
}

void xiiViewLightSliderAction::Execute(const xiiVariant& value)
{
  xiiQtEngineViewWidget* pView = qobject_cast<xiiQtEngineViewWidget*>(m_Context.m_pWindow);

  switch (m_ButtonType)
  {
    case xiiEngineViewLightSettingsEvent::Type::SkyLightIntensityChanged:
    {
      m_pSettings->SetSkyLightIntensity(value.ConvertTo<float>() / 10.0f);
    }
    break;
    case xiiEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged:
    {
      m_pSettings->SetDirectionalLightAngle(xiiAngle::Degree(value.ConvertTo<float>()));
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
    case xiiEngineViewLightSettingsEvent::Type::SkyLightIntensityChanged:
    {
      SetValue(xiiMath::Clamp((xiiInt32)(m_pSettings->GetSkyLightIntensity() * 10.0f), 0, 20));
    }
    break;
    case xiiEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged:
    {
      SetValue(xiiMath::Clamp((xiiInt32)(m_pSettings->GetDirectionalLightAngle().GetDegree()), -90, 90));
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
