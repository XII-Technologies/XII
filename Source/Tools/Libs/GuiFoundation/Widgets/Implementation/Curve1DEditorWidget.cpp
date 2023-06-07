#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Math.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>

#include <QGraphicsItem>
#include <QGraphicsSceneEvent>
#include <QMenu>
#include <QPainterPath>

#include <ToolsFoundation/Project/ToolsProject.h>

xiiDynamicArray<xiiString> xiiQtCurve1DEditorWidget::s_CurvePresets;

xiiQtCurve1DEditorWidget::xiiQtCurve1DEditorWidget(QWidget* pParent) :
  QWidget(pParent)
{
  setupUi(this);

  CurveEdit->SetGridBarWidget(GridBarWidget);

  connect(CurveEdit, &xiiQtCurveEditWidget::DeleteControlPointsEvent, this, &xiiQtCurve1DEditorWidget::onDeleteControlPoints);
  connect(CurveEdit, &xiiQtCurveEditWidget::DoubleClickEvent, this, &xiiQtCurve1DEditorWidget::onDoubleClick);
  connect(CurveEdit, &xiiQtCurveEditWidget::MoveControlPointsEvent, this, &xiiQtCurve1DEditorWidget::onMoveControlPoints);
  connect(CurveEdit, &xiiQtCurveEditWidget::MoveTangentsEvent, this, &xiiQtCurve1DEditorWidget::onMoveTangents);
  connect(CurveEdit, &xiiQtCurveEditWidget::BeginOperationEvent, this, &xiiQtCurve1DEditorWidget::onBeginOperation);
  connect(CurveEdit, &xiiQtCurveEditWidget::EndOperationEvent, this, &xiiQtCurve1DEditorWidget::onEndOperation);
  connect(CurveEdit, &xiiQtCurveEditWidget::ScaleControlPointsEvent, this, &xiiQtCurve1DEditorWidget::onScaleControlPoints);
  connect(CurveEdit, &xiiQtCurveEditWidget::ContextMenuEvent, this, &xiiQtCurve1DEditorWidget::onContextMenu);
  connect(CurveEdit, &xiiQtCurveEditWidget::SelectionChangedEvent, this, &xiiQtCurve1DEditorWidget::onSelectionChanged);
  connect(CurveEdit, &xiiQtCurveEditWidget::MoveCurveEvent, this, &xiiQtCurve1DEditorWidget::onMoveCurve);

  LinePosition->setEnabled(false);
  LineValue->setEnabled(false);

  if (s_CurvePresets.IsEmpty())
  {
    FindAllPresets();
  }
}

xiiQtCurve1DEditorWidget::~xiiQtCurve1DEditorWidget() = default;

void xiiQtCurve1DEditorWidget::SetCurveExtents(double fLowerBound, double fUpperBound, bool bLowerIsFixed, bool bUpperIsFixed)
{
  CurveEdit->m_fLowerExtent      = fLowerBound;
  CurveEdit->m_fUpperExtent      = fUpperBound;
  CurveEdit->m_bLowerExtentFixed = bLowerIsFixed;
  CurveEdit->m_bUpperExtentFixed = bUpperIsFixed;
}

void xiiQtCurve1DEditorWidget::SetCurveRanges(double fLowerRange, double fUpperRange)
{
  CurveEdit->m_fLowerRange = fLowerRange;
  CurveEdit->m_fUpperRange = fUpperRange;
}

void xiiQtCurve1DEditorWidget::SetCurves(const xiiCurveGroupData& curves)
{
  xiiQtScopedUpdatesDisabled ud(this);
  xiiQtScopedBlockSignals    bs(this);

  m_Curves.CloneFrom(curves);

  CurveEdit->SetCurves(&m_Curves);
  m_fCurveDuration = CurveEdit->GetMaxCurveExtent();

  UpdateSpinBoxes();
}


void xiiQtCurve1DEditorWidget::SetScrubberPosition(xiiUInt64 uiTick)
{
  CurveEdit->SetScrubberPosition(uiTick / 4800.0);
}


void xiiQtCurve1DEditorWidget::ClearSelection()
{
  CurveEdit->ClearSelection();
}

void xiiQtCurve1DEditorWidget::FrameCurve()
{
  CurveEdit->FrameCurve();
}

void xiiQtCurve1DEditorWidget::FrameSelection()
{
  CurveEdit->FrameSelection();
}

void xiiQtCurve1DEditorWidget::MakeRepeatable(bool bAdjustLastPoint)
{
  Q_EMIT BeginOperationEvent("Make Curve Repeatable");

  for (xiiUInt32 iCurveIdx = 0; iCurveIdx < m_Curves.m_Curves.GetCount(); ++iCurveIdx)
  {
    const auto& curve = *m_Curves.m_Curves[iCurveIdx];

    const xiiUInt32 uiNumCps = curve.m_ControlPoints.GetCount();
    if (uiNumCps < 2)
      continue;

    xiiInt64  iMinTick = curve.m_ControlPoints[0].m_iTick;
    xiiInt64  iMaxTick = curve.m_ControlPoints[0].m_iTick;
    xiiUInt32 uiMinCp  = 0;
    xiiUInt32 uiMaxCp  = 0;

    for (xiiUInt32 uiCpIdx = 1; uiCpIdx < uiNumCps; ++uiCpIdx)
    {
      const xiiInt64 x = curve.m_ControlPoints[uiCpIdx].m_iTick;

      if (x < iMinTick)
      {
        iMinTick = x;
        uiMinCp  = uiCpIdx;
      }
      if (x > iMaxTick)
      {
        iMaxTick = x;
        uiMaxCp  = uiCpIdx;
      }
    }

    if (uiMinCp == uiMaxCp)
      continue;

    // copy data, the first Q_EMIT may change the backing store
    const xiiCurveControlPointData cpLeft  = curve.m_ControlPoints[uiMinCp];
    const xiiCurveControlPointData cpRight = curve.m_ControlPoints[uiMaxCp];

    if (bAdjustLastPoint)
    {
      Q_EMIT CpMovedEvent(iCurveIdx, uiMaxCp, (xiiInt64)(m_fCurveDuration * 4800.0), cpLeft.m_fValue);
      Q_EMIT TangentMovedEvent(iCurveIdx, uiMaxCp, -cpLeft.m_RightTangent.x, -cpLeft.m_RightTangent.y, false);
    }
    else
    {
      Q_EMIT CpMovedEvent(iCurveIdx, uiMinCp, 0, cpRight.m_fValue);
      Q_EMIT TangentMovedEvent(iCurveIdx, uiMinCp, -cpRight.m_LeftTangent.x, -cpRight.m_LeftTangent.y, true);
    }
  }

  Q_EMIT EndOperationEvent(true);
}

