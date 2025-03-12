#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAsset.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>


xiiQtCurve1DAssetDocumentWindow::xiiQtCurve1DAssetDocumentWindow(xiiDocument* pDocument) :
  xiiQtDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(xiiMakeDelegate(&xiiQtCurve1DAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(xiiMakeDelegate(&xiiQtCurve1DAssetDocumentWindow::StructureEventHandler, this));

  // Menu Bar
  {
    xiiQtMenuBarActionMapView* pMenuBar = static_cast<xiiQtMenuBarActionMapView*>(menuBar());
    xiiActionContext           context;
    context.m_sMapping  = "Curve1DAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    xiiQtToolBarActionMapView* pToolBar = new xiiQtToolBarActionMapView("Toolbar", this);
    xiiActionContext           context;
    context.m_sMapping  = "Curve1DAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow   = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("Curve1DAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // Central Widget
  {
    m_pCurveEditor = new xiiQtCurve1DEditorWidget(this);

    QWidget* pWidget = new QWidget();
    pWidget->setObjectName("Group");
    pWidget->setLayout(new QVBoxLayout());
    pWidget->setContentsMargins(0, 0, 0, 0);

    pWidget->layout()->setContentsMargins(0, 0, 0, 0);
    pWidget->layout()->addWidget(new xiiQtAssetStatusIndicator((xiiAssetDocument*)GetDocument()));
    pWidget->layout()->addWidget(m_pCurveEditor);

    xiiQtDocumentPanel* pCentral = new xiiQtDocumentPanel(this, pDocument);
    pCentral->setObjectName("xiiQtDocumentPanel");
    pCentral->setWindowTitle("Curve");
    pCentral->setWidget(pWidget);

    m_pDockManager->setCentralWidget(pCentral);
  }

  connect(m_pCurveEditor, &xiiQtCurve1DEditorWidget::InsertCpEvent, this, &xiiQtCurve1DAssetDocumentWindow::onInsertCpAt);
  connect(m_pCurveEditor, &xiiQtCurve1DEditorWidget::CpMovedEvent, this, &xiiQtCurve1DAssetDocumentWindow::onCurveCpMoved);
  connect(m_pCurveEditor, &xiiQtCurve1DEditorWidget::CpDeletedEvent, this, &xiiQtCurve1DAssetDocumentWindow::onCurveCpDeleted);
  connect(m_pCurveEditor, &xiiQtCurve1DEditorWidget::TangentMovedEvent, this, &xiiQtCurve1DAssetDocumentWindow::onCurveTangentMoved);
  connect(m_pCurveEditor, &xiiQtCurve1DEditorWidget::TangentLinkEvent, this, &xiiQtCurve1DAssetDocumentWindow::onLinkCurveTangents);
  connect(m_pCurveEditor, &xiiQtCurve1DEditorWidget::CpTangentModeEvent, this, &xiiQtCurve1DAssetDocumentWindow::onCurveTangentModeChanged);

  connect(m_pCurveEditor, &xiiQtCurve1DEditorWidget::BeginOperationEvent, this, &xiiQtCurve1DAssetDocumentWindow::onCurveBeginOperation);
  connect(m_pCurveEditor, &xiiQtCurve1DEditorWidget::EndOperationEvent, this, &xiiQtCurve1DAssetDocumentWindow::onCurveEndOperation);
  connect(m_pCurveEditor, &xiiQtCurve1DEditorWidget::BeginCpChangesEvent, this, &xiiQtCurve1DAssetDocumentWindow::onCurveBeginCpChanges);
  connect(m_pCurveEditor, &xiiQtCurve1DEditorWidget::EndCpChangesEvent, this, &xiiQtCurve1DAssetDocumentWindow::onCurveEndCpChanges);

  if (false)
  {
    xiiQtDocumentPanel* pPropertyPanel = new xiiQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("Curve1DAssetDockWidget");
    pPropertyPanel->setWindowTitle("Curve1D Properties");
    pPropertyPanel->show();

    xiiQtPropertyGridWidget* pPropertyGrid = new xiiQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();

  UpdatePreview();
}

xiiQtCurve1DAssetDocumentWindow::~xiiQtCurve1DAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtCurve1DAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtCurve1DAssetDocumentWindow::StructureEventHandler, this));

  RestoreResource();
}

void xiiQtCurve1DAssetDocumentWindow::onCurveBeginOperation(QString name)
{
  xiiCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands(name.toUtf8().data());
}

