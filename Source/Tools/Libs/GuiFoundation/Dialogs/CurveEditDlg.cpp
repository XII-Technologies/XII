#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Dialogs/CurveEditDlg.moc.h>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

QByteArray xiiQtCurveEditDlg::s_LastDialogGeometry;

xiiQtCurveEditDlg::xiiQtCurveEditDlg(xiiObjectAccessorBase* pObjectAccessor, const xiiDocumentObject* pCurveObject, QWidget* pParent) :
  QDialog(pParent)
{
  m_pObjectAccessor = pObjectAccessor;
  m_pCurveObject    = pCurveObject;

  setupUi(this);

  xiiQtCurve1DEditorWidget* pEdit = CurveEditor;

  connect(pEdit, &xiiQtCurve1DEditorWidget::CpMovedEvent, this, &xiiQtCurveEditDlg::OnCpMovedEvent);
  connect(pEdit, &xiiQtCurve1DEditorWidget::CpDeletedEvent, this, &xiiQtCurveEditDlg::OnCpDeletedEvent);
  connect(pEdit, &xiiQtCurve1DEditorWidget::TangentMovedEvent, this, &xiiQtCurveEditDlg::OnTangentMovedEvent);
  connect(pEdit, &xiiQtCurve1DEditorWidget::InsertCpEvent, this, &xiiQtCurveEditDlg::OnInsertCpEvent);
  connect(pEdit, &xiiQtCurve1DEditorWidget::TangentLinkEvent, this, &xiiQtCurveEditDlg::OnTangentLinkEvent);
  connect(pEdit, &xiiQtCurve1DEditorWidget::CpTangentModeEvent, this, &xiiQtCurveEditDlg::OnCpTangentModeEvent);
  connect(pEdit, &xiiQtCurve1DEditorWidget::BeginCpChangesEvent, this, &xiiQtCurveEditDlg::OnBeginCpChangesEvent);
  connect(pEdit, &xiiQtCurve1DEditorWidget::EndCpChangesEvent, this, &xiiQtCurveEditDlg::OnEndCpChangesEvent);
  connect(pEdit, &xiiQtCurve1DEditorWidget::BeginOperationEvent, this, &xiiQtCurveEditDlg::OnBeginOperationEvent);
  connect(pEdit, &xiiQtCurve1DEditorWidget::EndOperationEvent, this, &xiiQtCurveEditDlg::OnEndOperationEvent);

  m_pShortcutUndo = new QShortcut(QKeySequence("Ctrl+Z"), this);
  m_pShortcutRedo = new QShortcut(QKeySequence("Ctrl+Y"), this);

  connect(m_pShortcutUndo, &QShortcut::activated, this, &xiiQtCurveEditDlg::on_actionUndo_triggered);
  connect(m_pShortcutRedo, &QShortcut::activated, this, &xiiQtCurveEditDlg::on_actionRedo_triggered);

  m_Curves.m_Curves.PushBack(XII_DEFAULT_NEW(xiiSingleCurveData));

  RetrieveCurveState();

  m_uiActionsUndoBaseline = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory()->GetUndoStackSize();
}

void xiiQtCurveEditDlg::RetrieveCurveState()
{
  auto& curve = m_Curves.m_Curves.PeekBack();

  xiiInt32 iNumPoints = 0;
  m_pObjectAccessor->GetCount(m_pCurveObject, "ControlPoints", iNumPoints).AssertSuccess();
  curve->m_ControlPoints.SetCount(iNumPoints);

  xiiVariant v;

  // get a local representation of the curve once, so that we can update the preview more efficiently
  for (xiiInt32 i = 0; i < iNumPoints; ++i)
  {
    const xiiDocumentObject* pPoint = m_pObjectAccessor->GetChildObject(m_pCurveObject, "ControlPoints", i);

    m_pObjectAccessor->GetValue(pPoint, "Tick", v).AssertSuccess();
    curve->m_ControlPoints[i].m_iTick = v.ConvertTo<xiiInt32>();

    m_pObjectAccessor->GetValue(pPoint, "Value", v).AssertSuccess();
    curve->m_ControlPoints[i].m_fValue = v.ConvertTo<double>();

    m_pObjectAccessor->GetValue(pPoint, "LeftTangent", v).AssertSuccess();
    curve->m_ControlPoints[i].m_LeftTangent = v.ConvertTo<xiiVec2>();

    m_pObjectAccessor->GetValue(pPoint, "RightTangent", v).AssertSuccess();
    curve->m_ControlPoints[i].m_RightTangent = v.ConvertTo<xiiVec2>();

    m_pObjectAccessor->GetValue(pPoint, "Linked", v).AssertSuccess();
    curve->m_ControlPoints[i].m_bTangentsLinked = v.ConvertTo<bool>();

    m_pObjectAccessor->GetValue(pPoint, "LeftTangentMode", v).AssertSuccess();
    curve->m_ControlPoints[i].m_LeftTangentMode = (xiiCurveTangentMode::Enum)v.ConvertTo<xiiInt32>();

    m_pObjectAccessor->GetValue(pPoint, "RightTangentMode", v).AssertSuccess();
    curve->m_ControlPoints[i].m_RightTangentMode = (xiiCurveTangentMode::Enum)v.ConvertTo<xiiInt32>();
  }
}

