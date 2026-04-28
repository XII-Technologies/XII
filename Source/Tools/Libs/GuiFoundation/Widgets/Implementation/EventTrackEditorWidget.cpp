/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Math.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/EventTrackEditorWidget.moc.h>
#include <QGraphicsItem>
#include <QGraphicsSceneEvent>
#include <QInputDialog>
#include <QMenu>
#include <QPainterPath>

xiiQtEventTrackEditorWidget::xiiQtEventTrackEditorWidget(QWidget* pParent) :
  QWidget(pParent)
{
  setupUi(this);

  EventTrackEdit->SetGridBarWidget(GridBarWidget);

  // make sure the track is visible and not completely squashed
  EventTrackEdit->setMinimumHeight(50);

  connect(EventTrackEdit, &xiiQtEventTrackWidget::DeleteControlPointsEvent, this, &xiiQtEventTrackEditorWidget::onDeleteControlPoints);
  connect(EventTrackEdit, &xiiQtEventTrackWidget::DoubleClickEvent, this, &xiiQtEventTrackEditorWidget::onDoubleClick);
  connect(EventTrackEdit, &xiiQtEventTrackWidget::MoveControlPointsEvent, this, &xiiQtEventTrackEditorWidget::onMoveControlPoints);
  connect(EventTrackEdit, &xiiQtEventTrackWidget::BeginOperationEvent, this, &xiiQtEventTrackEditorWidget::onBeginOperation);
  connect(EventTrackEdit, &xiiQtEventTrackWidget::EndOperationEvent, this, &xiiQtEventTrackEditorWidget::onEndOperation);
  // connect(EventTrackEdit, &xiiQtEventTrackWidget::ScaleControlPointsEvent, this, &xiiQtEventTrackEditorWidget::onScaleControlPoints);
  connect(EventTrackEdit, &xiiQtEventTrackWidget::ContextMenuEvent, this, &xiiQtEventTrackEditorWidget::onContextMenu);
  connect(EventTrackEdit, &xiiQtEventTrackWidget::SelectionChangedEvent, this, &xiiQtEventTrackEditorWidget::onSelectionChanged);

  LinePosition->setEnabled(false);

  DetermineAvailableEvents();
}

xiiQtEventTrackEditorWidget::~xiiQtEventTrackEditorWidget() = default;

void xiiQtEventTrackEditorWidget::SetData(const xiiEventTrackData& trackData, double fMinCurveLength)
{
  xiiQtScopedUpdatesDisabled ud(this);
  xiiQtScopedBlockSignals    bs(this);

  m_pData = &trackData;
  EventTrackEdit->SetData(&trackData, fMinCurveLength);

  UpdateSpinBoxes();
}

void xiiQtEventTrackEditorWidget::SetScrubberPosition(xiiUInt64 uiTick)
{
  EventTrackEdit->SetScrubberPosition(uiTick / 4800.0);
}

void xiiQtEventTrackEditorWidget::SetScrubberPosition(xiiTime time)
{
  EventTrackEdit->SetScrubberPosition(time.GetSeconds());
}

void xiiQtEventTrackEditorWidget::ClearSelection()
{
  EventTrackEdit->ClearSelection();
}

void xiiQtEventTrackEditorWidget::FrameCurve()
{
  EventTrackEdit->FrameCurve();
}

void xiiQtEventTrackEditorWidget::on_AddEventButton_clicked()
{
  QString name = QInputDialog::getText(this, "Add Type", "Event Type Name:");

  m_EventSet.AddAvailableEvent(name.toUtf8().data());

  if (m_EventSet.IsModified())
  {
    m_EventSet.WriteToDDL(":project/Editor/Events.ddl").IgnoreResult();

    FillEventComboBox(name.toUtf8().data());
  }
}

