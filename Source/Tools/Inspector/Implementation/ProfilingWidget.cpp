/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Inspector/InspectorPCH.h>
#include <Inspector/CaptureUi.h>
#include <Inspector/ProfilingWidget.h>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QGraphicsScene>
#include <QJsonDocument>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>
#include <algorithm>

using namespace xiiInspectorUi;
xiiQtProfilingWidget* xiiQtProfilingWidget::s_pWidget = nullptr;

namespace
{
  QString TraceId(const QJsonValue& value)
  {
    return value.isString() ? value.toString() : QString::number(value.toInteger());
  }
  QString LaneKey(const QJsonObject& event) { return TraceId(event["pid"]) + ":" + TraceId(event["tid"]); }
  double Percentile(QVector<double> values, double percentile)
  {
    if (values.isEmpty()) return 0;
    std::sort(values.begin(), values.end());
    return values[qBound(0, static_cast<int>(std::ceil(values.size() * percentile)) - 1, static_cast<int>(values.size()) - 1)];
  }
}

xiiQtProfilingWidget::xiiQtProfilingWidget(ads::CDockManager* manager) : xiiQtRenderingCapture(manager, "Rendering Profiler", 'PROF')
{
  s_pWidget = this;
  setObjectName("RenderingProfiler");
  auto* selection = new QHBoxLayout;
  m_pLayout->addLayout(selection);
  m_pHistory = new QComboBox;
  m_pHistory->setAccessibleName("Profiling capture history");
  m_pFrames = new QComboBox;
  m_pFrames->setAccessibleName("Profiling frame selection");
  m_pLanes = new QComboBox;
  m_pLanes->setAccessibleName("Profiling thread or GPU selection");
  m_pBudget = new QDoubleSpinBox;
  m_pBudget->setRange(0.1, 1000);
  m_pBudget->setDecimals(2);
  m_pBudget->setSuffix(" ms budget");
  m_pBudget->setValue(16.67);
  m_pBudget->setAccessibleName("Frame budget in milliseconds");
  selection->addWidget(m_pHistory, 1);
  selection->addWidget(m_pFrames, 1);
  selection->addWidget(m_pLanes, 1);
  selection->addWidget(m_pBudget);
  auto* fit = new QPushButton("Fit timeline");
  m_pControls->addWidget(fit);
  auto* tabs = new QTabWidget;
  m_pLayout->addWidget(tabs, 1);
  m_pTimeline = new QGraphicsScene(this);
  auto* timeline = new GraphicsView(m_pTimeline);
  timeline->setAccessibleName("CPU and GPU scope timeline");
  tabs->addTab(timeline, "Timeline");
  m_pFrameChart = new QGraphicsScene(this);
  auto* frames = new GraphicsView(m_pFrameChart);
  frames->setAccessibleName("Frame durations and budget chart");
  tabs->addTab(frames, "Frame history");
  m_pHotspots = Table({"Scope", "Thread / GPU", "Calls", "Inclusive ms", "Self ms", "Mean ms", "P95 ms", "Max ms"}, "Profiling hotspots");
  tabs->addTab(m_pHotspots, "Hotspots");
  m_pScopes = Table({"Scope", "Thread / GPU", "Start ms", "Duration ms", "Self ms", "Depth", "Function"}, "Profiling scopes");
  tabs->addTab(m_pScopes, "Scopes");
  m_pGpuPasses = Table({"Graph", "Pass", "Queue", "GPU ms", "Measured frame", "Capture frame", "Status"}, "GPU pass costs");
  m_pGpuPasses->setToolTip("Delayed GPU duration queries. Queue and pass costs may overlap; their sum is not frame wall time.");
  tabs->addTab(m_pGpuPasses, "GPU pass costs");
  connect(m_pHistory, &QComboBox::currentIndexChanged, this, [this](int index) { SelectCapture(index); });
  connect(m_pFrames, &QComboBox::currentIndexChanged, this, [this] { Rebuild(); });
  connect(m_pLanes, &QComboBox::currentIndexChanged, this, [this] { Rebuild(); });
  connect(m_pBudget, &QDoubleSpinBox::valueChanged, this, [this] { Rebuild(); });
  connect(fit, &QPushButton::clicked, this, [this, timeline, frames] {
    timeline->fitInView(m_pTimeline->itemsBoundingRect(), Qt::KeepAspectRatio);
    frames->fitInView(m_pFrameChart->itemsBoundingRect(), Qt::KeepAspectRatio);
  });
  connect(m_pFrameChart, &QGraphicsScene::selectionChanged, this, [this] {
    const auto selected = m_pFrameChart->selectedItems();
    if (!selected.isEmpty())
      m_pFrames->setCurrentIndex(selected.front()->data(0).toInt() + 1);
  });
}

