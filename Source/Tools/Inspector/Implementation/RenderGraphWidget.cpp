/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Inspector/InspectorPCH.h>

#include <Inspector/CaptureUi.h>
#include <Inspector/ProfilingWidget.h>
#include <Inspector/RenderGraphWidget.h>
#include <QCheckBox>
#include <QClipboard>
#include <QGraphicsScene>
#include <QJsonDocument>
#include <QLabel>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTabWidget>
#include <QVBoxLayout>

using namespace xiiInspectorUi;
xiiQtRenderGraphWidget* xiiQtRenderGraphWidget::s_pWidget = nullptr;

xiiQtRenderGraphWidget::xiiQtRenderGraphWidget(ads::CDockManager* manager) : xiiQtRenderingCapture(manager, "Render Graph", 'RGRF')
{
  s_pWidget = this;
  setObjectName("RenderGraphInspector");
  m_pGraphs = new QComboBox;
  m_pGraphs->setAccessibleName("Captured render graph");
  m_pControls->insertWidget(0, m_pGraphs);
  auto* fit = new QPushButton("Fit");
  m_pControls->addWidget(fit);
  auto* tabs = new QTabWidget;
  m_pLayout->addWidget(tabs, 1);
  auto* splitter = new QSplitter(Qt::Vertical);
  m_pGraphScene  = new QGraphicsScene(this);
  m_pGraphView   = new GraphicsView(m_pGraphScene);
  m_pGraphView->setAccessibleName("Render graph dependency diagram");
  splitter->addWidget(m_pGraphView);
  m_pDetails = new QPlainTextEdit;
  m_pDetails->setReadOnly(true);
  m_pDetails->setMaximumHeight(170);
  m_pDetails->setPlaceholderText("Select a pass node or table row for its dependencies, barriers, and stable identity.");
  splitter->addWidget(m_pDetails);
  tabs->addTab(splitter, "Dependencies");
  m_pPasses = Table({"Order", "Pass", "Queue", "GPU ms", "Timing frame", "Merge group", "Side effects", "Managed scope", "Culled"}, "Graph passes");
  tabs->addTab(m_pPasses, "Passes");
  m_pLifetimeScene   = new QGraphicsScene(this);
  auto* lifetimeView = new GraphicsView(m_pLifetimeScene);
  lifetimeView->setAccessibleName("Resource lifetimes in compiled pass order");
  tabs->addTab(lifetimeView, "Lifetimes");
  m_pResources = Table({"Resource", "Kind", "Lifetime flags", "First pass", "Last pass", "Alias group", "Stable ID", "Dimensions / bytes", "Format", "Mips"}, "Graph resources");
  tabs->addTab(m_pResources, "Resources");
  m_pBarriers = Table({"Index", "Resource", "Before", "After", "Transition", "Source queue", "Target queue", "Ownership", "Mip range", "Slice range"}, "Graph barriers");
  tabs->addTab(m_pBarriers, "Barriers");
  m_pVersions = Table({"Resource", "Version", "Parent", "Producer pass", "First pass", "Last pass", "Required state", "Current state", "Exported", "Stable ID"}, "Resource versions");
  m_pResources->setColumnHidden(6, true);
  m_pVersions->setColumnHidden(9, true);
  m_pResources->setColumnWidth(0, 260);
  m_pPasses->setColumnWidth(1, 260);
  m_pVersions->setColumnWidth(0, 260);
  for (auto* table : {m_pResources, m_pVersions})
  {
    table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(table, &QWidget::customContextMenuRequested, this, [table, this](QPoint point) {
      auto* item = table->itemAt(point);
      if (!item)
      {
        return;
      }
      QMenu menu;
      auto* copy = menu.addAction("Copy resource identifier");
      if (menu.exec(table->viewport()->mapToGlobal(point)) == copy)
      {
        QApplication::clipboard()->setText(item->text(table == m_pResources ? 6 : 9));
      }
    });
  }
  tabs->addTab(m_pVersions, "Versions");
  m_pSubmissions = Table({"Submission", "Queue", "Compiled pass order", "Wait submissions", "Signal value", "Wait values"}, "Queue submissions");
  tabs->addTab(m_pSubmissions, "Queue schedule");
  connect(m_pGraphs, &QComboBox::currentIndexChanged, this, [this] { Rebuild(); });
  connect(fit, &QPushButton::clicked, this, [this, lifetimeView] {
    m_pGraphView->fitInView(m_pGraphScene->itemsBoundingRect(), Qt::KeepAspectRatio);
    lifetimeView->fitInView(m_pLifetimeScene->itemsBoundingRect(), Qt::KeepAspectRatio);
  });
  connect(m_pGraphScene, &QGraphicsScene::selectionChanged, this, [this] {
    const auto selected = m_pGraphScene->selectedItems();
    if (!selected.isEmpty())
    {
      SelectPass(selected.front()->data(0).toInt());
    }
  });
  connect(m_pPasses, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* item) {
    if (item)
    {
      SelectPass(item->data(0, Qt::UserRole).toInt());
    }
  });
}