void xiiQtEventTrackEditorWidget::on_InsertEventButton_clicked()
{
  int    curveIdx = 0, cpIdx = 0;
  double posX = xiiMath::Max(EventTrackEdit->GetScrubberPosition(), 0.0);

  Q_EMIT InsertCpEvent(m_pData->TickFromTime(xiiTime::MakeFromSeconds(posX)), ComboType->currentText().toUtf8().data());
}

void xiiQtEventTrackEditorWidget::onDeleteControlPoints()
{
  xiiHybridArray<xiiUInt32, 32> selection;
  EventTrackEdit->GetSelection(selection);

  if (selection.IsEmpty())
    return;

  EventTrackEdit->ClearSelection();

  Q_EMIT BeginCpChangesEvent("Delete Events");

  selection.Sort([](xiiUInt32 lhs, xiiUInt32 rhs) -> bool { return lhs > rhs; });

  // delete sorted from back to front to prevent point indices becoming invalidated
  for (xiiUInt32 pt : selection)
  {
    Q_EMIT CpDeletedEvent(pt);
  }

  Q_EMIT EndCpChangesEvent();
}

void xiiQtEventTrackEditorWidget::onDoubleClick(double scenePosX, double epsilon)
{
  InsertCpAt(scenePosX, xiiMath::Abs(epsilon));
}

void xiiQtEventTrackEditorWidget::onMoveControlPoints(double x)
{
  m_fControlPointMove += x;

  xiiHybridArray<xiiUInt32, 32> selection;
  EventTrackEdit->GetSelection(selection);

  if (selection.IsEmpty())
    return;

  Q_EMIT BeginCpChangesEvent("Move Events");

  for (const auto& cpSel : selection)
  {
    auto& cp = m_DataCopy.m_ControlPoints[cpSel];

    double newPos = cp.GetTickAsTime().GetSeconds() + m_fControlPointMove;
    newPos        = xiiMath::Max(newPos, 0.0);

    Q_EMIT CpMovedEvent(cpSel, m_pData->TickFromTime(xiiTime::MakeFromSeconds(newPos)));
  }

  Q_EMIT EndCpChangesEvent();
}

// void xiiQtEventTrackEditorWidget::onScaleControlPoints(QPointF refPt, double scaleX, double scaleY)
//{
//  const auto selection = EventTrackEdit->GetSelection();
//
//  if (selection.IsEmpty())
//    return;
//
//  const xiiVec2d ref(refPt.x(), refPt.y());
//  const xiiVec2d scale(scaleX, scaleY);
//
//  Q_EMIT BeginCpChangesEvent("Scale Points");
//
//  for (const auto& cpSel : selection)
//  {
//    const auto& cp = m_CurvesBackup.m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint];
//    xiiVec2d newPos = ref + (xiiVec2d(cp.GetTickAsTime(), cp.m_fValue) - ref).CompMul(scale);
//    newPos.x = xiiMath::Max(newPos.x, 0.0);
//    newPos.y = xiiMath::Clamp(newPos.y, -100000.0, +100000.0);
//
//    Q_EMIT CpMovedEvent(cpSel.m_uiCurve, cpSel.m_uiPoint, m_Curves.TickFromTime(newPos.x), newPos.y);
//  }
//
//  Q_EMIT EndCpChangesEvent();
//}

void xiiQtEventTrackEditorWidget::onBeginOperation(QString name)
{
  m_fControlPointMove = 0;
  m_DataCopy          = *m_pData;

  Q_EMIT BeginOperationEvent(name);
}

void xiiQtEventTrackEditorWidget::onEndOperation(bool commit)
{
  Q_EMIT EndOperationEvent(commit);
}

void xiiQtEventTrackEditorWidget::onContextMenu(QPoint pos, QPointF scenePos)
{
  m_ContextMenuScenePos = scenePos;

  QMenu m(this);
  m.setDefaultAction(m.addAction("Add Event", this, SLOT(onAddPoint())));

  xiiHybridArray<xiiUInt32, 32> selection;
  EventTrackEdit->GetSelection(selection);

  if (!selection.IsEmpty())
  {
    m.addAction("Delete Events", QKeySequence(Qt::Key_Delete), this, SLOT(onDeleteControlPoints()));
  }

  m.addSeparator();

  m.addAction("Frame", QKeySequence(Qt::ControlModifier | Qt::Key_F), this, [this]() { FrameCurve(); });

  m.exec(pos);
}

