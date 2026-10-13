/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/GameObjectDocumentActions.h>
#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/QuadViewActions.h>
#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Visualizers/VisualizerAdapterRegistry.h>
#include <EditorPluginScene/Actions/LayerActions.h>
#include <EditorPluginScene/Actions/SceneActions.h>
#include <EditorPluginScene/Actions/SelectionActions.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Scene/Scene2DocumentWindow.moc.h>
#include <EditorPluginScene/Scene/SceneComponentPropertyMetaState.h>
#include <EditorPluginScene/Scene/SceneDocumentManager.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <EditorPluginScene/Visualizers/PointLightVisualizerAdapter.h>
#include <EditorPluginScene/Visualizers/ReflectionCaptureVisualizerAdapter.h>
#include <EditorPluginScene/Visualizers/SpotLightVisualizerAdapter.h>
#include <GraphicsCore/Components/Lights/PointLightComponent.h>
#include <GraphicsCore/Components/Lights/ReflectionCaptureComponent.h>
#include <GraphicsCore/Components/Lights/SpotLightComponent.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

static void ToolsProjectEventHandler(const xiiToolsProjectEvent& e);

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

void ToolsProjectEventHandler(const xiiToolsProjectEvent& e)
{
  if (e.m_Type == xiiToolsProjectEvent::Type::ProjectFirstSetup)
  {
    auto project = xiiToolsProject::GetSingleton();

    project->CreateSubFolder("Scenes");
    project->CreateSubFolder("Prefabs");

    for (auto& dm : xiiAssetDocumentManager::GetAllDocumentManagers())
    {
      if (dm->IsInstanceOf<xiiSceneDocumentManager>())
      {
        xiiDocument* doc;

        xiiStringBuilder path(project->GetProjectDirectory(), "/Scenes/Main.xiiScene");
        dm->CreateDocument("Scene", path, doc).IgnoreResult();
      }
    }
  }
}

void xiiSceneDocument_PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e);

QImage SliderImageGenerator_LightTemperature(xiiUInt32 uiWidth, xiiUInt32 uiHeight, double fMinValue, double fMaxValue)
{
  // can use a 1D image, height doesn't need to be all used
  QImage image = QImage(uiWidth, 1, QImage::Format::Format_RGB32);

  for (xiiUInt32 x = 0; x < uiWidth; ++x)
  {
    const double pos = (double)x / (uiWidth - 1.0);
    xiiColor     c   = xiiColor::MakeFromKelvin(static_cast<xiiUInt32>((pos * (fMaxValue - fMinValue)) + fMinValue));

    xiiColorGammaUB cg = c;
    image.setPixel(x, 0, qRgb(cg.r, cg.g, cg.b));
  }

  return image;
}

