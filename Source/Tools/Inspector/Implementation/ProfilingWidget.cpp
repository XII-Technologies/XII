/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Inspector/InspectorPCH.h>

#include <Inspector/CaptureUi.h>
#include <Inspector/ProfilingWidget.h>
#include <Inspector/RenderGraphWidget.h>
#include <QClipboard>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollArea>
#include <QSplitter>
#include <QTabWidget>
#include <QToolTip>
#include <QVBoxLayout>
#include <algorithm>
#include <functional>
#include <limits>
#include <numeric>

using namespace xiiInspectorUi;
using Scope                                                    = xiiInspectorProfilingSession::Scope;
xiiQtProfilingWidget* xiiQtProfilingWidget::s_pWidget          = nullptr;
xiiQtProfilingWidget* xiiQtProfilingWidget::s_pRenderingWidget = nullptr;

namespace
{
  double Percentile(QVector<double> values, double percentile)
  {
    if (values.isEmpty())
    {
      return 0;
    }
    const auto index = qBound(qsizetype(0), qsizetype(std::ceil(values.size() * percentile)) - 1, values.size() - 1);
    std::nth_element(values.begin(), values.begin() + index, values.end());
    return values[index];
  }
  QString GpuKey(const xiiInspectorProfilingSession::GpuSample& sample)
  {
    return QString("%1:%2:%3:%4").arg(sample.m_uiGraph).arg(sample.m_bSubmission).arg(sample.m_uiPass).arg(sample.m_uiQueue);
  }
  QString ThreadName(const xiiInspectorProfilingSession::Data& data, const QString& lane)
  {
    const auto name = data.m_Threads.value(lane);
    if (!name.isEmpty())
    {
      return name;
    }
    return "Unnamed thread";
  }
  // Update existing rows by stable identity. Preserve sorting, scroll, and selected operation.
  void UpdateRows(QTreeWidget* table, const QMap<QString, QStringList>& rows, const QMap<QString, QString>& details)
  {
    const bool sorting = table->isSortingEnabled();
    table->setSortingEnabled(false);
    QMap<QString, QTreeWidgetItem*> existing;
    for (int i = 0; i < table->topLevelItemCount(); ++i)
    {
      auto* item                                       = table->topLevelItem(i);
      existing[item->data(0, Qt::UserRole).toString()] = item;
    }
    for (auto it = rows.begin(); it != rows.end(); ++it)
    {
      auto* item = existing.take(it.key());
      if (!item)
      {
        item = new TableItem(table);
        item->setData(0, Qt::UserRole, it.key());
      }
      for (int column = 0; column < it.value().size(); ++column)
      {
        if (item->text(column) != it.value()[column])
        {
          item->setText(column, it.value()[column]);
        }
        item->setToolTip(column, details.value(it.key()));
      }
    }
    for (auto* item : existing)
    {
      delete item;
    }
    table->setSortingEnabled(sorting);
  }
} // namespace

/// Plots are painted directly: sample reception never creates thousands of QGraphicsItems.
class xiiQtProfilingPlot : public QWidget
{
public:
  struct Bar
  {
    QRectF  m_Rect;
    QString m_Name, m_Details, m_Key;
    QColor  m_Color;
  };
  QVector<Bar>                 m_Bars;
  QVector<double>              m_Values;
  QVector<QString>             m_Labels;
  QVector<quint64>             m_Frames;
  QStringList                  m_Lanes;
  QVector<int>                 m_LaneY;
  double                       m_fBudget = 16.67;
  double                       m_fZoom = 1, m_fPan = 0;
  bool                         m_bTimeline = false;
  QString                      m_Selected;
  QPoint                       m_LastMouse;
  std::function<void(quint64)> m_SelectFrame;
  std::function<void(QString)> m_SelectScope;