xiiQtProfilingWidget::~xiiQtProfilingWidget() { s_pWidget = nullptr; }
void xiiQtProfilingWidget::ProcessTelemetry(void*) { if (s_pWidget) s_pWidget->ReceiveMessages(); }

bool xiiQtProfilingWidget::Decode(const QJsonObject& object, QVector<Scope>& scopes, QMap<QString, QString>& lanes)
{
  if (!object["traceEvents"].isArray() || object["traceEvents"].toArray().size() > 200000)
    return false;
  QVector<Scope> decoded;
  QMap<QString, QString> names;
  QMap<QPair<QString, QString>, QVector<Scope>> pending;
  for (const auto& value : object["traceEvents"].toArray())
  {
    if (!value.isObject()) return false;
    const auto event = value.toObject();
    const QString phase = event["ph"].toString(), lane = LaneKey(event), name = event["name"].toString();
    if (phase == "M" && name == "thread_name")
      names[lane] = event["args"].toObject()["name"].toString();
    if (phase != "B" && phase != "E" && phase != "X") continue;
    const double begin = event["ts"].toDouble(-1);
    if (!std::isfinite(begin) || begin < 0) return false;
    if (phase == "B")
      pending[{lane, name}].push_back({name, lane, event["args"].toObject()["function"].toString(), begin, 0});
    else if (phase == "E")
    {
      auto& stack = pending[{lane, name}];
      if (stack.isEmpty()) continue;
      Scope scope = stack.takeLast();
      if (begin < scope.m_fBeginUs) return false;
      scope.m_fEndUs = begin;
      decoded.push_back(scope);
    }
    else
    {
      const double duration = event["dur"].toDouble(-1);
      if (!std::isfinite(duration) || duration < 0 || duration > 1e12) return false;
      decoded.push_back({name, lane, event["args"].toObject()["function"].toString(), begin, begin + duration});
    }
  }
  std::stable_sort(decoded.begin(), decoded.end(), [](const Scope& a, const Scope& b) {
    if (a.m_Lane != b.m_Lane) return a.m_Lane < b.m_Lane;
    return a.m_fBeginUs == b.m_fBeginUs ? a.m_fEndUs > b.m_fEndUs : a.m_fBeginUs < b.m_fBeginUs;
  });
  for (const auto& scope : decoded)
    if (!names.contains(scope.m_Lane)) names[scope.m_Lane] = "Thread " + scope.m_Lane;
  scopes = std::move(decoded);
  lanes = std::move(names);
  return true;
}

bool xiiQtProfilingWidget::ApplyCapture(const QJsonObject& object)
{
  QVector<Scope> scopes;
  QMap<QString, QString> lanes;
  if (!Decode(object, scopes, lanes)) return false;
  m_History.push_back(object);
  qint64 total = 0;
  for (const auto& capture : m_History) total += QJsonDocument(capture).toJson(QJsonDocument::Compact).size();
  while (m_History.size() > 30 || (total > 64 * 1024 * 1024 && m_History.size() > 1))
    total -= QJsonDocument(m_History.takeFirst()).toJson(QJsonDocument::Compact).size();
  const QSignalBlocker blocker(m_pHistory);
  m_pHistory->clear();
  for (int i = 0; i < m_History.size(); ++i) m_pHistory->addItem(QString("Capture %1").arg(i + 1));
  m_pHistory->setCurrentIndex(m_History.size() - 1);
  SelectCapture(m_History.size() - 1);
  return true;
}

