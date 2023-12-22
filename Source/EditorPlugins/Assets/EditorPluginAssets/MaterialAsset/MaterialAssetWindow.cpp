#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetManager.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetWindow.moc.h>
#include <EditorPluginAssets/VisualShader/VisualShaderScene.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/NodeEditor/NodeView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

////////////////////////////////////////////////////////////////////////
// xiiMaterialModelAction
////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMaterialModelAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiMaterialModelAction::xiiMaterialModelAction(const xiiActionContext& context, const char* szName, const char* szIconPath) :
  xiiEnumerationMenuAction(context, szName, szIconPath)
{
  InitEnumerationType(xiiGetStaticRTTI<xiiMaterialAssetPreview>());
}

xiiInt64 xiiMaterialModelAction::GetValue() const
{
  return static_cast<const xiiMaterialAssetDocument*>(m_Context.m_pDocument)->m_PreviewModel.GetValue();
}

void xiiMaterialModelAction::Execute(const xiiVariant& value)
{
  ((xiiMaterialAssetDocument*)m_Context.m_pDocument)->m_PreviewModel.SetValue(value.ConvertTo<xiiInt32>());
}

//////////////////////////////////////////////////////////////////////////
// xiiMaterialAssetActions
//////////////////////////////////////////////////////////////////////////

xiiActionDescriptorHandle xiiMaterialAssetActions::s_hMaterialModelAction;

void xiiMaterialAssetActions::RegisterActions()
{
  s_hMaterialModelAction = XII_REGISTER_DYNAMIC_MENU("MaterialAsset.Model", xiiMaterialModelAction, ":/EditorFramework/Icons/Perspective.svg");
}

void xiiMaterialAssetActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hMaterialModelAction);
}

void xiiMaterialAssetActions::MapToolbarActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hMaterialModelAction, "", 45.0f);
}


//////////////////////////////////////////////////////////////////////////
// xiiQtMaterialAssetDocumentWindow
//////////////////////////////////////////////////////////////////////////


xiiInt32             xiiQtMaterialAssetDocumentWindow::s_iNodeConfigWatchers = 0;
xiiDirectoryWatcher* xiiQtMaterialAssetDocumentWindow::s_pNodeConfigWatcher  = nullptr;