  explicit xiiQtProfilingPlot(bool timeline = false) : m_bTimeline(timeline)
  {
    setMinimumHeight(timeline ? 300 : 150);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setToolTip("Ctrl+wheel to zoom. Drag to pan. Click to select; double-click a scope to focus it.");
  }
  QRectF Map(const QRectF& rect) const
  {
    const double w = qMax(1, width() - 200);
    return {200 + (rect.x() * m_fZoom + m_fPan) * w, rect.y(), qMax(1.0, rect.width() * m_fZoom * w), rect.height()};
  }
  void paintEvent(QPaintEvent*) override
  {
    QPainter painter(this);
    painter.fillRect(rect(), QColor("#20242b"));
    painter.setPen(QColor("#aab4c0"));
    if (m_bTimeline)
    {
      for (int i = 0; i < m_Lanes.size(); ++i)
      {
        painter.drawText(QRectF(8, m_LaneY.value(i, 35 + i * 120), 184, 100), Qt::TextWordWrap, m_Lanes[i]);
      }
      painter.setClipRect(QRect(200, 0, width() - 200, height()));
      for (const auto& bar : m_Bars)
      {
        const auto r = Map(bar.m_Rect);
        if (!r.intersects(rect()))
        {
          continue;
        }
        painter.fillRect(r, bar.m_Color);
        if (bar.m_Key == m_Selected)
        {
          painter.setPen(Qt::white);
          painter.drawRect(r);
        }
        if (r.width() > 45)
        {
          painter.setPen(Qt::white);
          painter.drawText(r.adjusted(3, 0, -3, 0), Qt::AlignVCenter, painter.fontMetrics().elidedText(bar.m_Name, Qt::ElideRight, int(r.width()) - 6));
        }
      }
      return;
    }
    if (m_Values.isEmpty())
    {
      painter.drawText(rect(), Qt::AlignCenter, "Waiting for completed measurements…");
      return;
    }
    double       max     = qMax(m_fBudget * 1.25, Percentile(m_Values, 1));
    const double h       = qMax(1, height() - 34);
    const double w       = qMax(1, width() - 65);
    const double budgetY = 12 + h * (1 - m_fBudget / qMax(0.1, max));
    painter.setPen(QPen(QColor("#e8bc62"), 1, Qt::DashLine));
    painter.drawLine(QPointF(55, budgetY), QPointF(width(), budgetY));
    painter.drawText(4, int(budgetY), QString::number(m_fBudget, 'f', 2));
    QPainterPath path;
    bool         active = false;
    for (int i = 0; i < m_Values.size(); ++i)
    {
      const double x = 55 + (double(i) / qMax(1, int(m_Values.size()) - 1) * m_fZoom + m_fPan) * w;
      if (m_Values[i] < 0)
      {
        active = false;
        continue;
      }
      const double y = 12 + h * (1 - m_Values[i] / qMax(0.1, max));
      if (active)
      {
        path.lineTo(x, y);
      }
      else
      {
        path.moveTo(x, y);
      }
      active = true;
      if (m_Values[i] > m_fBudget)
      {
        painter.fillRect(QRectF(x - 1, y - 1, 3, 3), QColor("#e98678"));
      }
    }
    painter.setClipRect(QRect(55, 0, width() - 55, height()));
    painter.setPen(QPen(m_SelectFrame ? QColor("#56c8b0") : QColor("#b88ade"), 1.5));
    painter.drawPath(path);
    painter.setPen(QColor("#aab4c0"));
    painter.drawText(60, height() - 4, m_SelectFrame ? "CPU frame wall time • ms • click to select a frame" : "Selected GPU operation • ms • gaps indicate unavailable samples");
  }
  void wheelEvent(QWheelEvent* event) override
  {
    if (!event->modifiers().testFlag(Qt::ControlModifier))
    {
      QWidget::wheelEvent(event);
      return;
    }
    m_fZoom = qBound(1.0, m_fZoom * (event->angleDelta().y() > 0 ? 1.2 : 1 / 1.2), 1000.0);
    update();
    event->accept();
  }
  void mousePressEvent(QMouseEvent* event) override
  {
    m_LastMouse = event->pos();
    if (m_bTimeline)
    {
      for (const auto& bar : m_Bars)
      {
        if (Map(bar.m_Rect).contains(event->position()))
        {
          m_Selected = bar.m_Key;
          if (m_SelectScope)
          {
            m_SelectScope(m_Selected);
          }
          update();
          break;
        }
      }
    }
    if (!m_bTimeline && m_SelectFrame && !m_Frames.isEmpty())
    {
      const int index = qBound(0, int(((event->position().x() - 55) / qMax(1, width() - 65) - m_fPan) / m_fZoom * (m_Frames.size() - 1) + 0.5), int(m_Frames.size()) - 1);
      m_SelectFrame(m_Frames[index]);
    }
  }
  void mouseMoveEvent(QMouseEvent* event) override
  {
    if (event->buttons().testFlag(Qt::LeftButton))
    {
      m_fPan += double(event->pos().x() - m_LastMouse.x()) / qMax(1, width() - 200);
      m_LastMouse = event->pos();
      update();
    }
    if (m_bTimeline)
    {
      for (const auto& bar : m_Bars)
      {
        if (Map(bar.m_Rect).contains(event->position()))
        {
          QToolTip::showText(event->globalPosition().toPoint(), bar.m_Details, this);
          return;
        }
      }
    }
    else if (!m_Values.isEmpty())
    {
      const int i = qBound(0, int(((event->position().x() - 55) / qMax(1, width() - 65) - m_fPan) / m_fZoom * (m_Values.size() - 1) + 0.5), int(m_Values.size()) - 1);
      if (i < m_Labels.size())
      {
        QToolTip::showText(event->globalPosition().toPoint(), m_Labels[i], this);
      }
    }
  }
  void mouseDoubleClickEvent(QMouseEvent* event) override
  {
    if (!m_bTimeline)
    {
      return;
    }
    for (const auto& bar : m_Bars)
    {
      if (Map(bar.m_Rect).contains(event->position()))
      {
        m_fZoom = qMin(1000.0, 0.9 / qMax(0.001, bar.m_Rect.width()));
        m_fPan  = -bar.m_Rect.x() * m_fZoom;
        update();
        break;
      }
    }
  }
};
xiiQtProfilingWidget::xiiQtProfilingWidget(ads::CDockManager* manager, bool rendering, QSharedPointer<xiiInspectorProfilingSession> session) :
  ads::CDockWidget(manager, rendering ? "Rendering Profiler" : "Profiler"),
  m_pSession(session ? session : QSharedPointer<xiiInspectorProfilingSession>::create()),
  m_bRendering(rendering)
{
  if (rendering)
  {
    s_pRenderingWidget = this;
  }
  else
  {
    s_pWidget = this;
  }
  setObjectName(rendering ? "RenderingProfiler" : "ApplicationProfiler");
  auto* root     = new QWidget;
  auto* layout   = new QVBoxLayout(root);
  auto* controls = new QHBoxLayout;
  m_pPause       = new QPushButton("Pause");
  m_pPause->setObjectName("Pause profiling");
  auto* live = new QPushButton("Resume live");
  m_pRecord  = new QPushButton("Start recording");
  m_pRecord->setObjectName("Record profiling");
  m_pSnapshot = new QPushButton("Snapshot history");
  m_pSnapshot->setToolTip("Request the application’s existing profiling history once. Live monitoring and recording use incremental telemetry.");
  auto* open = new QPushButton("Open…");
  auto* save = new QPushButton("Save…");
  auto* fit  = new QPushButton("Fit");
  m_pFilter  = new QLineEdit;
  m_pFilter->setPlaceholderText(rendering ? "Search pass or graph…" : "Search scope or function…");
  m_pFilter->setAccessibleName("Profiling search");
  for (auto* button : {m_pPause, live, m_pRecord, m_pSnapshot, open, save, fit})
  {
    controls->addWidget(button);
  }
  controls->addWidget(m_pFilter, 1);
  layout->addLayout(controls);
  auto* limits   = new QHBoxLayout;
  m_pCaptureMode = new QComboBox;
  m_pCaptureMode->addItems({"Until stopped", "Frame count", "Duration"});
  m_pCaptureMode->setAccessibleName("Recording mode");
  m_pCaptureFrames = new QSpinBox;
  m_pCaptureFrames->setRange(1, 36000);
  m_pCaptureFrames->setValue(600);
  m_pCaptureFrames->setSuffix(" frames");
  m_pCaptureFrames->setAccessibleName("Capture frame limit");
  m_pCaptureFrames->setEnabled(false);
  m_pCaptureSeconds = new QDoubleSpinBox;
  m_pCaptureSeconds->setRange(0.1, 600);
  m_pCaptureSeconds->setValue(10);
  m_pCaptureSeconds->setSuffix(" seconds");
  m_pCaptureSeconds->setAccessibleName("Capture duration limit");
  m_pCaptureSeconds->setEnabled(false);
  m_pCaptureMode->setToolTip("All recordings also stop at the memory limit, 36,000 frames, or 10 minutes.");
  connect(m_pCaptureMode, &QComboBox::currentIndexChanged, this, [this](int mode) {
    m_pCaptureFrames->setEnabled(mode == 1);
    m_pCaptureSeconds->setEnabled(mode == 2);
  });
  m_pCaptureMemory = new QSpinBox;
  m_pCaptureMemory->setRange(1, 256);
  m_pCaptureMemory->setValue(64);
  m_pCaptureMemory->setSuffix(" MiB maximum");
  m_pCaptureMemory->setAccessibleName("Capture memory limit");
  m_pBudget = new QDoubleSpinBox;
  m_pBudget->setRange(0.1, 1000);
  m_pBudget->setDecimals(2);
  m_pBudget->setValue(16.67);
  m_pBudget->setSuffix(" ms budget");
  m_pBudget->setAccessibleName(rendering ? "Rendering budget" : "CPU frame budget");
  limits->addWidget(new QLabel("Capture:"));
  limits->addWidget(m_pCaptureMode);
  limits->addWidget(m_pCaptureFrames);
  limits->addWidget(m_pCaptureSeconds);
  limits->addWidget(m_pCaptureMemory);
  limits->addStretch();
  limits->addWidget(m_pBudget);
  layout->addLayout(limits);
  m_pOverview = new QLabel("Connect to an application with InspectorPlugin to monitor performance.");
  m_pOverview->setTextFormat(Qt::PlainText);
  m_pOverview->setWordWrap(true);
  m_pOverview->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_pOverview, &QWidget::customContextMenuRequested, this, [this](QPoint point) {
    QMenu menu;
    auto* copy = menu.addAction("Copy process identifier");
    copy->setEnabled(m_pSession->Display().m_uiProcessId != 0);
    if (menu.exec(m_pOverview->mapToGlobal(point)) == copy)
    {
      QApplication::clipboard()->setText(QString::number(m_pSession->Display().m_uiProcessId));
    }
  });
  m_pOverview->setStyleSheet("font-size: 15px; padding: 6px 0;");
  layout->addWidget(m_pOverview);
  auto* selection   = new QHBoxLayout;
  m_pCaptureHistory = new QComboBox;
  m_pCaptureHistory->setAccessibleName("Profiling capture history");
  m_pCaptureHistory->addItem("Live monitoring", QString{});
  selection->addWidget(m_pCaptureHistory, 1);
  connect(m_pCaptureHistory, &QComboBox::currentIndexChanged, this, [this](int index) {
    if (index == 0)
    {
      m_pSession->SetPaused(false);
    }
    else
    {
      m_pSession->SelectCapture(m_pCaptureHistory->currentData().toULongLong());
    }
    m_uiRevision = ~quint64(0);
    Refresh();
  });
  m_pFrames = new QComboBox;
  m_pFrames->setObjectName("Profiling frame selection");
  m_pFrames->addItem("Latest frame", QString{});
  m_pLastFrame = new QComboBox;
  m_pLastFrame->addItem("Single frame", QString{});
  m_pLastFrame->setAccessibleName("Selected range end frame");
  m_pLanes = new QComboBox;
  m_pLanes->addItem("All application threads", QString{});
  m_pLanes->setAccessibleName("Thread selection");
  m_pGraphs = new QComboBox;
  m_pGraphs->addItem("All views and graphs", QString{});
  m_pGraphs->setAccessibleName("Rendering graph filter");
  m_pQueues = new QComboBox;
  m_pQueues->addItems({"All queues", "Graphics Queue 0", "Compute Queue 1", "Transfer Queue 2"});
  m_pQueues->setAccessibleName("Rendering queue filter");
  for (auto* combo : {m_pFrames, m_pLastFrame, m_pLanes, m_pGraphs, m_pQueues})
  {
    selection->addWidget(combo, 1);
  }
  m_pGraphs->setVisible(rendering);
  m_pQueues->setVisible(rendering);
  if (rendering)
  {
    m_pFrames->setItemText(0, "All GPU recording frames");
    m_pFrames->setAccessibleName("GPU recording frame selection");
    m_pLastFrame->setAccessibleName("GPU recording range end");
  }
  m_pLanes->setVisible(!rendering);
  layout->addLayout(selection);
  m_pHistory = new xiiQtProfilingPlot;
  m_pHistory->setObjectName(rendering ? "GPU timing history" : "CPU frame history");
  m_pHistory->setMaximumHeight(210);
  layout->addWidget(m_pHistory);
  if (!rendering)
  {
    m_pHistory->m_SelectFrame = [this](quint64 frame) {
      m_pSession->SetPaused(true);
      Refresh();
      m_pFrames->setCurrentIndex(m_pFrames->findData(QString::number(frame)));
    };
    auto* tabs  = new QTabWidget;
    m_pTimeline = new xiiQtProfilingPlot(true);
    m_pTimeline->setAccessibleName("Application thread timeline");
    auto* scroll = new QScrollArea;
    scroll->setWidget(m_pTimeline);
    scroll->setWidgetResizable(true);
    tabs->addTab(scroll, "Thread timeline");
    m_pHotspots = Table({"Scope", "Thread", "Calls", "Inclusive ms", "Self ms", "Mean ms", "Min ms", "P95 ms", "Max ms"}, "Profiling hotspots");
    m_pHotspots->setColumnWidth(0, 260);
    m_pHotspots->sortItems(4, Qt::DescendingOrder);
    tabs->addTab(m_pHotspots, "CPU hotspots");
    m_pScopes = Table({"Scope", "Thread", "Start ms", "Duration ms", "Self ms", "Depth", "Function"}, "Profiling scopes");
    m_pScopes->setColumnWidth(0, 260);
    tabs->addTab(m_pScopes, "Scope details");
    layout->addWidget(tabs, 1);
    m_pTimeline->m_SelectScope = [this](const QString& key) {
      for (int i = 0; i < m_pScopes->topLevelItemCount(); ++i)
      {
        if (m_pScopes->topLevelItem(i)->data(0, Qt::UserRole).toString() == key)
        {
          m_pScopes->setCurrentItem(m_pScopes->topLevelItem(i));
        }
      }
    };
    connect(m_pHotspots, &QTreeWidget::itemDoubleClicked, this, [this, tabs](QTreeWidgetItem* item) {
      m_pFilter->setText(item->text(0));
      tabs->setCurrentIndex(0);
    });
  }
  else
  {
    m_pGpuPasses = Table({"View / Graph", "Pass / Submission", "Queue", "GPU ms", "Measured frame", "Samples", "Mean ms", "Min ms", "P95 ms", "Max ms"}, "GPU pass costs");
    m_pGpuPasses->setColumnWidth(0, 270);
    m_pGpuPasses->setColumnWidth(1, 260);
    m_pGpuPasses->sortItems(3, Qt::DescendingOrder);
    auto* tabs = new QTabWidget;
    tabs->addTab(m_pGpuPasses, "Graph and pass costs");
    m_pTimeline = new xiiQtProfilingPlot(true);
    m_pTimeline->setAccessibleName("GPU timestamp scopes in separate clock lanes");
    auto* scroll = new QScrollArea;
    scroll->setWidget(m_pTimeline);
    scroll->setWidgetResizable(true);
    tabs->addTab(scroll, "GPU timestamp scopes");
    layout->addWidget(tabs, 1);
    auto* locate = new QPushButton("Locate selected pass in Render Graph");
    layout->addWidget(locate);
    connect(locate, &QPushButton::clicked, this, [this] {
      auto* item = m_pGpuPasses->currentItem();
      if (!item || !xiiQtRenderGraphWidget::s_pWidget)
      {
        return;
      }
      const auto parts = item->data(0, Qt::UserRole).toString().split(':');
      if (parts.size() == 4 && parts[1] == "0")
      {
        xiiQtRenderGraphWidget::s_pWidget->LocatePass(parts[0], parts[2].toInt());
      }
    });
    connect(m_pGpuPasses, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* item) {
      m_SelectedGpu = item ? item->data(0, Qt::UserRole).toString() : QString{};
      m_uiRevision  = ~quint64(0);
    });
  }
  m_pStatus = new QLabel;
  m_pStatus->setTextFormat(Qt::PlainText);
  m_pStatus->setWordWrap(true);
  m_pStatus->setTextInteractionFlags(Qt::TextSelectableByMouse);
  layout->addWidget(m_pStatus);
  setWidget(root);
  for (auto* table : {m_pHotspots, m_pScopes, m_pGpuPasses})
  {
    if (!table)
    {
      continue;
    }
    table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(table, &QWidget::customContextMenuRequested, this, [table](QPoint point) {
      auto* item = table->itemAt(point);
      if (!item)
      {
        return;
      }
      QMenu menu;
      auto* copy = menu.addAction("Copy technical identity");
      if (menu.exec(table->viewport()->mapToGlobal(point)) == copy)
      {
        QApplication::clipboard()->setText(item->data(0, Qt::UserRole).toString());
      }
    });
  }
  connect(m_pPause, &QPushButton::clicked, this, [this] { m_pSession->SetPaused(!m_pSession->m_bPaused); Refresh(); });
  connect(live, &QPushButton::clicked, this, [this] {
    if (m_pSession->m_bRecording)
    {
      m_pSession->StopRecording(false);
    }
    m_pSession->SetPaused(false);
    m_pFrames->setCurrentIndex(0);
    m_pLastFrame->setCurrentIndex(0);
    Refresh();
  });
  connect(m_pRecord, &QPushButton::clicked, this, [this] {
    if (m_pSession->m_bRecording)
    {
      m_pSession->StopRecording();
    }
    else
    {
      m_pSession->StartRecording(m_pCaptureMode->currentIndex() == 1 ? m_pCaptureFrames->value() : 36000, m_pCaptureMode->currentIndex() == 2 ? m_pCaptureSeconds->value() : 600, qsizetype(m_pCaptureMemory->value()) * 1024 * 1024);
    }
    Refresh();
  });
  connect(m_pSnapshot, &QPushButton::clicked, this, [this] { RequestHistorySnapshot(); });
  connect(open, &QPushButton::clicked, this, [this] { OpenCapture(); });
  connect(save, &QPushButton::clicked, this, [this] { SaveCapture(); });
  connect(fit, &QPushButton::clicked, this, [this] {
    for (auto* plot : {m_pTimeline, m_pHistory})
    {
      if (plot)
      {
        plot->m_fZoom = 1;
        plot->m_fPan  = 0;
        plot->update();
      }
    }
  });
  auto changed = [this] {
    m_uiRevision = ~quint64(0);
    Refresh();
  };
  connect(m_pFilter, &QLineEdit::textChanged, this, changed);
  connect(m_pBudget, &QDoubleSpinBox::valueChanged, this, changed);
  for (auto* combo : {m_pFrames, m_pLastFrame, m_pLanes, m_pGraphs, m_pQueues})
  {
    connect(combo, &QComboBox::currentIndexChanged, this, changed);
  }
  auto* timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, [this] { Tick(); });
  timer->start(33);
  m_AnalysisTime.start();
  m_SubscriptionTime.start();
}

