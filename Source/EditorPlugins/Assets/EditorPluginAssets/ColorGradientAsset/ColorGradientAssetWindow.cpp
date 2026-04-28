/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/Widgets/ColorGradientEditorWidget.moc.h>

xiiQtColorGradientAssetDocumentWindow::xiiQtColorGradientAssetDocumentWindow(xiiDocument* pDocument) :
  xiiQtDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(xiiMakeDelegate(&xiiQtColorGradientAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(xiiMakeDelegate(&xiiQtColorGradientAssetDocumentWindow::StructureEventHandler, this));

  // Menu Bar
  {
    xiiQtMenuBarActionMapView* pMenuBar = static_cast<xiiQtMenuBarActionMapView*>(menuBar());
    xiiActionContext           context;
    context.m_sMapping  = "ColorGradientAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    xiiQtToolBarActionMapView* pToolBar = new xiiQtToolBarActionMapView("Toolbar", this);
    xiiActionContext           context;
    context.m_sMapping  = "ColorGradientAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("ColorGradientAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  m_bShowFirstTime  = true;
  m_pGradientEditor = new xiiQtColorGradientEditorWidget(this);


  // Central Widget
  {
    QWidget* pContainer = new QWidget(this);
    pContainer->setLayout(new QVBoxLayout());
    pContainer->layout()->addWidget(new xiiQtAssetStatusIndicator((xiiAssetDocument*)GetDocument()));
    pContainer->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    pContainer->layout()->addWidget(m_pGradientEditor);
    pContainer->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

    xiiQtDocumentPanel* pCentral = new xiiQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pCentral->setObjectName("xiiQtDocumentPanel");
    pCentral->setWindowTitle("Gradient");
    pCentral->setWidget(pContainer);

    m_pDockManager->setCentralWidget(pCentral);
  }

  connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::ColorCpAdded, this, &xiiQtColorGradientAssetDocumentWindow::onGradientColorCpAdded);
  connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::ColorCpMoved, this, &xiiQtColorGradientAssetDocumentWindow::onGradientColorCpMoved);
  connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::ColorCpDeleted, this, &xiiQtColorGradientAssetDocumentWindow::onGradientColorCpDeleted);
  connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::ColorCpChanged, this, &xiiQtColorGradientAssetDocumentWindow::onGradientColorCpChanged);

  connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::AlphaCpAdded, this, &xiiQtColorGradientAssetDocumentWindow::onGradientAlphaCpAdded);
  connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::AlphaCpMoved, this, &xiiQtColorGradientAssetDocumentWindow::onGradientAlphaCpMoved);
  connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::AlphaCpDeleted, this, &xiiQtColorGradientAssetDocumentWindow::onGradientAlphaCpDeleted);
  connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::AlphaCpChanged, this, &xiiQtColorGradientAssetDocumentWindow::onGradientAlphaCpChanged);

  connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::IntensityCpAdded, this, &xiiQtColorGradientAssetDocumentWindow::onGradientIntensityCpAdded);
  connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::IntensityCpMoved, this, &xiiQtColorGradientAssetDocumentWindow::onGradientIntensityCpMoved);
  connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::IntensityCpDeleted, this, &xiiQtColorGradientAssetDocumentWindow::onGradientIntensityCpDeleted);
  connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::IntensityCpChanged, this, &xiiQtColorGradientAssetDocumentWindow::onGradientIntensityCpChanged);

  connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::BeginOperation, this, &xiiQtColorGradientAssetDocumentWindow::onGradientBeginOperation);
  connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::EndOperation, this, &xiiQtColorGradientAssetDocumentWindow::onGradientEndOperation);

  connect(m_pGradientEditor, &xiiQtColorGradientEditorWidget::NormalizeRange, this, &xiiQtColorGradientAssetDocumentWindow::onGradientNormalizeRange);

  // property grid, if needed
  if (false)
  {
    xiiQtDocumentPanel* pPropertyPanel = new xiiQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("ColorGradientAssetDockWidget");
    pPropertyPanel->setWindowTitle("ColorGradient Properties");
    pPropertyPanel->show();

    xiiQtPropertyGridWidget* pPropertyGrid = new xiiQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();

  UpdatePreview();
}