void OnLoadPlugin()
{
  xiiToolsProject::GetSingleton()->s_Events.AddEventHandler(ToolsProjectEventHandler);

  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiSceneDocument_PropertyMetaStateEventHandler);

  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(OnDocumentManagerEvent));


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

  // misc
  xiiQtImageSliderWidget::s_ImageGenerators["LightTemperature"] = SliderImageGenerator_LightTemperature;

  // Menu Bar
  const char* MenuBars[] = {"EditorPluginScene_DocumentMenuBar", "EditorPluginScene_Scene2MenuBar"};
  for (const char* szMenuBar : MenuBars)
  {
    xiiActionMapManager::RegisterActionMap(szMenuBar, "AssetMenuBar");
    xiiStandardMenus::MapActions(szMenuBar, xiiStandardMenuTypes::Scene | xiiStandardMenuTypes::View);
    xiiDocumentActions::MapToolsActions(szMenuBar);
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
    pMap->HideAction(xiiDocumentActions::s_hSave, "G.File.Common");
    pMap->MapAction(xiiLayerActions::s_hSaveActiveLayer, "G.File.Common", 6.5f);
  }


  // Tool Bar
  const char* ToolBars[] = {"EditorPluginScene_DocumentToolBar", "EditorPluginScene_Scene2ToolBar"};
  for (const char* szToolBar : ToolBars)
  {
    xiiActionMapManager::RegisterActionMap(szToolBar, "AssetToolbar");

    xiiTransformGizmoActions::MapToolbarActions(szToolBar);
    xiiSceneGizmoActions::MapToolbarActions(szToolBar);
    xiiGameObjectDocumentActions::MapToolbarActions(szToolBar);
    xiiSceneActions::MapToolbarActions(szToolBar);
  }
  // Scene2 Tool bar adjustments
  {
    xiiActionMap* pMap = xiiActionMapManager::GetActionMap(ToolBars[1]);
    pMap->HideAction(xiiDocumentActions::s_hSave, "SaveCategory");
    pMap->MapAction(xiiLayerActions::s_hSaveActiveLayer, "SaveCategory", 1.0f);
    pMap->HideAction(xiiAssetActions::s_hTransformAsset, "AssetCategory");
  }

  // View Tool Bar
  xiiActionMapManager::RegisterActionMap("EditorPluginScene_ViewToolBar", "AssetViewToolbar");
  xiiViewActions::MapToolbarActions("EditorPluginScene_ViewToolBar", xiiViewActions::PerspectiveMode | xiiViewActions::RenderMode | xiiViewActions::ActivateRemoteProcess);
  xiiQuadViewActions::MapToolbarActions("EditorPluginScene_ViewToolBar");

  // Visualizers
  xiiVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(xiiGetStaticRTTI<xiiPointLightVisualizerAttribute>(), [](const xiiRTTI* pRtti) -> xiiVisualizerAdapter* { return XII_DEFAULT_NEW(xiiPointLightVisualizerAdapter); });
  xiiVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(xiiGetStaticRTTI<xiiSpotLightVisualizerAttribute>(), [](const xiiRTTI* pRtti) -> xiiVisualizerAdapter* { return XII_DEFAULT_NEW(xiiSpotLightVisualizerAdapter); });
  xiiVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(xiiGetStaticRTTI<xiiReflectionCaptureVisualizerAttribute>(), [](const xiiRTTI* pRtti) -> xiiVisualizerAdapter* { return XII_DEFAULT_NEW(xiiReflectionCaptureVisualizerAdapter); });

  // SceneGraph Context Menu
  xiiActionMapManager::RegisterActionMap("EditorPluginScene_ScenegraphContextMenu");
  xiiGameObjectSelectionActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu");
  xiiSelectionActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu");
  xiiEditActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu");

  // Layer Context Menu
  xiiActionMapManager::RegisterActionMap("EditorPluginScene_LayerContextMenu");
  xiiLayerActions::MapContextMenuActions("EditorPluginScene_LayerContextMenu");

  xiiActionMapManager::RegisterActionMap("EditorPluginScene_LayerToolbar");
  xiiLayerActions::MapToolbarActions("EditorPluginScene_LayerToolbar");

  // component property meta states
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiCameraComponent_PropertyMetaStateEventHandler);
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiReflectionCaptureComponent_PropertyMetaStateEventHandler);
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiLightComponent_PropertyMetaStateEventHandler);
}

void OnUnloadPlugin()
{
  auto& visualizers = xiiVisualizerAdapterRegistry::GetSingleton()->m_Factory;
  visualizers.UnregisterCreator(xiiGetStaticRTTI<xiiReflectionCaptureVisualizerAttribute>());
  visualizers.UnregisterCreator(xiiGetStaticRTTI<xiiSpotLightVisualizerAttribute>());
  visualizers.UnregisterCreator(xiiGetStaticRTTI<xiiPointLightVisualizerAttribute>());

  xiiPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(xiiSceneDocument_PropertyMetaStateEventHandler);

  xiiToolsProject::GetSingleton()->s_Events.RemoveEventHandler(ToolsProjectEventHandler);
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(OnDocumentManagerEvent));
  xiiPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(xiiCameraComponent_PropertyMetaStateEventHandler);
  xiiPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(xiiReflectionCaptureComponent_PropertyMetaStateEventHandler);
  xiiPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(xiiLightComponent_PropertyMetaStateEventHandler);


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