xiiQtProfilingWidget::~xiiQtProfilingWidget()
{
  if (m_bRendering)
  {
    s_pRenderingWidget = nullptr;
  }
  else
  {
    if (m_pSession->m_bSubscribed && xiiTelemetry::IsConnectedToServer())
    {
      xiiTelemetry::SendToServer('PSTR', 'STOP');
    }
    s_pWidget = nullptr;
  }
}

void xiiQtProfilingWidget::Tick()
{
  m_pSession->UpdateRecording();
  m_pSnapshot->setEnabled(xiiTelemetry::IsConnectedToServer() && !m_pSession->m_bRecording && !m_pSession->m_bSnapshotPending);
  if (!m_bRendering && m_pSession->m_bSnapshotPending && m_pSession->m_SnapshotTimer.elapsed() > 5000)
  {
    m_pSession->m_bSnapshotPending = false;
    m_pStatus->setText("No application history snapshot received. Check the InspectorPlugin connection.");
  }
  if (!m_bRendering)
  {
    const bool connected = xiiTelemetry::IsConnectedToServer();
    const bool visible   = (!isClosed() && isVisible()) || (s_pRenderingWidget && !s_pRenderingWidget->isClosed() && s_pRenderingWidget->isVisible());
    const bool needed    = connected && (m_pSession->m_bRecording || (visible && !m_pSession->m_bPaused));
    if (!connected && m_pSession->m_bSubscribed)
    {
      m_pSession->ResetConnection();
    }
    if (!needed && m_pSession->m_bSubscribed)
    {
      xiiTelemetry::SendToServer('PSTR', 'STOP');
      m_pSession->m_bSubscribed = false;
    }
    if (needed && (!m_pSession->m_bSubscribed || m_SubscriptionTime.elapsed() >= 1000))
    {
      if (!m_pSession->m_bSubscribed)
      {
        m_pSession->BeginStream(quint64(QDateTime::currentMSecsSinceEpoch()));
      }
      xiiTelemetryMessage message;
      message.SetMessageID('PSTR', 'SUBS');
      message.GetWriter() << m_pSession->m_uiSession;
      xiiTelemetry::SendToServer(message);
      m_SubscriptionTime.restart();
    }
  }
  if (isVisible() && !isClosed())
  {
    Refresh();
  }
}

