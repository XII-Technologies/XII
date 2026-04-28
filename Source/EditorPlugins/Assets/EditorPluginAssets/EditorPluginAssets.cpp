/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetObjects.h>
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
#include <EditorPluginAssets/VisualShader/VisualShaderActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

static void ConfigureAnimationGraphAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("AnimationGraphAssetMenuBar", "AssetMenuBar");
    xiiEditActions::MapActions("AnimationGraphAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("AnimationGraphAssetToolBar", "AssetToolbar");
  }
}

static void ConfigureTexture2DAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiTextureAssetProperties::PropertyMetaStateEventHandler);

  xiiTextureAssetActions::RegisterActions();

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("TextureAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("TextureAssetToolBar", "AssetToolbar");
    xiiTextureAssetActions::MapToolbarActions("TextureAssetToolBar");
  }
}

static void ConfigureTextureCubeAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiTextureCubeAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("TextureCubeAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("TextureCubeAssetToolBar", "AssetToolbar");
    xiiTextureAssetActions::MapToolbarActions("TextureCubeAssetToolBar");
  }
}

static void ConfigureLUTAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiLUTAssetProperties::PropertyMetaStateEventHandler);

  xiiLUTAssetActions::RegisterActions();

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("LUTAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("LUTAssetToolBar", "AssetToolbar");
  }
}

static void ConfigureMaterialAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiMaterialAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("MaterialAssetMenuBar", "AssetMenuBar");
    xiiDocumentActions::MapToolsActions("MaterialAssetMenuBar");
    xiiEditActions::MapActions("MaterialAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("MaterialAssetToolBar", "AssetToolbar");

    xiiMaterialAssetActions::RegisterActions();
    xiiMaterialAssetActions::MapToolbarActions("MaterialAssetToolBar");

    xiiVisualShaderActions::RegisterActions();
    xiiVisualShaderActions::MapActions("MaterialAssetToolBar");
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("MaterialAssetViewToolBar", "SimpleAssetViewToolbar");
  }
}

static void ConfigureRenderPipelineAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("RenderPipelineAssetMenuBar", "AssetMenuBar");
    xiiEditActions::MapActions("RenderPipelineAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("RenderPipelineAssetToolBar", "AssetToolbar");
  }
}

static void ConfigureMeshAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiMeshAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("MeshAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("MeshAssetToolBar", "AssetToolbar");
    xiiCommonAssetActions::MapToolbarActions("MeshAssetToolBar", xiiCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("MeshAssetViewToolBar", "SimpleAssetViewToolbar");
  }
}

static void ConfigureSurfaceAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("SurfaceAssetMenuBar", "AssetMenuBar");
    xiiDocumentActions::MapToolsActions("SurfaceAssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("SurfaceAssetToolBar", "AssetToolbar");
  }
}

static void ConfigureCollectionAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("CollectionAssetMenuBar", "AssetMenuBar");
    xiiDocumentActions::MapToolsActions("CollectionAssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("CollectionAssetToolBar", "AssetToolbar");
  }
}

static void ConfigureColorGradientAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("ColorGradientAssetMenuBar", "AssetMenuBar");
    xiiDocumentActions::MapToolsActions("ColorGradientAssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("ColorGradientAssetToolBar", "AssetToolbar");
  }
}

static void ConfigureCurve1DAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("Curve1DAssetMenuBar", "AssetMenuBar");
    xiiDocumentActions::MapToolsActions("Curve1DAssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("Curve1DAssetToolBar", "AssetToolbar");
  }
}

static void ConfigurePropertyAnimAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("PropertyAnimAssetMenuBar", "AssetMenuBar");
    xiiStandardMenus::MapActions("PropertyAnimAssetMenuBar", xiiStandardMenuTypes::Scene | xiiStandardMenuTypes::View);
    xiiDocumentActions::MapToolsActions("PropertyAnimAssetMenuBar");
    xiiGameObjectSelectionActions::MapActions("PropertyAnimAssetMenuBar");
    xiiGameObjectDocumentActions::MapMenuActions("PropertyAnimAssetMenuBar");
    xiiGameObjectDocumentActions::MapMenuSimulationSpeed("PropertyAnimAssetMenuBar");
    xiiTransformGizmoActions::MapMenuActions("PropertyAnimAssetMenuBar");
    xiiTranslateGizmoAction::MapActions("PropertyAnimAssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("PropertyAnimAssetToolBar", "AssetToolbar");
    xiiGameObjectContextActions::MapToolbarActions("PropertyAnimAssetToolBar");
    xiiGameObjectDocumentActions::MapToolbarActions("PropertyAnimAssetToolBar");
    xiiTransformGizmoActions::MapToolbarActions("PropertyAnimAssetToolBar");
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("PropertyAnimAssetViewToolBar", "AssetViewToolbar");
    xiiViewActions::MapToolbarActions("PropertyAnimAssetViewToolBar", xiiViewActions::PerspectiveMode | xiiViewActions::RenderMode | xiiViewActions::ActivateRemoteProcess);
    xiiQuadViewActions::MapToolbarActions("PropertyAnimAssetViewToolBar");
  }

  // SceneGraph Context Menu
  {
    xiiActionMapManager::RegisterActionMap("PropertyAnimAsset_ScenegraphContextMenu");
    xiiGameObjectSelectionActions::MapContextMenuActions("PropertyAnimAsset_ScenegraphContextMenu");
    xiiGameObjectContextActions::MapContextMenuActions("PropertyAnimAsset_ScenegraphContextMenu");
  }
}