void xiiQtProfilingWidget::SelectCapture(int index)
{
  if (index < 0 || index >= m_History.size()) return;
  SetDocument(m_History[index]);
  Decode(m_Document, m_Scopes, m_Lanes);
  const QSignalBlocker frameBlocker(m_pFrames), laneBlocker(m_pLanes);
  m_Frames.clear();
  m_pFrames->clear();
  m_pFrames->addItem("All captured frames");
  for (const auto& scope : m_Scopes)
  {
    if (m_Lanes.value(scope.m_Lane) == "Frames" && scope.m_Name.startsWith("Frame "))
    {
      m_Frames.push_back(scope);
      m_pFrames->addItem(scope.m_Name + QString(" • %1 ms").arg((scope.m_fEndUs - scope.m_fBeginUs) / 1000.0, 0, 'f', 2));
    }
  }
  const auto selected = m_pLanes->currentData().toString();
  m_pLanes->clear();
  m_pLanes->addItem("All threads and GPUs", QString{});
  for (auto it = m_Lanes.begin(); it != m_Lanes.end(); ++it) m_pLanes->addItem(it.value(), it.key());
  m_pLanes->setCurrentIndex(qMax(0, m_pLanes->findData(selected)));
  Rebuild();
}

void xiiQtProfilingWidget::ClearCapture()
{
  const QSignalBlocker history(m_pHistory), frames(m_pFrames), lanes(m_pLanes);
  m_History.clear(); m_Scopes.clear(); m_Frames.clear(); m_Lanes.clear();
  m_GpuGraphs.clear();
  m_pHistory->clear(); m_pFrames->clear(); m_pLanes->clear();
  Rebuild();
}