void xiiQtProfilingWidget::ProcessTelemetry(void*)
{
  if (!s_pWidget)
  {
    return;
  }
  auto&               session = *s_pWidget->m_pSession;
  xiiTelemetryMessage message;
  while (xiiTelemetry::RetrieveMessage('PSTR', message).Succeeded())
  {
    if (message.GetMessageID() != 'BTCH' || !session.m_bSubscribed)
    {
      continue;
    }
    if (!session.Receive(message.GetReader()))
    {
      session.m_bSubscribed = false;
      s_pWidget->m_pStatus->setText(session.m_ProtocolError.isEmpty() ? "Invalid live profiling metadata. Resubscribing." : session.m_ProtocolError);
      continue;
    }
    xiiTelemetryMessage ack;
    ack.SetMessageID('PSTR', 'ACKN');
    ack.GetWriter() << session.m_uiSession << session.m_uiSequence;
    xiiTelemetry::SendToServer(ack);
  }
  while (xiiTelemetry::RetrieveMessage('PSNP', message).Succeeded())
  {
    if (!session.m_bSnapshotPending)
    {
      continue;
    }
    session.m_bSnapshotPending = false;
    xiiUInt32 size             = 0;
    message.GetReader() >> size;
    if (size == 0 || size > 16 * 1024 * 1024)
    {
      s_pWidget->m_pStatus->setText("Invalid application history snapshot size.");
      continue;
    }
    QByteArray bytes(size, Qt::Uninitialized);
    if (message.GetReader().ReadBytes(bytes.data(), size) != size)
    {
      s_pWidget->m_pStatus->setText("Truncated application history snapshot.");
    }
    else if (message.GetMessageID() == 'ERRO')
    {
      s_pWidget->m_pStatus->setText(QString::fromUtf8(bytes));
    }
    else if (message.GetMessageID() != 'DATA' || !s_pWidget->LoadCapture(bytes))
    {
      s_pWidget->m_pStatus->setText("Unsupported application history snapshot. The previous capture is preserved.");
    }
  }
}

void xiiQtProfilingWidget::ResetStats()
{
  m_pSession->ResetConnection();
  m_pSession->m_Frozen = {};
  m_pSession->m_Captures.clear();
  m_pSession->m_uiSelectedCapture = 0;
  m_pSession->m_bPaused           = false;
  m_uiRevision                    = ~quint64(0);
  Refresh();
}

bool xiiQtProfilingWidget::ApplyCapture(const QJsonObject& object)
{
  if (!m_pSession->Import(object))
  {
    return false;
  }
  m_uiRevision = ~quint64(0);
  Refresh();
  return true;
}

bool xiiQtProfilingWidget::LoadCapture(const QByteArray& bytes)
{
  if (bytes.size() > 256 * 1024 * 1024)
  {
    return false;
  }
  QJsonParseError error;
  const auto      doc = QJsonDocument::fromJson(bytes, &error);
  return error.error == QJsonParseError::NoError && doc.isObject() && ApplyCapture(doc.object());
}

void xiiQtProfilingWidget::RequestCapture()
{
  m_pSession->StartRecording(1, 10, 64 * 1024 * 1024);
  // An explicit capture remains subscribed even when its dock is hidden.
  Tick();
}