static void ConfigureDecalAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiDecalAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("DecalAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("DecalAssetToolBar", "AssetToolbar");
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("DecalAssetViewToolBar", "SimpleAssetViewToolbar");
  }
}

static void ConfigureAnimationClipAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiAnimationClipAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("AnimationClipAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("AnimationClipAssetToolBar", "AssetToolbar");
    xiiCommonAssetActions::MapToolbarActions("AnimationClipAssetToolBar", xiiCommonAssetUiState::Loop | xiiCommonAssetUiState::Pause | xiiCommonAssetUiState::Restart | xiiCommonAssetUiState::SimulationSpeed | xiiCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("AnimationClipAssetViewToolBar", "SimpleAssetViewToolbar");
  }
}

static void ConfigureSkeletonAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiSkeletonAssetDocument::PropertyMetaStateEventHandler);

  xiiSkeletonActions::RegisterActions();

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("SkeletonAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("SkeletonAssetToolBar", "AssetToolbar");
    xiiCommonAssetActions::MapToolbarActions("SkeletonAssetToolBar", xiiCommonAssetUiState::Grid);
    xiiSkeletonActions::MapActions("SkeletonAssetToolBar");
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("SkeletonAssetViewToolBar", "SimpleAssetViewToolbar");
  }
}

static void ConfigureAnimatedMeshAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiAnimatedMeshAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("AnimatedMeshAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("AnimatedMeshAssetToolBar", "AssetToolbar");
    xiiCommonAssetActions::MapToolbarActions("AnimatedMeshAssetToolBar", xiiCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("AnimatedMeshAssetViewToolBar", "SimpleAssetViewToolbar");
  }
}

static void ConfigureImageDataAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("ImageDataAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("ImageDataAssetToolBar", "AssetToolbar");
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("ImageDataAssetViewToolBar", "SimpleAssetViewToolbar");
  }
}

static void ConfigureStateMachineAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("StateMachineAssetMenuBar", "AssetMenuBar");
    xiiEditActions::MapActions("StateMachineAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("StateMachineAssetToolBar", "AssetToolbar");
  }
}
static void ConfigureBlackboardTemplateAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("BlackboardTemplateAssetMenuBar", "AssetMenuBar");
    xiiEditActions::MapActions("BlackboardTemplateAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("BlackboardTemplateAssetToolBar", "AssetToolbar");
  }
}

static void ConfigureCustomDataAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("CustomDataAssetMenuBar", "AssetMenuBar");
    xiiDocumentActions::MapToolsActions("CustomDataAssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("CustomDataAssetToolBar", "AssetToolbar");
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
  ConfigureAnimationGraphAsset();
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
  ConfigureDecalAsset();
  ConfigureAnimationClipAsset();
  ConfigureSkeletonAsset();
  ConfigureAnimatedMeshAsset();
  ConfigureImageDataAsset();
  ConfigureStateMachineAsset();
  ConfigureBlackboardTemplateAsset();
  ConfigureCustomDataAsset();

  xiiDocumentManager::s_CustomActions["CustomAction_CreateShaderFromTemplate"] = CustomAction_CreateShaderFromTemplate;
}

void OnUnloadPlugin()
{
  xiiTextureAssetActions::UnregisterActions();
  xiiLUTAssetActions::UnregisterActions();
  xiiVisualShaderActions::UnregisterActions();
  xiiMaterialAssetActions::UnregisterActions();
  xiiSkeletonActions::UnregisterActions();

  xiiPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(xiiAnimatedMeshAssetProperties::PropertyMetaStateEventHandler);
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
