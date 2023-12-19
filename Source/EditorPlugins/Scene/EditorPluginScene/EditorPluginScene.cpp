#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/GameObjectDocumentActions.h>
#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/QuadViewActions.h>
#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Visualizers/VisualizerAdapterRegistry.h>
#include <EditorPluginScene/Actions/LayerActions.h>
#include <EditorPluginScene/Actions/SceneActions.h>
#include <EditorPluginScene/Actions/SelectionActions.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Scene/Scene2DocumentWindow.moc.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <EditorPluginScene/Visualizers/BoxReflectionProbeVisualizerAdapter.h>
#include <EditorPluginScene/Visualizers/PointLightVisualizerAdapter.h>
#include <EditorPluginScene/Visualizers/SpotLightVisualizerAdapter.h>
#include <GameEngine/Configuration/RendererProfileConfigs.h>
#include <GameEngine/Gameplay/GreyBoxComponent.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <GraphicsCore/Lights/BoxReflectionProbeComponent.h>
#include <GraphicsCore/Lights/PointLightComponent.h>
#include <GraphicsCore/Lights/SpotLightComponent.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

void OnDocumentManagerEvent(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiScene2Document>())
      {
        new xiiQtScene2DocumentWindow(static_cast<xiiScene2Document*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
      else if (e.m_pDocument->GetDynamicRTTI() == xiiGetStaticRTTI<xiiSceneDocument>())
      {
        new xiiQtSceneDocumentWindow(static_cast<xiiSceneDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ToolsProjectEventHandler(const xiiEditorAppEvent& e)
{
  if (e.m_Type == xiiEditorAppEvent::Type::BeforeApplyDataDirectories)
  {
    // xiiQtEditorApp::GetSingleton()->AddPluginDataDirDependency(">sdk/Data/Base", "base");
  }
}

void AssetCuratorEventHandler(const xiiAssetCuratorEvent& e)
{
  if (e.m_Type == xiiAssetCuratorEvent::Type::ActivePlatformChanged)
  {
    xiiSet<xiiString> allCamPipes;

    auto& dynEnum = xiiDynamicStringEnum::CreateDynamicEnum("CameraPipelines");

    for (xiiUInt32 profileIdx = 0; profileIdx < xiiAssetCurator::GetSingleton()->GetNumAssetProfiles(); ++profileIdx)
    {
      const xiiPlatformProfile* pProfile = xiiAssetCurator::GetSingleton()->GetAssetProfile(profileIdx);

      const xiiRenderPipelineProfileConfig* pConfig = pProfile->GetTypeConfig<xiiRenderPipelineProfileConfig>();

      for (auto it = pConfig->m_CameraPipelines.GetIterator(); it.IsValid(); ++it)
      {
        dynEnum.AddValidValue(it.Key(), true);
      }
    }
  }
}

void xiiCameraComponent_PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e);
void xiiSkyLightComponent_PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e);
void xiiGreyBoxComponent_PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e);

void xiiSceneDocument_PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e);

