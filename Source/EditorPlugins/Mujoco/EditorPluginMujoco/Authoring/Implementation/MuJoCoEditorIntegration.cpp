/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginMujoco/EditorPluginMujocoPCH.h>

#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginMujoco/Authoring/MuJoCoSceneInspector.h>
#include <EditorPluginMujoco/ModelAsset/MuJoCoModelAsset.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <ToolsFoundation/Selection/SelectionManager.h>

#include <QPointer>
#include <QVBoxLayout>

namespace
{
  constexpr const char*               AssetMenu = "MuJoCoModelAssetMenuBar";
  constexpr const char*               PanelName = "MuJoCoSceneInspector";
  xiiActionDescriptorHandle           s_hInspector;
  QList<QPointer<xiiQtDocumentPanel>> s_InspectorPanels;

  class xiiQtMuJoCoModelAssetWindow final : public xiiQtDocumentWindow
  {
  public:
    explicit xiiQtMuJoCoModelAssetWindow(xiiMuJoCoModelAssetDocument* document) : xiiQtDocumentWindow(document)
    {
      xiiActionContext context(document);
      context.m_sMapping = AssetMenu;
      context.m_pWindow  = this;
      static_cast<xiiQtMenuBarActionMapView*>(menuBar())->SetActionContext(context);
      auto* panel = new xiiQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, document);
      panel->setObjectName("MuJoCoModelAssetProperties");
      panel->setWindowTitle(tr("MuJoCo Model Properties"));
      auto* content = new QWidget(panel);
      auto* layout  = new QVBoxLayout(content);
      layout->addWidget(new xiiQtAssetStatusIndicator(document, content));
      auto* grid = new xiiQtPropertyGridWidget(content, document);
      layout->addWidget(grid, 1);
      panel->setWidget(content, ads::CDockWidget::ForceNoScrollArea);
      m_pDockManager->setCentralWidget(panel);
      document->GetSelectionManager()->SetSelection(document->GetPropertyObject());
      FinishWindowCreation();
    }
    xiiStringView GetWindowLayoutGroupName() const override { return "MuJoCoModelAsset"; }
  };

  void MuJoCoDocumentEvent(const xiiDocumentManager::Event& event)
  {
    if (event.m_Type == xiiDocumentManager::Event::Type::DocumentWindowRequested)
      if (auto* document = xiiDynamicCast<xiiMuJoCoModelAssetDocument*>(event.m_pDocument))
        new xiiQtMuJoCoModelAssetWindow(document);
  }
} // namespace

class xiiMuJoCoInspectorAction final : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMuJoCoInspectorAction, xiiButtonAction);

public:
  xiiMuJoCoInspectorAction(const xiiActionContext& context, const char* name, int) : xiiButtonAction(context, name, false, ":/EditorFramework/Icons/Visualizers.svg")
  {
    SetEnabled(xiiDynamicCast<const xiiSceneDocument*>(context.m_pDocument) != nullptr);
  }
  void Execute(const xiiVariant&) override
  {
    auto* scene = xiiDynamicCast<xiiSceneDocument*>(m_Context.m_pDocument);
    if (!scene)
      return;
    auto* window = qobject_cast<xiiQtEngineDocumentWindow*>(xiiQtDocumentWindow::FindWindowByDocument(scene->GetMainDocument()));
    if (!window)
      return;
    for (const auto& panel : s_InspectorPanels)
      if (panel && panel->dockManager() == window->m_pDockManager)
      {
        panel->toggleView(true);
        panel->raise();
        return;
      }
    auto* panel = new xiiQtDocumentPanel(window->GetContainerWindow()->GetDockManager(), window, scene);
    panel->setObjectName(PanelName);
    panel->setWindowTitle("MuJoCo Inspector");
    panel->setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetClosable, true);
    panel->setWidget(new xiiQtMuJoCoSceneInspector(*scene, *window), ads::CDockWidget::ForceNoScrollArea);
    window->m_pDockManager->addDockWidget(ads::RightDockWidgetArea, panel);
    s_InspectorPanels.removeIf([](const auto& entry) { return entry.isNull(); });
    s_InspectorPanels.push_back(panel);
  }
};

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMuJoCoInspectorAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

void xiiRegisterMuJoCoEditor()
{
  xiiActionMapManager::RegisterActionMap(AssetMenu, "AssetMenuBar");
  s_hInspector = XII_REGISTER_ACTION_1("MuJoCo Inspector", xiiActionScope::Document, "MuJoCo", "", xiiMuJoCoInspectorAction, 0);
  for (const char* name : {"EditorPluginScene_DocumentMenuBar", "EditorPluginScene_Scene2MenuBar"})
    if (auto* map = xiiActionMapManager::GetActionMap(name))
      map->MapAction(s_hInspector, "G.View", 31.0f);
  xiiDocumentManager::s_Events.AddEventHandler(MuJoCoDocumentEvent);
}

void xiiUnregisterMuJoCoEditor()
{
  xiiDocumentManager::s_Events.RemoveEventHandler(MuJoCoDocumentEvent);
  for (const auto& panel : s_InspectorPanels)
    delete panel.data();
  s_InspectorPanels.clear();
  for (const char* name : {"EditorPluginScene_DocumentMenuBar", "EditorPluginScene_Scene2MenuBar"})
    if (auto* pMap = xiiActionMapManager::GetActionMap(name))
      pMap->RemoveActionMapping(s_hInspector);
  xiiActionManager::UnregisterAction(s_hInspector);
  xiiActionMapManager::UnregisterActionMap(AssetMenu);
}

XII_PLUGIN_DEPENDENCY(xiiEditorPluginScene);
XII_PLUGIN_DEPENDENCY(xiiEnginePluginMujoco);

XII_PLUGIN_ON_LOADED()
{
  xiiRegisterMuJoCoEditor();
}

XII_PLUGIN_ON_UNLOADED()
{
  xiiUnregisterMuJoCoEditor();
}

XII_STATICLINK_FILE(EditorPluginMujoco, EditorPluginMujoco_Authoring_Implementation_MuJoCoEditorIntegration);