xiiQtCurveEditDlg::~xiiQtCurveEditDlg()
{
  s_LastDialogGeometry = saveGeometry();
}

void xiiQtCurveEditDlg::SetCurveColor(const xiiColor& color)
{
  m_Curves.m_Curves.PeekBack()->m_CurveColor = color;
}

void xiiQtCurveEditDlg::SetCurveExtents(double fLower, bool bLowerFixed, double fUpper, bool bUpperFixed)
{
  m_fLowerExtents = fLower;
  m_fUpperExtents = fUpper;
  m_bLowerFixed   = bLowerFixed;
  m_bUpperFixed   = bUpperFixed;
}

void xiiQtCurveEditDlg::SetCurveRanges(double fLower, double fUpper)
{
  m_fLowerRange = fLower;
  m_fUpperRange = fUpper;
}

void xiiQtCurveEditDlg::reject()
{
  // ignore
}

void xiiQtCurveEditDlg::accept()
{
  // ignore
}

void xiiQtCurveEditDlg::cancel()
{
  auto& cmd = *m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  cmd.Undo(cmd.GetUndoStackSize() - m_uiActionsUndoBaseline).AssertSuccess();

  QDialog::reject();
}

void xiiQtCurveEditDlg::UpdatePreview()
{
  xiiQtCurve1DEditorWidget* pEdit = CurveEditor;
  pEdit->SetCurveExtents(m_fLowerExtents, m_fUpperExtents, m_bLowerFixed, m_bUpperFixed);
  pEdit->SetCurveRanges(m_fLowerRange, m_fUpperRange);
  pEdit->SetCurves(m_Curves);
}

void xiiQtCurveEditDlg::closeEvent(QCloseEvent*)
{
  cancel();
}

void xiiQtCurveEditDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  UpdatePreview();
}

void xiiQtCurveEditDlg::OnCpMovedEvent(xiiUInt32 curveIdx, xiiUInt32 cpIdx, xiiInt64 iTickX, double newPosY)
{
  // update the local representation
  {
    auto& cp = m_Curves.m_Curves[curveIdx]->m_ControlPoints[cpIdx];

    if (cp.m_iTick != iTickX || cp.m_fValue != newPosY)
    {
      cp.m_iTick  = iTickX;
      cp.m_fValue = newPosY;
    }
  }

  // update the actual object
  {
    const xiiDocumentObject* pPoint = m_pObjectAccessor->GetChildObject(m_pCurveObject, "ControlPoints", cpIdx);

    m_pObjectAccessor->SetValue(pPoint, "Tick", iTickX).AssertSuccess();
    m_pObjectAccessor->SetValue(pPoint, "Value", newPosY).AssertSuccess();
  }
}

void xiiQtCurveEditDlg::OnCpDeletedEvent(xiiUInt32 curveIdx, xiiUInt32 cpIdx)
{
  // update the local representation
  {
    m_Curves.m_Curves[curveIdx]->m_ControlPoints.RemoveAtAndCopy(cpIdx);
  }

  // update the actual object
  {
    const xiiDocumentObject* pPoint = m_pObjectAccessor->GetChildObject(m_pCurveObject, "ControlPoints", cpIdx);
    m_pObjectAccessor->RemoveObject(pPoint).AssertSuccess();
  }
}

void xiiQtCurveEditDlg::OnTangentMovedEvent(xiiUInt32 curveIdx, xiiUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent)
{
  // update the local representation
  {
    auto& cp = m_Curves.m_Curves[curveIdx]->m_ControlPoints[cpIdx];

    if (rightTangent)
      cp.m_RightTangent.Set(newPosX, newPosY);
    else
      cp.m_LeftTangent.Set(newPosX, newPosY);
  }

  // update the actual object
  {
    const xiiDocumentObject* pPoint = m_pObjectAccessor->GetChildObject(m_pCurveObject, "ControlPoints", cpIdx);

    if (rightTangent)
      m_pObjectAccessor->SetValue(pPoint, "RightTangent", xiiVec2(newPosX, newPosY)).AssertSuccess();
    else
      m_pObjectAccessor->SetValue(pPoint, "LeftTangent", xiiVec2(newPosX, newPosY)).AssertSuccess();
  }
}

