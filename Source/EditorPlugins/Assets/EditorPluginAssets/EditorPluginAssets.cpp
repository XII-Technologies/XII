#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/CommonAssetActions.h>
#include <EditorFramework/Actions/GameObjectContextActions.h>
#include <EditorFramework/Actions/GameObjectDocumentActions.h>
#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/QuadViewActions.h>
#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Actions/ViewLightActions.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <EditorPluginAssets/DecalAsset/DecalAsset.h>
#include <EditorPluginAssets/Dialogs/ShaderTemplateDlg.moc.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetObjects.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetWindow.moc.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetWindow.moc.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonActions.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetWindow.moc.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetObjects.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetWindow.moc.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptActions.h>
#include <EditorPluginAssets/VisualShader/VisualShaderActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

static void ConfigureAnimationControllerAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("AnimationControllerAssetMenuBar").IgnoreResult();

    xiiStandardMenus::MapActions("AnimationControllerAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
    xiiProjectActions::MapActions("AnimationControllerAssetMenuBar");
    xiiDocumentActions::MapActions("AnimationControllerAssetMenuBar", "Menu.File", false);
    xiiAssetActions::MapMenuActions("AnimationControllerAssetMenuBar", "Menu.File");
    xiiCommandHistoryActions::MapActions("AnimationControllerAssetMenuBar", "Menu.Edit");
    xiiEditActions::MapActions("AnimationControllerAssetMenuBar", "Menu.Edit", false, false);
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("AnimationControllerAssetToolBar").IgnoreResult();
    xiiDocumentActions::MapActions("AnimationControllerAssetToolBar", "", true);
    xiiCommandHistoryActions::MapActions("AnimationControllerAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("AnimationControllerAssetToolBar", true);
  }
}