xiiQtMaterialAssetDocumentWindow::xiiQtMaterialAssetDocumentWindow(xiiMaterialAssetDocument* pDocument) :
  xiiQtEngineDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(xiiMakeDelegate(&xiiQtMaterialAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtMaterialAssetDocumentWindow::SelectionEventHandler, this));

  pDocument->m_VisualShaderEvents.AddEventHandler(xiiMakeDelegate(&xiiQtMaterialAssetDocumentWindow::VisualShaderEventHandler, this));

  // Menu Bar
  {
    xiiQtMenuBarActionMapView* pMenuBar = static_cast<xiiQtMenuBarActionMapView*>(menuBar());
    xiiActionContext           context;
    context.m_sMapping  = "MaterialAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    xiiQtToolBarActionMapView* pToolBar = new xiiQtToolBarActionMapView("Toolbar", this);
    xiiActionContext           context;
    context.m_sMapping  = "MaterialAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("MaterialAssetWindowToolBar");
    addToolBar(pToolBar);
  }


  // 3D View
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(xiiVec3(+1.6f, 0.5f, 0.3f), xiiVec3(0, 0, 0), xiiVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90, 0.01f, 100.0f);

    m_pViewWidget = new xiiQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureFixed(xiiVec3(0), xiiVec3(0.0f), xiiVec3(+0.23f, -0.04f, 0.02f));

    AddViewWidget(m_pViewWidget);
    xiiQtViewWidgetContainer* pContainer = new xiiQtViewWidgetContainer(nullptr, m_pViewWidget, "MaterialAssetViewToolBar");

    setCentralWidget(pContainer);
  }

  // Property Grid
  {
    xiiQtDocumentPanel* pPropertyPanel = new xiiQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("MaterialAssetDockWidget");
    pPropertyPanel->setWindowTitle("Material Properties");
    pPropertyPanel->show();

    xiiQtPropertyGridWidget* pPropertyGrid = new xiiQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);
  }

  // Visual Shader Editor
  {
    m_pVsePanel = new xiiQtDocumentPanel(this, pDocument);
    m_pVsePanel->setObjectName("VisualShaderDockWidget");
    m_pVsePanel->setWindowTitle("Visual Shader Editor");

    QSplitter* pSplitter = new QSplitter(Qt::Orientation::Horizontal, m_pVsePanel);

    m_pScene = new xiiQtVisualShaderScene(this);
    m_pScene->InitScene(static_cast<const xiiDocumentNodeManager*>(pDocument->GetObjectManager()));

    m_pNodeView = new xiiQtNodeView(m_pVsePanel);
    m_pNodeView->SetScene(m_pScene);
    pSplitter->addWidget(m_pNodeView);

    QWidget* pRightGroup = new QWidget(m_pVsePanel);
    pRightGroup->setLayout(new QVBoxLayout());

    QWidget* pButtonGroup = new QWidget(m_pVsePanel);
    pButtonGroup->setLayout(new QHBoxLayout());

    m_pOutputLine = new QTextEdit(m_pVsePanel);
    m_pOutputLine->setText("Transform the material asset to compile the Visual Shader.");
    m_pOutputLine->setReadOnly(true);

    m_pOpenShaderButton = new QPushButton(m_pVsePanel);
    m_pOpenShaderButton->setText("Open Shader File");
    connect(m_pOpenShaderButton, &QPushButton::clicked, this, &xiiQtMaterialAssetDocumentWindow::OnOpenShaderClicked);

    pButtonGroup->layout()->setContentsMargins(0, 0, 0, 0);
    pButtonGroup->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
    pButtonGroup->layout()->addWidget(m_pOpenShaderButton);

    pRightGroup->layout()->setContentsMargins(0, 0, 0, 0);
    pRightGroup->layout()->addWidget(m_pOutputLine);
    pRightGroup->layout()->addWidget(pButtonGroup);

    pSplitter->addWidget(pRightGroup);

    pSplitter->setStretchFactor(0, 10);
    pSplitter->setStretchFactor(1, 1);

    m_bVisualShaderEnabled = false;
    m_pVsePanel->setWidget(pSplitter);
    m_pVsePanel->setVisible(false);

    addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, m_pVsePanel);

    m_pVsePanel->setVisible(false);
  }

  pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

  UpdatePreview();

  UpdateNodeEditorVisibility();

  FinishWindowCreation();
}

xiiQtMaterialAssetDocumentWindow::~xiiQtMaterialAssetDocumentWindow()
{
  GetMaterialDocument()->m_VisualShaderEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtMaterialAssetDocumentWindow::VisualShaderEventHandler, this));

  RestoreResource();

  GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtMaterialAssetDocumentWindow::SelectionEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtMaterialAssetDocumentWindow::PropertyEventHandler, this));

  const bool bCustom = GetMaterialDocument()->GetPropertyObject()->GetTypeAccessor().GetValue("ShaderMode").ConvertTo<xiiInt64>() == xiiMaterialShaderMode::Custom;

  if (bCustom)
  {
    SetupDirectoryWatcher(false);
  }
}

void xiiQtMaterialAssetDocumentWindow::SetupDirectoryWatcher(bool needIt)
{
  if (needIt)
  {
    ++s_iNodeConfigWatchers;

    if (s_pNodeConfigWatcher == nullptr)
    {
      s_pNodeConfigWatcher = XII_DEFAULT_NEW(xiiDirectoryWatcher);

      xiiStringBuilder sSearchDir = xiiApplicationServices::GetSingleton()->GetApplicationDataFolder();
      sSearchDir.AppendPath("VisualShader");

      if (s_pNodeConfigWatcher->OpenDirectory(sSearchDir, xiiDirectoryWatcher::Watch::Writes).Failed())
        xiiLog::Warning("Could not register a file system watcher for changes to '{0}'", sSearchDir);
    }
  }
  else
  {
    --s_iNodeConfigWatchers;

    if (s_iNodeConfigWatchers == 0)
    {
      XII_DEFAULT_DELETE(s_pNodeConfigWatcher);
    }
  }
}

