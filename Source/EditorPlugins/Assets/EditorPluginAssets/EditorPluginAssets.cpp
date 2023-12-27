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
    xiiActionMapManager::RegisterActionMap("AnimationGraphAssetMenuBar").AssertSuccess();

    xiiStandardMenus::MapActions("AnimationGraphAssetMenuBar", xiiStandardMenuTypes::Default | xiiStandardMenuTypes::Edit);
    xiiProjectActions::MapActions("AnimationGraphAssetMenuBar");
    xiiDocumentActions::MapMenuActions("AnimationGraphAssetMenuBar");
    xiiAssetActions::MapMenuActions("AnimationGraphAssetMenuBar");
    xiiCommandHistoryActions::MapActions("AnimationGraphAssetMenuBar");
    xiiEditActions::MapActions("AnimationGraphAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("AnimationGraphAssetToolBar").AssertSuccess();
    xiiDocumentActions::MapToolbarActions("AnimationGraphAssetToolBar");
    xiiCommandHistoryActions::MapActions("AnimationGraphAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("AnimationGraphAssetToolBar", true);
  }
}

static void ConfigureTexture2DAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiTextureAssetProperties::PropertyMetaStateEventHandler);

  xiiTextureAssetActions::RegisterActions();

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("TextureAssetMenuBar").AssertSuccess();
    xiiStandardMenus::MapActions("TextureAssetMenuBar", xiiStandardMenuTypes::Default | xiiStandardMenuTypes::Edit);
    xiiProjectActions::MapActions("TextureAssetMenuBar");
    xiiDocumentActions::MapMenuActions("TextureAssetMenuBar");
    xiiAssetActions::MapMenuActions("TextureAssetMenuBar");
    xiiCommandHistoryActions::MapActions("TextureAssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("TextureAssetToolBar").AssertSuccess();
    xiiDocumentActions::MapToolbarActions("TextureAssetToolBar");
    xiiCommandHistoryActions::MapActions("TextureAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("TextureAssetToolBar", true);
    xiiTextureAssetActions::MapToolbarActions("TextureAssetToolBar");
  }
}

static void ConfigureTextureCubeAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiTextureCubeAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("TextureCubeAssetMenuBar").AssertSuccess();
    xiiStandardMenus::MapActions("TextureCubeAssetMenuBar", xiiStandardMenuTypes::Default | xiiStandardMenuTypes::Edit);
    xiiProjectActions::MapActions("TextureCubeAssetMenuBar");
    xiiDocumentActions::MapMenuActions("TextureCubeAssetMenuBar");
    xiiAssetActions::MapMenuActions("TextureCubeAssetMenuBar");
    xiiCommandHistoryActions::MapActions("TextureCubeAssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("TextureCubeAssetToolBar").AssertSuccess();
    xiiDocumentActions::MapToolbarActions("TextureCubeAssetToolBar");
    xiiCommandHistoryActions::MapActions("TextureCubeAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("TextureCubeAssetToolBar", true);
    xiiTextureAssetActions::MapToolbarActions("TextureCubeAssetToolBar");
  }
}

static void ConfigureLUTAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiLUTAssetProperties::PropertyMetaStateEventHandler);

  xiiLUTAssetActions::RegisterActions();

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("LUTAssetMenuBar").AssertSuccess();
    xiiStandardMenus::MapActions("LUTAssetMenuBar", xiiStandardMenuTypes::Default | xiiStandardMenuTypes::Edit);
    xiiProjectActions::MapActions("LUTAssetMenuBar");
    xiiDocumentActions::MapMenuActions("LUTAssetMenuBar");
    xiiAssetActions::MapMenuActions("LUTAssetMenuBar");
    xiiCommandHistoryActions::MapActions("LUTAssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("LUTAssetToolBar").AssertSuccess();
    xiiDocumentActions::MapToolbarActions("LUTAssetToolBar");
    xiiCommandHistoryActions::MapActions("LUTAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("LUTAssetToolBar", true);
  }
}