xiiQtColorGradientAssetDocumentWindow::~xiiQtColorGradientAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtColorGradientAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtColorGradientAssetDocumentWindow::StructureEventHandler, this));

  RestoreResource();
}

void xiiQtColorGradientAssetDocumentWindow::onGradientColorCpAdded(double posX, const xiiColorGammaUB& color)
{
  xiiColorGradientAssetDocument* pDoc = static_cast<xiiColorGradientAssetDocument*>(GetDocument());

  xiiCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Color Control Point");

  xiiAddObjectCommand cmdAdd;
  cmdAdd.m_Parent          = pDoc->GetPropertyObject()->GetGuid();
  cmdAdd.m_NewObjectGuid   = xiiUuid::MakeUuid();
  cmdAdd.m_sParentProperty = "ColorCPs";
  cmdAdd.m_pType           = xiiGetStaticRTTI<xiiColorControlPoint>();
  cmdAdd.m_Index           = -1;

  history->AddCommand(cmdAdd).AssertSuccess();

  xiiSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue  = pDoc->GetProperties()->TickFromTime(xiiTime::MakeFromSeconds(posX));
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Red";
  cmdSet.m_NewValue  = color.r;
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Green";
  cmdSet.m_NewValue  = color.g;
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Blue";
  cmdSet.m_NewValue  = color.b;
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}


void xiiQtColorGradientAssetDocumentWindow::onGradientAlphaCpAdded(double posX, xiiUInt8 alpha)
{
  xiiColorGradientAssetDocument* pDoc = static_cast<xiiColorGradientAssetDocument*>(GetDocument());

  xiiCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Alpha Control Point");

  xiiAddObjectCommand cmdAdd;
  cmdAdd.m_Parent          = pDoc->GetPropertyObject()->GetGuid();
  cmdAdd.m_NewObjectGuid   = xiiUuid::MakeUuid();
  cmdAdd.m_sParentProperty = "AlphaCPs";
  cmdAdd.m_pType           = xiiGetStaticRTTI<xiiAlphaControlPoint>();
  cmdAdd.m_Index           = -1;

  history->AddCommand(cmdAdd).AssertSuccess();

  xiiSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue  = pDoc->GetProperties()->TickFromTime(xiiTime::MakeFromSeconds(posX));
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Alpha";
  cmdSet.m_NewValue  = alpha;
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}


void xiiQtColorGradientAssetDocumentWindow::onGradientIntensityCpAdded(double posX, float intensity)
{
  xiiColorGradientAssetDocument* pDoc = static_cast<xiiColorGradientAssetDocument*>(GetDocument());

  xiiCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Intensity Control Point");

  xiiAddObjectCommand cmdAdd;
  cmdAdd.m_Parent          = pDoc->GetPropertyObject()->GetGuid();
  cmdAdd.m_NewObjectGuid   = xiiUuid::MakeUuid();
  cmdAdd.m_sParentProperty = "IntensityCPs";
  cmdAdd.m_pType           = xiiGetStaticRTTI<xiiIntensityControlPoint>();
  cmdAdd.m_Index           = -1;

  history->AddCommand(cmdAdd).AssertSuccess();

  xiiSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue  = pDoc->GetProperties()->TickFromTime(xiiTime::MakeFromSeconds(posX));
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Intensity";
  cmdSet.m_NewValue  = intensity;
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}

void xiiQtColorGradientAssetDocumentWindow::MoveCP(xiiInt32 idx, double newPosX, const char* szArrayName)
{
  xiiColorGradientAssetDocument* pDoc = static_cast<xiiColorGradientAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  xiiVariant objGuid = pProp->GetTypeAccessor().GetValue(szArrayName, idx);

  xiiCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Move Control Point");

  xiiSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<xiiUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue  = pDoc->GetProperties()->TickFromTime(xiiTime::MakeFromSeconds(newPosX));
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}

