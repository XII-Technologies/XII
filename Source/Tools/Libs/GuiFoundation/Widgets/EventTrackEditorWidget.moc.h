#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_EventTrackEditorWidget.h>

#include <QWidget>

class XII_GUIFOUNDATION_DLL xiiQtEventTrackEditorWidget : public QWidget, public Ui_EventTrackEditorWidget
{
  Q_OBJECT

public:
  explicit xiiQtEventTrackEditorWidget(QWidget* pParent);
  ~xiiQtEventTrackEditorWidget();

  void SetData(const xiiEventTrackData& data, double fMinCurveLength);
  void SetScrubberPosition(xiiUInt64 uiTick);
  void SetScrubberPosition(xiiTime time);
  void ClearSelection();

  void FrameCurve();

Q_SIGNALS:
  void CpMovedEvent(xiiUInt32 uiCpIdx, xiiInt64 iTickX);
  void CpDeletedEvent(xiiUInt32 uiCpIdx);
  void InsertCpEvent(xiiInt64 iTickX, const char* value);

  void BeginCpChangesEvent(QString sName);
  void EndCpChangesEvent();

  void BeginOperationEvent(QString sName);
  void EndOperationEvent(bool bCommit);

private Q_SLOTS:
  void on_LinePosition_editingFinished();
  void on_AddEventButton_clicked();
  void onDeleteControlPoints();
  void onDoubleClick(double scenePosX, double epsilon);
  void onMoveControlPoints(double x);
  void onBeginOperation(QString name);
  void onEndOperation(bool commit);
  // void onScaleControlPoints(QPointF refPt, double scaleX);
  void onContextMenu(QPoint pos, QPointF scenePos);
  void onAddPoint();
  void onSelectionChanged();

private:
  void InsertCpAt(double posX, double epsilon);
  void UpdateSpinBoxes();
  void DetermineAvailableEvents();
  void FillEventComboBox(xiiStringView sCurrent = {});

  const xiiEventTrackData* m_pData = nullptr;
  xiiEventTrackData        m_DataCopy;

  double      m_fControlPointMove;
  QPointF     m_ContextMenuScenePos;
  xiiEventSet m_EventSet;
};
