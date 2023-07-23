#pragma once

#include <Foundation/Math/Easing.h>
#include <Foundation/Tracks/Curve1D.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_Curve1DEditorWidget.h>

#include <QWidget>

class XII_GUIFOUNDATION_DLL xiiQtCurve1DEditorWidget : public QWidget, public Ui_Curve1DEditorWidget
{
  Q_OBJECT

public:
  explicit xiiQtCurve1DEditorWidget(QWidget* pParent);
  ~xiiQtCurve1DEditorWidget();

  void SetCurveExtents(double fLowerBound, double fUpperBound, bool bLowerIsFixed, bool bUpperIsFixed);
  void SetCurveRanges(double fLowerRange, double fUpperRange);

  void SetCurves(const xiiCurveGroupData& curveData);
  void SetScrubberPosition(xiiUInt64 uiTick);
  void ClearSelection();

  void FrameCurve();
  void FrameSelection();
  void MakeRepeatable(bool bAdjustLastPoint);
  void NormalizeCurveX(xiiUInt32 uiActiveCurve);
  void NormalizeCurveY(xiiUInt32 uiActiveCurve);
  void ClearAllPoints();
  void MirrorHorizontally(xiiUInt32 uiActiveCurve);
  void MirrorVertically(xiiUInt32 uiActiveCurve);

Q_SIGNALS:
  void CpMovedEvent(xiiUInt32 uiCurveIdx, xiiUInt32 uiCpIdx, xiiInt64 iTickX, double fNewPosY);
  void CpDeletedEvent(xiiUInt32 uiCurveIdx, xiiUInt32 uiCpIdx);
  void TangentMovedEvent(xiiUInt32 uiCurveIdx, xiiUInt32 uiCpIdx, float fNewPosX, float fNewPosY, bool bRightTangent);
  void InsertCpEvent(xiiUInt32 uiCurveIdx, xiiInt64 iTickX, double value);
  void TangentLinkEvent(xiiUInt32 uiCurveIdx, xiiUInt32 uiCpIdx, bool bLink);
  void CpTangentModeEvent(xiiUInt32 uiCurveIdx, xiiUInt32 uiCpIdx, bool bRightTangent, int iMode); // xiiCurveTangentMode

  void BeginCpChangesEvent(QString sName);
  void EndCpChangesEvent();

  void BeginOperationEvent(QString sName);
  void EndOperationEvent(bool bCommit);

private Q_SLOTS:
  void on_LinePosition_editingFinished();
  void on_LineValue_editingFinished();
  void onDeleteControlPoints();
  void onDoubleClick(const QPointF& scenePos, const QPointF& epsilon);
  void onMoveControlPoints(double x, double y);
  void onMoveTangents(float x, float y);
  void onBeginOperation(QString name);
  void onEndOperation(bool commit);
  void onScaleControlPoints(QPointF refPt, double scaleX, double scaleY);
  void onContextMenu(QPoint pos, QPointF scenePos);
  void onAddPoint();
  void onLinkTangents();
  void onBreakTangents();
  void onFlattenTangents();
  void onSelectionChanged();
  void onMoveCurve(xiiInt32 iCurve, double moveY);
  void onGenerateCurve(xiiEasingFunction::Enum function, bool inverse);
  void onSaveAsPreset();
  void onLoadPreset();

private:
  void      InsertCpAt(double posX, double value, xiiVec2d epsilon);
  bool      PickCurveAt(double x, double y, double fMaxDistanceY, xiiInt32& out_iCurveIdx, double& out_ValueY) const;
  bool      PickControlPointAt(double x, double y, xiiVec2d vMaxDistance, xiiInt32& out_iCurveIdx, xiiInt32& out_iCpIdx) const;
  void      UpdateSpinBoxes();
  void      SetTangentMode(xiiCurveTangentMode::Enum mode, bool bLeft, bool bRight);
  void      ClampPoint(double& x, double& y) const;
  void      SaveCurvePreset(xiiStringView sFile) const;
  xiiResult LoadCurvePreset(xiiStringView sFile);
  void      FindAllPresets();

  double            m_fCurveDuration;
  xiiVec2           m_vTangentMove;
  xiiVec2d          m_vControlPointMove;
  xiiCurveGroupData m_Curves;
  xiiCurveGroupData m_CurvesBackup;
  QPointF           m_ContextMenuScenePos;

  static xiiDynamicArray<xiiString> s_CurvePresets;
};