void xiiQtColorGradientAssetDocumentWindow::onGradientColorCpMoved(xiiInt32 idx, double newPosX)
{
  MoveCP(idx, newPosX, "ColorCPs");
}

void xiiQtColorGradientAssetDocumentWindow::onGradientAlphaCpMoved(xiiInt32 idx, double newPosX)
{
  MoveCP(idx, newPosX, "AlphaCPs");
}


void xiiQtColorGradientAssetDocumentWindow::onGradientIntensityCpMoved(xiiInt32 idx, double newPosX)
{
  MoveCP(idx, newPosX, "IntensityCPs");
}

void xiiQtColorGradientAssetDocumentWindow::RemoveCP(xiiInt32 idx, const char* szArrayName)
{
  xiiColorGradientAssetDocument* pDoc = static_cast<xiiColorGradientAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  xiiVariant objGuid = pProp->GetTypeAccessor().GetValue(szArrayName, idx);

  xiiCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Remove Control Point");

  xiiRemoveObjectCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<xiiUuid>();
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}

void xiiQtColorGradientAssetDocumentWindow::onGradientColorCpDeleted(xiiInt32 idx)
{
  RemoveCP(idx, "ColorCPs");
}


void xiiQtColorGradientAssetDocumentWindow::onGradientAlphaCpDeleted(xiiInt32 idx)
{
  RemoveCP(idx, "AlphaCPs");
}


void xiiQtColorGradientAssetDocumentWindow::onGradientIntensityCpDeleted(xiiInt32 idx)
{
  RemoveCP(idx, "IntensityCPs");
}


void xiiQtColorGradientAssetDocumentWindow::onGradientColorCpChanged(xiiInt32 idx, const xiiColorGammaUB& color)
{
  xiiColorGradientAssetDocument* pDoc = static_cast<xiiColorGradientAssetDocument*>(GetDocument());

  auto       pProp   = pDoc->GetPropertyObject();
  xiiVariant objGuid = pProp->GetTypeAccessor().GetValue("ColorCPs", idx);

  xiiCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Color");

  xiiSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<xiiUuid>();

  cmdSet.m_sProperty = "Red";
  cmdSet.m_NewValue  = color.r;
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Green";
  cmdSet.m_NewValue  = color.g;
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Blue";
  cmdSet.m_NewValue  = color.b;
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}


void xiiQtColorGradientAssetDocumentWindow::onGradientAlphaCpChanged(xiiInt32 idx, xiiUInt8 alpha)
{
  xiiColorGradientAssetDocument* pDoc = static_cast<xiiColorGradientAssetDocument*>(GetDocument());

  auto       pProp   = pDoc->GetPropertyObject();
  xiiVariant objGuid = pProp->GetTypeAccessor().GetValue("AlphaCPs", idx);

  xiiCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Alpha");

  xiiSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<xiiUuid>();

  cmdSet.m_sProperty = "Alpha";
  cmdSet.m_NewValue  = alpha;
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}

void xiiQtColorGradientAssetDocumentWindow::onGradientIntensityCpChanged(xiiInt32 idx, float intensity)
{
  xiiColorGradientAssetDocument* pDoc = static_cast<xiiColorGradientAssetDocument*>(GetDocument());

  auto       pProp   = pDoc->GetPropertyObject();
  xiiVariant objGuid = pProp->GetTypeAccessor().GetValue("IntensityCPs", idx);

  xiiCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Intensity");

  xiiSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<xiiUuid>();

  cmdSet.m_sProperty = "Intensity";
  cmdSet.m_NewValue  = intensity;
  history->AddCommand(cmdSet).AssertSuccess();

  history->FinishTransaction();
}


void xiiQtColorGradientAssetDocumentWindow::onGradientBeginOperation()
{
  xiiCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands("Modify Gradient");
}


void xiiQtColorGradientAssetDocumentWindow::onGradientEndOperation(bool commit)
{
  xiiCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();
}