void xiiQtCurve1DEditorWidget::NormalizeCurveX(xiiUInt32 uiActiveCurve)
{
  if (uiActiveCurve >= m_Curves.m_Curves.GetCount())
    return;

  xiiCurve1D CurveData;
  m_Curves.ConvertToRuntimeData(uiActiveCurve, CurveData);

  const xiiUInt32 numCPs = CurveData.GetNumControlPoints();

  if (numCPs < 2)
    return;

  CurveData.RecomputeExtents();

  double minX, maxX;
  CurveData.QueryExtents(minX, maxX);

  if (minX == 0 && maxX == 1)
    return;

  Q_EMIT BeginOperationEvent("Normalize Curve (X)");

  const float rangeNorm = 1.0f / (maxX - minX);

  for (xiiUInt32 i = 0; i < numCPs; ++i)
  {
    const auto& cp = CurveData.GetControlPoint(i);

    xiiVec2d pos = cp.m_Position;
    pos.x -= minX;
    pos.x *= rangeNorm;

    Q_EMIT CpMovedEvent(uiActiveCurve, i, m_Curves.TickFromTime(xiiTime::Seconds(pos.x)), pos.y);

    xiiVec2 lt = cp.m_LeftTangent;
    lt.x *= rangeNorm;
    Q_EMIT TangentMovedEvent(uiActiveCurve, i, lt.x, lt.y, false);

    xiiVec2 rt = cp.m_RightTangent;
    rt.x *= rangeNorm;
    Q_EMIT TangentMovedEvent(uiActiveCurve, i, rt.x, rt.y, true);
  }

  Q_EMIT EndOperationEvent(true);

  FrameCurve();
}

void xiiQtCurve1DEditorWidget::NormalizeCurveY(xiiUInt32 uiActiveCurve)
{
  if (uiActiveCurve >= m_Curves.m_Curves.GetCount())
    return;

  xiiCurve1D CurveData;
  m_Curves.ConvertToRuntimeData(uiActiveCurve, CurveData);

  const xiiUInt32 numCPs = CurveData.GetNumControlPoints();

  if (numCPs < 2)
    return;

  xiiCurve1D CurveDataSorted = CurveData;
  CurveDataSorted.SortControlPoints();
  CurveDataSorted.CreateLinearApproximation();

  double minY, maxY;
  CurveDataSorted.QueryExtremeValues(minY, maxY);

  if (minY == 0 && maxY == 1)
    return;

  Q_EMIT BeginOperationEvent("Normalize Curve (Y)");

  const float rangeNorm = 1.0f / (maxY - minY);

  for (xiiUInt32 i = 0; i < numCPs; ++i)
  {
    const auto& cp = CurveData.GetControlPoint(i);

    xiiVec2d pos = cp.m_Position;
    pos.y -= minY;
    pos.y *= rangeNorm;

    Q_EMIT CpMovedEvent(uiActiveCurve, i, m_Curves.TickFromTime(xiiTime::Seconds(pos.x)), pos.y);

    xiiVec2 lt = cp.m_LeftTangent;
    lt.y *= rangeNorm;
    Q_EMIT TangentMovedEvent(uiActiveCurve, i, lt.x, lt.y, false);

    xiiVec2 rt = cp.m_RightTangent;
    rt.y *= rangeNorm;
    Q_EMIT TangentMovedEvent(uiActiveCurve, i, rt.x, rt.y, true);
  }

  Q_EMIT EndOperationEvent(true);

  FrameCurve();
}

struct PtToDelete
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiCurveIdx;
  xiiUInt32 m_uiPointIdx;

  bool operator<(const PtToDelete& rhs) const { return m_uiPointIdx > rhs.m_uiPointIdx; }
};

void xiiQtCurve1DEditorWidget::ClearAllPoints()
{
  Q_EMIT BeginCpChangesEvent("Delete Points");

  xiiHybridArray<PtToDelete, 16> delOrder;

  for (xiiUInt32 curveIdx = 0; curveIdx < m_Curves.m_Curves.GetCount(); ++curveIdx)
  {
    xiiCurve1D curveData;
    m_Curves.m_Curves[curveIdx]->ConvertToRuntimeData(curveData);

    for (xiiUInt32 i = 0; i < curveData.GetNumControlPoints(); ++i)
    {
      auto& pt        = delOrder.ExpandAndGetRef();
      pt.m_uiCurveIdx = curveIdx;
      pt.m_uiPointIdx = i;
    }
  }

  delOrder.Sort();

  // Delete sorted from back to front to prevent point indices becoming invalidated
  for (const auto& pt : delOrder)
  {
    Q_EMIT CpDeletedEvent(pt.m_uiCurveIdx, pt.m_uiPointIdx);
  }

  m_Curves.Clear();

  Q_EMIT EndCpChangesEvent();
}

void xiiQtCurve1DEditorWidget::MirrorHorizontally(xiiUInt32 uiActiveCurve)
{
  if (uiActiveCurve >= m_Curves.m_Curves.GetCount())
    return;

  xiiCurve1D CurveData;
  m_Curves.ConvertToRuntimeData(uiActiveCurve, CurveData);

  const xiiUInt32 numCPs = CurveData.GetNumControlPoints();

  if (numCPs < 2)
    return;

  CurveData.RecomputeExtents();

  double minX, maxX;
  CurveData.QueryExtents(minX, maxX);

  double centerX = minX + (maxX - minX) * 0.5;

  Q_EMIT BeginOperationEvent("Mirror Curve Horizontally");

  for (xiiUInt32 i = 0; i < numCPs; ++i)
  {
    const auto& cp = CurveData.GetControlPoint(i);

    // mirror position around center
    xiiVec2d pos = cp.m_Position;
    pos.x        = centerX - (pos.x - centerX);

    Q_EMIT CpMovedEvent(uiActiveCurve, i, m_Curves.TickFromTime(xiiTime::Seconds(pos.x)), pos.y);

    xiiVec2 lt = cp.m_RightTangent;
    xiiVec2 rt = cp.m_LeftTangent;

    lt.x = -lt.x;
    rt.x = -rt.x;

    // swap tangents from left to right
    Q_EMIT TangentMovedEvent(uiActiveCurve, i, lt.x, lt.y, false);
    Q_EMIT TangentMovedEvent(uiActiveCurve, i, rt.x, rt.y, true);

    // swap tangent modes from left to right
    Q_EMIT CpTangentModeEvent(uiActiveCurve, i, false, (int)cp.m_TangentModeRight.GetValue());
    Q_EMIT CpTangentModeEvent(uiActiveCurve, i, true, (int)cp.m_TangentModeLeft.GetValue());
  }

  Q_EMIT EndOperationEvent(true);
}