void OnLoadPlugin()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiSceneDocument_PropertyMetaStateEventHandler);

  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(OnDocumentManagerEvent));

  xiiQtEditorApp::GetSingleton()->m_Events.AddEventHandler(ToolsProjectEventHandler);

  xiiAssetCurator::GetSingleton()->m_Events.AddEventHandler(AssetCuratorEventHandler);

  // Add built in tags
  {
    xiiToolsTagRegistry::AddTag(xiiToolsTag("Default", "Exclude From Export", true));
    xiiToolsTagRegistry::AddTag(xiiToolsTag("Default", "CastShadow", true));
    xiiToolsTagRegistry::AddTag(xiiToolsTag("Default", "SkyLight", true));
  }

  xiiSelectionActions::RegisterActions();
  xiiSceneGizmoActions::RegisterActions();
  xiiSceneActions::RegisterActions();
  xiiLayerActions::RegisterActions();

  // Menu Bar
  const char* MenuBars[] = {"EditorPluginScene_DocumentMenuBar", "EditorPluginScene_Scene2MenuBar"};
  for (const char* szMenuBar : MenuBars)
  {
    xiiActionMapManager::RegisterActionMap(szMenuBar).AssertSuccess();
    xiiStandardMenus::MapActions(szMenuBar, xiiStandardMenuTypes::Default | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Scene | xiiStandardMenuTypes::View);
    xiiProjectActions::MapActions(szMenuBar);
    xiiDocumentActions::MapMenuActions(szMenuBar);
    xiiAssetActions::MapMenuActions(szMenuBar);
    xiiDocumentActions::MapToolsActions(szMenuBar);
    xiiCommandHistoryActions::MapActions(szMenuBar);
    xiiTransformGizmoActions::MapMenuActions(szMenuBar);
    xiiSceneGizmoActions::MapMenuActions(szMenuBar);
    xiiGameObjectSelectionActions::MapActions(szMenuBar);
    xiiSelectionActions::MapActions(szMenuBar);
    xiiEditActions::MapActions(szMenuBar, true, true);
    xiiTranslateGizmoAction::MapActions(szMenuBar);
    xiiGameObjectDocumentActions::MapMenuActions(szMenuBar);
    xiiGameObjectDocumentActions::MapMenuSimulationSpeed(szMenuBar);
    xiiSceneActions::MapMenuActions(szMenuBar);
  }
  // Scene2 Menu bar adjustments
  {
    xiiActionMap* pMap = xiiActionMapManager::GetActionMap(MenuBars[1]);
    pMap->UnmapAction(xiiDocumentActions::s_hSave, "G.File.Common").AssertSuccess();
    pMap->MapAction(xiiLayerActions::s_hSaveActiveLayer, "G.File.Common", 6.5f);
  }


  // Tool Bar
  const char* ToolBars[] = {"EditorPluginScene_DocumentToolBar", "EditorPluginScene_Scene2ToolBar"};
  for (const char* szToolBar : ToolBars)
  {
    xiiActionMapManager::RegisterActionMap(szToolBar).AssertSuccess();
    xiiDocumentActions::MapToolbarActions(szToolBar);
    xiiCommandHistoryActions::MapActions(szToolBar, "");
    xiiTransformGizmoActions::MapToolbarActions(szToolBar);
    xiiSceneGizmoActions::MapToolbarActions(szToolBar);
    xiiGameObjectDocumentActions::MapToolbarActions(szToolBar);
    xiiSceneActions::MapToolbarActions(szToolBar);
  }
  // Scene2 Tool bar adjustments
  {
    xiiActionMap* pMap = xiiActionMapManager::GetActionMap(ToolBars[1]);
    pMap->UnmapAction(xiiDocumentActions::s_hSave, "SaveCategory").AssertSuccess();
    pMap->MapAction(xiiLayerActions::s_hSaveActiveLayer, "SaveCategory", 1.0f);
  }

  // View Tool Bar
  xiiActionMapManager::RegisterActionMap("EditorPluginScene_ViewToolBar").AssertSuccess();
  xiiViewActions::MapToolbarActions("EditorPluginScene_ViewToolBar", xiiViewActions::PerspectiveMode | xiiViewActions::RenderMode | xiiViewActions::ActivateRemoteProcess);
  xiiQuadViewActions::MapToolbarActions("EditorPluginScene_ViewToolBar");

  // Visualizers
  xiiVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(xiiGetStaticRTTI<xiiPointLightVisualizerAttribute>(), [](const xiiRTTI* pRtti) -> xiiVisualizerAdapter* { return XII_DEFAULT_NEW(xiiPointLightVisualizerAdapter); });
  xiiVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(xiiGetStaticRTTI<xiiSpotLightVisualizerAttribute>(), [](const xiiRTTI* pRtti) -> xiiVisualizerAdapter* { return XII_DEFAULT_NEW(xiiSpotLightVisualizerAdapter); });
  xiiVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(xiiGetStaticRTTI<xiiBoxReflectionProbeVisualizerAttribute>(), [](const xiiRTTI* pRtti) -> xiiVisualizerAdapter* { return XII_DEFAULT_NEW(xiiBoxReflectionProbeVisualizerAdapter); });

  // SceneGraph Context Menu
  xiiActionMapManager::RegisterActionMap("EditorPluginScene_ScenegraphContextMenu").AssertSuccess();
  xiiGameObjectSelectionActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu");
  xiiSelectionActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu");
  xiiEditActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu");

  // Layer Context Menu
  xiiActionMapManager::RegisterActionMap("EditorPluginScene_LayerContextMenu").AssertSuccess();
  xiiLayerActions::MapContextMenuActions("EditorPluginScene_LayerContextMenu");

  // component property meta states
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiCameraComponent_PropertyMetaStateEventHandler);
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiSkyLightComponent_PropertyMetaStateEventHandler);
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiGreyBoxComponent_PropertyMetaStateEventHandler);
}