void xiiQtColorGradientAssetDocumentWindow::onGradientNormalizeRange()
{
  if (xiiQtUiServices::GetSingleton()->MessageBoxQuestion("This will adjust the positions of all control points, such that the minimum is at 0 and the maximum at 1.\n\nContinue?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) !=
      QMessageBox::StandardButton::Yes)
    return;

  xiiColorGradientAssetDocument* pDoc = static_cast<xiiColorGradientAssetDocument*>(GetDocument());

  xiiColorGradient GradientData;
  pDoc->GetProperties()->FillGradientData(GradientData);

  double minX, maxX;
  if (!GradientData.GetExtents(minX, maxX))
    return;

  if ((minX == 0 && maxX == 1) || (minX >= maxX))
    return;

  xiiCommandHistory* history = GetDocument()->GetCommandHistory();

  const float rangeNorm = 1.0f / (maxX - minX);

  history->StartTransaction("Normalize Gradient Range");

  xiiUInt32 numRgb, numAlpha, numInt;
  GradientData.GetNumControlPoints(numRgb, numAlpha, numInt);

  for (xiiUInt32 i = 0; i < numRgb; ++i)
  {
    float x = GradientData.GetColorControlPoint(i).m_PosX;
    x -= minX;
    x *= rangeNorm;

    MoveCP(i, x, "ColorCPs");
  }

  for (xiiUInt32 i = 0; i < numAlpha; ++i)
  {
    float x = GradientData.GetAlphaControlPoint(i).m_PosX;
    x -= minX;
    x *= rangeNorm;

    MoveCP(i, x, "AlphaCPs");
  }

  for (xiiUInt32 i = 0; i < numInt; ++i)
  {
    float x = GradientData.GetIntensityControlPoint(i).m_PosX;
    x -= minX;
    x *= rangeNorm;

    MoveCP(i, x, "IntensityCPs");
  }

  history->FinishTransaction();

  m_pGradientEditor->FrameGradient();
}

void xiiQtColorGradientAssetDocumentWindow::UpdatePreview()
{
  xiiColorGradient GradientData;

  xiiColorGradientAssetDocument* pDoc = static_cast<xiiColorGradientAssetDocument*>(GetDocument());
  pDoc->GetProperties()->FillGradientData(GradientData);

  m_pGradientEditor->SetColorGradient(GradientData);

  if (m_bShowFirstTime)
  {
    m_bShowFirstTime = false;
    m_pGradientEditor->FrameGradient();
  }

  SendLiveResourcePreview();
}

void xiiQtColorGradientAssetDocumentWindow::PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e)
{
  UpdatePreview();
}

void xiiQtColorGradientAssetDocumentWindow::StructureEventHandler(const xiiDocumentObjectStructureEvent& e)
{
  UpdatePreview();
}

void xiiQtColorGradientAssetDocumentWindow::SendLiveResourcePreview()
{
  if (xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  xiiResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "ColorGradient";

  xiiStringBuilder tmp;
  msg.m_sResourceID = xiiConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  xiiContiguousMemoryStreamStorage streamStorage;
  xiiMemoryStreamWriter            memoryWriter(&streamStorage);

  xiiColorGradientAssetDocument* pDoc = xiiDynamicCast<xiiColorGradientAssetDocument*>(GetDocument());

  // Write Path
  xiiStringBuilder sAbsFilePath = pDoc->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("xiiColorGradient");

  // Write Header
  memoryWriter << sAbsFilePath;
  const xiiUInt64    uiHash = xiiAssetCurator::GetSingleton()->GetAssetDependencyHash(pDoc->GetGuid());
  xiiAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, pDoc->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).IgnoreResult();

  // Write Asset Data
  pDoc->WriteResource(memoryWriter);
  msg.m_Data = xiiArrayPtr<const xiiUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

  xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void xiiQtColorGradientAssetDocumentWindow::RestoreResource()
{
  xiiRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "ColorGradient";

  xiiStringBuilder tmp;
  msg.m_sResourceID = xiiConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}