void xiiQtCurve1DEditorWidget::MirrorVertically(xiiUInt32 uiActiveCurve)
{
  if (uiActiveCurve >= m_Curves.m_Curves.GetCount())
    return;

  xiiCurve1D CurveData;
  m_Curves.ConvertToRuntimeData(uiActiveCurve, CurveData);

  const xiiUInt32 numCPs = CurveData.GetNumControlPoints();

  if (numCPs < 2)
    return;

  CurveData.SortControlPoints();
  CurveData.CreateLinearApproximation();

  double minY, maxY;
  CurveData.QueryExtremeValues(minY, maxY);

  double centerY = minY + (maxY - minY) * 0.5;

  Q_EMIT BeginOperationEvent("Mirror Curve Vertically");

  for (xiiUInt32 i = 0; i < numCPs; ++i)
  {
    const auto& cp = CurveData.GetControlPoint(i);

    // mirror value around center
    xiiVec2d pos = cp.m_Position;
    pos.y        = centerY - (pos.y - centerY);

    Q_EMIT CpMovedEvent(uiActiveCurve, i, m_Curves.TickFromTime(xiiTime::Seconds(pos.x)), pos.y);

    xiiVec2 lt = cp.m_LeftTangent;
    xiiVec2 rt = cp.m_RightTangent;

    lt.y = -lt.y;
    rt.y = -rt.y;

    // swap tangents Y directions
    Q_EMIT TangentMovedEvent(uiActiveCurve, i, lt.x, lt.y, false);
    Q_EMIT TangentMovedEvent(uiActiveCurve, i, rt.x, rt.y, true);
  }

  Q_EMIT EndOperationEvent(true);
}

void xiiQtCurve1DEditorWidget::onDeleteControlPoints()
{
  const auto selection = CurveEdit->GetSelection();

  if (selection.IsEmpty())
    return;

  CurveEdit->ClearSelection();

  Q_EMIT BeginCpChangesEvent("Delete Points");

  xiiHybridArray<PtToDelete, 16> delOrder;

  for (const auto& item : selection)
  {
    auto& pt        = delOrder.ExpandAndGetRef();
    pt.m_uiCurveIdx = item.m_uiCurve;
    pt.m_uiPointIdx = item.m_uiPoint;
  }

  delOrder.Sort();

  // delete sorted from back to front to prevent point indices becoming invalidated
  for (const auto& pt : delOrder)
  {
    Q_EMIT CpDeletedEvent(pt.m_uiCurveIdx, pt.m_uiPointIdx);
  }

  Q_EMIT EndCpChangesEvent();
}

void xiiQtCurve1DEditorWidget::onDoubleClick(const QPointF& scenePos, const QPointF& epsilon)
{
  Q_EMIT BeginCpChangesEvent("Add Control Point");

  InsertCpAt(scenePos.x(), scenePos.y(), xiiVec2d(xiiMath::Abs(epsilon.x()), xiiMath::Abs(epsilon.y())));

  Q_EMIT EndCpChangesEvent();
}

void xiiQtCurve1DEditorWidget::onMoveControlPoints(double x, double y)
{
  m_vControlPointMove += xiiVec2d(x, y);

  const auto selection = CurveEdit->GetSelection();

  if (selection.IsEmpty())
    return;

  Q_EMIT BeginCpChangesEvent("Move Points");

  for (const auto& cpSel : selection)
  {
    const auto& cp     = m_CurvesBackup.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint];
    xiiVec2d    newPos = xiiVec2d(cp.GetTickAsTime().GetSeconds(), cp.m_fValue) + m_vControlPointMove;

    ClampPoint(newPos.x, newPos.y);

    Q_EMIT CpMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, m_Curves.TickFromTime(xiiTime::Seconds(newPos.x)), newPos.y);
  }

  Q_EMIT EndCpChangesEvent();
}

void xiiQtCurve1DEditorWidget::onScaleControlPoints(QPointF refPt, double scaleX, double scaleY)
{
  const auto selection = CurveEdit->GetSelection();

  if (selection.IsEmpty())
    return;

  const xiiVec2d ref(refPt.x(), refPt.y());
  const xiiVec2d scale(scaleX, scaleY);

  Q_EMIT BeginCpChangesEvent("Scale Points");

  for (const auto& cpSel : selection)
  {
    const auto& cp     = m_CurvesBackup.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint];
    xiiVec2d    newPos = ref + (xiiVec2d(cp.GetTickAsTime().GetSeconds(), cp.m_fValue) - ref).CompMul(scale);

    ClampPoint(newPos.x, newPos.y);

    Q_EMIT CpMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, m_Curves.TickFromTime(xiiTime::Seconds(newPos.x)), newPos.y);
  }

  Q_EMIT EndCpChangesEvent();
}

void xiiQtCurve1DEditorWidget::onMoveTangents(float x, float y)
{
  m_vTangentMove += xiiVec2(x, y);

  xiiInt32 iCurve;
  xiiInt32 iPoint;
  bool     bLeftTangent;

  if (!CurveEdit->GetSelectedTangent(iCurve, iPoint, bLeftTangent))
    return;

  Q_EMIT BeginCpChangesEvent("Move Tangents");

  {
    const auto& cp = m_CurvesBackup.m_Curves[iCurve]->m_ControlPoints[iPoint];
    xiiVec2     newPos;

    if (bLeftTangent)
      newPos = cp.m_LeftTangent + m_vTangentMove;
    else
      newPos = cp.m_RightTangent + m_vTangentMove;

    newPos.y = xiiMath::Clamp(newPos.y, -100000.0f, +100000.0f);

    Q_EMIT TangentMovedEvent(iCurve, iPoint, newPos.x, newPos.y, !bLeftTangent);

    if (cp.m_bTangentsLinked)
    {
      Q_EMIT TangentMovedEvent(iCurve, iPoint, -newPos.x, -newPos.y, bLeftTangent);
    }
  }

  Q_EMIT EndCpChangesEvent();
}

