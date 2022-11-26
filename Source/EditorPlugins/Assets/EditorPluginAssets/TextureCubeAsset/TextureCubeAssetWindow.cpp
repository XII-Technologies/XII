#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAsset.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetWindow.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

////////////////////////////////////////////////////////////////////////
// xiiTextureCubeChannelModeAction
////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureCubeChannelModeAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiTextureCubeChannelModeAction::xiiTextureCubeChannelModeAction(const xiiActionContext& context, const char* szName, const char* szIconPath) :
  xiiEnumerationMenuAction(context, szName, szIconPath)
{
  InitEnumerationType(xiiGetStaticRTTI<xiiTextureCubeChannelMode>());
}

xiiInt64 xiiTextureCubeChannelModeAction::GetValue() const
{
  return static_cast<const xiiTextureCubeAssetDocument*>(m_Context.m_pDocument)->m_ChannelMode.GetValue();
}

void xiiTextureCubeChannelModeAction::Execute(const xiiVariant& value)
{
  ((xiiTextureCubeAssetDocument*)m_Context.m_pDocument)->m_ChannelMode.SetValue(value.ConvertTo<xiiInt32>());
}

//////////////////////////////////////////////////////////////////////////
// xiiTextureCubeLodSliderAction
//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureCubeLodSliderAction, 1, xiiRTTINoAllocator);
XII_END_DYNAMIC_REFLECTED_TYPE;


xiiTextureCubeLodSliderAction::xiiTextureCubeLodSliderAction(const xiiActionContext& context, const char* szName) :
  xiiSliderAction(context, szName)
{
  m_pDocument = const_cast<xiiTextureCubeAssetDocument*>(static_cast<const xiiTextureCubeAssetDocument*>(context.m_pDocument));

  SetRange(-1, 13);
  SetValue(m_pDocument->m_iTextureLod);
}

void xiiTextureCubeLodSliderAction::Execute(const xiiVariant& value)
{
  const xiiInt32 iValue = value.Get<xiiInt32>();

  m_pDocument->m_iTextureLod = value.Get<xiiInt32>();
}


//////////////////////////////////////////////////////////////////////////
// xiiTextureCubeAssetActions
//////////////////////////////////////////////////////////////////////////

xiiActionDescriptorHandle xiiTextureCubeAssetActions::s_hTextureChannelMode;
xiiActionDescriptorHandle xiiTextureCubeAssetActions::s_hLodSlider;

void xiiTextureCubeAssetActions::RegisterActions()
{
  s_hTextureChannelMode =
    XII_REGISTER_DYNAMIC_MENU("TextureCubeAsset.ChannelMode", xiiTextureCubeChannelModeAction, ":/EditorFramework/Icons/RenderMode.png");
  s_hLodSlider = XII_REGISTER_ACTION_0("TextureCubeAsset.LodSlider", xiiActionScope::Document, "CompatibleAsset_Texture_Cube", "", xiiTextureCubeLodSliderAction);
}

void xiiTextureCubeAssetActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hTextureChannelMode);
  xiiActionManager::UnregisterAction(s_hLodSlider);
}

void xiiTextureCubeAssetActions::MapActions(const char* szMapping, const char* szPath)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(szMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hLodSlider, szPath, 14.0f);
  pMap->MapAction(s_hTextureChannelMode, szPath, 15.0f);
}


//////////////////////////////////////////////////////////////////////////
// xiiQtTextureCubeAssetDocumentWindow
//////////////////////////////////////////////////////////////////////////

xiiQtTextureCubeAssetDocumentWindow::xiiQtTextureCubeAssetDocumentWindow(xiiTextureCubeAssetDocument* pDocument) :
  xiiQtEngineDocumentWindow(pDocument)
{
  // Menu Bar
  {
    xiiQtMenuBarActionMapView* pMenuBar = static_cast<xiiQtMenuBarActionMapView*>(menuBar());
    xiiActionContext           context;
    context.m_sMapping  = "TextureCubeAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    xiiQtToolBarActionMapView* pToolBar = new xiiQtToolBarActionMapView("Toolbar", this);
    xiiActionContext           context;
    context.m_sMapping  = "TextureCubeAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("TextureCubeAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(xiiVec3(-2, 0, 0), xiiVec3(0, 0, 0), xiiVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new xiiQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureOrbitCameraVolume(xiiVec3(0), xiiVec3(1.0f), xiiVec3(-1, 0, 0));
    AddViewWidget(m_pViewWidget);
    xiiQtViewWidgetContainer* pContainer = new xiiQtViewWidgetContainer(this, m_pViewWidget, nullptr);

    setCentralWidget(pContainer);
  }

  {
    xiiQtDocumentPanel* pPropertyPanel = new xiiQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("TextureCubeAssetDockWidget");
    pPropertyPanel->setWindowTitle("Texture Properties");
    pPropertyPanel->show();

    xiiQtPropertyGridWidget* pPropertyGrid = new xiiQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();
}

void xiiQtTextureCubeAssetDocumentWindow::InternalRedraw()
{
  xiiEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  xiiQtEngineDocumentWindow::InternalRedraw();
}

void xiiQtTextureCubeAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    const xiiTextureCubeAssetDocument* pDoc = static_cast<const xiiTextureCubeAssetDocument*>(GetDocument());

    xiiDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "ChannelMode";
    msg.m_iValue    = pDoc->m_ChannelMode.GetValue();
    msg.m_fValue    = pDoc->m_iTextureLod;

    GetEditorEngineConnection()->SendMessage(&msg);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}