void xiiQtProfilingWidget::RequestHistorySnapshot()
{
  if (!xiiTelemetry::IsConnectedToServer() || m_pSession->m_bSnapshotPending || m_pSession->m_bRecording)
  {
    return;
  }
  m_pSession->SetPaused(true);
  m_pSession->m_bSnapshotPending = true;
  m_pSession->m_SnapshotTimer.restart();
  xiiTelemetry::SendToServer('PSNP', 'CAPT');
  Refresh();
  m_pStatus->setText("Waiting for the application's profiling history snapshot…");
}

void xiiQtProfilingWidget::SaveCapture()
{
  const auto path = QFileDialog::getSaveFileName(this, "Save profiling capture", {}, "Profiling JSON (*.json)");
  if (path.isEmpty())
  {
    return;
  }
  const auto bytes = QJsonDocument(m_pSession->Export()).toJson(QJsonDocument::Compact);
  QSaveFile  file(path);
  if (bytes.size() > 256 * 1024 * 1024 || !file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
  {
    m_pStatus->setText("Could not save capture. Check the 256 MiB file limit and destination permissions. " + file.errorString());
  }
}

void xiiQtProfilingWidget::OpenCapture()
{
  const auto path = QFileDialog::getOpenFileName(this, "Open profiling capture", {}, "Profiling JSON (*.json)");
  if (path.isEmpty())
  {
    return;
  }
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly) || file.size() > 256 * 1024 * 1024 || !LoadCapture(file.readAll()))
  {
    m_pStatus->setText("Invalid or unsupported capture. The previous capture is preserved.");
  }
}
void xiiQtProfilingWidget::Refresh()
{
  const auto& data  = m_pSession->Display();
  const bool  force = m_uiRevision == ~quint64(0);
  if (!force && m_uiRevision == m_pSession->m_uiRevision)
  {
    return;
  }
  m_uiRevision = m_pSession->m_uiRevision;
  {
    const QSignalBlocker blocker(m_pCaptureHistory);
    m_pCaptureHistory->clear();
    m_pCaptureHistory->addItem("Live monitoring", QString{});
    for (auto it = m_pSession->m_Captures.begin(); it != m_pSession->m_Captures.end(); ++it)
    {
      m_pCaptureHistory->addItem(QString("Capture %1 • %2 frames • %3").arg(it.key()).arg(it.value().m_Frames.size()).arg(Bytes(it.value().m_iBytes)), QString::number(it.key()));
    }
    if (m_pSession->m_bPaused)
    {
      const int selected = m_pCaptureHistory->findData(QString::number(m_pSession->m_uiSelectedCapture));
      if (selected >= 0)
      {
        m_pCaptureHistory->setCurrentIndex(selected);
      }
      else
      {
        m_pCaptureHistory->setItemText(0, "Paused live view");
      }
    }
  }
  m_pPause->setText(m_pSession->m_bPaused ? "Resume" : "Pause");
  m_pRecord->setText(m_pSession->m_bFinishing ? "Finishing GPU queries…" : m_pSession->m_bRecording ? "Stop recording" :
                                                                                                      "Start recording");
  m_pRecord->setEnabled(!m_pSession->m_bFinishing);
  m_pPause->setEnabled(!m_pSession->m_bRecording);
  const QString state      = m_pSession->m_bRecording ? "Recording" : m_pSession->m_bPaused ? "Frozen capture" :
                                                                                              "Live";
  const auto&   statistics = m_pSession->m_bRecording ? m_pSession->m_Frozen : data;
  m_pStatus->setText(QString("%1 • %2 frames • %3 CPU scopes • %4 GPU measurements • Dropped at producer: %5 scopes / %6 frames / %7 GPU samples%8")
                       .arg(state)
                       .arg(statistics.m_Frames.size())
                       .arg(statistics.m_Scopes.size())
                       .arg(statistics.m_Gpu.size())
                       .arg(statistics.m_uiDroppedScopes)
                       .arg(statistics.m_uiDroppedFrames)
                       .arg(statistics.m_uiDroppedGpu)
                       .arg(statistics.m_bTruncated ? " • Capture stopped at a memory, sample, or metadata limit" : ""));
  m_pOverview->setToolTip(QString("Process ID: %1\nCPU self time excludes recorded children; the application discard threshold still applies.").arg(data.m_uiProcessId == 0 ? QString("Unavailable") : QString::number(data.m_uiProcessId)));
  m_pHistory->m_fBudget = m_pBudget->value();
  if (m_bRendering)
  {
    if (force || m_AnalysisTime.elapsed() >= 100)
    {
      RebuildGpu();
      m_AnalysisTime.restart();
    }
    UpdateGpuHistory();
    m_pHistory->update();
    return;
  }
  const QSignalBlocker first(m_pFrames);
  const QSignalBlocker last(m_pLastFrame);
  const QSignalBlocker lanes(m_pLanes);
  QSet<QString>        ids;
  for (const auto& frame : data.m_Frames)
  {
    ids.insert(QString::number(frame.m_uiIndex));
  }
  for (auto* combo : {m_pFrames, m_pLastFrame})
  {
    for (int i = combo->count(); i-- > 1;)
    {
      if (!ids.contains(combo->itemData(i).toString()))
      {
        combo->removeItem(i);
      }
    }
    QSet<QString> existing;
    for (int i = 1; i < combo->count(); ++i)
    {
      existing.insert(combo->itemData(i).toString());
    }
    for (const auto& frame : data.m_Frames)
    {
      const auto id = QString::number(frame.m_uiIndex);
      if (!existing.contains(id))
      {
        combo->addItem(QString("Frame %1 • %2 ms").arg(frame.m_uiIndex).arg((frame.m_fEndUs - frame.m_fBeginUs) / 1000, 0, 'f', 2), id);
      }
    }
  }
  QSet<QString> activeLanes;
  for (const auto& scope : data.m_Scopes)
  {
    activeLanes.insert(scope.m_Lane);
  }
  for (int i = m_pLanes->count(); i-- > 1;)
  {
    if (!activeLanes.contains(m_pLanes->itemData(i).toString()))
    {
      m_pLanes->removeItem(i);
    }
  }
  QSet<QString> knownLanes;
  for (int i = 1; i < m_pLanes->count(); ++i)
  {
    knownLanes.insert(m_pLanes->itemData(i).toString());
  }
  for (const auto& scope : data.m_Scopes)
  {
    if (!knownLanes.contains(scope.m_Lane) && knownLanes.size() < 1024)
    {
      m_pLanes->addItem(ThreadName(data, scope.m_Lane), scope.m_Lane);
      knownLanes.insert(scope.m_Lane);
    }
  }
  m_pHistory->m_Values.clear();
  m_pHistory->m_Frames.clear();
  m_pHistory->m_Labels.clear();
  QVector<double> frameTimes;
  quint64         previousFrame = 0;
  for (const auto& frame : data.m_Frames)
  {
    if (previousFrame != 0 && frame.m_uiIndex > previousFrame + 1)
    {
      m_pHistory->m_Values.push_back(-1);
      m_pHistory->m_Frames.push_back(previousFrame);
      m_pHistory->m_Labels.push_back("Missing CPU frame samples");
    }
    previousFrame   = frame.m_uiIndex;
    const double ms = (frame.m_fEndUs - frame.m_fBeginUs) / 1000;
    frameTimes.push_back(ms);
    m_pHistory->m_Values.push_back(ms);
    m_pHistory->m_Frames.push_back(frame.m_uiIndex);
    m_pHistory->m_Labels.push_back(QString("Frame %1\n%2 ms").arg(frame.m_uiIndex).arg(ms, 0, 'f', 3));
  }
  const auto&  times    = frameTimes;
  const double current  = times.isEmpty() ? 0 : times.back();
  const double average  = times.isEmpty() ? 0 : std::accumulate(times.begin(), times.end(), 0.0) / times.size();
  const int    overruns = std::count_if(times.begin(), times.end(), [this](double value) { return value > m_pBudget->value(); });
  m_pOverview->setText(QString("%1 • CPU %2 ms / %3 FPS • Average %4 ms • P95 %5 ms • Max %6 ms • %7 budget overruns")
                         .arg(data.m_Application.isEmpty() ? "Application" : data.m_Application)
                         .arg(current, 0, 'f', 2)
                         .arg(current > 0 ? 1000 / current : 0, 0, 'f', 1)
                         .arg(average, 0, 'f', 2)
                         .arg(Percentile(times, 0.95), 0, 'f', 2)
                         .arg(Percentile(times, 1), 0, 'f', 2)
                         .arg(overruns));
  if (times.isEmpty())
  {
    m_pOverview->setText("Application CPU frame timing unavailable. Connect to a profiling-enabled application, or open a capture.");
  }
  m_pHistory->update();
  double begin = 0;
  double end   = 0;
  if (!data.m_Frames.isEmpty())
  {
    auto       frame = data.m_Frames.back();
    const auto id    = m_pFrames->currentData().toString();
    for (const auto& candidate : data.m_Frames)
    {
      if (QString::number(candidate.m_uiIndex) == id)
      {
        frame = candidate;
      }
    }
    begin             = frame.m_fBeginUs;
    end               = frame.m_fEndUs;
    const auto lastId = m_pLastFrame->currentData().toString();
    for (const auto& candidate : data.m_Frames)
    {
      if (QString::number(candidate.m_uiIndex) == lastId)
      {
        begin = qMin(begin, candidate.m_fBeginUs);
        end   = qMax(end, candidate.m_fEndUs);
      }
    }
  }
  else
  {
    for (const auto& scope : data.m_Scopes)
    {
      begin = begin == 0 ? scope.m_fBeginUs : qMin(begin, scope.m_fBeginUs);
      end   = qMax(end, scope.m_fEndUs);
    }
  }
  const bool updateTables = force || m_AnalysisTime.elapsed() >= 100;
  RebuildCpu(begin, end, updateTables);
  if (updateTables)
  {
    m_AnalysisTime.restart();
  }
}

