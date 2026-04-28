/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
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
  auto pDocument   = context.m_pDocument;
  m_pValueProperty = xiiReflectionUtils::GetMemberProperty(pDocument->GetDynamicRTTI(), "ChannelMode");

  const xiiRTTI* pEnumRTTI = m_pValueProperty != nullptr ? m_pValueProperty->GetSpecificType() : xiiGetStaticRTTI<xiiTextureChannelMode>();
  InitEnumerationType(pEnumRTTI);
}

xiiInt64 xiiTextureChannelModeAction::GetValue() const
{
  xiiVariant value = 0;
  if (m_pValueProperty)
  {
    value = xiiReflectionUtils::GetMemberPropertyValue(m_pValueProperty, m_Context.m_pDocument);
  }
  return value.ConvertTo<xiiInt64>();
}

void xiiTextureChannelModeAction::Execute(const xiiVariant& value)
{
  if (m_pValueProperty)
  {
    xiiReflectionUtils::SetMemberPropertyValue(m_pValueProperty, m_Context.m_pDocument, value);
  }
}

//////////////////////////////////////////////////////////////////////////
// xiiTextureLodSliderAction
//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureLodSliderAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;


xiiTextureLodSliderAction::xiiTextureLodSliderAction(const xiiActionContext& context, const char* szName) :
  xiiSliderAction(context, szName)
{
  auto pDocument   = context.m_pDocument;
  m_pValueProperty = xiiReflectionUtils::GetMemberProperty(pDocument->GetDynamicRTTI(), "TextureLod");

  xiiVariant currentValue = -1;
  if (m_pValueProperty)
  {
    currentValue = xiiReflectionUtils::GetMemberPropertyValue(m_pValueProperty, pDocument);
  }

  SetRange(-1, 13);
  SetValue(currentValue.ConvertTo<int>());
}

void xiiTextureLodSliderAction::Execute(const xiiVariant& value)
{
  if (m_pValueProperty)
  {
    xiiReflectionUtils::SetMemberPropertyValue(m_pValueProperty, m_Context.m_pDocument, value);
  }
}


//////////////////////////////////////////////////////////////////////////
// xiiTextureAssetActions
//////////////////////////////////////////////////////////////////////////

xiiActionDescriptorHandle xiiTextureAssetActions::s_hTextureChannelMode;
xiiActionDescriptorHandle xiiTextureAssetActions::s_hLodSlider;

void xiiTextureAssetActions::RegisterActions()
{
  s_hTextureChannelMode = XII_REGISTER_DYNAMIC_MENU("TextureAsset.ChannelMode", xiiTextureChannelModeAction, ":/EditorFramework/Icons/RenderMode.svg");
  s_hLodSlider          = XII_REGISTER_ACTION_0("TextureAsset.LodSlider", xiiActionScope::Document, "Texture 2D", "", xiiTextureLodSliderAction);
}

void xiiTextureAssetActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hTextureChannelMode);
  xiiActionManager::UnregisterAction(s_hLodSlider);
}

void xiiTextureAssetActions::MapToolbarActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hLodSlider, "", 14.0f);
  pMap->MapAction(s_hTextureChannelMode, "", 15.0f);
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
    SetTargetFrameRate(25);

    m_ViewConfig.m_Camera.LookAt(xiiVec3(-2, 0, 0), xiiVec3(0, 0, 0), xiiVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new xiiQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureFixed(xiiVec3(0), xiiVec3(0.0f), xiiVec3(-1, 0, 0));
    AddViewWidget(m_pViewWidget);
    xiiQtViewWidgetContainer* pContainer = new xiiQtViewWidgetContainer(GetContainerWindow()->GetDockManager(), this, m_pViewWidget, nullptr);

    m_pDockManager->setCentralWidget(pContainer);
  }

  {
    xiiQtDocumentPanel* pPropertyPanel = new xiiQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("TextureAssetDockWidget");
    pPropertyPanel->setWindowTitle("Texture Properties");
    pPropertyPanel->show();

    xiiQtPropertyGridWidget* pPropertyGrid = new xiiQtPropertyGridWidget(pPropertyPanel, pDocument);

    QWidget* pWidget = new QWidget();
    pWidget->setObjectName("Group");
    pWidget->setLayout(new QVBoxLayout());
    pWidget->setContentsMargins(0, 0, 0, 0);

    pWidget->layout()->setContentsMargins(0, 0, 0, 0);
    pWidget->layout()->addWidget(new xiiQtAssetStatusIndicator(GetDocument()));
    pWidget->layout()->addWidget(pPropertyGrid);

    pPropertyPanel->setWidget(pWidget, ads::CDockWidget::ForceNoScrollArea);

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);

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
    const xiiTextureAssetDocument*   pDoc   = static_cast<const xiiTextureAssetDocument*>(GetDocument());
    const xiiTextureAssetProperties* pProps = pDoc->GetProperties();

    {
      xiiDocumentConfigMsgToEngine msg;
      msg.m_sWhatToDo = "SetChannelMode";
      msg.m_iValue    = pDoc->m_ChannelMode.GetValue();
      msg.m_fValue    = pProps->m_fAlphaThreshold;
      GetEditorEngineConnection()->SendMessage(&msg);
    }

    {
      xiiDocumentConfigMsgToEngine msg;
      msg.m_sWhatToDo = "SetLodLevel";
      msg.m_iValue    = pDoc->m_iTextureLod;
      GetEditorEngineConnection()->SendMessage(&msg);
    }
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}