static void ConfigureTexture2DAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiTextureAssetProperties::PropertyMetaStateEventHandler);

  xiiTextureAssetActions::RegisterActions();

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("TextureAssetMenuBar").IgnoreResult();
    xiiStandardMenus::MapActions("TextureAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
    xiiProjectActions::MapActions("TextureAssetMenuBar");
    xiiDocumentActions::MapActions("TextureAssetMenuBar", "Menu.File", false);
    xiiAssetActions::MapMenuActions("TextureAssetMenuBar", "Menu.File");
    xiiCommandHistoryActions::MapActions("TextureAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("TextureAssetToolBar").IgnoreResult();
    xiiDocumentActions::MapActions("TextureAssetToolBar", "", true);
    xiiCommandHistoryActions::MapActions("TextureAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("TextureAssetToolBar", true);
    xiiTextureAssetActions::MapActions("TextureAssetToolBar", "");
  }
}

static void ConfigureTextureCubeAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiTextureCubeAssetProperties::PropertyMetaStateEventHandler);

  xiiTextureCubeAssetActions::RegisterActions();

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("TextureCubeAssetMenuBar").IgnoreResult();
    xiiStandardMenus::MapActions("TextureCubeAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
    xiiProjectActions::MapActions("TextureCubeAssetMenuBar");
    xiiDocumentActions::MapActions("TextureCubeAssetMenuBar", "Menu.File", false);
    xiiAssetActions::MapMenuActions("TextureCubeAssetMenuBar", "Menu.File");
    xiiCommandHistoryActions::MapActions("TextureCubeAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("TextureCubeAssetToolBar").IgnoreResult();
    xiiDocumentActions::MapActions("TextureCubeAssetToolBar", "", true);
    xiiCommandHistoryActions::MapActions("TextureCubeAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("TextureCubeAssetToolBar", true);
    xiiTextureAssetActions::MapActions("TextureCubeAssetToolBar", "");
  }
}

static void ConfigureLUTAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiLUTAssetProperties::PropertyMetaStateEventHandler);

  xiiLUTAssetActions::RegisterActions();

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("LUTAssetMenuBar").IgnoreResult();
    xiiStandardMenus::MapActions("LUTAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
    xiiProjectActions::MapActions("LUTAssetMenuBar");
    xiiDocumentActions::MapActions("LUTAssetMenuBar", "Menu.File", false);
    xiiAssetActions::MapMenuActions("LUTAssetMenuBar", "Menu.File");
    xiiCommandHistoryActions::MapActions("LUTAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("LUTAssetToolBar").IgnoreResult();
    xiiDocumentActions::MapActions("LUTAssetToolBar", "", true);
    xiiCommandHistoryActions::MapActions("LUTAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("LUTAssetToolBar", true);
  }
}

static void ConfigureMaterialAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiMaterialAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("MaterialAssetMenuBar").IgnoreResult();
    xiiStandardMenus::MapActions("MaterialAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
    xiiProjectActions::MapActions("MaterialAssetMenuBar");
    xiiDocumentActions::MapActions("MaterialAssetMenuBar", "Menu.File", false);
    xiiDocumentActions::MapToolsActions("MaterialAssetMenuBar", "Menu.Tools");
    xiiAssetActions::MapMenuActions("MaterialAssetMenuBar", "Menu.File");
    xiiCommandHistoryActions::MapActions("MaterialAssetMenuBar", "Menu.Edit");
    xiiEditActions::MapActions("MaterialAssetMenuBar", "Menu.Edit", false, false);
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("MaterialAssetToolBar").IgnoreResult();
    xiiDocumentActions::MapActions("MaterialAssetToolBar", "", true);
    xiiCommandHistoryActions::MapActions("MaterialAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("MaterialAssetToolBar", true);

    xiiMaterialAssetActions::RegisterActions();
    xiiMaterialAssetActions::MapActions("MaterialAssetToolBar", "");

    xiiVisualShaderActions::RegisterActions();
    xiiVisualShaderActions::MapActions("MaterialAssetToolBar");
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("MaterialAssetViewToolBar").IgnoreResult();
    xiiViewActions::MapActions("MaterialAssetViewToolBar", "", xiiViewActions::RenderMode | xiiViewActions::ActivateRemoteProcess);
    xiiViewLightActions::MapActions("MaterialAssetViewToolBar", "");
  }
}

static void ConfigureRenderPipelineAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("RenderPipelineAssetMenuBar").IgnoreResult();

    xiiStandardMenus::MapActions("RenderPipelineAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
    xiiProjectActions::MapActions("RenderPipelineAssetMenuBar");
    xiiDocumentActions::MapActions("RenderPipelineAssetMenuBar", "Menu.File", false);
    xiiAssetActions::MapMenuActions("RenderPipelineAssetMenuBar", "Menu.File");
    xiiCommandHistoryActions::MapActions("RenderPipelineAssetMenuBar", "Menu.Edit");
    xiiEditActions::MapActions("RenderPipelineAssetMenuBar", "Menu.Edit", false, false);
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("RenderPipelineAssetToolBar").IgnoreResult();
    xiiDocumentActions::MapActions("RenderPipelineAssetToolBar", "", true);
    xiiCommandHistoryActions::MapActions("RenderPipelineAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("RenderPipelineAssetToolBar", true);
  }
}

static void ConfigureMeshAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiMeshAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("MeshAssetMenuBar").IgnoreResult();
    xiiStandardMenus::MapActions("MeshAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
    xiiProjectActions::MapActions("MeshAssetMenuBar");
    xiiDocumentActions::MapActions("MeshAssetMenuBar", "Menu.File", false);
    xiiAssetActions::MapMenuActions("MeshAssetMenuBar", "Menu.File");
    xiiCommandHistoryActions::MapActions("MeshAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("MeshAssetToolBar").IgnoreResult();
    xiiDocumentActions::MapActions("MeshAssetToolBar", "", true);
    xiiCommandHistoryActions::MapActions("MeshAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("MeshAssetToolBar", true);
    xiiCommonAssetActions::MapActions("MeshAssetToolBar", "", xiiCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("MeshAssetViewToolBar").IgnoreResult();
    xiiViewActions::MapActions("MeshAssetViewToolBar", "", xiiViewActions::RenderMode | xiiViewActions::ActivateRemoteProcess);
    xiiViewLightActions::MapActions("MeshAssetViewToolBar", "");
  }
}

static void ConfigureSurfaceAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("SurfaceAssetMenuBar").IgnoreResult();
    xiiStandardMenus::MapActions("SurfaceAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
    xiiProjectActions::MapActions("SurfaceAssetMenuBar");
    xiiDocumentActions::MapActions("SurfaceAssetMenuBar", "Menu.File", false);
    xiiAssetActions::MapMenuActions("SurfaceAssetMenuBar", "Menu.File");
    xiiDocumentActions::MapToolsActions("SurfaceAssetMenuBar", "Menu.Tools");
    xiiCommandHistoryActions::MapActions("SurfaceAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("SurfaceAssetToolBar").IgnoreResult();
    xiiDocumentActions::MapActions("SurfaceAssetToolBar", "", true);
    xiiCommandHistoryActions::MapActions("SurfaceAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("SurfaceAssetToolBar", true);
  }
}

static void ConfigureCollectionAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("CollectionAssetMenuBar").IgnoreResult();
    xiiStandardMenus::MapActions("CollectionAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
    xiiProjectActions::MapActions("CollectionAssetMenuBar");
    xiiDocumentActions::MapActions("CollectionAssetMenuBar", "Menu.File", false);
    xiiAssetActions::MapMenuActions("CollectionAssetMenuBar", "Menu.File");
    xiiDocumentActions::MapToolsActions("CollectionAssetMenuBar", "Menu.Tools");
    xiiCommandHistoryActions::MapActions("CollectionAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("CollectionAssetToolBar").IgnoreResult();
    xiiDocumentActions::MapActions("CollectionAssetToolBar", "", true);
    xiiCommandHistoryActions::MapActions("CollectionAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("CollectionAssetToolBar", true);
  }
}

static void ConfigureColorGradientAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("ColorGradientAssetMenuBar").IgnoreResult();
    xiiStandardMenus::MapActions("ColorGradientAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
    xiiProjectActions::MapActions("ColorGradientAssetMenuBar");
    xiiDocumentActions::MapActions("ColorGradientAssetMenuBar", "Menu.File", false);
    xiiAssetActions::MapMenuActions("ColorGradientAssetMenuBar", "Menu.File");
    xiiDocumentActions::MapToolsActions("ColorGradientAssetMenuBar", "Menu.Tools");
    xiiCommandHistoryActions::MapActions("ColorGradientAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("ColorGradientAssetToolBar").IgnoreResult();
    xiiDocumentActions::MapActions("ColorGradientAssetToolBar", "", true);
    xiiCommandHistoryActions::MapActions("ColorGradientAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("ColorGradientAssetToolBar", true);
  }
}

static void ConfigureCurve1DAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("Curve1DAssetMenuBar").IgnoreResult();
    xiiStandardMenus::MapActions("Curve1DAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
    xiiProjectActions::MapActions("Curve1DAssetMenuBar");
    xiiDocumentActions::MapActions("Curve1DAssetMenuBar", "Menu.File", false);
    xiiAssetActions::MapMenuActions("Curve1DAssetMenuBar", "Menu.File");
    xiiDocumentActions::MapToolsActions("Curve1DAssetMenuBar", "Menu.Tools");
    xiiCommandHistoryActions::MapActions("Curve1DAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("Curve1DAssetToolBar").IgnoreResult();
    xiiDocumentActions::MapActions("Curve1DAssetToolBar", "", true);
    xiiCommandHistoryActions::MapActions("Curve1DAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("Curve1DAssetToolBar", true);
  }
}

static void ConfigurePropertyAnimAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("PropertyAnimAssetMenuBar").IgnoreResult();
    xiiStandardMenus::MapActions("PropertyAnimAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Scene | xiiStandardMenuTypes::View | xiiStandardMenuTypes::Help);
    xiiProjectActions::MapActions("PropertyAnimAssetMenuBar");
    xiiDocumentActions::MapActions("PropertyAnimAssetMenuBar", "Menu.File", false);
    xiiAssetActions::MapMenuActions("PropertyAnimAssetMenuBar", "Menu.File");
    xiiDocumentActions::MapToolsActions("PropertyAnimAssetMenuBar", "Menu.Tools");
    xiiCommandHistoryActions::MapActions("PropertyAnimAssetMenuBar", "Menu.Edit");
    xiiGameObjectSelectionActions::MapActions("PropertyAnimAssetMenuBar", "Menu.Edit");
    xiiGameObjectDocumentActions::MapMenuActions("PropertyAnimAssetMenuBar", "Menu.View");
    xiiGameObjectDocumentActions::MapMenuSimulationSpeed("PropertyAnimAssetMenuBar", "Menu.Scene");
    xiiTransformGizmoActions::MapMenuActions("PropertyAnimAssetMenuBar", "Menu.Edit");
    xiiTranslateGizmoAction::MapActions("PropertyAnimAssetMenuBar", "Menu.Edit/Gizmo.Menu");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("PropertyAnimAssetToolBar").IgnoreResult();
    xiiDocumentActions::MapActions("PropertyAnimAssetToolBar", "", true);
    xiiCommandHistoryActions::MapActions("PropertyAnimAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("PropertyAnimAssetToolBar", true);
    xiiGameObjectContextActions::MapToolbarActions("PropertyAnimAssetToolBar", "");
    xiiGameObjectDocumentActions::MapToolbarActions("PropertyAnimAssetToolBar", "");
    xiiTransformGizmoActions::MapToolbarActions("PropertyAnimAssetToolBar", "");
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("PropertyAnimAssetViewToolBar").IgnoreResult();
    xiiViewActions::MapActions("PropertyAnimAssetViewToolBar", "", xiiViewActions::PerspectiveMode | xiiViewActions::RenderMode | xiiViewActions::ActivateRemoteProcess);
    xiiQuadViewActions::MapActions("PropertyAnimAssetViewToolBar", "");
  }

  // SceneGraph Context Menu
  {
    xiiActionMapManager::RegisterActionMap("PropertyAnimAsset_ScenegraphContextMenu").IgnoreResult();
    xiiGameObjectSelectionActions::MapContextMenuActions("PropertyAnimAsset_ScenegraphContextMenu", "");
    xiiGameObjectContextActions::MapContextMenuActions("PropertyAnimAsset_ScenegraphContextMenu", "");
  }
}

static void ConfigureVisualScriptAsset()
{
  xiiVisualScriptActions::RegisterActions();

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("VisualScriptAssetMenuBar_Legacy").IgnoreResult();
    xiiStandardMenus::MapActions("VisualScriptAssetMenuBar_Legacy", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
    xiiProjectActions::MapActions("VisualScriptAssetMenuBar_Legacy");
    xiiDocumentActions::MapActions("VisualScriptAssetMenuBar_Legacy", "Menu.File", false);
    xiiAssetActions::MapMenuActions("VisualScriptAssetMenuBar_Legacy", "Menu.File");
    xiiCommandHistoryActions::MapActions("VisualScriptAssetMenuBar_Legacy", "Menu.Edit");
    xiiEditActions::MapActions("VisualScriptAssetMenuBar_Legacy", "Menu.Edit", false, false);
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("VisualScriptAssetToolBar_Legacy").IgnoreResult();
    xiiDocumentActions::MapActions("VisualScriptAssetToolBar_Legacy", "", true);
    xiiCommandHistoryActions::MapActions("VisualScriptAssetToolBar_Legacy", "");
    xiiAssetActions::MapToolBarActions("VisualScriptAssetToolBar_Legacy", true);
    xiiVisualScriptActions::MapActions("VisualScriptAssetToolBar_Legacy", "");
  }
}

static void ConfigureDecalAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiDecalAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("DecalAssetMenuBar").IgnoreResult();
    xiiStandardMenus::MapActions("DecalAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
    xiiProjectActions::MapActions("DecalAssetMenuBar");
    xiiDocumentActions::MapActions("DecalAssetMenuBar", "Menu.File", false);
    xiiAssetActions::MapMenuActions("DecalAssetMenuBar", "Menu.File");
    xiiCommandHistoryActions::MapActions("DecalAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("DecalAssetToolBar").IgnoreResult();
    xiiDocumentActions::MapActions("DecalAssetToolBar", "", true);
    xiiCommandHistoryActions::MapActions("DecalAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("DecalAssetToolBar", true);
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("DecalAssetViewToolBar").IgnoreResult();
    xiiViewActions::MapActions("DecalAssetViewToolBar", "", xiiViewActions::RenderMode | xiiViewActions::ActivateRemoteProcess);
    xiiViewLightActions::MapActions("DecalAssetViewToolBar", "");
  }
}

static void ConfigureAnimationClipAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiAnimationClipAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("AnimationClipAssetMenuBar").IgnoreResult();
    xiiStandardMenus::MapActions("AnimationClipAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
    xiiProjectActions::MapActions("AnimationClipAssetMenuBar");
    xiiDocumentActions::MapActions("AnimationClipAssetMenuBar", "Menu.File", false);
    xiiAssetActions::MapMenuActions("AnimationClipAssetMenuBar", "Menu.File");
    xiiCommandHistoryActions::MapActions("AnimationClipAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("AnimationClipAssetToolBar").IgnoreResult();
    xiiDocumentActions::MapActions("AnimationClipAssetToolBar", "", true);
    xiiCommandHistoryActions::MapActions("AnimationClipAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("AnimationClipAssetToolBar", true);
    xiiCommonAssetActions::MapActions("AnimationClipAssetToolBar", "", xiiCommonAssetUiState::Loop | xiiCommonAssetUiState::Pause | xiiCommonAssetUiState::Restart | xiiCommonAssetUiState::SimulationSpeed | xiiCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("AnimationClipAssetViewToolBar").IgnoreResult();
    xiiViewActions::MapActions("AnimationClipAssetViewToolBar", "", xiiViewActions::RenderMode | xiiViewActions::ActivateRemoteProcess);
    xiiViewLightActions::MapActions("AnimationClipAssetViewToolBar", "");
  }
}

static void ConfigureSkeletonAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiSkeletonAssetDocument::PropertyMetaStateEventHandler);

  xiiSkeletonActions::RegisterActions();

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("SkeletonAssetMenuBar").IgnoreResult();
    xiiStandardMenus::MapActions("SkeletonAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
    xiiProjectActions::MapActions("SkeletonAssetMenuBar");
    xiiDocumentActions::MapActions("SkeletonAssetMenuBar", "Menu.File", false);
    xiiAssetActions::MapMenuActions("SkeletonAssetMenuBar", "Menu.File");
    xiiCommandHistoryActions::MapActions("SkeletonAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("SkeletonAssetToolBar").IgnoreResult();
    xiiDocumentActions::MapActions("SkeletonAssetToolBar", "", true);
    xiiCommandHistoryActions::MapActions("SkeletonAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("SkeletonAssetToolBar", true);
    xiiCommonAssetActions::MapActions("SkeletonAssetToolBar", "", xiiCommonAssetUiState::Grid);
    xiiSkeletonActions::MapActions("SkeletonAssetToolBar", "");
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("SkeletonAssetViewToolBar").IgnoreResult();
    xiiViewActions::MapActions("SkeletonAssetViewToolBar", "", xiiViewActions::RenderMode | xiiViewActions::ActivateRemoteProcess);
    xiiViewLightActions::MapActions("SkeletonAssetViewToolBar", "");
  }
}

static void ConfigureAnimatedMeshAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("AnimatedMeshAssetMenuBar").IgnoreResult();
    xiiStandardMenus::MapActions("AnimatedMeshAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
    xiiProjectActions::MapActions("AnimatedMeshAssetMenuBar");
    xiiDocumentActions::MapActions("AnimatedMeshAssetMenuBar", "Menu.File", false);
    xiiAssetActions::MapMenuActions("AnimatedMeshAssetMenuBar", "Menu.File");
    xiiCommandHistoryActions::MapActions("AnimatedMeshAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("AnimatedMeshAssetToolBar").IgnoreResult();
    xiiDocumentActions::MapActions("AnimatedMeshAssetToolBar", "", true);
    xiiCommandHistoryActions::MapActions("AnimatedMeshAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("AnimatedMeshAssetToolBar", true);
    xiiCommonAssetActions::MapActions("AnimatedMeshAssetToolBar", "", xiiCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("AnimatedMeshAssetViewToolBar").IgnoreResult();
    xiiViewActions::MapActions("AnimatedMeshAssetViewToolBar", "", xiiViewActions::RenderMode | xiiViewActions::ActivateRemoteProcess);
    xiiViewLightActions::MapActions("AnimatedMeshAssetViewToolBar", "");
  }
}

static void ConfigureImageDataAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("ImageDataAssetMenuBar").IgnoreResult();
    xiiStandardMenus::MapActions("ImageDataAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
    xiiProjectActions::MapActions("ImageDataAssetMenuBar");
    xiiDocumentActions::MapActions("ImageDataAssetMenuBar", "Menu.File", false);
    xiiAssetActions::MapMenuActions("ImageDataAssetMenuBar", "Menu.File");
    xiiCommandHistoryActions::MapActions("ImageDataAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("ImageDataAssetToolBar").IgnoreResult();
    xiiDocumentActions::MapActions("ImageDataAssetToolBar", "", true);
    xiiCommandHistoryActions::MapActions("ImageDataAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("ImageDataAssetToolBar", true);
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("ImageDataAssetViewToolBar").IgnoreResult();
    xiiViewActions::MapActions("ImageDataAssetViewToolBar", "", xiiViewActions::RenderMode | xiiViewActions::ActivateRemoteProcess);
    xiiViewLightActions::MapActions("ImageDataAssetViewToolBar", "");
  }
}

static void ConfigureStateMachineAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("StateMachineAssetMenuBar").IgnoreResult();

    xiiStandardMenus::MapActions("StateMachineAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
    xiiProjectActions::MapActions("StateMachineAssetMenuBar");
    xiiDocumentActions::MapActions("StateMachineAssetMenuBar", "Menu.File", false);
    xiiAssetActions::MapMenuActions("StateMachineAssetMenuBar", "Menu.File");
    xiiCommandHistoryActions::MapActions("StateMachineAssetMenuBar", "Menu.Edit");
    xiiEditActions::MapActions("StateMachineAssetMenuBar", "Menu.Edit", false, false);
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("StateMachineAssetToolBar").IgnoreResult();
    xiiDocumentActions::MapActions("StateMachineAssetToolBar", "", true);
    xiiCommandHistoryActions::MapActions("StateMachineAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("StateMachineAssetToolBar", true);
  }
}

xiiVariant CustomAction_CreateShaderFromTemplate(const xiiDocument* pDoc)
{
  xiiQtShaderTemplateDlg dlg(nullptr, pDoc);

  if (dlg.exec() == QDialog::Accepted)
  {
    xiiStringBuilder abs;
    if (xiiFileSystem::ResolvePath(dlg.m_sResult, &abs, nullptr).Succeeded())
    {
      if (!xiiQtUiServices::GetSingleton()->OpenFileInDefaultProgram(abs))
      {
        xiiQtUiServices::GetSingleton()->MessageBoxInformation(xiiFmt("There is no default program set to open shader files:\n\n{}", abs));
      }
    }

    return dlg.m_sResult;
  }

  return {};
}

void OnLoadPlugin()
{
  ConfigureAnimationControllerAsset();
  ConfigureTexture2DAsset();
  ConfigureTextureCubeAsset();
  ConfigureLUTAsset();
  ConfigureMaterialAsset();
  ConfigureRenderPipelineAsset();
  ConfigureMeshAsset();
  ConfigureSurfaceAsset();
  ConfigureCollectionAsset();
  ConfigureColorGradientAsset();
  ConfigureCurve1DAsset();
  ConfigurePropertyAnimAsset();
  ConfigureVisualScriptAsset();
  ConfigureDecalAsset();
  ConfigureAnimationClipAsset();
  ConfigureSkeletonAsset();
  ConfigureAnimatedMeshAsset();
  ConfigureImageDataAsset();
  ConfigureStateMachineAsset();

  xiiDocumentManager::s_CustomActions["CustomAction_CreateShaderFromTemplate"] = CustomAction_CreateShaderFromTemplate;
}

void OnUnloadPlugin()
{
  xiiTextureAssetActions::UnregisterActions();
  xiiTextureCubeAssetActions::UnregisterActions();
  xiiLUTAssetActions::UnregisterActions();
  xiiVisualShaderActions::UnregisterActions();
  xiiVisualScriptActions::UnregisterActions();
  xiiMaterialAssetActions::UnregisterActions();
  xiiSkeletonActions::UnregisterActions();

  xiiPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(xiiMeshAssetProperties::PropertyMetaStateEventHandler);
  xiiPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(xiiTextureAssetProperties::PropertyMetaStateEventHandler);
  xiiPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(xiiDecalAssetProperties::PropertyMetaStateEventHandler);
  xiiPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(xiiTextureCubeAssetProperties::PropertyMetaStateEventHandler);
  xiiPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(xiiMaterialAssetProperties::PropertyMetaStateEventHandler);
  xiiPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(xiiSkeletonAssetDocument::PropertyMetaStateEventHandler);
  xiiPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(xiiAnimationClipAssetProperties::PropertyMetaStateEventHandler);
}

XII_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

XII_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