void xiiQtCurveEditDlg::OnInsertCpEvent(xiiUInt32 curveIdx, xiiInt64 tickX, double value)
{
  // update the local representation
  {
    xiiCurveControlPointData cp;
    cp.m_iTick  = tickX;
    cp.m_fValue = value;

    m_Curves.m_Curves[curveIdx]->m_ControlPoints.PushBack(cp);
  }

  // update the actual object
  {
    xiiUuid guid;
    m_pObjectAccessor->AddObject(m_pCurveObject, "ControlPoints", -1, xiiGetStaticRTTI<xiiCurveControlPointData>(), guid).AssertSuccess();

    const xiiDocumentObject* pPoint = m_pObjectAccessor->GetObject(guid);

    m_pObjectAccessor->SetValue(pPoint, "Tick", tickX).AssertSuccess();
    m_pObjectAccessor->SetValue(pPoint, "Value", value).AssertSuccess();
  }
}

void xiiQtCurveEditDlg::OnTangentLinkEvent(xiiUInt32 curveIdx, xiiUInt32 cpIdx, bool bLink)
{
  // update the local representation
  {
    auto& cp             = m_Curves.m_Curves[curveIdx]->m_ControlPoints[cpIdx];
    cp.m_bTangentsLinked = bLink;
  }

  // update the actual object
  {
    const xiiDocumentObject* pPoint = m_pObjectAccessor->GetChildObject(m_pCurveObject, "ControlPoints", cpIdx);

    m_pObjectAccessor->SetValue(pPoint, "Linked", bLink).AssertSuccess();
  }
}

void xiiQtCurveEditDlg::OnCpTangentModeEvent(xiiUInt32 curveIdx, xiiUInt32 cpIdx, bool rightTangent, int mode)
{
  // update the local representation
  {
    auto& cp = m_Curves.m_Curves[curveIdx]->m_ControlPoints[cpIdx];

    if (rightTangent)
      cp.m_RightTangentMode = (xiiCurveTangentMode::Enum)mode;
    else
      cp.m_LeftTangentMode = (xiiCurveTangentMode::Enum)mode;
  }

  // update the actual object
  {
    const xiiDocumentObject* pPoint = m_pObjectAccessor->GetChildObject(m_pCurveObject, "ControlPoints", cpIdx);

    if (rightTangent)
      m_pObjectAccessor->SetValue(pPoint, "RightTangentMode", mode).AssertSuccess();
    else
      m_pObjectAccessor->SetValue(pPoint, "LeftTangentMode", mode).AssertSuccess();
  }
}

void xiiQtCurveEditDlg::OnBeginCpChangesEvent(QString name)
{
  m_pObjectAccessor->StartTransaction(name.toUtf8().data());
}

void xiiQtCurveEditDlg::OnEndCpChangesEvent()
{
  m_pObjectAccessor->FinishTransaction();

  UpdatePreview();
}

void xiiQtCurveEditDlg::OnBeginOperationEvent(QString name)
{
  m_pObjectAccessor->BeginTemporaryCommands(name.toUtf8().data());
}

void xiiQtCurveEditDlg::OnEndOperationEvent(bool commit)
{
  if (commit)
    m_pObjectAccessor->FinishTemporaryCommands();
  else
    m_pObjectAccessor->CancelTemporaryCommands();

  UpdatePreview();
}

void xiiQtCurveEditDlg::on_actionUndo_triggered()
{
  auto& cmd = *m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();

  if (cmd.CanUndo() && cmd.GetUndoStackSize() > m_uiActionsUndoBaseline)
  {
    cmd.Undo().IgnoreResult();

    RetrieveCurveState();
    UpdatePreview();
  }
}

void xiiQtCurveEditDlg::on_actionRedo_triggered()
{
  auto& cmd = *m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();

  if (cmd.CanRedo())
  {
    cmd.Redo().IgnoreResult();

    RetrieveCurveState();
    UpdatePreview();
  }
}

void xiiQtCurveEditDlg::on_ButtonOk_clicked()
{
  QDialog::accept();
}

void xiiQtCurveEditDlg::on_ButtonCancel_clicked()
{
  cancel();
}
