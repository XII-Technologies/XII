/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once

#include <Inspector/RenderingCapture.h>
#include <QMap>
#include <QVector>

class QComboBox;
class QDoubleSpinBox;
class QGraphicsScene;
class QTreeWidget;

class xiiQtProfilingWidget : public xiiQtRenderingCapture
{
public:
  xiiQtProfilingWidget(ads::CDockManager* manager);
  ~xiiQtProfilingWidget() override;
  static xiiQtProfilingWidget* s_pWidget;
  static void ProcessTelemetry(void*);
  bool ApplyCapture(const QJsonObject& object) override;
  void ApplyGraphCapture(const QJsonObject& object);

  struct Scope
  {
    QString m_Name, m_Lane, m_Function;
    double m_fBeginUs = 0, m_fEndUs = 0;
  };
  /// Accept Chrome complete events and the engine's named begin/end pairs. Metadata is retained.
  static bool Decode(const QJsonObject& object, QVector<Scope>& scopes, QMap<QString, QString>& lanes);

protected:
  void ClearCapture() override;
  void Rebuild() override;

private:
  void SelectCapture(int index);
  void RebuildGpu();
  QComboBox* m_pHistory;
  QComboBox* m_pFrames;
  QComboBox* m_pLanes;
  QDoubleSpinBox* m_pBudget;
  QGraphicsScene* m_pTimeline;
  QGraphicsScene* m_pFrameChart;
  QTreeWidget* m_pHotspots;
  QTreeWidget* m_pScopes;
  QTreeWidget* m_pGpuPasses;
  QMap<QString, QJsonObject> m_GpuGraphs;
  QVector<QJsonObject> m_History;
  QVector<Scope> m_Scopes;
  QVector<Scope> m_Frames;
  QMap<QString, QString> m_Lanes;
};
