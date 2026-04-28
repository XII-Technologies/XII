/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiQtCurve1DEditorWidget;

class xiiQtCurve1DAssetDocumentWindow : public xiiQtDocumentWindow
{
  Q_OBJECT

public:
  xiiQtCurve1DAssetDocumentWindow(xiiDocument* pDocument);
  ~xiiQtCurve1DAssetDocumentWindow();

  virtual xiiStringView GetWindowLayoutGroupName() const override { return "Curve1DAsset"; }

private Q_SLOTS:
  void onInsertCpAt(xiiUInt32 uiCurveIdx, xiiInt64 tickX, double newPosY);
  void onCurveCpMoved(xiiUInt32 curveIdx, xiiUInt32 cpIdx, xiiInt64 iTickX, double newPosY);
  void onCurveCpDeleted(xiiUInt32 curveIdx, xiiUInt32 cpIdx);
  void onCurveTangentMoved(xiiUInt32 curveIdx, xiiUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent);
  void onLinkCurveTangents(xiiUInt32 curveIdx, xiiUInt32 cpIdx, bool bLink);
  void onCurveTangentModeChanged(xiiUInt32 curveIdx, xiiUInt32 cpIdx, bool rightTangent, int mode);

  void onCurveBeginOperation(QString name);
  void onCurveEndOperation(bool commit);
  void onCurveBeginCpChanges(QString name);
  void onCurveEndCpChanges();

private:
  void UpdatePreview();

  void SendLiveResourcePreview();
  void RestoreResource();

  void PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);
  void StructureEventHandler(const xiiDocumentObjectStructureEvent& e);

  xiiQtCurve1DEditorWidget* m_pCurveEditor;
};