void xiiQtCurve1DEditorWidget::onBeginOperation(QString name)
{
  m_CurvesBackup.CloneFrom(m_Curves);
  m_vTangentMove.SetZero();
  m_vControlPointMove.SetZero();

  Q_EMIT BeginOperationEvent(name);
}

void xiiQtCurve1DEditorWidget::onEndOperation(bool commit)
{
  Q_EMIT EndOperationEvent(commit);
}

void xiiQtCurve1DEditorWidget::onContextMenu(QPoint pos, QPointF scenePos)
{
  const bool bIsCurveNonEmpty = !m_Curves.m_Curves.IsEmpty() && !m_Curves.m_Curves[0]->m_ControlPoints.IsEmpty();

  m_ContextMenuScenePos = scenePos;

  QMenu m(this);
  m.setDefaultAction(m.addAction("Add Point\tDbl Click", this, SLOT(onAddPoint())));

  const auto& selection = CurveEdit->GetSelection();

  if (bIsCurveNonEmpty)
  {
    QMenu* cmSel = m.addMenu("Selection");
    cmSel->addAction("Select All\tCtrl+A", this, [this]() { CurveEdit->SelectAll(); });

    if (!selection.IsEmpty())
    {
      cmSel->addAction("Clear Selection\tESC", this, [this]() { CurveEdit->ClearSelection(); });

      cmSel->addAction(
        "Frame Selection\tShift+F", this, [this]() { FrameSelection(); });

      cmSel->addSeparator();

      cmSel->addAction("Delete Points\tDel", this, SLOT(onDeleteControlPoints()));
      cmSel->addSeparator();
      cmSel->addAction("Link Tangents", this, SLOT(onLinkTangents()));
      cmSel->addAction("Break Tangents", this, SLOT(onBreakTangents()));
      cmSel->addAction("Flatten Tangents", this, SLOT(onFlattenTangents()));

      QMenu* cmLT = cmSel->addMenu("Left Tangents");
      QMenu* cmRT = cmSel->addMenu("Right Tangents");
      QMenu* cmBT = cmSel->addMenu("Both Tangents");

      cmLT->addAction("Auto", this, [this]() { SetTangentMode(xiiCurveTangentMode::Auto, true, false); });
      cmLT->addAction("Bezier", this, [this]() { SetTangentMode(xiiCurveTangentMode::Bezier, true, false); });
      cmLT->addAction("Fixed Length", this, [this]() { SetTangentMode(xiiCurveTangentMode::FixedLength, true, false); });
      cmLT->addAction("Linear", this, [this]() { SetTangentMode(xiiCurveTangentMode::Linear, true, false); });

      cmRT->addAction("Auto", this, [this]() { SetTangentMode(xiiCurveTangentMode::Auto, false, true); });
      cmRT->addAction("Bezier", this, [this]() { SetTangentMode(xiiCurveTangentMode::Bezier, false, true); });
      cmRT->addAction("Fixed Length", this, [this]() { SetTangentMode(xiiCurveTangentMode::FixedLength, false, true); });
      cmRT->addAction("Linear", this, [this]() { SetTangentMode(xiiCurveTangentMode::Linear, false, true); });

      cmBT->addAction("Auto", this, [this]() { SetTangentMode(xiiCurveTangentMode::Auto, true, true); });
      cmBT->addAction("Bezier", this, [this]() { SetTangentMode(xiiCurveTangentMode::Bezier, true, true); });
      cmBT->addAction("Fixed Length", this, [this]() { SetTangentMode(xiiCurveTangentMode::FixedLength, true, true); });
      cmBT->addAction("Linear", this, [this]() { SetTangentMode(xiiCurveTangentMode::Linear, true, true); });
    }

    {
      QMenu* cm = m.addMenu("Curve");
      cm->addSeparator();
      cm->addAction("Mirror Horizontally", this, [this]() { MirrorHorizontally(0); });
      cm->addAction("Mirror Vertically", this, [this]() { MirrorVertically(0); });
      cm->addAction("Normalize X", this, [this]() { NormalizeCurveX(0); });
      cm->addAction("Normalize Y", this, [this]() { NormalizeCurveY(0); });
      cm->addAction("Loop: Adjust Last Point", this, [this]() { MakeRepeatable(true); });
      cm->addAction("Loop: Adjust First Point", this, [this]() { MakeRepeatable(false); });
      cm->addAction("Clear Curve", this, [this]() { ClearAllPoints(); });

      cm->addAction(
        "Frame Curve\tCtrl+F", this, [this]() { FrameCurve(); });
    }
  }

  QMenu* presentsMenu = m.addMenu("Presets");

  {
    if (bIsCurveNonEmpty)
    {
      presentsMenu->addAction("Save As Preset...", this, &xiiQtCurve1DEditorWidget::onSaveAsPreset);
    }

    presentsMenu->addAction("Load Preset...", this, &xiiQtCurve1DEditorWidget::onLoadPreset);
    presentsMenu->addSeparator();
  }

  {
    QMenu* cm = presentsMenu->addMenu("Constants");

    cm->addAction("Constant Zero", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::ConstantZero, false); });
    cm->addAction("Constant Quarter", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::ConstantQuarter, false); });
    cm->addAction("Constant Half", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::ConstantHalf, false); });
    cm->addAction("Constant Three Fourths", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::ConstantThreeFourths, false); });
    cm->addAction("Constant One", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::ConstantOne, false); });
  }

  /// Direct

  {
    QMenu* curveMenu = presentsMenu->addMenu("Ease In");

    curveMenu->addAction("Linear", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InLinear, false); });
    curveMenu->addAction("Sine", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InSine, false); });
    curveMenu->addAction("Quadratic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InQuad, false); });
    curveMenu->addAction("Cubic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InCubic, false); });
    curveMenu->addAction("Quartic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InQuartic, false); });
    curveMenu->addAction("Quintic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InQuintic, false); });
    curveMenu->addAction("Exponential", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InExpo, false); });
    curveMenu->addAction("Circ", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InCirc, false); });
    curveMenu->addAction("Back", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InBack, false); });
    curveMenu->addAction("Elastic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InElastic, false); });
    curveMenu->addAction("Bounce", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InBounce, false); });
  }

  {
    QMenu* curveMenu = presentsMenu->addMenu("Ease Out");

    curveMenu->addAction("Linear", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::OutLinear, false); });
    curveMenu->addAction("Sine", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::OutSine, false); });
    curveMenu->addAction("Quadratic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::OutQuad, false); });
    curveMenu->addAction("Cubic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::OutCubic, false); });
    curveMenu->addAction("Quartic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::OutQuartic, false); });
    curveMenu->addAction("Quintic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::OutQuintic, false); });
    curveMenu->addAction("Exponential", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::OutExpo, false); });
    curveMenu->addAction("Circ", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::OutCirc, false); });
    curveMenu->addAction("Back", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::OutBack, false); });
    curveMenu->addAction("Elastic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::OutElastic, false); });
    curveMenu->addAction("Bounce", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::OutBounce, false); });
  }

  {
    QMenu* curveMenu = presentsMenu->addMenu("Ease In Out");

    curveMenu->addAction("Linear", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InOutLinear, false); });
    curveMenu->addAction("Sine", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InOutSine, false); });
    curveMenu->addAction("Quadratic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InOutQuad, false); });
    curveMenu->addAction("Cubic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InOutCubic, false); });
    curveMenu->addAction("Quartic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InOutQuartic, false); });
    curveMenu->addAction("Quintic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InOutQuintic, false); });
    curveMenu->addAction("Exponential", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InOutExpo, false); });
    curveMenu->addAction("Circ", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InOutCirc, false); });
    curveMenu->addAction("Back", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InOutBack, false); });
    curveMenu->addAction("Elastic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InOutElastic, false); });
    curveMenu->addAction("Bounce", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InOutBounce, false); });
  }

  /// Inverse

  {
    QMenu* curveMenu = presentsMenu->addMenu("Ease In Inverse");

    curveMenu->addAction("Linear", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InLinear, false); });
    curveMenu->addAction("Sine", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InSine, false); });
    curveMenu->addAction("Quadratic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InQuad, false); });
    curveMenu->addAction("Cubic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InCubic, false); });
    curveMenu->addAction("Quartic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InQuartic, false); });
    curveMenu->addAction("Quintic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InQuintic, false); });
    curveMenu->addAction("Exponential", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InExpo, false); });
    curveMenu->addAction("Circ", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InCirc, false); });
    curveMenu->addAction("Back", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InBack, false); });
    curveMenu->addAction("Elastic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InElastic, false); });
    curveMenu->addAction("Bounce", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InBounce, false); });
  }

  {
    QMenu* curveMenu = presentsMenu->addMenu("Ease Out Inverse");

    curveMenu->addAction("Linear", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::OutLinear, false); });
    curveMenu->addAction("Sine", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::OutSine, false); });
    curveMenu->addAction("Quadratic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::OutQuad, false); });
    curveMenu->addAction("Cubic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::OutCubic, false); });
    curveMenu->addAction("Quartic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::OutQuartic, false); });
    curveMenu->addAction("Quintic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::OutQuintic, false); });
    curveMenu->addAction("Exponential", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::OutExpo, false); });
    curveMenu->addAction("Circ", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::OutCirc, false); });
    curveMenu->addAction("Back", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::OutBack, false); });
    curveMenu->addAction("Elastic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::OutElastic, false); });
    curveMenu->addAction("Bounce", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::OutBounce, false); });
  }

  {
    QMenu* curveMenu = presentsMenu->addMenu("Ease In Out Inverse");

    curveMenu->addAction("Linear", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InOutLinear, false); });
    curveMenu->addAction("Sine", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InOutSine, false); });
    curveMenu->addAction("Quadratic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InOutQuad, false); });
    curveMenu->addAction("Cubic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InOutCubic, false); });
    curveMenu->addAction("Quartic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InOutQuartic, false); });
    curveMenu->addAction("Quintic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InOutQuintic, false); });
    curveMenu->addAction("Exponential", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InOutExpo, false); });
    curveMenu->addAction("Circ", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InOutCirc, false); });
    curveMenu->addAction("Back", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InOutBack, false); });
    curveMenu->addAction("Elastic", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InOutElastic, false); });
    curveMenu->addAction("Bounce", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::InOutBounce, false); });
  }

  /// Other

  {
    QMenu* curveMenu = presentsMenu->addMenu("Ease In Hold Out");

    curveMenu->addAction("Conical", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::Conical, false); });
    curveMenu->addAction("Fade In / Fade Out", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::FadeInFadeOut, false); });
    curveMenu->addAction("Fade In / Hold / Fade Out", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::FadeInHoldFadeOut, false); });
    curveMenu->addAction("Bell", this, [this]() { onGenerateCurve(xiiMath::xiiEasingFunctions::Bell, false); });
  }

  // Show all available presets from disk in a hierarchical menu structure
  {
    xiiMap<xiiString, QMenu*> subMenus;
    subMenus[""] = presentsMenu;

    auto GetSubMenu = [&](const xiiStringBuilder& sPath, auto getSubMenu2) {
      auto it = subMenus.Find(sPath);
      if (it.IsValid())
        return it.Value();

      xiiStringBuilder parent = sPath;
      parent.PathParentDirectory();
      parent.Trim("/");

      QMenu* pParentMenu = getSubMenu2(parent, getSubMenu2);
      QMenu* pMenu       = pParentMenu->addMenu(sPath.GetFileName().GetData(parent));
      subMenus[sPath]    = pMenu;

      return pMenu;
    };

    xiiStringBuilder sPresetName, sPresetPath;
    for (const auto& preset : s_CurvePresets)
    {
      sPresetPath = xiiPathUtils::GetFileDirectory(preset);
      sPresetName = xiiPathUtils::GetFileName(preset);

      sPresetPath.Trim("/");

      GetSubMenu(sPresetPath, GetSubMenu)->addAction(sPresetName.GetData(), [this, preset]() { LoadCurvePreset(preset).IgnoreResult(); });
    }
  }

  m.exec(pos);
}