void xiiQtProfilingWidget::Rebuild()
{
  RebuildGpu();
  m_pTimeline->clear(); m_pFrameChart->clear();
  m_pHotspots->setSortingEnabled(false); m_pScopes->setSortingEnabled(false);
  m_pHotspots->clear(); m_pScopes->clear();
  if (m_Scopes.isEmpty())
  {
    m_pStatus->setText("No completed profiling scopes. Enable XII_USE_PROFILING in the application and capture again.");
    return;
  }
  double begin = m_Scopes.front().m_fBeginUs, end = begin;
  for (const auto& scope : m_Scopes) { begin = qMin(begin, scope.m_fBeginUs); end = qMax(end, scope.m_fEndUs); }
  const int frameIndex = m_pFrames->currentIndex() - 1;
  if (frameIndex >= 0 && frameIndex < m_Frames.size()) { begin = m_Frames[frameIndex].m_fBeginUs; end = m_Frames[frameIndex].m_fEndUs; }
  const double scale = 1200.0 / qMax(1.0, end - begin);
  const QString filter = m_pFilter->text(), selectedLane = m_pLanes->currentData().toString();
  struct Sample { Scope m_Scope; double m_fSelf; int m_iDepth; };
  QVector<Sample> samples;
  QMap<QString, QVector<int>> stacks;
  QMap<QString, int> depths;
  for (const auto& scope : m_Scopes)
  {
    if (scope.m_fEndUs < begin || scope.m_fBeginUs > end || (!selectedLane.isEmpty() && selectedLane != scope.m_Lane)) continue;
    Scope clipped = scope;
    clipped.m_fBeginUs = qMax(begin, scope.m_fBeginUs); clipped.m_fEndUs = qMin(end, scope.m_fEndUs);
    auto& stack = stacks[scope.m_Lane];
    while (!stack.isEmpty() && (samples[stack.back()].m_Scope.m_fEndUs <= clipped.m_fBeginUs || samples[stack.back()].m_Scope.m_fEndUs < clipped.m_fEndUs)) stack.pop_back();
    const double duration = clipped.m_fEndUs - clipped.m_fBeginUs;
    if (!stack.isEmpty()) samples[stack.back()].m_fSelf = qMax(0.0, samples[stack.back()].m_fSelf - duration);
    const int depth = stack.size();
    stack.push_back(samples.size());
    samples.push_back({clipped, duration, depth});
    depths[scope.m_Lane] = qMax(depths.value(scope.m_Lane), depth + 1);
  }
  QMap<QString, double> laneY;
  double y = 35;
  for (auto it = depths.begin(); it != depths.end(); ++it)
  {
    laneY[it.key()] = y;
    auto* label = m_pTimeline->addSimpleText(m_Lanes.value(it.key())); label->setBrush(Qt::white); label->setPos(0, y);
    y += 30 + it.value() * 22;
  }
  for (int tick = 0; tick <= 10; ++tick)
  {
    const double x = 200 + tick * 120;
    m_pTimeline->addLine(x, 25, x, y, QPen(QColor("#38404a")))->setZValue(-1);
    auto* label = m_pTimeline->addSimpleText(QString::number((end - begin) / 1000.0 * tick / 10, 'f', 2) + " ms"); label->setBrush(Qt::white); label->setPos(x, 0);
  }
  struct Aggregate { QString m_Name, m_Lane; QVector<double> m_Durations; double m_fInclusive = 0, m_fSelf = 0, m_fMax = 0; };
  QMap<QPair<QString, QString>, Aggregate> totals;
  int visible = 0;
  for (const auto& sample : samples)
  {
    const auto& scope = sample.m_Scope;
    if (!filter.isEmpty() && !scope.m_Name.contains(filter, Qt::CaseInsensitive) && !scope.m_Function.contains(filter, Qt::CaseInsensitive)) continue;
    const double ms = (scope.m_fEndUs - scope.m_fBeginUs) / 1000.0;
    auto& aggregate = totals[{scope.m_Lane, scope.m_Name}];
    aggregate.m_Name = scope.m_Name; aggregate.m_Lane = scope.m_Lane;
    aggregate.m_Durations.push_back(ms); aggregate.m_fInclusive += ms; aggregate.m_fSelf += sample.m_fSelf / 1000.0; aggregate.m_fMax = qMax(aggregate.m_fMax, ms);
    Row(m_pScopes, {scope.m_Name, m_Lanes.value(scope.m_Lane), QString::number((scope.m_fBeginUs - begin) / 1000.0, 'f', 3), QString::number(ms, 'f', 3), QString::number(sample.m_fSelf / 1000.0, 'f', 3), QString::number(sample.m_iDepth), scope.m_Function});
    if (visible++ < 20000)
    {
      const QColor color = QColor::fromHsv(static_cast<int>(qHash(scope.m_Name) % 360), 135, 190);
      const QRectF rect(200 + (scope.m_fBeginUs - begin) * scale, laneY[scope.m_Lane] + sample.m_iDepth * 22, qMax(1.0, (scope.m_fEndUs - scope.m_fBeginUs) * scale), 20);
      auto* bar = m_pTimeline->addRect(rect, QPen(Qt::NoPen), color);
      bar->setToolTip(scope.m_Name + QString("\n%1\nInclusive: %2 ms\nSelf: %3 ms\n%4").arg(m_Lanes.value(scope.m_Lane)).arg(ms, 0, 'f', 3).arg(sample.m_fSelf / 1000.0, 0, 'f', 3).arg(scope.m_Function));
      if (rect.width() > 70)
      {
        auto* text = new QGraphicsSimpleTextItem(QFontMetrics(QApplication::font()).elidedText(scope.m_Name, Qt::ElideRight, rect.width() - 6), bar);
        text->setBrush(Qt::white); text->setPos(rect.topLeft() + QPointF(3, 1));
      }
    }
  }
  for (const auto& aggregate : totals)
    Row(m_pHotspots, {aggregate.m_Name, m_Lanes.value(aggregate.m_Lane), QString::number(aggregate.m_Durations.size()), QString::number(aggregate.m_fInclusive, 'f', 3), QString::number(aggregate.m_fSelf, 'f', 3), QString::number(aggregate.m_fInclusive / aggregate.m_Durations.size(), 'f', 3), QString::number(Percentile(aggregate.m_Durations, 0.95), 'f', 3), QString::number(aggregate.m_fMax, 'f', 3)});
  m_pHotspots->setSortingEnabled(true); m_pHotspots->sortItems(3, Qt::DescendingOrder);
  m_pScopes->setSortingEnabled(true);
  QVector<double> frameTimes;
  double maxFrame = m_pBudget->value();
  int overBudget = 0;
  for (const auto& frame : m_Frames) { const double ms = (frame.m_fEndUs - frame.m_fBeginUs) / 1000.0; frameTimes.push_back(ms); maxFrame = qMax(maxFrame, ms); overBudget += ms > m_pBudget->value(); }
  const double chartScale = 180.0 / qMax(0.1, maxFrame);
  auto* chartLabel = m_pFrameChart->addSimpleText("Frame wall time (ms) • click a bar to inspect that frame"); chartLabel->setBrush(Qt::white);
  for (int i = 0; i < frameTimes.size(); ++i)
  {
    const double height = frameTimes[i] * chartScale;
    auto* bar = m_pFrameChart->addRect(i * 16, 210 - height, 13, height, QPen(Qt::NoPen), frameTimes[i] > m_pBudget->value() ? QColor("#db7069") : QColor("#43a88e"));
    bar->setFlag(QGraphicsItem::ItemIsSelectable); bar->setData(0, i); bar->setToolTip(m_Frames[i].m_Name + QString("\n%1 ms").arg(frameTimes[i], 0, 'f', 3));
  }
  m_pFrameChart->addLine(0, 210 - m_pBudget->value() * chartScale, qMax(100, static_cast<int>(frameTimes.size()) * 16), 210 - m_pBudget->value() * chartScale, QPen(QColor("#e8bc62"), 2));
  m_pStatus->setText(QString("%1 completed scopes • %2 ms window • %3 frames • Frame P50 %4 ms / P95 %5 ms / max %6 ms • %7 over budget. Inclusive scope totals may overlap across threads and nested scopes.%8")
    .arg(samples.size()).arg((end - begin) / 1000.0, 0, 'f', 2).arg(frameTimes.size()).arg(Percentile(frameTimes, 0.5), 0, 'f', 2).arg(Percentile(frameTimes, 0.95), 0, 'f', 2).arg(Percentile(frameTimes, 1), 0, 'f', 2).arg(overBudget)
    .arg(visible > 20000 ? " Timeline limited to 20,000 bars; tables retain all scopes." : ""));
  m_pTimeline->setSceneRect(m_pTimeline->itemsBoundingRect().adjusted(-10, -10, 10, 10));
  m_pFrameChart->setSceneRect(m_pFrameChart->itemsBoundingRect().adjusted(-10, -10, 10, 10));
}