void OnUnloadPlugin()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(xiiSceneDocument_PropertyMetaStateEventHandler);

  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(OnDocumentManagerEvent));
  xiiQtEditorApp::GetSingleton()->m_Events.RemoveEventHandler(ToolsProjectEventHandler);
  xiiAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(AssetCuratorEventHandler);
  xiiPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(xiiGreyBoxComponent_PropertyMetaStateEventHandler);
  xiiPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(xiiSkyLightComponent_PropertyMetaStateEventHandler);
  xiiPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(xiiCameraComponent_PropertyMetaStateEventHandler);


  xiiSelectionActions::UnregisterActions();
  xiiSceneGizmoActions::UnregisterActions();
  xiiLayerActions::UnregisterActions();
  xiiSceneActions::UnregisterActions();
}

XII_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

XII_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

void xiiCameraComponent_PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e)
{
  static const xiiRTTI* pRtti = xiiRTTI::FindTypeByName("xiiCameraComponent");
  XII_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  const xiiInt64 usage          = e.m_pObject->GetTypeAccessor().GetValue("UsageHint").ConvertTo<xiiInt64>();
  const bool     isRenderTarget = (usage == 3); // xiiCameraUsageHint::RenderTarget

  auto& props = *e.m_pPropertyStates;

  props["RenderTarget"].m_Visibility       = isRenderTarget ? xiiPropertyUiState::Default : xiiPropertyUiState::Disabled;
  props["RenderTargetOffset"].m_Visibility = isRenderTarget ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
  props["RenderTargetSize"].m_Visibility   = isRenderTarget ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
}

void xiiSkyLightComponent_PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e)
{
  static const xiiRTTI* pRtti = xiiRTTI::FindTypeByName("xiiSkyLightComponent");
  XII_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  const xiiInt64 iReflectionProbeMode = e.m_pObject->GetTypeAccessor().GetValue("ReflectionProbeMode").ConvertTo<xiiInt64>();
  const bool     bIsStatic            = (iReflectionProbeMode == 0); // xiiReflectionProbeMode::Static

  auto& props = *e.m_pPropertyStates;

  props["CubeMap"].m_Visibility = bIsStatic ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
  // props["RenderTargetOffset"].m_Visibility = isRenderTarget ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
  // props["RenderTargetSize"].m_Visibility = isRenderTarget ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
}

void xiiGreyBoxComponent_PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e)
{
  static const xiiRTTI* pRtti = xiiRTTI::FindTypeByName("xiiGreyBoxComponent");
  XII_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  auto& props = *e.m_pPropertyStates;

  const xiiInt64 iShapeType = e.m_pObject->GetTypeAccessor().GetValue("Shape").ConvertTo<xiiInt64>();

  props["Detail"].m_Visibility       = xiiPropertyUiState::Invisible;
  props["Detail"].m_sNewLabelText    = "Detail";
  props["Curvature"].m_Visibility    = xiiPropertyUiState::Invisible;
  props["Thickness"].m_Visibility    = xiiPropertyUiState::Invisible;
  props["SlopedTop"].m_Visibility    = xiiPropertyUiState::Invisible;
  props["SlopedBottom"].m_Visibility = xiiPropertyUiState::Invisible;

  switch (iShapeType)
  {
    case xiiGreyBoxShape::Box:
      break;
    case xiiGreyBoxShape::RampX:
    case xiiGreyBoxShape::RampY:
      break;
    case xiiGreyBoxShape::Column:
      props["Detail"].m_Visibility = xiiPropertyUiState::Default;
      break;
    case xiiGreyBoxShape::StairsX:
    case xiiGreyBoxShape::StairsY:
      props["Detail"].m_Visibility    = xiiPropertyUiState::Default;
      props["Curvature"].m_Visibility = xiiPropertyUiState::Default;
      props["SlopedTop"].m_Visibility = xiiPropertyUiState::Default;
      props["Detail"].m_sNewLabelText = "Steps";
      break;
    case xiiGreyBoxShape::ArchX:
    case xiiGreyBoxShape::ArchY:
      props["Detail"].m_Visibility    = xiiPropertyUiState::Default;
      props["Curvature"].m_Visibility = xiiPropertyUiState::Default;
      props["Thickness"].m_Visibility = xiiPropertyUiState::Default;
      break;
    case xiiGreyBoxShape::SpiralStairs:
      props["Detail"].m_Visibility       = xiiPropertyUiState::Default;
      props["Curvature"].m_Visibility    = xiiPropertyUiState::Default;
      props["Thickness"].m_Visibility    = xiiPropertyUiState::Default;
      props["SlopedTop"].m_Visibility    = xiiPropertyUiState::Default;
      props["SlopedBottom"].m_Visibility = xiiPropertyUiState::Default;
      props["Detail"].m_sNewLabelText    = "Steps";
      break;
  }
}