void xiiQtCurve1DEditorWidget::onAddPoint()
{
  Q_EMIT BeginCpChangesEvent("Add Control Point");

  InsertCpAt(m_ContextMenuScenePos.x(), m_ContextMenuScenePos.y(), xiiVec2d::ZeroVector());

  Q_EMIT EndCpChangesEvent();
}

void xiiQtCurve1DEditorWidget::onLinkTangents()
{
  const auto& selection = CurveEdit->GetSelection();

  Q_EMIT BeginOperationEvent("Link Tangents");

  for (const auto& cpSel : selection)
  {
    Q_EMIT TangentLinkEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, true);
  }

  Q_EMIT EndOperationEvent(true);
}

void xiiQtCurve1DEditorWidget::onBreakTangents()
{
  const auto& selection = CurveEdit->GetSelection();

  Q_EMIT BeginOperationEvent("Break Tangents");

  for (const auto& cpSel : selection)
  {
    Q_EMIT TangentLinkEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, false);
  }

  Q_EMIT EndOperationEvent(true);
}


void xiiQtCurve1DEditorWidget::onFlattenTangents()
{
  const auto& selection = CurveEdit->GetSelection();

  Q_EMIT BeginOperationEvent("Flatten Tangents");

  for (const auto& cpSel : selection)
  {
    // don't use references, the signals may move the data in memory
    const xiiVec2 tL = m_Curves.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint].m_LeftTangent;
    const xiiVec2 tR = m_Curves.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint].m_RightTangent;

    // clamp the X position, to prevent tangents with zero length
    Q_EMIT TangentMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, xiiMath::Min(tL.x, -0.02f), 0, false);
    Q_EMIT TangentMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, xiiMath::Max(tR.x, +0.02f), 0, true);
  }

  Q_EMIT EndOperationEvent(true);
}

