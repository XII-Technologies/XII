/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once

#include <Inspector/RenderingCapture.h>
#include <QJsonArray>
#include <QMap>

class QComboBox;
class QTreeWidget;
class QGraphicsScene;
class QGraphicsView;
class QPlainTextEdit;

class xiiQtRenderGraphWidget : public xiiQtRenderingCapture
{
public:
  xiiQtRenderGraphWidget(ads::CDockManager* manager);
  ~xiiQtRenderGraphWidget() override;
  static xiiQtRenderGraphWidget* s_pWidget;
  static void                    ProcessTelemetry(void*);
  bool                           ApplyCapture(const QJsonObject& object) override;

protected:
  void ClearCapture() override;
  void Rebuild() override;

private:
  void                       SelectPass(int index);
  QComboBox*                 m_pGraphs;
  QTreeWidget*               m_pPasses;
  QTreeWidget*               m_pResources;
  QTreeWidget*               m_pBarriers;
  QTreeWidget*               m_pVersions;
  QTreeWidget*               m_pSubmissions;
  QGraphicsScene*            m_pGraphScene;
  QGraphicsScene*            m_pLifetimeScene;
  QGraphicsView*             m_pGraphView;
  QPlainTextEdit*            m_pDetails;
  QMap<QString, QJsonObject> m_Graphs;
  QString                    m_sSelectedPassId;
};