void xiiQtProfilingWidget::RebuildCpu(double begin, double end, bool updateTables)
{
  const auto&    data   = m_pSession->Display();
  const QString  lane   = m_pLanes->currentData().toString();
  const QString  filter = m_pFilter->text();
  QVector<Scope> scopes;
  for (const auto& scope : data.m_Scopes)
  {
    if (scope.m_fEndUs > begin && scope.m_fBeginUs < end && (lane.isEmpty() || scope.m_Lane == lane))
    {
      scopes.push_back(scope);
    }
  }
  std::sort(scopes.begin(), scopes.end(), [](const Scope& a, const Scope& b) {
    if (a.m_Lane != b.m_Lane)
    {
      return a.m_Lane < b.m_Lane;
    }
    return a.m_fBeginUs == b.m_fBeginUs ? a.m_fEndUs > b.m_fEndUs : a.m_fBeginUs < b.m_fBeginUs;
  });
  struct Sample
  {
    Scope   m_Scope;
    double  m_fSelf = 0, m_fCoveredUntil = 0;
    int     m_iDepth = 0;
    QString m_Key;
  };
  QVector<Sample>             samples;
  QMap<QString, QVector<int>> stacks;
  QMap<QString, int>          depths;
  for (auto scope : scopes)
  {
    scope.m_fBeginUs = qMax(begin, scope.m_fBeginUs);
    scope.m_fEndUs   = qMin(end, scope.m_fEndUs);
    auto& stack      = stacks[scope.m_Lane];
    while (!stack.isEmpty() && (samples[stack.back()].m_Scope.m_fEndUs <= scope.m_fBeginUs || samples[stack.back()].m_Scope.m_fEndUs < scope.m_fEndUs))
    {
      stack.pop_back();
    }
    if (!stack.isEmpty())
    {
      auto& parent           = samples[stack.back()];
      parent.m_fSelf         = qMax(0.0, parent.m_fSelf - qMax(0.0, scope.m_fEndUs - qMax(scope.m_fBeginUs, parent.m_fCoveredUntil)));
      parent.m_fCoveredUntil = qMax(parent.m_fCoveredUntil, scope.m_fEndUs);
    }
    const int depth = stack.size();
    stack.push_back(samples.size());
    const auto key = scope.m_Lane + ":" + QString::number(scope.m_uiId);
    samples.push_back({scope, scope.m_fEndUs - scope.m_fBeginUs, scope.m_fBeginUs, depth, key});
    depths[scope.m_Lane] = qMax(depths.value(scope.m_Lane), depth + 1);
  }
  struct Aggregate
  {
    QString         m_Name, m_Lane;
    QVector<double> m_Durations;
    double          m_fInclusive = 0, m_fSelf = 0;
  };
  QMap<QString, Aggregate>   aggregates;
  QMap<QString, QStringList> rows;
  QMap<QString, QStringList> totals;
  QMap<QString, QString>     details;
  QMap<QString, int>         laneIndices;
  m_pTimeline->m_Lanes.clear();
  m_pTimeline->m_LaneY.clear();
  int laneY = 35;
  m_pTimeline->m_Bars.clear();
  for (auto it = depths.begin(); it != depths.end(); ++it)
  {
    laneIndices[it.key()] = laneY;
    m_pTimeline->m_LaneY.push_back(laneY);
    laneY += 30 + it.value() * 22;
    m_pTimeline->m_Lanes.push_back(ThreadName(data, it.key()));
  }
  int visible = 0;
  for (const auto& sample : samples)
  {
    const auto& scope = sample.m_Scope;
    if (!filter.isEmpty() && !scope.m_Name.contains(filter, Qt::CaseInsensitive) && !scope.m_Function.contains(filter, Qt::CaseInsensitive))
    {
      continue;
    }
    const double  ms        = (scope.m_fEndUs - scope.m_fBeginUs) / 1000;
    const QString operation = scope.m_Lane + QChar(0x1f) + scope.m_Name + QChar(0x1f) + scope.m_Function;
    auto&         aggregate = aggregates[operation];
    aggregate.m_Name        = scope.m_Name;
    aggregate.m_Lane        = scope.m_Lane;
    aggregate.m_Durations.push_back(ms);
    aggregate.m_fInclusive += ms;
    aggregate.m_fSelf += sample.m_fSelf / 1000;
    // The identity is independent of clipped timestamps and display names.
    const QString key = sample.m_Key;
    details[key]      = QString("%1\n%2\nInclusive %3 ms • Self %4 ms\nThread identity: %5\n%6")
                          .arg(scope.m_Name, ThreadName(data, scope.m_Lane))
                          .arg(ms, 0, 'f', 3)
                          .arg(sample.m_fSelf / 1000, 0, 'f', 3)
                          .arg(scope.m_Lane, scope.m_Function);
    if (visible < 20000)
    {
      rows[key] = {scope.m_Name, ThreadName(data, scope.m_Lane), QString::number((scope.m_fBeginUs - begin) / 1000, 'f', 3), QString::number(ms, 'f', 3), QString::number(sample.m_fSelf / 1000, 'f', 3), QString::number(sample.m_iDepth), scope.m_Function};
    }
    if (visible++ < 20000)
    {
      m_pTimeline->m_Bars.push_back({QRectF((scope.m_fBeginUs - begin) / qMax(1.0, end - begin), laneIndices[scope.m_Lane] + sample.m_iDepth * 22, (scope.m_fEndUs - scope.m_fBeginUs) / qMax(1.0, end - begin), 20), scope.m_Name, details[key], key, QColor::fromHsv(int(qHash(scope.m_Name) % 360), 125, 175)});
    }
  }
  for (auto it = aggregates.begin(); it != aggregates.end(); ++it)
  {
    const auto& value = it.value();
    totals[it.key()]  = {value.m_Name, ThreadName(data, value.m_Lane), QString::number(value.m_Durations.size()), QString::number(value.m_fInclusive, 'f', 3), QString::number(value.m_fSelf, 'f', 3), QString::number(value.m_fInclusive / value.m_Durations.size(), 'f', 3), QString::number(Percentile(value.m_Durations, 0), 'f', 3), QString::number(Percentile(value.m_Durations, 0.95), 'f', 3), QString::number(Percentile(value.m_Durations, 1), 'f', 3)};
    details[it.key()] = "Thread identity: " + value.m_Lane + "\nInclusive totals overlap across threads and nested scopes.";
  }
  if (updateTables)
  {
    UpdateRows(m_pScopes, rows, details);
    UpdateRows(m_pHotspots, totals, details);
  }
  if (visible > 20000)
  {
    m_pStatus->setText(m_pStatus->text() + " • Timeline and scope rows show the first 20,000 matches; hotspots include all matches.");
  }
  m_pTimeline->setMinimumHeight(qMax(300, laneY + 20));
  m_pTimeline->update();
}