xiiQtRenderGraphWidget::~xiiQtRenderGraphWidget() { s_pWidget = nullptr; }
void xiiQtRenderGraphWidget::ProcessTelemetry(void*)
{
  if (s_pWidget)
  {
    s_pWidget->ReceiveMessages();
  }
}

bool xiiQtRenderGraphWidget::ApplyCapture(const QJsonObject& object)
{
  if (object["version"].toInt() != 1 || object["kind"] != "renderGraph" || !object["id"].isString() || !object["frame"].isString())
  {
    return false;
  }
  for (const auto* key : {"passes", "resources", "barriers", "versions", "submissions", "mergeGroups"})
  {
    if (!object[key].isArray() || object[key].toArray().size() > 100000)
    {
      return false;
    }
  }
  if (object["passes"].toArray().size() > 4096 || object["resources"].toArray().size() > 16384)
  {
    return false;
  }
  QSet<qint64>      indices;
  QMap<qint64, int> orders;
  for (const auto& value : object["passes"].toArray())
  {
    const auto pass  = value.toObject();
    const auto index = pass["index"].toInteger(-1);
    if (!value.isObject() || !pass["name"].isString() || index < 0 || index >= object["passes"].toArray().size() || indices.contains(index) || pass["queue"].toInt(-1) < 0 || pass["queue"].toInt() > 2)
    {
      return false;
    }
    indices.insert(index);
    orders[index] = orders.size();
  }
  qint64     edges     = 0;
  const auto passes    = object["passes"].toArray();
  const auto resources = object["resources"].toArray();
  for (const auto& value : passes)
  {
    const auto pass = value.toObject();
    for (const auto& dependency : pass["dependencies"].toArray())
    {
      if (!indices.contains(dependency.toInteger(-1)) || orders.value(dependency.toInteger()) >= orders.value(pass["index"].toInteger()) || ++edges > 65536)
      {
        return false;
      }
    }
    for (const auto* key : {"preBarriers", "postBarriers", "splitBegins"})
    {
      for (const auto& barrier : pass[key].toArray())
      {
        if (barrier.toInteger(-1) < 0 || barrier.toInteger() >= object["barriers"].toArray().size())
        {
          return false;
        }
      }
    }
  }
  for (const auto& value : resources)
  {
    if (!value.isObject())
    {
      return false;
    }
    const auto resource = value.toObject();
    const auto first    = resource["first"].toInteger(-1);
    const auto last     = resource["last"].toInteger(-1);
    if (first == 4294967295LL && last == 4294967295LL)
    {
      continue;
    }
    if (first < 0 || last < first || last >= passes.size())
    {
      return false;
    }
  }
  for (const auto& value : object["barriers"].toArray())
  {
    if (!value.isObject() || value.toObject()["resource"].toInteger(-1) < 0 || value.toObject()["resource"].toInteger() >= resources.size())
    {
      return false;
    }
  }
  for (const auto& value : object["submissions"].toArray())
  {
    if (!value.isObject())
    {
      return false;
    }
    for (const auto& pass : value.toObject()["passes"].toArray())
    {
      if (pass.toInteger(-1) < 0 || pass.toInteger() >= passes.size())
      {
        return false;
      }
    }
  }
  // Bound cached graphs by count and encoded size, including offline imports.
  const QString id = object["instance"].toString(object["id"].toString());
  m_Graphs.insert(id, object);
  qint64 total = 0;
  for (const auto& graph : m_Graphs)
  {
    total += QJsonDocument(graph).toJson(QJsonDocument::Compact).size();
  }
  while (m_Graphs.size() > 64 || (total > 64 * 1024 * 1024 && m_Graphs.size() > 1))
  {
    auto oldest = m_Graphs.begin();
    if (oldest.key() == id)
    {
      ++oldest;
    }
    total -= QJsonDocument(oldest.value()).toJson(QJsonDocument::Compact).size();
    m_Graphs.erase(oldest);
  }
  const auto           selected = m_pGraphs->currentData().toString();
  const QSignalBlocker blocker(m_pGraphs);
  m_pGraphs->clear();
  QMap<quint64, QString> names;
  for (auto it = m_Graphs.begin(); it != m_Graphs.end(); ++it)
  {
    names[it.key().toULongLong()] = (it.value()["view"].toString().isEmpty() ? QString{} : it.value()["view"].toString() + " / ") + it.value()["name"].toString("Render graph");
  }
  for (auto it = m_Graphs.begin(); it != m_Graphs.end(); ++it)
  {
    m_pGraphs->addItem(GraphLabel(names, it.key().toULongLong()), it.key());
    m_pGraphs->setItemData(m_pGraphs->count() - 1, "Graph instance: " + it.key(), Qt::ToolTipRole);
  }
  const int previous = m_pGraphs->findData(selected);
  m_pGraphs->setCurrentIndex(previous >= 0 ? previous : m_pGraphs->findData(id));
  Rebuild();
  if (xiiQtProfilingWidget::s_pRenderingWidget)
  {
    xiiQtProfilingWidget::s_pRenderingWidget->ApplyGraphCapture(object);
  }
  if (m_sPendingGraph == id)
  {
    const auto pending = m_iPendingPass;
    m_sPendingGraph.clear();
    LocatePass(id, pending);
  }
  return true;
}