void xiiQtCurve1DEditorWidget::InsertCpAt(double posX, double value, xiiVec2d epsilon)
{
  int curveIdx = 0, cpIdx = 0;

  ClampPoint(posX, value);

  // do not insert at a point where a CP already exists
  if (PickControlPointAt(posX, value, epsilon, curveIdx, cpIdx))
    return;

  if (!PickCurveAt(posX, value, epsilon.y, curveIdx, value))
  {
    // by default insert into curve 0
    curveIdx = 0;
  }

  Q_EMIT InsertCpEvent(curveIdx, m_Curves.TickFromTime(xiiTime::Seconds(posX)), value);
}


bool xiiQtCurve1DEditorWidget::PickCurveAt(double x, double y, double fMaxDistanceY, xiiInt32& out_iCurveIdx, double& out_ValueY) const
{
  out_iCurveIdx = -1;
  xiiCurve1D CurveData;

  for (xiiUInt32 i = 0; i < m_Curves.m_Curves.GetCount(); ++i)
  {
    m_Curves.ConvertToRuntimeData(i, CurveData);

    CurveData.SortControlPoints();
    CurveData.CreateLinearApproximation();

    double minVal, maxVal;
    CurveData.QueryExtents(minVal, maxVal);

    const double val = CurveData.Evaluate(x);

    const double dist = xiiMath::Abs(val - y);
    if (dist < fMaxDistanceY)
    {
      fMaxDistanceY = dist;
      out_iCurveIdx = i;
      out_ValueY    = val;
    }
  }

  return out_iCurveIdx >= 0;
}

bool xiiQtCurve1DEditorWidget::PickControlPointAt(double x, double y, xiiVec2d vMaxDistance, xiiInt32& out_iCurveIdx, xiiInt32& out_iCpIdx) const
{
  const xiiVec2d at(x, y);

  out_iCurveIdx = -1;
  out_iCpIdx    = -1;

  xiiCurve1D CurveData;

  for (xiiUInt32 iCurve = 0; iCurve < m_Curves.m_Curves.GetCount(); ++iCurve)
  {
    m_Curves.ConvertToRuntimeData(iCurve, CurveData);

    for (xiiUInt32 iCP = 0; iCP < CurveData.GetNumControlPoints(); ++iCP)
    {
      const auto&    cp   = CurveData.GetControlPoint(iCP);
      const xiiVec2d dist = cp.m_Position - at;

      if (xiiMath::Abs(dist.x) <= vMaxDistance.x && xiiMath::Abs(dist.y) <= vMaxDistance.y)
      {
        vMaxDistance.x = xiiMath::Abs(dist.x);
        vMaxDistance.y = xiiMath::Abs(dist.y);

        out_iCurveIdx = iCurve;
        out_iCpIdx    = iCP;
      }
    }
  }

  return out_iCpIdx >= 0;
}

void xiiQtCurve1DEditorWidget::onSelectionChanged()
{
  UpdateSpinBoxes();
}


void xiiQtCurve1DEditorWidget::onMoveCurve(xiiInt32 iCurve, double moveY)
{
  m_vControlPointMove.y += moveY;

  Q_EMIT BeginCpChangesEvent("Move Curve");

  const auto& curve    = *m_CurvesBackup.m_Curves[iCurve];
  xiiUInt32   uiNumCps = curve.m_ControlPoints.GetCount();
  for (xiiUInt32 i = 0; i < uiNumCps; ++i)
  {
    const xiiInt64 x = curve.m_ControlPoints[i].m_iTick;
    const float    y = curve.m_ControlPoints[i].m_fValue + m_vControlPointMove.y;

    Q_EMIT CpMovedEvent(iCurve, i, x, y);
  }

  Q_EMIT EndCpChangesEvent();
}