static void ConfigureMaterialAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiMaterialAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("MaterialAssetMenuBar").AssertSuccess();
    xiiStandardMenus::MapActions("MaterialAssetMenuBar", xiiStandardMenuTypes::Default | xiiStandardMenuTypes::Edit);
    xiiProjectActions::MapActions("MaterialAssetMenuBar");
    xiiDocumentActions::MapMenuActions("MaterialAssetMenuBar");
    xiiDocumentActions::MapToolsActions("MaterialAssetMenuBar");
    xiiAssetActions::MapMenuActions("MaterialAssetMenuBar");
    xiiCommandHistoryActions::MapActions("MaterialAssetMenuBar");
    xiiEditActions::MapActions("MaterialAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("MaterialAssetToolBar").AssertSuccess();
    xiiDocumentActions::MapToolbarActions("MaterialAssetToolBar");
    xiiCommandHistoryActions::MapActions("MaterialAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("MaterialAssetToolBar", true);

    xiiMaterialAssetActions::RegisterActions();
    xiiMaterialAssetActions::MapToolbarActions("MaterialAssetToolBar");

    xiiVisualShaderActions::RegisterActions();
    xiiVisualShaderActions::MapActions("MaterialAssetToolBar");
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("MaterialAssetViewToolBar").AssertSuccess();
    xiiViewActions::MapToolbarActions("MaterialAssetViewToolBar", xiiViewActions::RenderMode | xiiViewActions::ActivateRemoteProcess);
    xiiViewLightActions::MapToolbarActions("MaterialAssetViewToolBar");
  }
}

static void ConfigureRenderPipelineAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("RenderPipelineAssetMenuBar").AssertSuccess();

    xiiStandardMenus::MapActions("RenderPipelineAssetMenuBar", xiiStandardMenuTypes::Default | xiiStandardMenuTypes::Edit);
    xiiProjectActions::MapActions("RenderPipelineAssetMenuBar");
    xiiDocumentActions::MapMenuActions("RenderPipelineAssetMenuBar");
    xiiAssetActions::MapMenuActions("RenderPipelineAssetMenuBar");
    xiiCommandHistoryActions::MapActions("RenderPipelineAssetMenuBar");
    xiiEditActions::MapActions("RenderPipelineAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("RenderPipelineAssetToolBar").AssertSuccess();
    xiiDocumentActions::MapToolbarActions("RenderPipelineAssetToolBar");
    xiiCommandHistoryActions::MapActions("RenderPipelineAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("RenderPipelineAssetToolBar", true);
  }
}