void xiiQtEventTrackEditorWidget::onAddPoint()
{
  InsertCpAt(m_ContextMenuScenePos.x(), 0.0f);
}

void xiiQtEventTrackEditorWidget::InsertCpAt(double posX, double epsilon)
{
  int curveIdx = 0, cpIdx = 0;
  posX = xiiMath::Max(posX, 0.0);

  Q_EMIT InsertCpEvent(m_pData->TickFromTime(xiiTime::MakeFromSeconds(posX)), ComboType->currentText().toUtf8().data());
}

void xiiQtEventTrackEditorWidget::onSelectionChanged()
{
  UpdateSpinBoxes();
}

void xiiQtEventTrackEditorWidget::UpdateSpinBoxes()
{
  xiiHybridArray<xiiUInt32, 32> selection;
  EventTrackEdit->GetSelection(selection);

  xiiQtScopedBlockSignals _1(LinePosition, SelectedTypeLabel);

  if (selection.IsEmpty())
  {
    LinePosition->setText(QString());
    LinePosition->setEnabled(false);
    SelectedTypeLabel->setText("Event: none");
    return;
  }

  const double fPos = m_pData->m_ControlPoints[selection[0]].GetTickAsTime().GetSeconds();

  LinePosition->setEnabled(true);

  xiiStringBuilder labelText("Event: ", m_pData->m_ControlPoints[selection[0]].m_sEvent.GetString());

  bool bMultipleTicks = false;
  for (xiiUInt32 i = 1; i < selection.GetCount(); ++i)
  {
    const xiiString& sName = m_pData->m_ControlPoints[selection[i]].m_sEvent.GetString();
    const double     fPos2 = m_pData->m_ControlPoints[selection[i]].GetTickAsTime().GetSeconds();

    if (!labelText.FindSubString(sName))
    {
      labelText.Append(", ", sName);
    }

    if (fPos2 != fPos)
    {
      bMultipleTicks = true;
      break;
    }
  }

  LinePosition->setText(bMultipleTicks ? QString() : QString::number(fPos, 'f', 2));
  SelectedTypeLabel->setText(labelText.GetData());
}

void xiiQtEventTrackEditorWidget::DetermineAvailableEvents()
{
  m_EventSet.ReadFromDDL(":project/Editor/Events.ddl").IgnoreResult();

  FillEventComboBox(nullptr);
}

void xiiQtEventTrackEditorWidget::FillEventComboBox(xiiStringView sCurrent)
{
  xiiStringBuilder tmp;
  QString          prev = sCurrent.GetData(tmp);

  if (prev.isEmpty())
    prev = ComboType->currentText();

  ComboType->clear();

  for (const xiiString& type : m_EventSet.GetAvailableEvents())
  {
    ComboType->addItem(type.GetData());
  }

  ComboType->setCurrentText(prev);
}

void xiiQtEventTrackEditorWidget::on_LinePosition_editingFinished()
{
  QString sValue = LinePosition->text();

  bool         ok    = false;
  const double value = sValue.toDouble(&ok);
  if (!ok)
    return;

  if (value < 0)
    return;

  xiiHybridArray<xiiUInt32, 32> selection;
  EventTrackEdit->GetSelection(selection);
  if (selection.IsEmpty())
    return;

  Q_EMIT BeginCpChangesEvent("Set Event Time");

  xiiInt64 tick = m_pData->TickFromTime(xiiTime::MakeFromSeconds(value));

  for (const auto& cpSel : selection)
  {
    if (m_pData->m_ControlPoints[cpSel].m_iTick != tick)
      Q_EMIT CpMovedEvent(cpSel, tick);
  }

  Q_EMIT EndCpChangesEvent();
}