void xiiQtCurve1DAssetDocumentWindow::onCurveEndOperation(bool commit)
{
  xiiCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();

  UpdatePreview();
}

void xiiQtCurve1DAssetDocumentWindow::onCurveBeginCpChanges(QString name)
{
  GetDocument()->GetCommandHistory()->StartTransaction(name.toUtf8().data());
}

void xiiQtCurve1DAssetDocumentWindow::onCurveEndCpChanges()
{
  GetDocument()->GetCommandHistory()->FinishTransaction();

  UpdatePreview();
}

void xiiQtCurve1DAssetDocumentWindow::onInsertCpAt(xiiUInt32 uiCurveIdx, xiiInt64 tickX, double clickPosY)
{
  xiiCurve1DAssetDocument* pDoc = static_cast<xiiCurve1DAssetDocument*>(GetDocument());

  xiiCommandHistory* history = pDoc->GetCommandHistory();

  if (pDoc->GetPropertyObject()->GetTypeAccessor().GetCount("Curves") == 0)
  {
    // no curves allocated yet, add one

    xiiAddObjectCommand cmdAddCurve;
    cmdAddCurve.m_Parent          = pDoc->GetPropertyObject()->GetGuid();
    cmdAddCurve.m_NewObjectGuid   = xiiUuid::MakeUuid();
    cmdAddCurve.m_sParentProperty = "Curves";
    cmdAddCurve.m_pType           = xiiGetStaticRTTI<xiiSingleCurveData>();
    cmdAddCurve.m_Index           = -1;

    history->AddCommand(cmdAddCurve).AssertSuccess();
  }

  const xiiVariant curveGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Curves", uiCurveIdx);

  xiiAddObjectCommand cmdAdd;
  cmdAdd.m_Parent          = curveGuid.Get<xiiUuid>();
  cmdAdd.m_NewObjectGuid   = xiiUuid::MakeUuid();
  cmdAdd.m_sParentProperty = "ControlPoints";
  cmdAdd.m_pType           = xiiGetStaticRTTI<xiiCurveControlPointData>();
  cmdAdd.m_Index           = -1;

  history->AddCommand(cmdAdd).AssertSuccess();

  xiiSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue  = tickX;
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Value";
  cmdSet.m_NewValue  = clickPosY;
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "LeftTangent";
  cmdSet.m_NewValue  = xiiVec2(-0.1f, 0.0f);
  history->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "RightTangent";
  cmdSet.m_NewValue  = xiiVec2(+0.1f, 0.0f);
  history->AddCommand(cmdSet).AssertSuccess();
}

void xiiQtCurve1DAssetDocumentWindow::onCurveCpMoved(xiiUInt32 curveIdx, xiiUInt32 cpIdx, xiiInt64 iTickX, double newPosY)
{
  iTickX = xiiMath::Max<xiiInt64>(iTickX, 0);

  xiiCurve1DAssetDocument* pDoc = static_cast<xiiCurve1DAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const xiiVariant         curveGuid    = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const xiiDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<xiiUuid>());
  const xiiVariant         cpGuid       = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  xiiSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<xiiUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue  = iTickX;
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();

  cmdSet.m_sProperty = "Value";
  cmdSet.m_NewValue  = newPosY;
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void xiiQtCurve1DAssetDocumentWindow::onCurveCpDeleted(xiiUInt32 curveIdx, xiiUInt32 cpIdx)
{
  xiiCurve1DAssetDocument* pDoc = static_cast<xiiCurve1DAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const xiiVariant         curveGuid    = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const xiiDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<xiiUuid>());
  const xiiVariant         cpGuid       = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  if (!cpGuid.IsValid())
    return;

  xiiRemoveObjectCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<xiiUuid>();
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void xiiQtCurve1DAssetDocumentWindow::onCurveTangentMoved(xiiUInt32 curveIdx, xiiUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent)
{
  xiiCurve1DAssetDocument* pDoc = static_cast<xiiCurve1DAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const xiiVariant         curveGuid    = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const xiiDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<xiiUuid>());
  const xiiVariant         cpGuid       = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  xiiSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<xiiUuid>();

  // clamp tangents to one side
  if (rightTangent)
    newPosX = xiiMath::Max(newPosX, 0.0f);
  else
    newPosX = xiiMath::Min(newPosX, 0.0f);

  cmdSet.m_sProperty = rightTangent ? "RightTangent" : "LeftTangent";
  cmdSet.m_NewValue  = xiiVec2(newPosX, newPosY);
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void xiiQtCurve1DAssetDocumentWindow::onLinkCurveTangents(xiiUInt32 curveIdx, xiiUInt32 cpIdx, bool bLink)
{
  xiiCurve1DAssetDocument* pDoc = static_cast<xiiCurve1DAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const xiiVariant         curveGuid    = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const xiiDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<xiiUuid>());
  const xiiVariant         cpGuid       = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  xiiSetObjectPropertyCommand cmdLink;
  cmdLink.m_Object    = cpGuid.Get<xiiUuid>();
  cmdLink.m_sProperty = "Linked";
  cmdLink.m_NewValue  = bLink;
  GetDocument()->GetCommandHistory()->AddCommand(cmdLink).AssertSuccess();

  if (bLink)
  {
    const xiiVec2 leftTangent = pDoc->GetProperties()->m_Curves[curveIdx]->m_ControlPoints[cpIdx].m_LeftTangent;
    const xiiVec2 rightTangent(-leftTangent.x, -leftTangent.y);

    onCurveTangentMoved(curveIdx, cpIdx, rightTangent.x, rightTangent.y, true);
  }
}