xiiMaterialAssetDocument* xiiQtMaterialAssetDocumentWindow::GetMaterialDocument()
{
  return static_cast<xiiMaterialAssetDocument*>(GetDocument());
}

void xiiQtMaterialAssetDocumentWindow::InternalRedraw()
{
  xiiEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  if (s_pNodeConfigWatcher)
  {
    s_pNodeConfigWatcher->EnumerateChanges(xiiMakeDelegate(&xiiQtMaterialAssetDocumentWindow::OnVseConfigChanged, this));
  }
  xiiQtEngineDocumentWindow::InternalRedraw();
}


void xiiQtMaterialAssetDocumentWindow::showEvent(QShowEvent* event)
{
  xiiQtEngineDocumentWindow::showEvent(event);

  m_pVsePanel->setVisible(m_bVisualShaderEnabled);
}

void xiiQtMaterialAssetDocumentWindow::OnOpenShaderClicked(bool)
{
  xiiAssetDocumentManager* pManager = (xiiAssetDocumentManager*)GetMaterialDocument()->GetDocumentManager();

  xiiString sAutoGenShader = pManager->GetAbsoluteOutputFileName(GetMaterialDocument()->GetAssetDocumentTypeDescriptor(), GetMaterialDocument()->GetDocumentPath(), xiiMaterialAssetDocumentManager::s_szShaderOutputTag);

  if (xiiOSFile::ExistsFile(sAutoGenShader))
  {
    xiiQtUiServices::OpenFileInDefaultProgram(sAutoGenShader);
  }
  else
  {
    xiiStringBuilder msg;
    msg.Format("The auto generated file does not exist (yet).\nThe supposed location is '{0}'", sAutoGenShader);

    xiiQtUiServices::GetSingleton()->MessageBoxInformation(msg);
  }
}

void xiiQtMaterialAssetDocumentWindow::UpdatePreview()
{
  if (xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  xiiResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "Material";

  xiiStringBuilder tmp;
  msg.m_sResourceID = xiiConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  xiiContiguousMemoryStreamStorage streamStorage;
  xiiMemoryStreamWriter            memoryWriter(&streamStorage);

  // Write Path
  xiiStringBuilder sAbsFilePath = GetMaterialDocument()->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("xiiMaterialBin");
  // Write Header
  memoryWriter << sAbsFilePath;
  const xiiUInt64    uiHash = xiiAssetCurator::GetSingleton()->GetAssetDependencyHash(GetMaterialDocument()->GetGuid());
  xiiAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, GetMaterialDocument()->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).IgnoreResult();

  // Write Asset Data
  if (GetMaterialDocument()->WriteMaterialAsset(memoryWriter, xiiAssetCurator::GetSingleton()->GetActiveAssetProfile(), false).Failed())
    return;

  msg.m_Data = xiiArrayPtr<const xiiUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

  xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void xiiQtMaterialAssetDocumentWindow::PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e)
{
  if (e.m_pObject == GetMaterialDocument()->GetPropertyObject() && e.m_sProperty == "ShaderMode")
  {
    UpdateNodeEditorVisibility();
  }

  UpdatePreview();

  if (e.m_sProperty == "ShaderMode" ||
      e.m_sProperty == "BLEND_MODE" ||
      e.m_sProperty == "BaseMaterial")
  {
    xiiDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "InvalidateCache";

    GetEditorEngineConnection()->SendMessage(&msg);
  }
}