void xiiQtProfilingWidget::RebuildGpu()
{
  const auto& data = m_pSession->Display();
  {
    const QSignalBlocker blocker(m_pGraphs);
    for (int i = m_pGraphs->count(); i-- > 1;)
    {
      if (!data.m_Graphs.contains(m_pGraphs->itemData(i).toString().toULongLong()))
      {
        m_pGraphs->removeItem(i);
      }
    }
    for (auto it = data.m_Graphs.begin(); it != data.m_Graphs.end(); ++it)
    {
      const auto key   = QString::number(it.key());
      const auto index = m_pGraphs->findData(key);
      if (index < 0)
      {
        m_pGraphs->addItem(GraphLabel(data.m_Graphs, it.key()), key);
      }
      else
      {
        m_pGraphs->setItemText(index, GraphLabel(data.m_Graphs, it.key()));
      }
    }
  }
  {
    QMap<quint64, bool> frames;
    for (const auto& sample : data.m_Gpu)
    {
      frames[sample.m_uiFrame] = true;
    }
    for (auto* combo : {m_pFrames, m_pLastFrame})
    {
      const QSignalBlocker blocker(combo);
      QSet<QString>        existing;
      for (int i = combo->count(); i-- > 1;)
      {
        const auto id = combo->itemData(i).toString();
        if (!frames.contains(id.toULongLong()))
        {
          combo->removeItem(i);
        }
        else
        {
          existing.insert(id);
        }
      }
      for (auto it = frames.begin(); it != frames.end(); ++it)
      {
        const auto id = QString::number(it.key());
        if (!existing.contains(id))
        {
          combo->addItem("GPU frame " + id, id);
        }
      }
    }
  }
  const auto selectedFrame = m_pFrames->currentData().toString();
  const auto rangeEnd      = m_pLastFrame->currentData().toString();
  quint64    firstFrame    = selectedFrame.isEmpty() ? 0 : selectedFrame.toULongLong();
  quint64    lastFrame     = selectedFrame.isEmpty() ? std::numeric_limits<quint64>::max() : firstFrame;
  if (!selectedFrame.isEmpty() && !rangeEnd.isEmpty())
  {
    firstFrame = qMin(firstFrame, rangeEnd.toULongLong());
    lastFrame  = qMax(lastFrame, rangeEnd.toULongLong());
  }
  struct Aggregate
  {
    xiiInspectorProfilingSession::GpuSample m_Last;
    QVector<double>                         m_Durations;
    int                                     m_iUnavailable = 0;
  };
  QMap<QString, Aggregate> values;
  const QString            filter = m_pFilter->text();
  for (const auto& sample : data.m_Gpu)
  {
    if (sample.m_uiFrame < firstFrame || sample.m_uiFrame > lastFrame)
    {
      continue;
    }
    const auto graph = GraphLabel(data.m_Graphs, sample.m_uiGraph);
    if ((!m_pGraphs->currentData().toString().isEmpty() && m_pGraphs->currentData().toString() != QString::number(sample.m_uiGraph)) || (m_pQueues->currentIndex() > 0 && m_pQueues->currentIndex() - 1 != int(sample.m_uiQueue)) || (!filter.isEmpty() && !sample.m_Name.contains(filter, Qt::CaseInsensitive) && !graph.contains(filter, Qt::CaseInsensitive)))
    {
      continue;
    }
    auto& value = values[GpuKey(sample)];
    if (sample.m_uiFrame >= value.m_Last.m_uiFrame)
    {
      value.m_Last = sample;
    }
    if (sample.m_bAvailable)
    {
      value.m_Durations.push_back(sample.m_fMs);
    }
    else
    {
      ++value.m_iUnavailable;
    }
  }
  QMap<QString, QStringList>         rows;
  QMap<QString, QString>             details;
  QMap<QPair<quint64, QString>, int> duplicates;
  for (const auto& value : values)
  {
    ++duplicates[{value.m_Last.m_uiGraph, value.m_Last.m_Name}];
  }
  for (auto it = values.begin(); it != values.end(); ++it)
  {
    const auto&  value     = it.value();
    const auto&  sample    = value.m_Last;
    const auto   graph     = GraphLabel(data.m_Graphs, sample.m_uiGraph);
    const auto&  durations = value.m_Durations;
    const double mean      = durations.isEmpty() ? -1 : std::accumulate(durations.begin(), durations.end(), 0.0) / durations.size();
    rows[it.key()]         = {graph, duplicates.value({sample.m_uiGraph, sample.m_Name}) > 1 ? sample.m_Name + QString(" · %1 %2").arg(sample.m_bSubmission ? "Submission" : "Pass").arg(sample.m_uiPass + 1) : sample.m_Name, QueueName(sample.m_uiQueue), sample.m_bAvailable ? QString::number(sample.m_fMs, 'f', 3) : "Unavailable", QString::number(sample.m_uiFrame), QString::number(durations.size()), mean < 0 ? "Unavailable" : QString::number(mean, 'f', 3), durations.isEmpty() ? "Unavailable" : QString::number(Percentile(durations, 0), 'f', 3), durations.isEmpty() ? "Unavailable" : QString::number(Percentile(durations, 0.95), 'f', 3), durations.isEmpty() ? "Unavailable" : QString::number(Percentile(durations, 1), 'f', 3)};
    details[it.key()]      = QString("%1 / %2\nGraph instance: %3\n%4 index: %5\nGPU recording frame: %6\nUnavailable samples: %7\nDurations have no absolute GPU timestamps. Submission costs may overlap across queues; their sum is not GPU frame wall time.")
                               .arg(graph, sample.m_Name)
                               .arg(sample.m_uiGraph)
                               .arg(sample.m_bSubmission ? "Submission" : "Pass")
                               .arg(sample.m_uiPass)
                               .arg(sample.m_uiFrame)
                               .arg(value.m_iUnavailable);
  }
  UpdateRows(m_pGpuPasses, rows, details);
  if (m_SelectedGpu.isEmpty() && !values.isEmpty())
  {
    m_SelectedGpu = values.firstKey();
    for (int i = 0; i < m_pGpuPasses->topLevelItemCount(); ++i)
    {
      if (m_pGpuPasses->topLevelItem(i)->data(0, Qt::UserRole).toString() == m_SelectedGpu)
      {
        m_pGpuPasses->setCurrentItem(m_pGpuPasses->topLevelItem(i));
      }
    }
  }
  UpdateGpuHistory();
  auto available = m_pHistory->m_Values;
  available.erase(std::remove_if(available.begin(), available.end(), [](double value) { return value < 0; }), available.end());
  m_pOverview->setText(QString("GPU • %1 operations • %2 measurements • Selected operation P95 %3 • GPU frame wall time unavailable")
                         .arg(values.size())
                         .arg(data.m_Gpu.size())
                         .arg(available.isEmpty() ? "Unavailable" : QString::number(Percentile(available, 0.95), 'f', 3) + " ms"));
  m_pOverview->setToolTip("Each submission and pass retains its recording frame. Query results arrive asynchronously. Graphics, compute, and transfer work can overlap.");
}