void xiiQtCurve1DAssetDocumentWindow::onCurveTangentModeChanged(xiiUInt32 curveIdx, xiiUInt32 cpIdx, bool rightTangent, int mode)
{
  xiiCurve1DAssetDocument* pDoc = static_cast<xiiCurve1DAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  const xiiVariant         curveGuid    = pProp->GetTypeAccessor().GetValue("Curves", curveIdx);
  const xiiDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<xiiUuid>());
  const xiiVariant         cpGuid       = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  xiiSetObjectPropertyCommand cmd;
  cmd.m_Object    = cpGuid.Get<xiiUuid>();
  cmd.m_sProperty = rightTangent ? "RightTangentMode" : "LeftTangentMode";
  cmd.m_NewValue  = mode;
  GetDocument()->GetCommandHistory()->AddCommand(cmd).AssertSuccess();

  // sync current curve back
  if (false)
  {
    // generally works, but would need some work to make it perfect

    xiiCurve1D curve;
    pDoc->GetProperties()->m_Curves[curveIdx]->ConvertToRuntimeData(curve);
    curve.SortControlPoints();
    curve.ApplyTangentModes();

    for (xiiUInt32 i = 0; i < curve.GetNumControlPoints(); ++i)
    {
      const auto& cp = curve.GetControlPoint(i);
      if (cp.m_uiOriginalIndex == cpIdx)
      {
        if (rightTangent)
          onCurveTangentMoved(curveIdx, cpIdx, cp.m_RightTangent.x, cp.m_RightTangent.y, true);
        else
          onCurveTangentMoved(curveIdx, cpIdx, cp.m_LeftTangent.x, cp.m_LeftTangent.y, false);

        break;
      }
    }
  }
}

void xiiQtCurve1DAssetDocumentWindow::UpdatePreview()
{
  xiiCurve1DAssetDocument* pDoc = static_cast<xiiCurve1DAssetDocument*>(GetDocument());

  m_pCurveEditor->SetCurveExtents(0, 0.1f, true, false);
  m_pCurveEditor->SetCurves(*pDoc->GetProperties());

  SendLiveResourcePreview();
}

void xiiQtCurve1DAssetDocumentWindow::PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e)
{
  UpdatePreview();
}

void xiiQtCurve1DAssetDocumentWindow::StructureEventHandler(const xiiDocumentObjectStructureEvent& e)
{
  UpdatePreview();
}

void xiiQtCurve1DAssetDocumentWindow::SendLiveResourcePreview()
{
  if (xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  xiiResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "Curve1D";

  xiiStringBuilder tmp;
  msg.m_sResourceID = xiiConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  xiiContiguousMemoryStreamStorage streamStorage;
  xiiMemoryStreamWriter            memoryWriter(&streamStorage);

  xiiCurve1DAssetDocument* pDoc = xiiDynamicCast<xiiCurve1DAssetDocument*>(GetDocument());

  // Write Path
  xiiStringBuilder sAbsFilePath = pDoc->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("xiiCurve1D");

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

void xiiQtCurve1DAssetDocumentWindow::RestoreResource()
{
  xiiRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "Curve1D";

  xiiStringBuilder tmp;
  msg.m_sResourceID = xiiConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}