static void ConfigureMeshAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiMeshAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("MeshAssetMenuBar").AssertSuccess();
    xiiStandardMenus::MapActions("MeshAssetMenuBar", xiiStandardMenuTypes::Default | xiiStandardMenuTypes::Edit);
    xiiProjectActions::MapActions("MeshAssetMenuBar");
    xiiDocumentActions::MapMenuActions("MeshAssetMenuBar");
    xiiAssetActions::MapMenuActions("MeshAssetMenuBar");
    xiiCommandHistoryActions::MapActions("MeshAssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("MeshAssetToolBar").AssertSuccess();
    xiiDocumentActions::MapToolbarActions("MeshAssetToolBar");
    xiiCommandHistoryActions::MapActions("MeshAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("MeshAssetToolBar", true);
    xiiCommonAssetActions::MapToolbarActions("MeshAssetToolBar", xiiCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("MeshAssetViewToolBar").AssertSuccess();
    xiiViewActions::MapToolbarActions("MeshAssetViewToolBar", xiiViewActions::RenderMode | xiiViewActions::ActivateRemoteProcess);
    xiiViewLightActions::MapToolbarActions("MeshAssetViewToolBar");
  }
}

static void ConfigureSurfaceAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("SurfaceAssetMenuBar").AssertSuccess();
    xiiStandardMenus::MapActions("SurfaceAssetMenuBar", xiiStandardMenuTypes::Default | xiiStandardMenuTypes::Edit);
    xiiProjectActions::MapActions("SurfaceAssetMenuBar");
    xiiDocumentActions::MapMenuActions("SurfaceAssetMenuBar");
    xiiAssetActions::MapMenuActions("SurfaceAssetMenuBar");
    xiiDocumentActions::MapToolsActions("SurfaceAssetMenuBar");
    xiiCommandHistoryActions::MapActions("SurfaceAssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("SurfaceAssetToolBar").AssertSuccess();
    xiiDocumentActions::MapToolbarActions("SurfaceAssetToolBar");
    xiiCommandHistoryActions::MapActions("SurfaceAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("SurfaceAssetToolBar", true);
  }
}

static void ConfigureCollectionAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("CollectionAssetMenuBar").AssertSuccess();
    xiiStandardMenus::MapActions("CollectionAssetMenuBar", xiiStandardMenuTypes::Default | xiiStandardMenuTypes::Edit);
    xiiProjectActions::MapActions("CollectionAssetMenuBar");
    xiiDocumentActions::MapMenuActions("CollectionAssetMenuBar");
    xiiAssetActions::MapMenuActions("CollectionAssetMenuBar");
    xiiDocumentActions::MapToolsActions("CollectionAssetMenuBar");
    xiiCommandHistoryActions::MapActions("CollectionAssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("CollectionAssetToolBar").AssertSuccess();
    xiiDocumentActions::MapToolbarActions("CollectionAssetToolBar");
    xiiCommandHistoryActions::MapActions("CollectionAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("CollectionAssetToolBar", true);
  }
}

static void ConfigureColorGradientAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("ColorGradientAssetMenuBar").AssertSuccess();
    xiiStandardMenus::MapActions("ColorGradientAssetMenuBar", xiiStandardMenuTypes::Default | xiiStandardMenuTypes::Edit);
    xiiProjectActions::MapActions("ColorGradientAssetMenuBar");
    xiiDocumentActions::MapMenuActions("ColorGradientAssetMenuBar");
    xiiAssetActions::MapMenuActions("ColorGradientAssetMenuBar");
    xiiDocumentActions::MapToolsActions("ColorGradientAssetMenuBar");
    xiiCommandHistoryActions::MapActions("ColorGradientAssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("ColorGradientAssetToolBar").AssertSuccess();
    xiiDocumentActions::MapToolbarActions("ColorGradientAssetToolBar");
    xiiCommandHistoryActions::MapActions("ColorGradientAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("ColorGradientAssetToolBar", true);
  }
}

static void ConfigureCurve1DAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("Curve1DAssetMenuBar").AssertSuccess();
    xiiStandardMenus::MapActions("Curve1DAssetMenuBar", xiiStandardMenuTypes::Default | xiiStandardMenuTypes::Edit);
    xiiProjectActions::MapActions("Curve1DAssetMenuBar");
    xiiDocumentActions::MapMenuActions("Curve1DAssetMenuBar");
    xiiAssetActions::MapMenuActions("Curve1DAssetMenuBar");
    xiiDocumentActions::MapToolsActions("Curve1DAssetMenuBar");
    xiiCommandHistoryActions::MapActions("Curve1DAssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("Curve1DAssetToolBar").AssertSuccess();
    xiiDocumentActions::MapToolbarActions("Curve1DAssetToolBar");
    xiiCommandHistoryActions::MapActions("Curve1DAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("Curve1DAssetToolBar", true);
  }
}

static void ConfigurePropertyAnimAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("PropertyAnimAssetMenuBar").AssertSuccess();
    xiiStandardMenus::MapActions("PropertyAnimAssetMenuBar", xiiStandardMenuTypes::Default | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Scene | xiiStandardMenuTypes::View);
    xiiProjectActions::MapActions("PropertyAnimAssetMenuBar");
    xiiDocumentActions::MapMenuActions("PropertyAnimAssetMenuBar");
    xiiAssetActions::MapMenuActions("PropertyAnimAssetMenuBar");
    xiiDocumentActions::MapToolsActions("PropertyAnimAssetMenuBar");
    xiiCommandHistoryActions::MapActions("PropertyAnimAssetMenuBar");
    xiiGameObjectSelectionActions::MapActions("PropertyAnimAssetMenuBar");
    xiiGameObjectDocumentActions::MapMenuActions("PropertyAnimAssetMenuBar");
    xiiGameObjectDocumentActions::MapMenuSimulationSpeed("PropertyAnimAssetMenuBar");
    xiiTransformGizmoActions::MapMenuActions("PropertyAnimAssetMenuBar");
    xiiTranslateGizmoAction::MapActions("PropertyAnimAssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("PropertyAnimAssetToolBar").AssertSuccess();
    xiiDocumentActions::MapToolbarActions("PropertyAnimAssetToolBar");
    xiiCommandHistoryActions::MapActions("PropertyAnimAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("PropertyAnimAssetToolBar", true);
    xiiGameObjectContextActions::MapToolbarActions("PropertyAnimAssetToolBar");
    xiiGameObjectDocumentActions::MapToolbarActions("PropertyAnimAssetToolBar");
    xiiTransformGizmoActions::MapToolbarActions("PropertyAnimAssetToolBar");
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("PropertyAnimAssetViewToolBar").AssertSuccess();
    xiiViewActions::MapToolbarActions("PropertyAnimAssetViewToolBar", xiiViewActions::PerspectiveMode | xiiViewActions::RenderMode | xiiViewActions::ActivateRemoteProcess);
    xiiQuadViewActions::MapToolbarActions("PropertyAnimAssetViewToolBar");
  }

  // SceneGraph Context Menu
  {
    xiiActionMapManager::RegisterActionMap("PropertyAnimAsset_ScenegraphContextMenu").AssertSuccess();
    xiiGameObjectSelectionActions::MapContextMenuActions("PropertyAnimAsset_ScenegraphContextMenu");
    xiiGameObjectContextActions::MapContextMenuActions("PropertyAnimAsset_ScenegraphContextMenu");
  }
}

static void ConfigureDecalAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiDecalAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("DecalAssetMenuBar").AssertSuccess();
    xiiStandardMenus::MapActions("DecalAssetMenuBar", xiiStandardMenuTypes::Default | xiiStandardMenuTypes::Edit);
    xiiProjectActions::MapActions("DecalAssetMenuBar");
    xiiDocumentActions::MapMenuActions("DecalAssetMenuBar");
    xiiAssetActions::MapMenuActions("DecalAssetMenuBar");
    xiiCommandHistoryActions::MapActions("DecalAssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("DecalAssetToolBar").AssertSuccess();
    xiiDocumentActions::MapToolbarActions("DecalAssetToolBar");
    xiiCommandHistoryActions::MapActions("DecalAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("DecalAssetToolBar", true);
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("DecalAssetViewToolBar").AssertSuccess();
    xiiViewActions::MapToolbarActions("DecalAssetViewToolBar", xiiViewActions::RenderMode | xiiViewActions::ActivateRemoteProcess);
    xiiViewLightActions::MapToolbarActions("DecalAssetViewToolBar");
  }
}

static void ConfigureAnimationClipAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiAnimationClipAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("AnimationClipAssetMenuBar").AssertSuccess();
    xiiStandardMenus::MapActions("AnimationClipAssetMenuBar", xiiStandardMenuTypes::Default | xiiStandardMenuTypes::Edit);
    xiiProjectActions::MapActions("AnimationClipAssetMenuBar");
    xiiDocumentActions::MapMenuActions("AnimationClipAssetMenuBar");
    xiiAssetActions::MapMenuActions("AnimationClipAssetMenuBar");
    xiiCommandHistoryActions::MapActions("AnimationClipAssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("AnimationClipAssetToolBar").AssertSuccess();
    xiiDocumentActions::MapToolbarActions("AnimationClipAssetToolBar");
    xiiCommandHistoryActions::MapActions("AnimationClipAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("AnimationClipAssetToolBar", true);
    xiiCommonAssetActions::MapToolbarActions("AnimationClipAssetToolBar", xiiCommonAssetUiState::Loop | xiiCommonAssetUiState::Pause | xiiCommonAssetUiState::Restart | xiiCommonAssetUiState::SimulationSpeed | xiiCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("AnimationClipAssetViewToolBar").AssertSuccess();
    xiiViewActions::MapToolbarActions("AnimationClipAssetViewToolBar", xiiViewActions::RenderMode | xiiViewActions::ActivateRemoteProcess);
    xiiViewLightActions::MapToolbarActions("AnimationClipAssetViewToolBar");
  }
}

static void ConfigureSkeletonAsset()
{
  xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiSkeletonAssetDocument::PropertyMetaStateEventHandler);

  xiiSkeletonActions::RegisterActions();

  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("SkeletonAssetMenuBar").AssertSuccess();
    xiiStandardMenus::MapActions("SkeletonAssetMenuBar", xiiStandardMenuTypes::Default | xiiStandardMenuTypes::Edit);
    xiiProjectActions::MapActions("SkeletonAssetMenuBar");
    xiiDocumentActions::MapMenuActions("SkeletonAssetMenuBar");
    xiiAssetActions::MapMenuActions("SkeletonAssetMenuBar");
    xiiCommandHistoryActions::MapActions("SkeletonAssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("SkeletonAssetToolBar").AssertSuccess();
    xiiDocumentActions::MapToolbarActions("SkeletonAssetToolBar");
    xiiCommandHistoryActions::MapActions("SkeletonAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("SkeletonAssetToolBar", true);
    xiiCommonAssetActions::MapToolbarActions("SkeletonAssetToolBar", xiiCommonAssetUiState::Grid);
    xiiSkeletonActions::MapActions("SkeletonAssetToolBar");
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("SkeletonAssetViewToolBar").AssertSuccess();
    xiiViewActions::MapToolbarActions("SkeletonAssetViewToolBar", xiiViewActions::RenderMode | xiiViewActions::ActivateRemoteProcess);
    xiiViewLightActions::MapToolbarActions("SkeletonAssetViewToolBar");
  }
}

static void ConfigureAnimatedMeshAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("AnimatedMeshAssetMenuBar").AssertSuccess();
    xiiStandardMenus::MapActions("AnimatedMeshAssetMenuBar", xiiStandardMenuTypes::Default | xiiStandardMenuTypes::Edit);
    xiiProjectActions::MapActions("AnimatedMeshAssetMenuBar");
    xiiDocumentActions::MapMenuActions("AnimatedMeshAssetMenuBar");
    xiiAssetActions::MapMenuActions("AnimatedMeshAssetMenuBar");
    xiiCommandHistoryActions::MapActions("AnimatedMeshAssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("AnimatedMeshAssetToolBar").AssertSuccess();
    xiiDocumentActions::MapToolbarActions("AnimatedMeshAssetToolBar");
    xiiCommandHistoryActions::MapActions("AnimatedMeshAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("AnimatedMeshAssetToolBar", true);
    xiiCommonAssetActions::MapToolbarActions("AnimatedMeshAssetToolBar", xiiCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("AnimatedMeshAssetViewToolBar").AssertSuccess();
    xiiViewActions::MapToolbarActions("AnimatedMeshAssetViewToolBar", xiiViewActions::RenderMode | xiiViewActions::ActivateRemoteProcess);
    xiiViewLightActions::MapToolbarActions("AnimatedMeshAssetViewToolBar");
  }
}

static void ConfigureImageDataAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("ImageDataAssetMenuBar").AssertSuccess();
    xiiStandardMenus::MapActions("ImageDataAssetMenuBar", xiiStandardMenuTypes::Default | xiiStandardMenuTypes::Edit);
    xiiProjectActions::MapActions("ImageDataAssetMenuBar");
    xiiDocumentActions::MapMenuActions("ImageDataAssetMenuBar");
    xiiAssetActions::MapMenuActions("ImageDataAssetMenuBar");
    xiiCommandHistoryActions::MapActions("ImageDataAssetMenuBar");
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("ImageDataAssetToolBar").AssertSuccess();
    xiiDocumentActions::MapToolbarActions("ImageDataAssetToolBar");
    xiiCommandHistoryActions::MapActions("ImageDataAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("ImageDataAssetToolBar", true);
  }

  // View Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("ImageDataAssetViewToolBar").AssertSuccess();
    xiiViewActions::MapToolbarActions("ImageDataAssetViewToolBar", xiiViewActions::RenderMode | xiiViewActions::ActivateRemoteProcess);
    xiiViewLightActions::MapToolbarActions("ImageDataAssetViewToolBar");
  }
}

static void ConfigureStateMachineAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("StateMachineAssetMenuBar").AssertSuccess();

    xiiStandardMenus::MapActions("StateMachineAssetMenuBar", xiiStandardMenuTypes::Default | xiiStandardMenuTypes::Edit);
    xiiProjectActions::MapActions("StateMachineAssetMenuBar");
    xiiDocumentActions::MapMenuActions("StateMachineAssetMenuBar");
    xiiAssetActions::MapMenuActions("StateMachineAssetMenuBar");
    xiiCommandHistoryActions::MapActions("StateMachineAssetMenuBar");
    xiiEditActions::MapActions("StateMachineAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("StateMachineAssetToolBar").AssertSuccess();
    xiiDocumentActions::MapToolbarActions("StateMachineAssetToolBar");
    xiiCommandHistoryActions::MapActions("StateMachineAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("StateMachineAssetToolBar", true);
  }
}
static void ConfigureBlackboardTemplateAsset()
{
  // Menu Bar
  {
    xiiActionMapManager::RegisterActionMap("BlackboardTemplateAssetMenuBar").AssertSuccess();

    xiiStandardMenus::MapActions("BlackboardTemplateAssetMenuBar", xiiStandardMenuTypes::Default | xiiStandardMenuTypes::Edit);
    xiiProjectActions::MapActions("BlackboardTemplateAssetMenuBar");
    xiiDocumentActions::MapMenuActions("BlackboardTemplateAssetMenuBar");
    xiiAssetActions::MapMenuActions("BlackboardTemplateAssetMenuBar");
    xiiCommandHistoryActions::MapActions("BlackboardTemplateAssetMenuBar");
    xiiEditActions::MapActions("BlackboardTemplateAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    xiiActionMapManager::RegisterActionMap("BlackboardTemplateAssetToolBar").AssertSuccess();
    xiiDocumentActions::MapToolbarActions("BlackboardTemplateAssetToolBar");
    xiiCommandHistoryActions::MapActions("BlackboardTemplateAssetToolBar", "");
    xiiAssetActions::MapToolBarActions("BlackboardTemplateAssetToolBar", true);
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

  xiiDocumentManager::s_CustomActions["CustomAction_CreateShaderFromTemplate"] = CustomAction_CreateShaderFromTemplate;
}

void OnUnloadPlugin()
{
  xiiTextureAssetActions::UnregisterActions();
  xiiLUTAssetActions::UnregisterActions();
  xiiVisualShaderActions::UnregisterActions();
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
