#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetWindow.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

////////////////////////////////////////////////////////////////////////
// xiiTextureChannelModeAction
////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureChannelModeAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiTextureChannelModeAction::xiiTextureChannelModeAction(const xiiActionContext& context, const char* szName, const char* szIconPath) :
  xiiEnumerationMenuAction(context, szName, szIconPath)
{
  InitEnumerationType(xiiGetStaticRTTI<xiiTextureChannelMode>());
}

xiiInt64 xiiTextureChannelModeAction::GetValue() const
{
  return static_cast<const xiiTextureAssetDocument*>(m_Context.m_pDocument)->m_ChannelMode.GetValue();
}

void xiiTextureChannelModeAction::Execute(const xiiVariant& value)
{
  ((xiiTextureAssetDocument*)m_Context.m_pDocument)->m_ChannelMode.SetValue(value.ConvertTo<xiiInt32>());
}

//////////////////////////////////////////////////////////////////////////
// xiiTextureLodSliderAction
//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureLodSliderAction, 1, xiiRTTINoAllocator);
XII_END_DYNAMIC_REFLECTED_TYPE;


xiiTextureLodSliderAction::xiiTextureLodSliderAction(const xiiActionContext& context, const char* szName) :
  xiiSliderAction(context, szName)
{
  m_pDocument = const_cast<xiiTextureAssetDocument*>(static_cast<const xiiTextureAssetDocument*>(context.m_pDocument));

  SetRange(-1, 13);
  SetValue(m_pDocument->m_iTextureLod);
}

void xiiTextureLodSliderAction::Execute(const xiiVariant& value)
{
  const xiiInt32 iValue = value.Get<xiiInt32>();

  m_pDocument->m_iTextureLod = value.Get<xiiInt32>();
}


//////////////////////////////////////////////////////////////////////////
// xiiTextureAssetActions
//////////////////////////////////////////////////////////////////////////

xiiActionDescriptorHandle xiiTextureAssetActions::s_hTextureChannelMode;
xiiActionDescriptorHandle xiiTextureAssetActions::s_hLodSlider;

void xiiTextureAssetActions::RegisterActions()
{
  s_hTextureChannelMode = XII_REGISTER_DYNAMIC_MENU("TextureAsset.ChannelMode", xiiTextureChannelModeAction, ":/EditorFramework/Icons/RenderMode.png");
  s_hLodSlider          = XII_REGISTER_ACTION_0("TextureAsset.LodSlider", xiiActionScope::Document, "Texture 2D", "", xiiTextureLodSliderAction);
}

void xiiTextureAssetActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hTextureChannelMode);
  xiiActionManager::UnregisterAction(s_hLodSlider);
}

void xiiTextureAssetActions::MapActions(const char* szMapping, const char* szPath)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(szMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hLodSlider, szPath, 14.0f);
  pMap->MapAction(s_hTextureChannelMode, szPath, 15.0f);
}


//////////////////////////////////////////////////////////////////////////
// xiiQtTextureAssetDocumentWindow
//////////////////////////////////////////////////////////////////////////

xiiQtTextureAssetDocumentWindow::xiiQtTextureAssetDocumentWindow(xiiTextureAssetDocument* pDocument) :
  xiiQtEngineDocumentWindow(pDocument)
{
  // Menu Bar
  {
    xiiQtMenuBarActionMapView* pMenuBar = static_cast<xiiQtMenuBarActionMapView*>(menuBar());
    xiiActionContext           context;
    context.m_sMapping  = "TextureAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    xiiQtToolBarActionMapView* pToolBar = new xiiQtToolBarActionMapView("Toolbar", this);
    xiiActionContext           context;
    context.m_sMapping  = "TextureAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("TextureAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(xiiVec3(-2, 0, 0), xiiVec3(0, 0, 0), xiiVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new xiiQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureFixed(xiiVec3(0), xiiVec3(0.0f), xiiVec3(-1, 0, 0));
    AddViewWidget(m_pViewWidget);
    xiiQtViewWidgetContainer* pContainer = new xiiQtViewWidgetContainer(this, m_pViewWidget, nullptr);

    setCentralWidget(pContainer);
  }

  {
    xiiQtDocumentPanel* pPropertyPanel = new xiiQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("TextureAssetDockWidget");
    pPropertyPanel->setWindowTitle("Texture Properties");
    pPropertyPanel->show();

    xiiQtPropertyGridWidget* pPropertyGrid = new xiiQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();
}

void xiiQtTextureAssetDocumentWindow::InternalRedraw()
{
  xiiEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  xiiQtEngineDocumentWindow::InternalRedraw();
}

void xiiQtTextureAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    const xiiTextureAssetDocument* pDoc = static_cast<const xiiTextureAssetDocument*>(GetDocument());

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