void xiiQtCurve1DEditorWidget::onGenerateCurve(xiiMath::xiiEasingFunctions function, bool inverse)
{
  Q_EMIT BeginCpChangesEvent("Generate Curve");

  // Delete all existing control points
  ClearAllPoints();

#if 1

  xiiCurve1D cmp;

  const xiiUInt32 uiFrames = m_Curves.m_uiFramesPerSecond / 2;
  const double    invFps   = 1.0 / uiFrames;

  struct Sample
  {
    double m_fPos          = 0;
    double m_fCorrectValue = 0;
    bool   m_bInserted     = false;
  };

  xiiHybridArray<Sample, 60> samples;
  samples.SetCount(uiFrames + 1);

  for (xiiUInt32 i = 0; i <= uiFrames; ++i)
  {
    const double x = i * invFps;

    samples[i].m_fPos          = x;
    samples[i].m_fCorrectValue = xiiMath::GetEasingValue<double>(function, x, inverse);
  }

  auto AddPt = [&](xiiUInt32 uiIdx) {
    samples[uiIdx].m_bInserted = true;
    const double x             = samples[uiIdx].m_fPos;
    const double y             = samples[uiIdx].m_fCorrectValue;

    cmp.AddControlPoint(x).m_Position.y = y;
    InsertCpAt(x, y, xiiVec2d::ZeroVector());
  };

  AddPt(0);
  AddPt(samples.GetCount() - 1);

  // Only add points that are necessary to reach the target curve within a certain error threshold
  // We find the point that has the highest error (along Y) and insert that
  // Then we check all points again and find the next worst point, until no point violates the error threshold anymore
  // This loop is O(n*n), but apparently no problem for the 30 samples that we currently use
  // This loop is O(n*n), but apparently no problem for the 30 samples that we currently use
  while (true)
  {
    cmp.SortControlPoints();
    cmp.CreateLinearApproximation();

    double    fMaxError     = 0.01; // This is the error threshold
    xiiUInt32 uiMaxErrorIdx = 0xffffffff;

    for (xiiUInt32 idx = 0; idx < samples.GetCount(); ++idx)
    {
      auto& sample = samples[idx];
      if (sample.m_bInserted)
        continue;

      const double eval = cmp.Evaluate(sample.m_fPos);
      const double err  = xiiMath::Abs(eval - sample.m_fCorrectValue);

      if (err > fMaxError)
      {
        fMaxError     = eval;
        uiMaxErrorIdx = idx;
      }
    }

    if (uiMaxErrorIdx == 0xffffffff)
      break;

    AddPt(uiMaxErrorIdx);
  }

#else

  const double fps = m_Curves.m_uiFramesPerSecond;

  for (xiiUInt32 i = 0; i <= m_Curves.m_uiFramesPerSecond; i += 2)
  {
    const double x = i / fps;
    InsertCpAt(x, xiiMath::GetEasingValue<double>(easingFunction, x, inverse), xiiVec2d::ZeroVector());
  }

#endif

  Q_EMIT EndCpChangesEvent();
}

static QString s_sPresetSaveDir;

void xiiQtCurve1DEditorWidget::onSaveAsPreset()
{
  if (s_sPresetSaveDir.isEmpty())
  {
    s_sPresetSaveDir = xiiToolsProject::GetSingleton()->GetProjectDirectory().GetData();
    s_sPresetSaveDir.append("/Editor/Presets/Curves");

    xiiOSFile::CreateDirectoryStructure(s_sPresetSaveDir.toUtf8().data()).IgnoreResult();
  }

  QString sFile = QFileDialog::getSaveFileName(QApplication::activeWindow(), "Save Curve as Preset", s_sPresetSaveDir, "Curve Presets (*.xiiCurvePreset)", nullptr, QFileDialog::Option::DontResolveSymlinks);

  if (sFile.isEmpty())
    return;

  s_sPresetSaveDir = sFile;

  SaveCurvePreset(sFile.toUtf8().data());

  FindAllPresets();
}

void xiiQtCurve1DEditorWidget::SaveCurvePreset(const char* szFile) const
{
  xiiFileWriter file;
  if (file.Open(szFile).Failed())
    return;

  file.WriteVersion(1);

  file << m_Curves.m_uiFramesPerSecond;
  file << m_Curves.m_Curves.GetCount();

  XII_ASSERT_DEBUG(m_Curves.m_Curves.GetCount() == 1, "Only one curve at a time is currently supported.");

  for (xiiUInt32 curveIdx = 0; curveIdx < m_Curves.m_Curves.GetCount(); ++curveIdx)
  {
    const auto& curve = m_Curves.m_Curves[curveIdx];
    file << curve->m_CurveColor;
    file << curve->m_ControlPoints.GetCount();

    for (xiiUInt32 cpIdx = 0; cpIdx < curve->m_ControlPoints.GetCount(); ++cpIdx)
    {
      const auto& cp = curve->m_ControlPoints[cpIdx];

      file << cp.m_iTick;
      file << cp.m_fValue;
      file << cp.m_bTangentsLinked;
      file << cp.m_LeftTangentMode;
      file << cp.m_RightTangentMode;
      file << cp.m_LeftTangent;
      file << cp.m_RightTangent;
    }
  }
}

void xiiQtCurve1DEditorWidget::onLoadPreset()
{
  if (s_sPresetSaveDir.isEmpty())
  {
    s_sPresetSaveDir = xiiToolsProject::GetSingleton()->GetProjectDirectory().GetData();
    s_sPresetSaveDir.append("/Editor/Presets/Curves");

    if (!xiiOSFile::ExistsDirectory(s_sPresetSaveDir.toUtf8().data()))
    {
      // maybe fall back to the Base directory instead ?
      xiiOSFile::CreateDirectoryStructure(s_sPresetSaveDir.toUtf8().data()).IgnoreResult();
    }
  }

  QString sFile = QFileDialog::getOpenFileName(QApplication::activeWindow(), "Load Curve from Preset", s_sPresetSaveDir, "Curve Presets (*.xiiCurvePreset)", nullptr, QFileDialog::Option::DontResolveSymlinks);

  if (sFile.isEmpty())
    return;

  s_sPresetSaveDir = sFile;

  LoadCurvePreset(sFile.toUtf8().data()).IgnoreResult();
}

xiiResult xiiQtCurve1DEditorWidget::LoadCurvePreset(const char* szFile)
{
  xiiStringBuilder sPath = szFile;

  if (!sPath.IsAbsolutePath())
  {
    sPath.Prepend("Editor/Presets/Curves/");
  }

  xiiFileReader file;
  if (file.Open(sPath).Failed())
    return XII_FAILURE;

  const xiiTypeVersion version = file.ReadVersion(1);

  Q_EMIT BeginCpChangesEvent("Load Preset");

  // Delete all existing control points
  ClearAllPoints();

  file >> m_Curves.m_uiFramesPerSecond;

  xiiUInt32 uiNumCurves = 0;
  file >> uiNumCurves;

  XII_ASSERT_DEBUG(uiNumCurves == 1, "Only one curve at a time is currently supported.");
  uiNumCurves = 1;

  for (xiiUInt32 curveIdx = 0; curveIdx < uiNumCurves; ++curveIdx)
  {
    xiiColorGammaUB curveColor;
    xiiUInt32       uiNumCPs = 0;
    file >> curveColor;
    file >> uiNumCPs;

    for (xiiUInt32 cpIdx = 0; cpIdx < uiNumCPs; ++cpIdx)
    {
      xiiInt64                     iTick           = 0;
      double                       fValue          = 0;
      bool                         bTangentsLinked = false;
      xiiEnum<xiiCurveTangentMode> LeftTangentMode;
      xiiEnum<xiiCurveTangentMode> RightTangentMode;
      xiiVec2                      LeftTangent;
      xiiVec2                      RightTangent;

      file >> iTick;
      file >> fValue;
      file >> bTangentsLinked;
      file >> LeftTangentMode;
      file >> RightTangentMode;
      file >> LeftTangent;
      file >> RightTangent;

      Q_EMIT InsertCpEvent(curveIdx, iTick, fValue);
      Q_EMIT TangentLinkEvent(curveIdx, cpIdx, bTangentsLinked);
      Q_EMIT CpTangentModeEvent(curveIdx, cpIdx, false, LeftTangentMode.GetValue());
      Q_EMIT CpTangentModeEvent(curveIdx, cpIdx, true, RightTangentMode.GetValue());
      Q_EMIT TangentMovedEvent(curveIdx, cpIdx, LeftTangent.x, LeftTangent.y, false);
      Q_EMIT TangentMovedEvent(curveIdx, cpIdx, RightTangent.x, RightTangent.y, true);
    }
  }

  Q_EMIT EndCpChangesEvent();

  return XII_SUCCESS;
}