void xiiQtMaterialAssetDocumentWindow::SelectionEventHandler(const xiiSelectionManagerEvent& e)
{
  if (GetDocument()->GetSelectionManager()->IsSelectionEmpty())
  {
    // delayed execution
    QTimer::singleShot(1, [this]() {
      // Check again if the selection is empty. This could have changed due to the delayed execution.
      if (GetDocument()->GetSelectionManager()->IsSelectionEmpty())
      {
        GetDocument()->GetSelectionManager()->SetSelection(GetMaterialDocument()->GetPropertyObject());
      } });
  }
}

void xiiQtMaterialAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    const xiiMaterialAssetDocument* pDoc = static_cast<const xiiMaterialAssetDocument*>(GetDocument());

    xiiDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PreviewModel";
    msg.m_iValue    = pDoc->m_PreviewModel.GetValue();

    GetEditorEngineConnection()->SendMessage(&msg);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}

void xiiQtMaterialAssetDocumentWindow::RestoreResource()
{
  xiiRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "Material";

  xiiStringBuilder tmp;
  msg.m_sResourceID = xiiConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void xiiQtMaterialAssetDocumentWindow::UpdateNodeEditorVisibility()
{
  const bool bCustom = GetMaterialDocument()->GetPropertyObject()->GetTypeAccessor().GetValue("ShaderMode").ConvertTo<xiiInt64>() == xiiMaterialShaderMode::Custom;

  m_pVsePanel->setVisible(bCustom);

  // when this is called during construction, it seems to be overridden again (probably by the dock widget code or the splitter)
  // by delaying it a bit, we have the last word
  QTimer::singleShot(100, this, [this, bCustom]() { m_pVsePanel->setVisible(bCustom); });

  if (m_bVisualShaderEnabled != bCustom)
  {
    m_bVisualShaderEnabled = bCustom;

    SetupDirectoryWatcher(bCustom);
  }
}

void xiiQtMaterialAssetDocumentWindow::OnVseConfigChanged(xiiStringView sFilename, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type)
{
  if (type != xiiDirectoryWatcherType::File || !xiiPathUtils::HasExtension(sFilename, "DDL"))
    return;

  // lalala ... this is to allow writes to the file to 'hopefully' finish before we try to read it
  xiiThreadUtils::Sleep(xiiTime::Milliseconds(100));

  xiiVisualShaderTypeRegistry::GetSingleton()->UpdateNodeData(sFilename);

  // TODO: We write an invalid hash in the file, should maybe compute the correct one on the fly
  // but that would involve the asset curator which would also save / transform everything which is
  // not what we want.
  xiiAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(0, GetMaterialDocument()->GetAssetTypeVersion());
  GetMaterialDocument()->RecreateVisualShaderFile(AssetHeader).LogFailure();
}

void xiiQtMaterialAssetDocumentWindow::VisualShaderEventHandler(const xiiMaterialVisualShaderEvent& e)
{
  xiiStringBuilder text;

  if (e.m_Type == xiiMaterialVisualShaderEvent::VisualShaderNotUsed)
  {
    text = "<span style=\"color:#bbbb00;\">Visual Shader is not used by the material.</span><br><br>Change the ShaderMode in the asset "
           "properties to enable Visual Shader mode.";
  }
  else
  {
    if (e.m_Type == xiiMaterialVisualShaderEvent::TransformSucceeded)
      text = "<span style=\"color:#00ff00;\">Visual Shader was transformed successfully.</span><br><br>";
    else
      text = "<span style=\"color:#ff8800;\">Visual Shader is invalid:</span><br><br>";

    xiiStringBuilder err = e.m_sTransformError;

    xiiHybridArray<xiiStringView, 16> lines;
    err.Split(false, lines, "\n");

    for (const xiiStringView& line : lines)
    {
      if (line.StartsWith("Error:"))
        text.AppendFormat("<span style=\"color:#ff2200;\">{0}</span><br>", line);
      else if (line.StartsWith("Warning:"))
        text.AppendFormat("<span style=\"color:#ffaa00;\">{0}</span><br>", line);
      else
        text.Append(line);
    }
    UpdatePreview();
  }

  m_pOutputLine->setAcceptRichText(true);
  m_pOutputLine->setHtml(text.GetData());
}