void xiiQtRenderGraphWidget::ClearCapture()
{
  m_sSelectedPassId.clear();
  m_Graphs.clear();
  m_pGraphs->clear();
  Rebuild();
}

void xiiQtRenderGraphWidget::Rebuild()
{
  const QSignalBlocker selectionBlocker(m_pGraphScene);
  for (auto* table : {m_pPasses, m_pResources, m_pBarriers, m_pVersions, m_pSubmissions})
  {
    table->setSortingEnabled(false);
    table->clear();
  }
  m_pGraphScene->clear();
  m_pLifetimeScene->clear();
  m_pDetails->clear();
  const auto graph = m_Graphs.value(m_pGraphs->currentData().toString());
  SetDocument(graph);
  if (graph.isEmpty())
  {
    return;
  }
  const auto stats = graph["statistics"].toObject();
  m_pStatus->setText(QString("Frame %1 • %2 passes (%3 culled) • %4 barriers • %5 merge groups • %6 aliased resources • Compile cache: %7 • %8")
                       .arg(graph["frame"].toString())
                       .arg(stats["compiled"].toInt())
                       .arg(stats["culled"].toInt())
                       .arg(stats["barriers"].toInt())
                       .arg(stats["mergeGroups"].toInt())
                       .arg(stats["aliasedResources"].toInt())
                       .arg(stats["cached"].toBool() ? "hit" : "miss")
                       .arg(graph["succeeded"].toBool() ? (graph["gpuProfiling"].toBool() ? "GPU timings are delayed; see timing frame." : "GPU profiling disabled in application.") : "Execution failed: " + graph["error"].toString()));
  const QString              filter = m_pFilter->text();
  const auto                 passes = graph["passes"].toArray();
  QMap<int, QPointF>         positions;
  QMap<int, int>             depth;
  QMap<int, int>             queuesByIndex;
  QMap<QPair<int, int>, int> rows;
  for (int order = 0; order < passes.size(); ++order)
  {
    const auto pass      = passes[order].toObject();
    const int  index     = pass["index"].toInt();
    const int  queue     = pass["queue"].toInt();
    queuesByIndex[index] = queue;
    int column           = 0;
    for (const auto& dependency : pass["dependencies"].toArray())
    {
      column = qMax(column, depth.value(dependency.toInt(), -1) + 1);
    }
    depth[index]     = column;
    const int row    = rows[{column, queue}]++;
    positions[index] = QPointF(180 + column * 260, 40 + queue * 300 + row * 76);
    auto* item       = Row(m_pPasses, {QString::number(order), pass["name"].toString(), QueueName(queue), pass["timingAvailable"].toBool() ? QString::number(pass["gpuMs"].toDouble(), 'f', 3) : "Unavailable", pass["timingFrame"].toString(), Index(pass["mergeGroup"]), pass["sideEffects"].toBool() ? "Yes" : "", pass["managed"].toBool() ? "Yes" : ""}, filter);
    item->setData(0, Qt::UserRole, index);
    item->setText(8, pass["culled"].toBool() ? "Yes" : "");
  }
  // Size queue lanes from their busiest dependency column so wide graphs never overlap lanes.
  double queueY[3] = {40, 40, 40};
  for (int queue = 1; queue < 3; ++queue)
  {
    int maxRows = 1;
    for (auto it = rows.begin(); it != rows.end(); ++it)
    {
      if (it.key().second == queue - 1)
      {
        maxRows = qMax(maxRows, it.value());
      }
    }
    queueY[queue] = queueY[queue - 1] + 70 + maxRows * 76;
  }
  for (auto it = positions.begin(); it != positions.end(); ++it)
  {
    const int queue = queuesByIndex.value(it.key());
    it.value().ry() += queueY[queue] - (40 + queue * 300);
  }
  for (int queue = 0; queue < 3; ++queue)
  {
    auto* label = m_pGraphScene->addSimpleText(QueueName(queue));
    label->setBrush(QueueColor(queue));
    label->setPos(0, queueY[queue]);
  }
  for (const auto& value : passes)
  {
    const auto pass     = value.toObject();
    const int  index    = pass["index"].toInt();
    const auto position = positions[index];
    const bool match    = filter.isEmpty() || pass["name"].toString().contains(filter, Qt::CaseInsensitive);
    for (const auto& dependency : pass["dependencies"].toArray())
    {
      if (!positions.contains(dependency.toInt()))
      {
        continue;
      }
      const QPointF start = positions[dependency.toInt()] + QPointF(220, 28);
      const QPointF end   = position + QPointF(0, 28);
      QPainterPath  path(start);
      path.cubicTo(start + QPointF(35, 0), end - QPointF(35, 0), end);
      m_pGraphScene->addPath(path, QPen(QColor("#788392"), 1.5))->setZValue(-1);
      QPolygonF arrow;
      arrow << end << end + QPointF(-8, -4) << end + QPointF(-8, 4);
      m_pGraphScene->addPolygon(arrow, QPen(Qt::NoPen), QColor("#788392"));
    }
    auto* node = m_pGraphScene->addRect(QRectF(position, QSizeF(220, 56)), QPen(QueueColor(pass["queue"].toInt()), 2), QueueColor(pass["queue"].toInt()).darker(220));
    node->setFlag(QGraphicsItem::ItemIsSelectable);
    node->setData(0, index);
    node->setData(1, pass["id"].toString());
    if (pass["id"].toString() == m_sSelectedPassId)
    {
      node->setSelected(true);
    }
    node->setOpacity(match ? 1 : 0.25);
    if (pass["culled"].toBool())
    {
      node->setBrush(QColor("#484b50"));
    }
    node->setToolTip(pass["name"].toString() + "\nID: " + pass["id"].toString() + "\nDependencies: " + Indices(pass["dependencies"].toArray()));
    const QString name = QFontMetrics(QApplication::font()).elidedText(pass["name"].toString(), Qt::ElideRight, 200);
    auto*         text = new QGraphicsSimpleTextItem(name + "\n" + (pass["timingAvailable"].toBool() ? QString::number(pass["gpuMs"].toDouble(), 'f', 3) + " GPU ms" : "GPU timing unavailable"), node);
    text->setPos(position + QPointF(8, 7));
    text->setBrush(Qt::white);
  }
  const auto             resources = graph["resources"].toArray();
  QMap<QString, QString> resourceNames;
  int                    lifetimeRow = 0;
  for (int index = 0; index < resources.size(); ++index)
  {
    const auto    resource                   = resources[index].toObject();
    const QString name                       = ResourceName(resource, index);
    resourceNames[resource["id"].toString()] = name;
    const int   flags                        = resource["flags"].toInt();
    QStringList flagNames;
    if (flags & 1)
    {
      flagNames << "Transient";
    }
    if (flags & 2)
    {
      flagNames << "External";
    }
    if (flags & 4)
    {
      flagNames << "Persistent";
    }
    Row(m_pResources, {name, resource["texture"].toBool() ? "Texture" : "Buffer", flagNames.join(" | "), Index(resource["first"]), Index(resource["last"]), Index(resource["alias"]), resource["id"].toString(), resource["texture"].toBool() ? QString("%1 × %2 × %3").arg(resource["width"].toInt()).arg(resource["height"].toInt()).arg(resource["depthOrSlices"].toInt()) : Bytes(resource["bufferBytes"].toString().toULongLong()), resource["format"].toString(), resource["texture"].toBool() ? Index(resource["mips"]) : QString{}}, filter);
    if ((!filter.isEmpty() && !name.contains(filter, Qt::CaseInsensitive)) || resource["first"].toDouble() == 4294967295.0)
    {
      continue;
    }
    auto* label = m_pLifetimeScene->addSimpleText(QFontMetrics(QApplication::font()).elidedText(name, Qt::ElideRight, 250));
    label->setBrush(Qt::white);
    label->setPos(0, 30 + lifetimeRow * 26);
    const double first = resource["first"].toDouble();
    const double last  = resource["last"].toDouble();
    const QColor color = flags & 2 ? QColor("#d6a052") : QueueColor(resource["alias"].toInt() % 3);
    auto*        bar   = m_pLifetimeScene->addRect(270 + first * 28, 30 + lifetimeRow * 26, qMax(1.0, last - first + 1) * 28, 20, QPen(Qt::NoPen), color);
    bar->setToolTip(name + "\nCompiled pass range: " + Index(resource["first"]) + "–" + Index(resource["last"]) + "\nAlias group: " + Index(resource["alias"]));
    ++lifetimeRow;
  }
  auto* axis = m_pLifetimeScene->addSimpleText("Compiled pass order ->   (bars show logical lifetimes; colors identify alias groups / external resources)");
  axis->setBrush(Qt::white);
  for (int i = 0; i < passes.size(); ++i)
  {
    auto* tick = m_pLifetimeScene->addSimpleText(QString::number(i));
    tick->setBrush(QColor("#aab4c0"));
    tick->setPos(270 + i * 28, 8);
  }
  const auto barriers = graph["barriers"].toArray();
  for (int index = 0; index < barriers.size(); ++index)
  {
    const auto    barrier  = barriers[index].toObject();
    const int     resource = barrier["resource"].toInt(-1);
    const QString name     = resource >= 0 && resource < resources.size() ? ResourceName(resources[resource].toObject(), resource) : "Unknown resource";
    const QString type     = barrier["type"].toInt() == 0 ? "Immediate" : barrier["type"].toInt() == 1 ? "Split begin" :
                                                                                                         "Split end";
    Row(m_pBarriers, {QString::number(index), name, barrier["beforeName"].toString(QString("0x%1").arg(barrier["before"].toInteger(), 0, 16)), barrier["afterName"].toString(QString("0x%1").arg(barrier["after"].toInteger(), 0, 16)), type, QueueName(barrier["sourceQueue"].toInt()), QueueName(barrier["targetQueue"].toInt()), barrier["ownershipTransfer"].toBool() ? "Transfer" : "", Index(barrier["firstMip"]) + " + " + Index(barrier["mips"]), Index(barrier["firstSlice"]) + " + " + Index(barrier["slices"])}, filter);
  }
  QMap<QString, QString> passNames;
  for (const auto& value : passes)
  {
    passNames[value.toObject()["id"].toString()] = value.toObject()["name"].toString();
  }
  for (const auto& value : graph["versions"].toArray())
  {
    const auto version = value.toObject();
    Row(m_pVersions, {resourceNames.value(version["resource"].toString()), Index(version["version"]), version["parent"].toInt() == 65535 ? "-" : Index(version["parent"]), passNames.value(version["producer"].toString(), "External / no producer"), Index(version["first"]), Index(version["last"]), version["requiredStateName"].toString(QString("0x%1").arg(version["requiredState"].toInteger(), 0, 16)), version["currentStateName"].toString(QString("0x%1").arg(version["currentState"].toInteger(), 0, 16)), version["exported"].toBool() ? "Yes" : "", version["id"].toString()}, filter);
  }
  const auto submissions = graph["submissions"].toArray();
  for (int index = 0; index < submissions.size(); ++index)
  {
    const auto  submission = submissions[index].toObject();
    QStringList waits;
    for (const auto& wait : submission["waitValues"].toArray())
    {
      waits << wait.toString();
    }
    Row(m_pSubmissions, {QString::number(index), QueueName(submission["queue"].toInt()), Indices(submission["passes"].toArray()), Indices(submission["waits"].toArray()), submission["signal"].toString(), waits.join(", ")}, filter);
  }
  for (auto* table : {m_pPasses, m_pResources, m_pBarriers, m_pVersions, m_pSubmissions})
  {
    table->setSortingEnabled(true);
    for (int row = 0; row < table->topLevelItemCount(); ++row)
    {
      for (int column = 0; column < table->columnCount(); ++column)
      {
        table->topLevelItem(row)->setToolTip(column, table->topLevelItem(row)->text(0) + "\n" + table->topLevelItem(row)->text(table == m_pResources ? 6 : table == m_pVersions ? 9 :
                                                                                                                                                                                  0));
      }
    }
  }
  m_pGraphScene->setSceneRect(m_pGraphScene->itemsBoundingRect().adjusted(-15, -15, 15, 15));
  m_pLifetimeScene->setSceneRect(m_pLifetimeScene->itemsBoundingRect().adjusted(-15, -15, 15, 15));
  for (const auto& value : passes)
  {
    if (value.toObject()["id"].toString() == m_sSelectedPassId)
    {
      SelectPass(value.toObject()["index"].toInt());
    }
  }
}

void xiiQtRenderGraphWidget::SelectPass(int index)
{
  for (const auto& value : m_Document["passes"].toArray())
  {
    if (value.toObject()["index"].toInt() == index)
    {
      m_sSelectedPassId = value.toObject()["id"].toString();
      m_pDetails->setPlainText(QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Indented)));
      return;
    }
  }
}

void xiiQtRenderGraphWidget::LocatePass(const QString& graph, int pass)
{
  toggleView(true);
  raise();
  const int index = m_pGraphs->findData(graph);
  if (index < 0)
  {
    m_sPendingGraph = graph;
    m_iPendingPass  = pass;
    RequestCapture();
    return;
  }
  m_pGraphs->setCurrentIndex(index);
  for (const auto& value : m_Document["passes"].toArray())
  {
    if (value.toObject()["index"].toInt() == pass)
    {
      m_sSelectedPassId = value.toObject()["id"].toString();
    }
  }
  Rebuild();
  SelectPass(pass);
  for (auto* item : m_pGraphScene->selectedItems())
  {
    m_pGraphView->ensureVisible(item);
  }
}
