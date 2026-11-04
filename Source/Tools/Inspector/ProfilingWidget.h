/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Inspector/ProfilingSession.h>
#include <QElapsedTimer>
#include <QSharedPointer>
#include <ads/DockWidget.h>

class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTreeWidget;
class xiiQtProfilingPlot;

class xiiQtProfilingWidget : public ads::CDockWidget
{
public:
  explicit xiiQtProfilingWidget(ads::CDockManager* manager, bool rendering = false, QSharedPointer<xiiInspectorProfilingSession> session = {});
  ~xiiQtProfilingWidget() override;
  static xiiQtProfilingWidget*                 s_pWidget;
  static xiiQtProfilingWidget*                 s_pRenderingWidget;
  static void                                  ProcessTelemetry(void*);
  void                                         ResetStats();
  bool                                         ApplyCapture(const QJsonObject& object);
  bool                                         LoadCapture(const QByteArray& bytes);
  void                                         ApplyGraphCapture(const QJsonObject& object);
  void                                         RequestCapture();
  void                                         RequestHistorySnapshot();
  void                                         Refresh();
  QSharedPointer<xiiInspectorProfilingSession> GetSession() const { return m_pSession; }

private:
  void                                         Tick();
  void                                         RebuildCpu(double begin, double end, bool updateTables);
  void                                         RebuildGpu();
  void                                         UpdateGpuHistory();
  void                                         SaveCapture();
  void                                         OpenCapture();
  QSharedPointer<xiiInspectorProfilingSession> m_pSession;
  bool                                         m_bRendering;
  quint64                                      m_uiRevision = ~quint64(0);
  QElapsedTimer                                m_AnalysisTime, m_SubscriptionTime;
  QLabel*                                      m_pStatus;
  QLabel*                                      m_pOverview;
  QLineEdit*                                   m_pFilter;
  QComboBox*                                   m_pCaptureHistory;
  QComboBox*                                   m_pFrames;
  QComboBox*                                   m_pLastFrame;
  QComboBox*                                   m_pLanes;
  QComboBox*                                   m_pGraphs;
  QComboBox*                                   m_pQueues;
  QComboBox*                                   m_pCaptureMode;
  QSpinBox*                                    m_pCaptureFrames;
  QDoubleSpinBox*                              m_pCaptureSeconds;
  QSpinBox*                                    m_pCaptureMemory;
  QDoubleSpinBox*                              m_pBudget;
  QPushButton*                                 m_pPause;
  QPushButton*                                 m_pRecord;
  QPushButton*                                 m_pSnapshot;
  QTreeWidget*                                 m_pHotspots  = nullptr;
  QTreeWidget*                                 m_pScopes    = nullptr;
  QTreeWidget*                                 m_pGpuPasses = nullptr;
  xiiQtProfilingPlot*                          m_pTimeline  = nullptr;
  xiiQtProfilingPlot*                          m_pHistory;
  QString                                      m_SelectedGpu;
};