void xiiQtCurve1DEditorWidget::FindAllPresets()
{
  s_CurvePresets.Clear();

#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)

  xiiFileSystemIterator fsIt;

  xiiFileSystem::StartSearch(fsIt, "Editor/Presets/Curves", xiiFileSystemIteratorFlags::ReportFilesRecursive);

  xiiStringBuilder sFilePath;

  for (; fsIt.IsValid(); fsIt.Next())
  {
    if (!xiiPathUtils::HasExtension(fsIt.GetStats().m_sName, "xiiCurvePreset"))
      continue;

    fsIt.GetStats().GetFullPath(sFilePath);
    sFilePath.MakeCleanPath();

    sFilePath.MakeRelativeTo(fsIt.GetCurrentSearchTerm()).AssertSuccess();

    s_CurvePresets.PushBack(sFilePath);
  }

  s_CurvePresets.Sort();

#endif
}

void xiiQtCurve1DEditorWidget::UpdateSpinBoxes()
{
  const auto& selection = CurveEdit->GetSelection();

  xiiQtScopedBlockSignals _1(LinePosition, LineValue);

  if (selection.IsEmpty())
  {
    LinePosition->setText(QString());
    LineValue->setText(QString());

    LinePosition->setEnabled(false);
    LineValue->setEnabled(false);
    return;
  }

  const auto&  pt0  = m_Curves.m_Curves[selection[0].m_uiCurve]->m_ControlPoints[selection[0].m_uiPoint];
  const double fPos = pt0.GetTickAsTime().GetSeconds();
  const double fVal = pt0.m_fValue;

  LinePosition->setEnabled(true);
  LineValue->setEnabled(true);

  bool bMultipleTicks = false;
  for (xiiUInt32 i = 1; i < selection.GetCount(); ++i)
  {
    const auto& pt = m_Curves.m_Curves[selection[i].m_uiCurve]->m_ControlPoints[selection[i].m_uiPoint];

    if (pt.GetTickAsTime().GetSeconds() != fPos)
    {
      bMultipleTicks = true;
      break;
    }
  }

  bool bMultipleValues = false;
  for (xiiUInt32 i = 1; i < selection.GetCount(); ++i)
  {
    const auto& pt = m_Curves.m_Curves[selection[i].m_uiCurve]->m_ControlPoints[selection[i].m_uiPoint];

    if (pt.m_fValue != fVal)
    {
      bMultipleValues = true;
      LineValue->setText(QString());
      break;
    }
  }

  LinePosition->setText(bMultipleTicks ? QString() : QString::number(fPos, 'f', 2));
  LineValue->setText(bMultipleValues ? QString() : QString::number(fVal, 'f', 3));
}

void xiiQtCurve1DEditorWidget::on_LinePosition_editingFinished()
{
  QString sValue = LinePosition->text();

  bool         ok    = false;
  const double value = sValue.toDouble(&ok);
  if (!ok)
    return;

  if (value < 0)
    return;

  const auto& selection = CurveEdit->GetSelection();
  if (selection.IsEmpty())
    return;

  Q_EMIT BeginCpChangesEvent("Set Time");

  for (const auto& cpSel : selection)
  {
    const auto& cp = m_Curves.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint];

    xiiInt32 iTick = m_Curves.TickFromTime(xiiTime::Seconds(value));
    if (cp.m_iTick != iTick)
      Q_EMIT CpMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, iTick, cp.m_fValue);
  }

  Q_EMIT EndCpChangesEvent();
}

void xiiQtCurve1DEditorWidget::on_LineValue_editingFinished()
{
  QString sValue = LineValue->text();

  bool         ok    = false;
  const double value = sValue.toDouble(&ok);
  if (!ok)
    return;

  const auto& selection = CurveEdit->GetSelection();
  if (selection.IsEmpty())
    return;

  Q_EMIT BeginCpChangesEvent("Set Value");

  for (const auto& cpSel : selection)
  {
    const auto& cp = m_Curves.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint];

    if (cp.m_fValue != value)
      Q_EMIT CpMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, cp.m_iTick, value);
  }

  Q_EMIT EndCpChangesEvent();
}

void xiiQtCurve1DEditorWidget::SetTangentMode(xiiCurveTangentMode::Enum mode, bool bLeft, bool bRight)
{
  const auto& selection = CurveEdit->GetSelection();
  if (selection.IsEmpty())
    return;

  Q_EMIT BeginCpChangesEvent("Set Tangent Mode");

  for (const auto& cpSel : selection)
  {
    if (bLeft)
      Q_EMIT CpTangentModeEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, false, (int)mode);

    if (bRight)
      Q_EMIT CpTangentModeEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, true, (int)mode);
  }

  Q_EMIT EndCpChangesEvent();
}

void xiiQtCurve1DEditorWidget::ClampPoint(double& x, double& y) const
{
  if (CurveEdit->m_bLowerExtentFixed)
    x = xiiMath::Max(x, CurveEdit->m_fLowerExtent);
  if (CurveEdit->m_bUpperExtentFixed)
    x = xiiMath::Min(x, CurveEdit->m_fUpperExtent);

  y = xiiMath::Clamp(y, CurveEdit->m_fLowerRange, CurveEdit->m_fUpperRange);
}