void xiiQtProfilingWidget::ApplyGraphCapture(const QJsonObject& object)
{
  const QString id = object["id"].toString();
  // Retain only costs in this dock, rather than duplicating full graph captures.
  QJsonObject costs;
  for (const auto* key : {"id", "name", "frame", "gpuProfiling", "passes"}) costs[key] = object[key];
  m_GpuGraphs[id] = costs;
  if (m_GpuGraphs.size() > 64)
  {
    auto oldest = m_GpuGraphs.begin();
    if (oldest.key() == id) ++oldest;
    m_GpuGraphs.erase(oldest);
  }
  RebuildGpu();
}

void xiiQtProfilingWidget::RebuildGpu()
{
  m_pGpuPasses->setSortingEnabled(false);
  m_pGpuPasses->clear();
  for (const auto& graph : m_GpuGraphs)
    for (const auto& value : graph["passes"].toArray())
    {
      const auto pass = value.toObject();
      Row(m_pGpuPasses, {graph["name"].toString(), pass["name"].toString(), QueueName(pass["queue"].toInt()), pass["timingAvailable"].toBool() ? QString::number(pass["gpuMs"].toDouble(), 'f', 3) : "Unavailable", pass["timingFrame"].toString(), graph["frame"].toString(), graph["gpuProfiling"].toBool() ? (pass["timingAvailable"].toBool() ? "Resolved (delayed)" : "Warmup / unsupported queue") : "Profiling disabled"}, m_pFilter->text());
    }
  m_pGpuPasses->setSortingEnabled(true);
  m_pGpuPasses->sortItems(3, Qt::DescendingOrder);
}