void xiiQtProfilingWidget::UpdateGpuHistory()
{
  m_pHistory->m_Values.clear();
  m_pHistory->m_Labels.clear();
  const auto parts = m_SelectedGpu.split(':');
  if (parts.size() == 4)
  {
    const quint64                                           graph      = parts[0].toULongLong();
    const bool                                              submission = parts[1] == "1";
    const auto                                              pass       = parts[2].toUInt();
    const auto                                              queue      = parts[3].toUInt();
    QVector<const xiiInspectorProfilingSession::GpuSample*> samples;
    for (const auto& sample : m_pSession->Display().m_Gpu)
    {
      if (sample.m_uiGraph == graph && sample.m_bSubmission == submission && sample.m_uiPass == pass && sample.m_uiQueue == queue)
      {
        samples.push_back(&sample);
      }
    }
    std::stable_sort(samples.begin(), samples.end(), [](const auto* a, const auto* b) { return a->m_uiFrame < b->m_uiFrame; });
    quint64 previousFrame = 0;
    for (const auto* sample : samples)
    {
      if (previousFrame != 0 && sample->m_uiFrame > previousFrame + 1)
      {
        m_pHistory->m_Values.push_back(-1);
        m_pHistory->m_Labels.push_back("No GPU sample for the intervening recording frames");
      }
      previousFrame = sample->m_uiFrame;
      m_pHistory->m_Values.push_back(sample->m_bAvailable ? sample->m_fMs : -1);
      m_pHistory->m_Labels.push_back(sample->m_Name + QString("\nGPU frame %1\n%2").arg(sample->m_uiFrame).arg(sample->m_bAvailable ? QString::number(sample->m_fMs, 'f', 3) + " ms" : "Unavailable"));
    }
  }
  // Timestamp scopes have a different contract from duration-only render graph measurements.
  // Keep each supplied GPU clock domain separate, without positioning it on the CPU timeline.
  m_pTimeline->m_Bars.clear();
  m_pTimeline->m_Lanes.clear();
  m_pTimeline->m_LaneY.clear();
  QMap<QString, QVector<Scope>> lanes;
  for (const auto& scope : m_pSession->Display().m_GpuScopes)
  {
    lanes[scope.m_Lane].push_back(scope);
  }
  int y = 35;
  for (auto it = lanes.begin(); it != lanes.end(); ++it)
  {
    const auto label = it.key().startsWith("GPU ") ? it.key() : m_pSession->Display().m_Threads.value(it.key(), QString("GPU timestamp lane %1").arg(m_pTimeline->m_Lanes.size() + 1));
    m_pTimeline->m_Lanes.push_back(label + "\nGPU timestamp domain");
    m_pTimeline->m_LaneY.push_back(y);
    auto& scopes = it.value();
    std::sort(scopes.begin(), scopes.end(), [](const Scope& a, const Scope& b) { return a.m_fBeginUs == b.m_fBeginUs ? a.m_fEndUs > b.m_fEndUs : a.m_fBeginUs < b.m_fBeginUs; });
    const double begin = scopes.front().m_fBeginUs;
    double       end   = begin;
    for (const auto& scope : scopes)
    {
      end = qMax(end, scope.m_fEndUs);
    }
    QVector<double> stack;
    int             depthMax = 1;
    for (const auto& scope : scopes)
    {
      while (!stack.isEmpty() && stack.back() <= scope.m_fBeginUs)
      {
        stack.pop_back();
      }
      const int depth = stack.size();
      stack.push_back(scope.m_fEndUs);
      depthMax = qMax(depthMax, depth + 1);
      if (!m_pFilter->text().isEmpty() && !scope.m_Name.contains(m_pFilter->text(), Qt::CaseInsensitive))
      {
        continue;
      }
      if (m_pTimeline->m_Bars.size() < 20000)
      {
        m_pTimeline->m_Bars.push_back({QRectF((scope.m_fBeginUs - begin) / qMax(1.0, end - begin), y + depth * 22, (scope.m_fEndUs - scope.m_fBeginUs) / qMax(1.0, end - begin), 20), scope.m_Name, scope.m_Name + QString("\n%1 ms\nIndependent GPU timestamp domain. No application frame identity was supplied.").arg((scope.m_fEndUs - scope.m_fBeginUs) / 1000, 0, 'f', 3), scope.m_Lane + ":" + QString::number(scope.m_fBeginUs, 'f', 3), QColor("#946ac4")});
      }
    }
    y += 30 + depthMax * 22;
  }
  m_pTimeline->setMinimumHeight(qMax(300, y + 20));
  m_pTimeline->update();
}

void xiiQtProfilingWidget::ApplyGraphCapture(const QJsonObject& object)
{
  // Explicit snapshots may seed offline costs, but never modify a recording or frozen capture.
  if (m_pSession->m_bRecording)
  {
    return;
  }
  xiiInspectorProfilingSession::Data batch;
  const auto                         id = object["instance"].toString(object["id"].toString()).toULongLong();
  batch.m_Graphs[id]                    = (object["view"].toString().isEmpty() ? QString{} : object["view"].toString() + " / ") + object["name"].toString();
  batch.m_fReceivedUs                   = m_pSession->m_Live.m_fReceivedUs;
  batch.m_uiDroppedScopes               = m_pSession->m_Live.m_uiDroppedScopes;
  batch.m_uiDroppedFrames               = m_pSession->m_Live.m_uiDroppedFrames;
  batch.m_uiDroppedGpu                  = m_pSession->m_Live.m_uiDroppedGpu;
  if (m_pSession->m_Live.m_Gpu.isEmpty())
  {
    for (const auto& value : object["passes"].toArray())
    {
      const auto pass = value.toObject();
      batch.m_Gpu.push_back({id, pass["timingFrame"].toString().toULongLong(), quint32(pass["index"].toInt()), quint32(pass["queue"].toInt()), false, pass["timingAvailable"].toBool(), pass["gpuMs"].toDouble(), pass["name"].toString()});
    }
  }
  m_pSession->Append(batch);
  m_uiRevision = ~quint64(0);
  Refresh();
}
